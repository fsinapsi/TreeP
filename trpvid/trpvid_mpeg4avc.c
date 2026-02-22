/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "./trpvid_internal.h"
#include "JM-ldecod/global.h"
#include "JM-ldecod/frame.h"
#include "JM-ldecod/sei.h"
#include "JM-ldecod/nalu.h"
#include "JM-ldecod/vlc.h"

static int se_v( Bitstream *bitstream );
static int ue_v( Bitstream *bitstream );
static int u_1( Bitstream *bitstream );
static int u_v( int LenInBits, Bitstream *bitstream );
static int i_v( int LenInBits, Bitstream *bitstream );
static int my_RBSPtoSODB( uns8b *buf, int last_byte_pos );
static int my_EBSPtoRBSP( uns8b *buf, uns32b size );
static uns32b avc_next_start_code( uns8b *buf, uns32b size );
static uns8b decode_matroska_codec_private( trp_vid_t *vid, int *slice_cnt );
static uns8b decode_matroska_nal( trp_vid_t *vid, uns8b *buf, uns32b *pos, uns32b *size, uns32b nal_size_size, int *slice_cnt );
static uns8b decode_avi_nal( trp_vid_t *vid, int *slice_cnt );
static uns8b decode_nal( trp_vid_t *vid, uns8b *buf, uns32b size, int *slice_cnt );
static sps_t *sps_by_id( trp_vid_t *vid, int seq_parameter_set_id );
static pps_t *pps_by_id( trp_vid_t *vid, int pic_parameter_set_id );
static uns8b decode_sei( trp_vid_t *vid, uns8b *buf, uns32b size );
static uns8b *decode_scaling_list( Bitstream *b, int size );
static uns8b decode_sps( trp_vid_t *vid, uns8b *buf, uns32b size );
static uns8b decode_pps( trp_vid_t *vid, uns8b *buf, uns32b size );
static uns8b decode_slice( trp_vid_t *vid, uns8b *buf, uns32b size, int nal_ref_idc, int slice_cnt );

static int se_v( Bitstream *bitstream )
{
    int used_bits;

    return read_se_v( "", bitstream, &used_bits );
}

static int ue_v( Bitstream *bitstream )
{
    int used_bits;

    return read_ue_v( "", bitstream, &used_bits );
}

static int u_1( Bitstream *bitstream )
{
    int used_bits;

    return (int)read_u_1( "", bitstream, &used_bits );
}

static int u_v( int LenInBits, Bitstream *bitstream )
{
    int used_bits;

    return read_u_v( LenInBits, "", bitstream, &used_bits );
}

static int i_v( int LenInBits, Bitstream *bitstream )
{
    int used_bits;

    return read_i_v( LenInBits, "", bitstream, &used_bits );
}

static int my_RBSPtoSODB( uns8b *buf, int last_byte_pos )
{
    int ctr_bit, bitoffset = 0;

    for ( ctr_bit = buf[ last_byte_pos - 1 ] & ( 0x01 << bitoffset ) ;
          ctr_bit == 0 ;
          ctr_bit = buf[ last_byte_pos - 1 ] & ( 0x01 << bitoffset ) )
        if ( ++bitoffset == 8 ) {
            if ( last_byte_pos == 0 )
                return -1;
            /*
             if( last_byte_pos == 0 )
             printf( " Panic: All zero data sequence in RBSP \n" );
             assert( last_byte_pos != 0 );
             */
            last_byte_pos--;
            bitoffset = 0;
        }
    return last_byte_pos;
}

static int my_EBSPtoRBSP( uns8b *buf, uns32b size )
{
    uns32b i, j, count = 0;

    for ( i = j = 0 ; i < size ; i++ ) {
        if ( ( count == 2 ) && ( buf[ i ] == 0x03 ) ) {
            i++;
            count = 0;
        }
        buf[ j++ ] = buf[ i ];
        if ( buf[ i ] == 0 )
            count++;
        else
            count = 0;
    }
    return j;
}

static uns32b avc_next_start_code( uns8b *buf, uns32b size )
{
    uns32b i, count = 0;

    for ( i = 0 ; i < size ; i++ ) {
        if ( ( ( count == 2 ) || ( count == 3 ) ) && ( buf[ i ] == 0x01 ) )
            return i + 1;
        if ( buf[ i ] == 0 )
            count++;
        else
            count = 0;
    }
    return size;
}

uns8b trp_vid_parse_mpeg4avc( trp_vid_t *vid )
{
    int slice_cnt = 0;

    if ( vid->matroska ) {
        /*
         si tratta sicuramente di AVC
         */
        if ( vid->matroska_codec_private )
            return 1;
        vid->matroska_codec_private = trp_gc_malloc( vid->buf_size );
        memcpy( vid->matroska_codec_private, vid->buf, vid->buf_size );
        vid->matroska_size = vid->buf_size;
        return 0;
    }
    if ( vid->matroska_codec_private )
        if ( decode_matroska_codec_private( vid, &slice_cnt ) )
            return 1;
    if ( vid->matroska_size ) {
        uns32b pos = 0, size;

        for ( size = vid->buf_size ; size ; )
            if ( decode_matroska_nal( vid, vid->buf, &pos, &size, vid->matroska_size, &slice_cnt ) )
                return 1;
        return 0;
    }
    return decode_avi_nal( vid, &slice_cnt );
}

static uns8b decode_matroska_codec_private( trp_vid_t *vid, int *slice_cnt )
{
    uns32b size, pos, nal_size_size, numsps, numpps;

    size = vid->matroska_size;

    if ( size < 6 )
        return 1;
    size -= 6;

    nal_size_size = ( vid->matroska_codec_private[ 4 ] & 3 ) + 1;
    numsps = vid->matroska_codec_private[ 5 ] & 0x1f;
    pos = 6;

    for ( ; numsps ; numsps-- ) {
        if ( size == 0 )
            break;
        if ( decode_matroska_nal( vid, vid->matroska_codec_private, &pos, &size, 2, slice_cnt ) )
            return 1;
    }
    if ( size ) {
        numpps = vid->matroska_codec_private[ pos++ ];
        size--;
        for ( ; numpps ; numpps-- ) {
            if ( size == 0 )
                break;
            if ( decode_matroska_nal( vid, vid->matroska_codec_private, &pos, &size, 2, slice_cnt ) )
                return 1;
        }
    }

    trp_gc_free( vid->matroska_codec_private );
    vid->matroska_codec_private = NULL;
    vid->matroska_size = nal_size_size;
    return 0;
}

static uns8b decode_matroska_nal( trp_vid_t *vid, uns8b *buf, uns32b *pos, uns32b *size, uns32b nal_size_size, int *slice_cnt )
{
    uns32b i, nal_size = 0;

    if ( *size < nal_size_size )
        return 1;
    *size = *size - nal_size_size;

    for ( i = 0 ; i < nal_size_size ; i++ ) {
        nal_size = ( nal_size << 8 ) | buf[ *pos ];
        *pos = *pos + 1;
    }

    if ( *size < nal_size )
        return 1;
    *size = *size - nal_size;

    if ( decode_nal( vid, buf + *pos, nal_size, slice_cnt ) )
        return 1;

    *pos = *pos + nal_size;
    return 0;
}

static uns8b decode_avi_nal( trp_vid_t *vid, int *slice_cnt )
{
    uns32b size, i;
    uns8b *p;

    size = vid->buf_size;
    p = vid->buf;

    i = avc_next_start_code( p, size );
    if ( i == size ) {
        vid->error = "Il frame non ha un valido start-code";
        return 1;
    }
    do {
        p += i;
        size -= i;
        i = avc_next_start_code( p, size );
        decode_nal( vid, p, ( i == size ) ? i : i - 4, slice_cnt );
    } while ( i < size );
    if ( ( vid->bitstream_type == 0 ) &&
         ( vid->sps_cnt == 0 ) &&
         ( vid->pps_cnt == 0 ) )
        return 1;
    return 0;
}

static uns8b decode_nal( trp_vid_t *vid, uns8b *buf, uns32b size, int *slice_cnt )
{
    uns8b nal_ref_idc, nal_unit_type;

    if ( size < 1 )
        return 1;

    nal_ref_idc = ( buf[ 0 ] >> 5 ) & 3;
    nal_unit_type = buf[ 0 ] & 0x1f;

    buf++;
    size--;

    size = my_EBSPtoRBSP( buf, size );

#if 0
    fprintf( stderr, "size = %lu, nal unit type = %d\n", size, (int)nal_unit_type );
#endif

    switch ( nal_unit_type ) {
    case NALU_TYPE_SLICE:
    case NALU_TYPE_IDR:
        vid->idr_flag = ( nal_unit_type == NALU_TYPE_IDR );
        if ( decode_slice( vid, buf, size, (int)nal_ref_idc, *slice_cnt ) ) {
            vid->error = "Syntax error (NALU SLICE)";
            return 1;
        }
        *slice_cnt = *slice_cnt + 1;
        break;
    case NALU_TYPE_SPS:
        if ( decode_sps( vid, buf, size ) ) {
            vid->error = "Syntax error (NALU SPS)";
            return 1;
        }
        break;
    case NALU_TYPE_PPS:
        if ( decode_pps( vid, buf, size ) ) {
            vid->error = "Syntax error (NALU PPS)";
            return 1;
        }
        break;
    case NALU_TYPE_SEI:
        if ( decode_sei( vid, buf, size ) ) {
            vid->error = "Syntax error (NALU SEI)";
            return 1;
        }
        break;
    case NALU_TYPE_DPA:
        if ( decode_slice( vid, buf, size, (int)nal_ref_idc, *slice_cnt ) ) {
            vid->error = "Syntax error (NALU DPA)";
            return 1;
        }
        *slice_cnt = *slice_cnt + 1;
        break;
    default:
        break;
    }
    return 0;
}

static sps_t *sps_by_id( trp_vid_t *vid, int seq_parameter_set_id )
{
    uns32b i;
    sps_t *res = NULL;

    for ( i = 0 ; i < vid->sps_cnt ; i++ )
        if ( vid->sps[ i ]->seq_parameter_set_id == seq_parameter_set_id ) {
            res = vid->sps[ i ];
            break;
        }
    return res;
}

static pps_t *pps_by_id( trp_vid_t *vid, int pic_parameter_set_id )
{
    uns32b i;
    pps_t *res = NULL;

    for ( i = 0 ; i < vid->pps_cnt ; i++ )
        if ( vid->pps[ i ]->pic_parameter_set_id == pic_parameter_set_id ) {
            res = vid->pps[ i ];
            break;
        }
    return res;
}

static uns8b decode_sei( trp_vid_t *vid, uns8b *buf, uns32b size )
{
    uns32b t, s;

    do {
        for ( t = 0 ; ; ) {
            if ( size < 1 )
                return 1;
            size--;
            t += *buf;
            if ( *buf++ != 0xff )
                break;
        }
        for ( s = 0 ; ; ) {
            if ( size < 1 )
                return 1;
            size--;
            s += *buf;
            if ( *buf++ != 0xff )
                break;
        }

        if ( size <= s ) /* <= per garantire la guardia del ciclo... */
            return 1;
        size -= s;

        switch ( t ) {
        case SEI_USER_DATA_UNREGISTERED:
            if ( s > 16 )
                trp_vid_store_userdata( vid, buf + 16, s - 16 );
            break;
        default:
            break;
        }
        buf += s;
    } while ( *buf != 0x80 );
    /*
     con ( size == 1 ) occasionalmente fallisce...
     */
    return ( size > 0 ) ? 0 : 1;
}

static uns8b *decode_scaling_list( Bitstream *b, int size )
{
    extern uns8b _ZZ_SCAN4[];
    extern uns8b _ZZ_SCAN8[];
    int j, scanj;
    int delta_scale, lastScale = 8, nextScale = 8;
    uns8b *list;

    list = trp_gc_malloc( size );
    for ( j = 0 ; j < size ; j++ ) {
        scanj = ( size == 16 ) ? _ZZ_SCAN4[ j ] : _ZZ_SCAN8[ j ];

        if ( nextScale ) {
            delta_scale = se_v( b );
            nextScale = ( lastScale + delta_scale + 256 ) % 256;
            if ( nextScale ) {
                lastScale = nextScale;
            } else {
                if ( scanj == 0 ) {
                    trp_gc_free( list );
                    list = NULL;
                    break;
                }
            }
        }
        if ( list )
            list[ scanj ] = lastScale;
    }
    return list;
}

static uns8b decode_sps( trp_vid_t *vid, uns8b *buf, uns32b size )
{
    Bitstream b;
    sps_t sps;

    b.streamBuffer = buf;
    b.frame_bitoffset = 0;
    b.bitstream_length = my_RBSPtoSODB( buf, size );
    if ( b.bitstream_length < 0 )
        return 1;

    memset( &sps, 0, sizeof( sps_t ) );

    sps.profile_idc = u_v( 8, &b );

    if ( ( sps.profile_idc != 66 ) &&
         ( sps.profile_idc != 77 ) &&
         ( sps.profile_idc != 88 ) &&
         ( sps.profile_idc != FREXT_HP ) &&
         ( sps.profile_idc != FREXT_Hi10P ) &&
         ( sps.profile_idc != FREXT_Hi422 ) &&
         ( sps.profile_idc != FREXT_Hi444 ) ) {
        vid->error = "Analisi SPS fallita...";
        return 1;
    }

    (void)u_1( &b ); /* constrained_set0_flag */
    (void)u_1( &b ); /* constrained_set1_flag */
    (void)u_1( &b ); /* constrained_set2_flag */
    (void)u_1( &b ); /* constrained_set3_flag */
    (void)u_v( 4, &b ); /* reserved_zero_4bits */
    sps.level_idc = u_v( 8, &b );
    sps.seq_parameter_set_id = ue_v( &b );
    sps.chroma_format_idc = 1;
    if ( ( sps.profile_idc == FREXT_HP    ) ||
         ( sps.profile_idc == FREXT_Hi10P ) ||
         ( sps.profile_idc == FREXT_Hi422 ) ||
         ( sps.profile_idc == FREXT_Hi444 ) ) {
        sps.chroma_format_idc = ue_v( &b );
        if ( sps.chroma_format_idc == YUV444 )
            (void)u_1( &b ); /* separate_colour_plane_flag */
        (void)ue_v( &b ); /* bit_depth_luma_minus8 */
        (void)ue_v( &b ); /* bit_depth_chroma_minus8 */
        (void)u_1( &b ); /* lossless_qpprime_y_zero_flag */
        if ( u_1( &b ) ) { /* seq_scaling_matrix_present_flag */
            int n_ScalingList, i;

            n_ScalingList = ( sps.chroma_format_idc != YUV444 ) ? 8 : 12;
            for( i = 0 ; i < n_ScalingList ; i++ )
                if ( u_1( &b ) ) /* seq_scaling_list_present_flag[i] */
                    sps.scaling_list[ i ] = decode_scaling_list( &b, ( i < 6 ) ? 16 : 64 );
        }
    }
    sps.log2_max_frame_num_minus4 = ue_v( &b );
    sps.pic_order_cnt_type = ue_v( &b );
    if ( sps.pic_order_cnt_type == 0 ) {
        sps.log2_max_pic_order_cnt_lsb_minus4 = ue_v( &b );
    } else if ( sps.pic_order_cnt_type == 1 ) {
        int i;

        sps.delta_pic_order_always_zero_flag = u_1( &b );
        se_v( &b ); /* offset_for_non_ref_pic */
        (void)se_v( &b ); /* offset_for_top_to_bottom_field */
        sps.num_ref_frames_in_pic_order_cnt_cycle = ue_v( &b );
        for ( i = 0 ; i < sps.num_ref_frames_in_pic_order_cnt_cycle ; i++ )
            (void)se_v( &b ); /* offset_for_ref_frame[i] */
    }
    sps.num_ref_frames = ue_v( &b );
    (void)u_1( &b ); /* gaps_in_frame_num_value_allowed_flag */
    vid->width = ( ue_v( &b ) + 1 ) * MB_BLOCK_SIZE; /* pic_width_in_mbs_minus1 */
    vid->height = ( ue_v( &b ) + 1 ) * MB_BLOCK_SIZE; /* pic_height_in_map_units_minus1 */
    sps.frame_mbs_only_flag = u_1( &b ); /* frame_mbs_only_flag */
    if ( !( sps.frame_mbs_only_flag ) ) {
        (void)u_1( &b ); /* mb_adaptive_frame_field_flag */
        ( vid->height ) <<= 1;
    }
    (void)u_1( &b ); /* direct_8x8_inference_flag */
    if ( u_1( &b ) ) { /* frame_cropping_flag */
        uns32b i, j;
        /* frame_cropping_rect_left_offset */
        /* frame_cropping_rect_right_offset */
        /* frame_cropping_rect_top_offset */
        /* frame_cropping_rect_bottom_offset */
        i = ( ue_v( &b ) + ue_v( &b ) ) << 1;
        j = ( ue_v( &b ) + ue_v( &b ) ) << 1;
        if ( ( i <= vid->width ) && ( j <= vid->height ) ) {
            vid->width -= i;
            vid->height -= j;
        }
    }
    if ( u_1( &b ) ) { /* vui_parameters_present_flag */
        if ( u_1( &b ) ) { /* VUI: aspect_ratio_info_present_flag */
            sps.vui_seq_parameters_aspect_ratio_idc = u_v( 8, &b );
            if ( sps.vui_seq_parameters_aspect_ratio_idc == 255 ) {
                sps.vui_seq_parameters_sar_width = u_v( 16, &b );
                sps.vui_seq_parameters_sar_height = u_v( 16, &b );
            }
        } else {
        }

        /*
         FIXME da finire...
         */
    }
    {
        int i;
        sps_t *newsps;

        if ( newsps = sps_by_id( vid, sps.seq_parameter_set_id ) ) {
            for ( i = 0 ; i < 8 ; i++ )
                trp_gc_free( newsps->scaling_list[ i ] );
        } else {
            sps_t **s;

            newsps = trp_gc_malloc( sizeof( sps_t ) );
            if ( ( s = GC_realloc( vid->sps, sizeof( sps_t * ) * ( vid->sps_cnt + 1 ) ) ) == NULL ) {
                trp_gc_free( newsps );
                for ( i = 0 ; i < 8 ; i++ )
                    trp_gc_free( sps.scaling_list[ i ] );
                vid->error = "Impossibile allocare memoria";
                return 1;
            }
            vid->sps = s;
            vid->sps[ vid->sps_cnt++ ] = newsps;
        }
        memcpy( newsps, &sps, sizeof( sps_t ) );
    }
    return 0;
}

static uns8b decode_pps( trp_vid_t *vid, uns8b *buf, uns32b size )
{
    Bitstream b;
    pps_t pps;

    b.streamBuffer = buf;
    b.frame_bitoffset = 0;
    b.bitstream_length = my_RBSPtoSODB( buf, size );
    if ( b.bitstream_length < 0 )
        return 1;

    memset( &pps, 0, sizeof( pps_t ) );

    pps.pic_parameter_set_id = ue_v( &b );
    pps.seq_parameter_set_id = ue_v( &b );
    pps.entropy_coding_mode_flag = u_1( &b );
    pps.pic_order_present_flag = u_1( &b );
    pps.num_slice_groups_minus1 = ue_v( &b );

    /* FMO stuff begins here */
    if ( pps.num_slice_groups_minus1 > 0 ) {
        uns32b i;

        switch ( ue_v( &b ) ) { /* slice_group_map_type */
        case 0:
            for ( i = 0 ; i <= pps.num_slice_groups_minus1 ; i++ )
                (void)ue_v( &b ); /* run_length_minus1 [i] */
            break;
        case 2:
            for ( i = 0 ; i < pps.num_slice_groups_minus1 ; i++ ) {
                (void)ue_v( &b ); /* top_left [i] */
                (void)ue_v( &b ); /* bottom_right [i] */
            }
            break;
        case 3:
        case 4:
        case 5:
            (void)u_1( &b ); /* slice_group_change_direction_flag */
            (void)ue_v( &b ); /* slice_group_change_rate_minus1 */
            break;
        case 6:
            if ( pps.num_slice_groups_minus1 + 1 > 4 )
                pps.NumberBitsPerSliceGroupId = 3;
            else if ( pps.num_slice_groups_minus1 + 1 > 2 )
                pps.NumberBitsPerSliceGroupId = 2;
            else
                pps.NumberBitsPerSliceGroupId = 1;
            pps.num_slice_group_map_units_minus1 = ue_v( &b ); /* num_slice_group_map_units_minus1 */
            for ( i = 0 ; i <= pps.num_slice_group_map_units_minus1 ; i++ )
                (void)u_v( pps.NumberBitsPerSliceGroupId, &b ); /* slice_group_id[i] */
            break;
        default:
            break;
        }
    }
    /* FMO stuff ends here */

    pps.num_ref_idx_l0_active_minus1 = ue_v( &b );
    pps.num_ref_idx_l1_active_minus1 = ue_v( &b );
    pps.weighted_pred_flag = u_1( &b );
    pps.weighted_bipred_idc = u_v( 2, &b );
    pps.pic_init_qp_minus26 = se_v( &b );
    pps.pic_init_qs_minus26 = se_v( &b );
    pps.chroma_qp_index_offset = se_v( &b );
    pps.deblocking_filter_control_present_flag = u_1( &b );
    pps.constrained_intra_pred_flag = u_1( &b );
    pps.redundant_pic_cnt_present_flag = u_1( &b );
    if( more_rbsp_data( b.streamBuffer, b.frame_bitoffset, b.bitstream_length ) ) {
        /* Fidelity Range Extensions Stuff */
        pps.transform_8x8_mode_flag = u_1( &b );
        if ( u_1( &b ) ) { /* pic_scaling_matrix_present_flag */
            int n_ScalingList, i;
            sps_t *sps;

            if ( ( sps = sps_by_id( vid, pps.seq_parameter_set_id ) ) == NULL ) {
                vid->error = "Invalid SPS reference in PPS";
                return 1;
            }
            n_ScalingList = 6 + ( ( sps->chroma_format_idc != YUV444 ) ? 2 : 6 ) * pps.transform_8x8_mode_flag;
            for ( i = 0 ; i < n_ScalingList ; i++ )
                if ( u_1( &b ) ) /* pic_scaling_list_present_flag[i] */
                    pps.scaling_list[ i ] = decode_scaling_list( &b, ( i < 6 ) ? 16 : 64 );
        }
        pps.second_chroma_qp_index_offset = se_v( &b );
    } else {
        pps.second_chroma_qp_index_offset = pps.chroma_qp_index_offset;
    }
    {
        int i;
        pps_t *newpps;

        if ( newpps = pps_by_id( vid, pps.pic_parameter_set_id ) ) {
            for ( i = 0 ; i < 12 ; i++ )
                trp_gc_free( newpps->scaling_list[ i ] );
        } else {
            pps_t **p;

            newpps = trp_gc_malloc( sizeof( pps_t ) );
            if ( ( p = GC_realloc( vid->pps, sizeof( pps_t * ) * ( vid->pps_cnt + 1 ) ) ) == NULL ) {
                trp_gc_free( newpps );
                for ( i = 0 ; i < 12 ; i++ )
                    trp_gc_free( pps.scaling_list[ i ] );
                vid->error = "Impossibile allocare memoria";
                return 1;
            }
            vid->pps = p;
            vid->pps[ vid->pps_cnt++ ] = newpps;
        }
        memcpy( newpps, &pps, sizeof( pps_t ) );
    }
    return 0;
}

static uns8b decode_slice( trp_vid_t *vid, uns8b *buf, uns32b size, int nal_ref_idc, int slice_cnt )
{
    Bitstream b;
    int type, frame_num, qp, qpsp, field_pic_flag;
    int num_ref_idx_l0_active, num_ref_idx_l1_active;
    PictureStructure structure;
    pps_t *pps;
    sps_t *sps;

    b.streamBuffer = buf;
    b.frame_bitoffset = 0;
    b.bitstream_length = my_RBSPtoSODB( buf, size );
    if ( b.bitstream_length < 0 )
        return 1;

    (void)ue_v( &b ); /* first_mb_in_slice / start_mb_nr */
    type = ue_v( &b ); /* slice_type */
    if ( type > 4 )
        type -= 5;
    pps = pps_by_id( vid, ue_v( &b ) ); /* pic_parameter_set_id */
    if ( pps == NULL ) {
        vid->error = "Invalid PPS reference in slice";
        return 1;
    }
    sps = sps_by_id( vid, pps->seq_parameter_set_id );
    if ( sps == NULL ) {
        vid->error = "Invalid SPS reference in PPS";
        return 1;
    }

    frame_num = u_v( sps->log2_max_frame_num_minus4 + 4, &b );

    if ( sps->frame_mbs_only_flag ) {
        structure = FRAME;
        field_pic_flag = 0;
    } else {
        field_pic_flag = u_1( &b ); /* field_pic_flag */
        if ( field_pic_flag )
            structure = u_1( &b ) ? BOTTOM_FIELD : TOP_FIELD; /* bottom_field_flag */
        else
            structure = FRAME;
    }

    if ( vid->idr_flag )
        (void)ue_v( &b ); /* idr_pic_id */

    if ( sps->pic_order_cnt_type == 0 ) {
        (void)u_v( sps->log2_max_pic_order_cnt_lsb_minus4 + 4, &b ); /* pic_order_cnt_lsb */
        if( pps->pic_order_present_flag && !field_pic_flag )
            (void)se_v( &b ); /* delta_pic_order_cnt_bottom */
    }
    if ( ( sps->pic_order_cnt_type == 1 ) && !( sps->delta_pic_order_always_zero_flag ) ) {
        (void)se_v( &b ); /* delta_pic_order_cnt[0] */
        if( pps->pic_order_present_flag && !field_pic_flag )
            (void)se_v( &b ); /* delta_pic_order_cnt[1] */
    } else {
    }

    if ( pps->redundant_pic_cnt_present_flag )
        (void)ue_v( &b ); /* redundant_pic_cnt */

    if ( type == B_SLICE )
        (void)u_1( &b ); /* direct_spatial_mv_pred_flag */

    num_ref_idx_l0_active = pps->num_ref_idx_l0_active_minus1 + 1;
    num_ref_idx_l1_active = pps->num_ref_idx_l1_active_minus1 + 1;

    if ( ( type == P_SLICE ) || ( type == SP_SLICE ) || ( type == B_SLICE ) ) {
        if ( u_1( &b ) ) { /* num_ref_idx_override_flag */
            num_ref_idx_l0_active = ue_v( &b ) + 1; /* num_ref_idx_l0_active_minus1 */
            if ( type == B_SLICE )
                num_ref_idx_l1_active = ue_v( &b ) + 1; /* num_ref_idx_l1_active_minus1 */
        }
    }
    if ( type != B_SLICE )
        num_ref_idx_l1_active = 0;

    /* begin ref_pic_list_reordering */

    if ( ( type != I_SLICE ) && ( type != SI_SLICE ) )
        if ( u_1( &b ) ) { /* ref_pic_list_reordering_flag_l0 */
            int i = 0, val;

            do {
                val = ue_v( &b ); /* reordering_of_pic_nums_idc_l0[i] */
                if ( ( val == 0 ) || ( val == 1 ) )
                    (void)ue_v( &b ); /* abs_diff_pic_num_minus1_l0[i] */
                else if ( val == 2 )
                    (void)ue_v( &b ); /* long_term_pic_idx_l0[i] */
                i++;
                /* assert (i>img->num_ref_idx_l0_active); */
            } while ( val != 3 );
        }
    if ( type == B_SLICE )
        if ( u_1( &b ) ) { /* ref_pic_list_reordering_flag_l1 */
            int i = 0, val;

            do {
                val = ue_v( &b ); /* reordering_of_pic_nums_idc_l1[i] */
                if ( ( val == 0 ) || ( val == 1 ) )
                    (void)ue_v( &b ); /* abs_diff_pic_num_minus1_l1[i] */
                else if ( val == 2 )
                    (void)ue_v( &b ); /* long_term_pic_idx_l1[i] */
                i++;
                /* assert (i>img->num_ref_idx_l1_active); */
            } while ( val != 3 );
        }

    /* end ref_pic_list_reordering */

    if ( ( pps->weighted_pred_flag &&
           ( ( type == P_SLICE ) || ( type == SP_SLICE ) ) ) ||
         ( ( pps->weighted_bipred_idc == 1 ) && ( type == B_SLICE ) ) ) {

        /* begin pred_weight_table */

        int i, j, chroma_weight_flag_l0, chroma_weight_flag_l1;

        (void)ue_v( &b ); /* luma_log2_weight_denom */
        if ( sps->chroma_format_idc != YUV400 )
            (void)ue_v( &b ); /* chroma_log2_weight_denom */

        for ( i = 0 ; i < num_ref_idx_l0_active ; i++ ) {
            if ( u_1( &b ) ) { /* luma_weight_flag_l0 */
                (void)se_v( &b ); /* wp_weight[0][i][0] */
                (void)se_v( &b ); /* wp_offset[0][i][0] */
            }

            if ( sps->chroma_format_idc != YUV400 ) {
                chroma_weight_flag_l0 = u_1( &b );
                for ( j = 1 ; j < 3 ; j++ ) {
                    if ( chroma_weight_flag_l0 ) {
                        (void)se_v( &b ); /* wp_weight[0][i][j] */
                        (void)se_v( &b ); /* wp_offset[0][i][j] */
                    }
                }
            }
        }
        if ( ( type == B_SLICE ) && ( pps->weighted_bipred_idc == 1 ) )
            for ( i = 0 ; i < num_ref_idx_l1_active ; i++ ) {
                if ( u_1( &b ) ) { /* luma_weight_flag_l1 */
                    (void)se_v( &b ); /* wp_weight[1][i][0] */
                    (void)se_v( &b ); /* wp_offset[1][i][0] */
                }

                if ( sps->chroma_format_idc != YUV400 ) {
                    chroma_weight_flag_l1 = u_1( &b );
                    for ( j = 1 ; j < 3 ; j++ ) {
                        if ( chroma_weight_flag_l1 ) {
                            (void)se_v( &b ); /* wp_weight[1][i][j] */
                            (void)se_v( &b ); /* wp_offset[1][i][j] */
                        }
                    }
                }
            }

        /* end pred_weight_table */

    }

    if ( nal_ref_idc ) {

        /* begin dec_ref_pic_marking */

        if ( vid->idr_flag )
        {
            (void)u_1( &b ); /* no_output_of_prior_pics_flag */
            (void)u_1( &b ); /* long_term_reference_flag */
        } else {
            if ( u_1( &b ) ) { /* adaptive_ref_pic_buffering_flag */
                int val;

                do {
                    val = ue_v( &b ); /* memory_management_control_operation */

                    if ( ( val == 1 ) || ( val == 3 ) )
                        (void)ue_v( &b ); /* difference_of_pic_nums_minus1 */
                    if ( val == 2 )
                        (void)ue_v( &b ); /* long_term_pic_num */
                    if ( ( val == 3 ) || ( val == 6 ) )
                        (void)ue_v( &b ); /* long_term_frame_idx */
                    if ( val == 4 )
                        (void)ue_v( &b ); /* max_long_term_pic_idx_plus1 */
                } while ( val != 0 );
            }
        }

        /* end dec_ref_pic_marking */

    }

    if ( pps->entropy_coding_mode_flag &&
         ( type != I_SLICE ) && ( type != SI_SLICE ) )
        (void)ue_v( &b ); /* model_number */

    qp = 26 + pps->pic_init_qp_minus26 + se_v( &b ); /* slice_qp_delta */

#if 0
    if ( qp < 0 ) {
        vid->error = "QP < 0";
        return 1;
    }
#endif

    if ( ( type == SP_SLICE ) || ( type == SI_SLICE ) ) {
        if ( type == SP_SLICE )
            (void)u_1( &b ); /* sp_for_switch_flag */
        qpsp = 26 + pps->pic_init_qs_minus26 + se_v( &b ); /* slice_qs_delta */
    }

    if ( slice_cnt == 0 )
        trp_vid_update_qscale( vid, 3, type, qp );

#if 0
    printf( "frame di tipo %d, frame num = %d, structure = %d, qp = %d\n",
            type, frame_num, (int)structure, qp );
#endif

    /*
     FIXME da finire...
     */

    return 0;
}

