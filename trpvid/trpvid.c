/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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

static uns8b trp_vid_print( trp_print_t *p, trp_vid_t *obj );
static uns8b trp_vid_close_basic( uns8b flags, trp_vid_t *obj );
static void trp_vid_finalize( void *obj, void *data );
static trp_obj_t *trp_vid_length( trp_vid_t *vid );
static trp_obj_t *trp_vid_width( trp_vid_t *vid );
static trp_obj_t *trp_vid_height( trp_vid_t *vid );
static uns8b trp_vid_parse_internal( trp_vid_t *vid, trp_obj_t *size, trp_raw_t *stripped, uns8b matroska );

uns8b _ZZ_SCAN4[] = {
     0,  1,  4,  8,
     5,  2,  3,  6,
     9, 12, 13, 10,
     7, 11, 14, 15
};

uns8b _ZZ_SCAN8[] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

uns8b trp_vid_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_VID ] = trp_vid_print;
    _trp_close_fun[ TRP_VID ] = trp_vid_close;
    _trp_length_fun[ TRP_VID ] = trp_vid_length;
    _trp_width_fun[ TRP_VID ] = trp_vid_width;
    _trp_height_fun[ TRP_VID ] = trp_vid_height;
    return 0;
}

static uns8b trp_vid_print( trp_print_t *p, trp_vid_t *obj )
{
    if ( trp_print_char_star( p, "#vid" ) )
        return 1;
    if ( obj->fp == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

uns8b trp_vid_close( trp_vid_t *obj )
{
    return trp_vid_close_basic( 1, obj );
}

static uns8b trp_vid_close_basic( uns8b flags, trp_vid_t *obj )
{
    if ( obj->fp ) {
        uns32b i, j;

        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->buf );
        trp_gc_free( obj->intra_quant_matrix );
        trp_gc_free( obj->inter_quant_matrix );
        trp_gc_free( obj->matroska_codec_private );
        free( obj->qscale );
        for ( i = 0 ; i < obj->userdata_cnt ; i++ )
            free( obj->userdata[ i ] );
        free( obj->userdata );
        for ( i = 0 ; i < obj->sps_cnt ; i++ ) {
            for ( j = 0 ; j < 12 ; j++ )
                trp_gc_free( obj->sps[ i ]->scaling_list[ j ] );
            trp_gc_free( obj->sps[ i ] );
        }
        trp_gc_free( obj->sps );
        for ( i = 0 ; i < obj->pps_cnt ; i++ ) {
            for ( j = 0 ; j < 12 ; j++ )
                trp_gc_free( obj->pps[ i ]->scaling_list[ j ] );
            trp_gc_free( obj->pps[ i ] );
        }
        trp_gc_free( obj->pps );
        free( obj->mp4_sample_size );
        free( obj->mp4_sample_to_chunk );
        free( obj->mp4_chunk_offset );
        memset( obj, 0, sizeof( trp_vid_t ) );
        obj->tipo = TRP_VID;
    }
    return 0;
}

static void trp_vid_finalize( void *obj, void *data )
{
    trp_vid_close_basic( 0, (trp_vid_t *)obj );
}

static trp_obj_t *trp_vid_length( trp_vid_t *vid )
{
    return vid->fp ? trp_sig64( vid->cnt ) : UNDEF;
}

static trp_obj_t *trp_vid_width( trp_vid_t *vid )
{
    return ( vid->fp && vid->width ) ? trp_sig64( vid->width ) : UNDEF;
}

static trp_obj_t *trp_vid_height( trp_vid_t *vid )
{
    return ( vid->fp && vid->height ) ? trp_sig64( vid->height ) : UNDEF;
}

trp_obj_t *trp_vid_create( trp_obj_t *f )
{
    trp_vid_t *vid;
    FILE *fp;

    if ( ( fp = trp_file_readable_fp( f ) ) == NULL )
        return UNDEF;
    vid = trp_gc_malloc_finalize( sizeof( trp_vid_t ), trp_vid_finalize );
    memset( vid, 0, sizeof( trp_vid_t ) );
    vid->tipo = TRP_VID;
    vid->fp = fp;
    vid->packed = 2;
    vid->tff = 2;
    vid->alternate_scan = 2;
    vid->max_frame_size = -1;
    return (trp_obj_t *)vid;
}

uns32b trp_vid_effective_qscale( uns32b qscale, sig8b bitstream_type )
{
    uns32b maxq = ( bitstream_type == 3 ) ? MAX_QSCALE_AVC : MAX_QSCALE_ASP;
    return ( qscale <= maxq ) ? qscale : maxq;
}

void trp_vid_update_qscale( trp_vid_t *vid, sig8b bitstream_type, uns32b typ, uns32b qscale )
{
    vid->cnt_qscale[ trp_vid_effective_qscale( qscale, bitstream_type ) ][ typ ]++;
    vid->cnt_qscale_cnt[ typ ]++;
    if ( qscale > vid->cnt_qscale_max[ typ ] )
        vid->cnt_qscale_max[ typ ] = qscale;
    vid->cnt_qscale_avg[ typ ] += qscale;
    vid->cnt_qscale_var[ typ ] += qscale * qscale;
    if ( vid->cnt_vop % 16384 == 0 )
        vid->qscale = (frameinfo_t *)trp_realloc( vid->qscale,
                                                  sizeof( frameinfo_t ) *
                                                  ( vid->cnt_vop + 16384 ) );
    vid->qscale[ vid->cnt_vop ].size = vid->tmp_frame_size - vid->tmp_frame_pos;
    vid->qscale[ vid->cnt_vop ].typ = typ;
    vid->qscale[ vid->cnt_vop ].qscale = qscale;
    vid->tmp_frame_cnt++;
    if ( vid->tmp_frame_cnt > 1 )
        vid->qscale[ vid->cnt_vop - 1 ].size -= ( vid->tmp_frame_size - vid->tmp_frame_pos );
#if 0
    if ( ( bitstream_type != 3 ) && ( typ == 2 ) && vid->cnt_vop ) {
        if ( vid->qscale[ vid->cnt_vop - 1 ].typ != 2 ) {
            vid->qscale[ vid->cnt_vop ].size = vid->qscale[ vid->cnt_vop - 1 ].size;
            vid->qscale[ vid->cnt_vop ].typ = vid->qscale[ vid->cnt_vop - 1 ].typ;
            vid->qscale[ vid->cnt_vop ].qscale = vid->qscale[ vid->cnt_vop - 1 ].qscale;
            vid->qscale[ vid->cnt_vop - 1 ].size = vid->tmp_frame_size - vid->tmp_frame_pos;
            vid->qscale[ vid->cnt_vop - 1 ].typ = typ;
            vid->qscale[ vid->cnt_vop - 1 ].qscale = qscale;
        }
    }
#endif
    vid->max_frame_size = -1;
    vid->cnt_vop++;
}

void trp_vid_store_userdata( trp_vid_t *vid, uns8b *src, uns32b size )
{
    uns32b i;
    uns8b *nub;
    int e, ver, build;
    char last;

    if ( size == 0 )
        return;
    for ( i = 0 ; i < vid->userdata_cnt ; i++ )
        if ( strncmp( vid->userdata[ i ], src, size ) == 0 )
            return;
    vid->userdata = (uns8b **)trp_realloc( vid->userdata, ( vid->userdata_cnt + 1 ) * sizeof( uns8b * ) );
    nub = trp_malloc( size + 1 );
    memcpy( nub, src, size );
    nub[ size ] = 0;
    vid->userdata[ vid->userdata_cnt++ ] = nub;

    vid->divx_version = 0;
    vid->divx_build = 0;

    /* divx detection */
    e = sscanf( nub, "DivX%dBuild%d%c", &ver, &build, &last );
    if ( e < 2 )
        e = sscanf( nub, "DivX%db%d%c", &ver, &build, &last );
    if( e >= 2 ) {
        vid->divx_version = ver;
        vid->divx_build = build;
    }
}

void trp_vid_calculate_max_avg_frame_size( trp_vid_t *vid )
{
    if ( vid->max_frame_size == -1 ) {
        uns32b i, cnt = 0;
        sig32b size;
        uns64b tot = 0;

        for ( i = 0 ; i < vid->cnt_vop ; i++ ) {
            if ( size = vid->qscale[ i ].size ) {
                cnt++;
                tot += size;
            }
            if ( vid->max_frame_size < size )
                vid->max_frame_size = size;
        }
        vid->avg_frame_size = ( tot + cnt / 2 ) / cnt;
    }
}

uns8b trp_vid_check( trp_obj_t *obj, trp_vid_t **vid )
{
    if ( obj->tipo != TRP_VID )
        return 1;
    *vid = (trp_vid_t *)obj;
    return (*vid)->fp ? 0 : 1;
}

uns8b trp_vid_parse( trp_obj_t *vid, trp_obj_t *size, trp_obj_t *stripped )
{
    if ( stripped )
        if ( stripped->tipo != TRP_RAW )
            return 1;
    return trp_vid_parse_internal( (trp_vid_t *)vid, size, (trp_raw_t *)stripped, 0 );
}

uns8b trp_vid_parse_matroska( trp_obj_t *vid, trp_obj_t *size )
{
    return trp_vid_parse_internal( (trp_vid_t *)vid, size, (trp_raw_t *)NULL, 1 );
}

static uns8b trp_vid_parse_internal( trp_vid_t *vid, trp_obj_t *size, trp_raw_t *stripped, uns8b matroska )
{
    uns32b ssize, original_size, ll;
    uns8b res = 1;

    if ( ( vid->tipo != TRP_VID ) ||
         trp_cast_uns32b( size, &ssize ) )
        return 1;
    if ( ( vid->fp == NULL ) ||
         ( vid->bitstream_type == -1 ) )
        return 1;
    if ( matroska == 0 )
        vid->cnt++;
    if ( ssize == 0 ) {
        trp_vid_update_qscale( vid, vid->bitstream_type, 5, 0 );
        return 0;
    }
    if ( stripped )
        ll = stripped->len;
    else
        ll = 0;
    vid->tmp_frame_size = vid->tmp_frame_pos = vid->tmp_frame_cnt = 0;
    original_size = ssize + ll;
    /*
     ottimizzazione per velocizzare l'analisi dei DivX 3.11
     */
    if ( vid->bitstream_type == 2 )
        ssize = 1;
    if ( ssize + ll > vid->buf_alloc ) {
        vid->buf = trp_realloc( vid->buf, ssize + ll );
        vid->buf_alloc = ssize + ll;
    }
    if ( ll )
        memcpy( vid->buf, stripped->data, ll );
    if ( fread( vid->buf + ll, ssize, 1, vid->fp ) != 1 ) {
        vid->error = "Errore di lettura";
        return 1;
    }
    if ( ( original_size - ll == 1 ) && ( vid->buf[ 0 ] == 0x7f ) ) {
        trp_vid_update_qscale( vid, vid->bitstream_type, 5, 0 );
        return 0;
    }
    ssize += ll;
    vid->buf_size = ssize;
    vid->buf_pos = 0;
    vid->tmp_frame_size = original_size;
    switch ( vid->bitstream_type ) {
    case 0:
        vid->matroska = matroska;
        res = trp_vid_parse_mpeg4asp( vid );
        if ( res == 0 ) {
            vid->bitstream_type = 1;
        } else {
            vid->buf_size = ssize;
            vid->buf_pos = 0;
            res = trp_vid_parse_mpeg4avc( vid );
            if ( res == 0 ) {
                vid->bitstream_type = 3;
                vid->error = NULL;
            } else {
                vid->buf_size = ssize;
                vid->buf_pos = 0;
                res = trp_vid_parse_msmpeg4( vid );
                if ( res == 0 ) {
                    vid->bitstream_type = 2;
                    vid->error = NULL;
                    vid->par = 1;
                    vid->packed = 0;
                } else {
                    vid->error = "unknown bitstream";
                    vid->bitstream_type = -1;
                }
            }
        }
        break;
    case 1:
        vid->matroska = 0;
        res = trp_vid_parse_mpeg4asp( vid );
        break;
    case 2:
        res = trp_vid_parse_msmpeg4( vid );
        break;
    case 3:
        vid->matroska = 0;
        res = trp_vid_parse_mpeg4avc( vid );
        break;
    }
    return res;
}



