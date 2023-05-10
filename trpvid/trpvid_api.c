/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

trp_obj_t *trp_vid_bitstream_type( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->bitstream_type );
}

trp_obj_t *trp_vid_error( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->error == NULL )
        return UNDEF;
    return trp_cord( vid->error );
}

trp_obj_t *trp_vid_userdata( trp_obj_t *obj )
{
    trp_vid_t *vid;
    trp_obj_t *res;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    for ( res = NIL, i = vid->userdata_cnt ; i ; )
        res = trp_cons( trp_cord( vid->userdata[ --i ] ), res );
    return res;
}

trp_obj_t *trp_vid_qpneg( trp_obj_t *obj )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    for ( i = 0 ; i < vid->cnt_vop ; )
        if ( vid->qscale[ i++ ].qscale < 0 )
            return TRP_TRUE;
    return TRP_FALSE;
}

trp_obj_t *trp_vid_missing_vol( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return vid->missing_vol ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_cnt_vol( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->cnt_vol );
}

trp_obj_t *trp_vid_cnt_vop( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->cnt_vop );
}

trp_obj_t *trp_vid_max_frame_size( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    trp_vid_calculate_max_avg_frame_size( vid );
    return trp_sig64( vid->max_frame_size );
}

trp_obj_t *trp_vid_avg_frame_size( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    trp_vid_calculate_max_avg_frame_size( vid );
    return trp_sig64( vid->avg_frame_size );
}

trp_obj_t *trp_vid_cnt_size_frame( trp_obj_t *obj, trp_obj_t *fno )
{
    trp_vid_t *vid;
    uns32b frameno;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( fno, &frameno ) )
        return UNDEF;
    if ( frameno >= vid->cnt_vop )
        return UNDEF;
    return trp_sig64( vid->qscale[ frameno ].size );
}

trp_obj_t *trp_vid_cnt_type_frame( trp_obj_t *obj, trp_obj_t *fno )
{
    trp_vid_t *vid;
    uns32b frameno;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( fno, &frameno ) )
        return UNDEF;
    if ( frameno >= vid->cnt_vop )
        return UNDEF;
    return trp_sig64( vid->qscale[ frameno ].typ );
}

trp_obj_t *trp_vid_cnt_qscale_frame( trp_obj_t *obj, trp_obj_t *fno )
{
    trp_vid_t *vid;
    uns32b frameno;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( fno, &frameno ) )
        return UNDEF;
    if ( frameno >= vid->cnt_vop )
        return UNDEF;
    return trp_sig64( vid->qscale[ frameno ].qscale );
}

trp_obj_t *trp_vid_cnt_qscale( trp_obj_t *obj, trp_obj_t *qs, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb )
{
    trp_vid_t *vid;
    uns32b res, qscale, typ, tosub_a, tosub_b;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( qs, &qscale ) ||
         trp_cast_uns32b( ttyp, &typ ) ||
         trp_cast_uns32b( tosa, &tosub_a ) ||
         trp_cast_uns32b( tosb, &tosub_b ) )
        return UNDEF;
    if ( ( qscale > MAX_QSCALE_AVC ) ||
         ( typ >= 7 ) )
        return UNDEF;
    res = vid->cnt_qscale[ qscale ][ typ ];
    if ( tosub_a || tosub_b )
        if ( tosub_a + tosub_b >= vid->cnt_vop ) {
            res = 0;
        } else {
            uns32b i;

            for ( i = 0 ; i < tosub_a ; i++ )
                if ( ( vid->qscale[ i ].typ == typ ) &&
                     ( trp_vid_effective_qscale( vid->qscale[ i ].qscale, vid->bitstream_type ) == qscale ) )
                    res--;
            for ( i = vid->cnt_vop - tosub_b ; i < vid->cnt_vop ; i++ )
                if ( ( vid->qscale[ i ].typ == typ ) &&
                     ( trp_vid_effective_qscale( vid->qscale[ i ].qscale, vid->bitstream_type ) == qscale ) )
                    res--;
        }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_cnt_qscale_cnt( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb )
{
    trp_vid_t *vid;
    uns32b res, typ, tosub_a, tosub_b;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( ttyp, &typ ) ||
         trp_cast_uns32b( tosa, &tosub_a ) ||
         trp_cast_uns32b( tosb, &tosub_b ) )
        return UNDEF;
    if ( typ >= 7 )
        return UNDEF;
    res = vid->cnt_qscale_cnt[ typ ];
    if ( tosub_a || tosub_b )
        if ( tosub_a + tosub_b >= vid->cnt_vop ) {
            res = 0;
        } else {
            uns32b i;

            for ( i = 0 ; i < tosub_a ; i++ )
                if ( vid->qscale[ i ].typ == typ )
                    res--;
            for ( i = vid->cnt_vop - tosub_b ; i < vid->cnt_vop ; i++ )
                if ( vid->qscale[ i ].typ == typ )
                    res--;
        }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_cnt_qscale_max( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb )
{
    trp_vid_t *vid;
    uns32b res, typ, tosub_a, tosub_b;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( ttyp, &typ ) ||
         trp_cast_uns32b( tosa, &tosub_a ) ||
         trp_cast_uns32b( tosb, &tosub_b ) )
        return UNDEF;
    if ( typ >= 7 )
        return UNDEF;
    res = vid->cnt_qscale_max[ typ ];
    if ( tosub_a || tosub_b )
        if ( tosub_a + tosub_b >= vid->cnt_vop ) {
            res = 0;
        } else {
            uns32b i;

            res = 0;
            for ( i = tosub_a ; i < vid->cnt_vop - tosub_b ; i++ )
                if ( ( vid->qscale[ i ].typ == typ ) &&
                     ( vid->qscale[ i ].qscale > res ) )
                    res = vid->qscale[ i ].qscale;
        }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_cnt_qscale_avg( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb )
{
    trp_vid_t *vid;
    sig32b res;
    uns32b typ, tosub_a, tosub_b;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( ttyp, &typ ) ||
         trp_cast_uns32b( tosa, &tosub_a ) ||
         trp_cast_uns32b( tosb, &tosub_b ) )
        return UNDEF;
    if ( typ >= 7 )
        return UNDEF;
    res = vid->cnt_qscale_avg[ typ ];
    if ( tosub_a || tosub_b )
        if ( tosub_a + tosub_b >= vid->cnt_vop ) {
            res = 0;
        } else {
            uns32b i;

            for ( i = 0 ; i < tosub_a ; i++ )
                if ( vid->qscale[ i ].typ == typ )
                    res -= vid->qscale[ i ].qscale;
            for ( i = vid->cnt_vop - tosub_b ; i < vid->cnt_vop ; i++ )
                if ( vid->qscale[ i ].typ == typ )
                    res -= vid->qscale[ i ].qscale;
        }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_cnt_qscale_var( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb )
{
    trp_vid_t *vid;
    uns32b res, typ, tosub_a, tosub_b;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( ttyp, &typ ) ||
         trp_cast_uns32b( tosa, &tosub_a ) ||
         trp_cast_uns32b( tosb, &tosub_b ) )
        return UNDEF;
    if ( typ >= 7 )
        return UNDEF;
    res = vid->cnt_qscale_var[ typ ];
    if ( tosub_a || tosub_b )
        if ( tosub_a + tosub_b >= vid->cnt_vop ) {
            res = 0;
        } else {
            uns32b i, j;

            for ( i = 0 ; i < tosub_a ; i++ )
                if ( vid->qscale[ i ].typ == typ ) {
                    j = vid->qscale[ i ].qscale;
                    res -= ( j * j );
                }
            for ( i = vid->cnt_vop - tosub_b ; i < vid->cnt_vop ; i++ )
                if ( vid->qscale[ i ].typ == typ ) {
                    j = vid->qscale[ i ].qscale;
                    res -= ( j * j );
                }
        }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_par( trp_obj_t *obj, trp_obj_t *sps_idx )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->bitstream_type == 3 ) {
        uns32b idx;

        if ( sps_idx == NULL )
            return UNDEF;
        if ( trp_cast_uns32b( sps_idx, &idx ) )
            return UNDEF;
        if ( idx >= vid->sps_cnt )
            return UNDEF;
        return trp_sig64( vid->sps[ idx ]->vui_seq_parameters_aspect_ratio_idc );
    }
    return trp_sig64( vid->par );
}

trp_obj_t *trp_vid_par_w( trp_obj_t *obj, trp_obj_t *sps_idx )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->bitstream_type == 3 ) {
        uns32b idx;

        if ( sps_idx == NULL )
            return UNDEF;
        if ( trp_cast_uns32b( sps_idx, &idx ) )
            return UNDEF;
        if ( idx >= vid->sps_cnt )
            return UNDEF;
        idx = vid->sps[ idx ]->vui_seq_parameters_sar_width;
        return idx ? trp_sig64( idx ) : UNDEF;
    }
    return trp_sig64( vid->par_w );
}

trp_obj_t *trp_vid_par_h( trp_obj_t *obj, trp_obj_t *sps_idx )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->bitstream_type == 3 ) {
        uns32b idx;

        if ( sps_idx == NULL )
            return UNDEF;
        if ( trp_cast_uns32b( sps_idx, &idx ) )
            return UNDEF;
        if ( idx >= vid->sps_cnt )
            return UNDEF;
        idx = vid->sps[ idx ]->vui_seq_parameters_sar_height;
        return idx ? trp_sig64( idx ) : UNDEF;
    }
    return trp_sig64( vid->par_h );
}

trp_obj_t *trp_vid_packed( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->packed == 2 )
        return UNDEF;
    return vid->packed ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_interlaced( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return vid->interlaced ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_tff( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->tff == 2 )
        return UNDEF;
    return vid->tff ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_alternate_scan( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( vid->alternate_scan == 2 )
        return UNDEF;
    return vid->alternate_scan ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_sprite_enable( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->sprite_enable );
}

trp_obj_t *trp_vid_sprite_warping_points( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->sprite_warping_points );
}

trp_obj_t *trp_vid_mpeg_quant( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return vid->mpeg_quant ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_quant_matrix( trp_obj_t *obj, trp_obj_t *nmtx, trp_obj_t *idx, trp_obj_t *sps_idx )
{
    trp_vid_t *vid;
    uns32b nmatrix, i;
    uns8b *mt;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( nmtx, &nmatrix ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( vid->bitstream_type == 3 ) {
        uns32b sidx;
        int sps;

        if ( sps_idx == NULL )
            return UNDEF;
        if ( ( nmatrix >= 24 ) ||
             trp_cast_uns32b( sps_idx, &sidx ) )
            return UNDEF;
        if ( sidx >= vid->sps_cnt )
            return UNDEF;
        /*
         passando un numero di matrice compreso tra 0 e 11
         si accede alle matrici di un sps
         (sps_idx e' un indice di sps;
         passando un numero di matrice compreso tra 12 e 23
         si accede alle matrici di un pps
         (sps_idx e' un indice di pps.
         */
        if ( nmatrix >= 12 ) {
            nmatrix -= 12;
            sps = 0;
        } else {
            sps = 1;
        }
        if ( ( ( nmatrix < 6 ) && ( i >= 16 ) ) ||
             ( i >= 64 ) )
            return UNDEF;
        mt = sps ? vid->sps[ sidx ]->scaling_list[ nmatrix ] :
            vid->pps[ sidx ]->scaling_list[ nmatrix ];
    } else {
        if ( ( nmatrix >= 2 ) ||
             ( i >= 64 ) )
            return UNDEF;
        mt = ( nmatrix == 0 ) ? vid->intra_quant_matrix : vid->inter_quant_matrix;
    }
    if ( mt == NULL )
        return UNDEF;
    return trp_sig64( mt[ i ] );
}

trp_obj_t *trp_vid_qpel( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return vid->qpel ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_max_bframes( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->max_bframes );
}

trp_obj_t *trp_vid_cnt_bframes( trp_obj_t *obj, trp_obj_t *n )
{
    trp_vid_t *vid;
    uns32b nn;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( n, &nn ) )
        return UNDEF;
    if ( nn > MAX_BFRAMES )
        return UNDEF;
    return trp_sig64( vid->cnt_bframes[ nn ] );
}

trp_obj_t *trp_vid_cnt_warp_points_used( trp_obj_t *obj, trp_obj_t *points )
{
    trp_vid_t *vid;
    uns32b pnts;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( points, &pnts ) )
        return UNDEF;
    if ( pnts > MAX_WARPING_POINTS )
        return UNDEF;
    return trp_sig64( vid->cnt_warp_points_used[ pnts ] );
}

trp_obj_t *trp_vid_time_inc_bits( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->time_inc_bits );
}

trp_obj_t *trp_vid_sps_cnt( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->sps_cnt );
}

trp_obj_t *trp_vid_sps_id( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->sps_cnt )
        return UNDEF;
    return trp_sig64( vid->sps[ i ]->seq_parameter_set_id );
}

trp_obj_t *trp_vid_pps_cnt( trp_obj_t *obj )
{
    trp_vid_t *vid;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    return trp_sig64( vid->pps_cnt );
}

trp_obj_t *trp_vid_pps_id( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return trp_sig64( vid->pps[ i ]->pic_parameter_set_id );
}

trp_obj_t *trp_vid_pps_sps_id( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return trp_sig64( vid->pps[ i ]->seq_parameter_set_id );
}

trp_obj_t *trp_vid_num_ref_frames( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->sps_cnt )
        return UNDEF;
    return trp_sig64( vid->sps[ i ]->num_ref_frames );
}

trp_obj_t *trp_vid_profile_idc( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->sps_cnt )
        return UNDEF;
    return trp_sig64( vid->sps[ i ]->profile_idc );
}

trp_obj_t *trp_vid_level_idc( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->sps_cnt )
        return UNDEF;
    return trp_sig64( vid->sps[ i ]->level_idc );
}

trp_obj_t *trp_vid_chroma_format_idc( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->sps_cnt )
        return UNDEF;
    return trp_sig64( vid->sps[ i ]->chroma_format_idc );
}

trp_obj_t *trp_vid_entropy_coding_mode( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return vid->pps[ i ]->entropy_coding_mode_flag ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_weighted_pred( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return vid->pps[ i ]->weighted_pred_flag ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_vid_weighted_bipred_idc( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return trp_sig64( vid->pps[ i ]->weighted_bipred_idc );
}

trp_obj_t *trp_vid_transform_8x8_mode_flag( trp_obj_t *obj, trp_obj_t *idx )
{
    trp_vid_t *vid;
    uns32b i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( idx, &i ) )
        return UNDEF;
    if ( i >= vid->pps_cnt )
        return UNDEF;
    return vid->pps[ i ]->transform_8x8_mode_flag ? TRP_TRUE : TRP_FALSE;
}

