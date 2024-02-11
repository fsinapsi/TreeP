/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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

#include "../trp/trp.h"
#include "./trpavi.h"
#include "./avilib.h"

typedef struct {
    uns8b tipo;
    avi_t *avi;
} trp_avi_t;

static uns8b trp_avi_print( trp_print_t *p, trp_avi_t *obj );
static uns8b trp_avi_close( trp_avi_t *obj );
static uns8b trp_avi_close_basic( uns8b flags, trp_avi_t *obj );
static void trp_avi_finalize( void *obj, void *data );
static trp_obj_t *trp_avi_width( trp_avi_t *obj );
static trp_obj_t *trp_avi_height( trp_avi_t *obj );
static avi_t *trp_avi_get( trp_obj_t *obj );

uns8b trp_avi_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_AVI ] = trp_avi_print;
    _trp_close_fun[ TRP_AVI ] = trp_avi_close;
    _trp_width_fun[ TRP_AVI ] = trp_avi_width;
    _trp_height_fun[ TRP_AVI ] = trp_avi_height;
    return 0;
}

static uns8b trp_avi_print( trp_print_t *p, trp_avi_t *obj )
{
    if ( trp_print_char_star( p, "#avi" ) )
        return 1;
    if ( obj->avi == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_avi_close( trp_avi_t *obj )
{
    return trp_avi_close_basic( 1, obj );
}

static uns8b trp_avi_close_basic( uns8b flags, trp_avi_t *obj )
{
    if ( obj->avi ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        AVI_close( obj->avi );
        obj->avi = NULL;
    }
    return 0;
}

static void trp_avi_finalize( void *obj, void *data )
{
    trp_avi_close_basic( 0, (trp_avi_t *)obj );
}

static trp_obj_t *trp_avi_width( trp_avi_t *obj )
{
    return obj->avi ? trp_sig64( obj->avi->width ) : UNDEF;
}

static trp_obj_t *trp_avi_height( trp_avi_t *obj )
{
    return obj->avi ? trp_sig64( obj->avi->height ) : UNDEF;
}

static avi_t *trp_avi_get( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_AVI )
        return NULL;
    return ((trp_avi_t *)obj)->avi;
}

trp_obj_t *trp_avi_open_input_file( trp_obj_t *path, trp_obj_t *getindex )
{
    trp_avi_t *obj;
    uns8b *p;
    avi_t *avi;

    if ( getindex == NULL )
        getindex = TRP_TRUE;
    if ( !TRP_BOOLP( getindex ) )
        return UNDEF;
    p = trp_csprint( path );
    avi = AVI_open_input_file( p, ( getindex == TRP_TRUE ) ? 1 : 0 );
    trp_csprint_free( p );
    if ( avi == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avi_t ), trp_avi_finalize );
    obj->tipo = TRP_AVI;
    obj->avi = avi;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_avi_has_index( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );
    if ( avi == NULL )
        return TRP_FALSE;
    return avi->video_index ? TRP_TRUE : TRP_FALSE;
}

uns8b trp_avi_set_audio_track( trp_obj_t *obj, trp_obj_t *n )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b nn;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( n, &nn ) )
        return 1;
    if ( nn >= avi->anum )
        return 1;
    avi->aptr = nn;
    return 0;
}

trp_obj_t *trp_avi_audio_tracks( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->anum );
}

trp_obj_t *trp_avi_audio_format( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].a_fmt );
}

trp_obj_t *trp_avi_audio_delay( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_math_ratio( trp_sig64( (sig64b)(avi->track[avi->aptr].a_start) *
                                      (sig64b)(avi->track[avi->aptr].a_scale) ),
                           trp_sig64( avi->track[avi->aptr].padrate ),
                           NULL );
}

trp_obj_t *trp_avi_audio_chunks( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].audio_chunks );
}

trp_obj_t *trp_avi_audio_fpos( trp_obj_t *obj, trp_obj_t *chunk )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( chunk, &fno ) )
        return UNDEF;
    if ( fno >= avi->track[avi->aptr].audio_chunks )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].audio_index[fno].pos );
}

trp_obj_t *trp_avi_audio_size( trp_obj_t *obj, trp_obj_t *chunk )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( chunk, &fno ) )
        return UNDEF;
    if ( fno >= avi->track[avi->aptr].audio_chunks )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].audio_index[fno].len );
}

trp_obj_t *trp_avi_audio_streamsize( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].audio_bytes );
}

trp_obj_t *trp_avi_audio_channels( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].a_chans );
}

trp_obj_t *trp_avi_audio_frequency( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].a_rate );
}

trp_obj_t *trp_avi_audio_vbr( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return avi->track[avi->aptr].a_vbr ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_avi_audio_samplerate( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_math_ratio( trp_sig64( avi->track[avi->aptr].padrate ),
                           trp_sig64( avi->track[avi->aptr].a_scale ),
                           NULL );
}

trp_obj_t *trp_avi_audio_mp3rate( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].mp3rate );
}

trp_obj_t *trp_avi_audio_padrate( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->track[avi->aptr].padrate );
}

trp_obj_t *trp_avi_video_compressor( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    if ( avi->compressor2 == NULL )
        return UNDEF;
    return trp_cord( avi->compressor2 );
}

trp_obj_t *trp_avi_video_delay( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_math_ratio( trp_sig64( (sig64b)(avi->v_start) *
                                      (sig64b)(avi->v_scale) ),
                           trp_sig64( avi->v_rate ),
                           NULL );
}

trp_obj_t *trp_avi_video_frames( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->video_frames );
}

trp_obj_t *trp_avi_video_keyframes( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( AVI_keyframes( avi, NULL, NULL ) );
}

trp_obj_t *trp_avi_video_fpos( trp_obj_t *obj, trp_obj_t *frame )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( frame, &fno ) )
        return UNDEF;
    if ( fno >= avi->video_frames )
        return UNDEF;
    return trp_sig64( avi->video_index[fno].pos );
}

trp_obj_t *trp_avi_video_size( trp_obj_t *obj, trp_obj_t *frame )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( frame, &fno ) )
        return UNDEF;
    if ( fno >= avi->video_frames )
        return UNDEF;
    return trp_sig64( avi->video_index[fno].len );
}

trp_obj_t *trp_avi_video_max_size( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno, m;

    if ( avi == NULL )
        return UNDEF;
    for ( fno = 0, m = 0 ; fno < avi->video_frames ; fno++ )
        if ( avi->video_index[fno].len > m )
            m = avi->video_index[fno].len;
    return trp_sig64( m );
}

trp_obj_t *trp_avi_video_streamsize( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_sig64( avi->v_streamsize );
}

trp_obj_t *trp_avi_video_framerate( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_math_ratio( trp_sig64( avi->v_rate ),
                           trp_sig64( avi->v_scale ),
                           NULL );
}

trp_obj_t *trp_avi_video_bitrate( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );

    if ( avi == NULL )
        return UNDEF;
    return trp_math_times( trp_math_ratio( trp_sig64( avi->v_streamsize ),
                                           trp_sig64( avi->video_frames ),
                                           trp_sig64( avi->v_scale ),
                                           trp_sig64( 125 ),
                                           NULL ),
                           trp_sig64( avi->v_rate ),
                           NULL );
}

trp_obj_t *trp_avi_video_min_keyint( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );
    unsigned long i;

    if ( avi == NULL )
        return UNDEF;
    if ( AVI_keyframes( avi, &i, NULL ) < 0 )
        return UNDEF;
    return trp_sig64( (sig64b)i );
}

trp_obj_t *trp_avi_video_max_keyint( trp_obj_t *obj )
{
    avi_t *avi = trp_avi_get( obj );
    unsigned long i;

    if ( avi == NULL )
        return UNDEF;
    if ( AVI_keyframes( avi, NULL, &i ) < 0 )
        return UNDEF;
    return trp_sig64( (sig64b)i );
}

trp_obj_t *trp_avi_video_frame_is_keyframe( trp_obj_t *obj, trp_obj_t *frame )
{
    avi_t *avi = trp_avi_get( obj );
    uns32b fno;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( frame, &fno ) )
        return UNDEF;
    if ( fno >= avi->video_frames )
        return UNDEF;
    return ( avi->video_index[fno].key == 0x10 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_avi_video_read( trp_obj_t *obj, trp_obj_t *frame )
{
    return UNDEF;
#if 0
    avi_t *avi = trp_avi_get( obj );
    trp_raw_t *raw;
    char *vidbuf = NULL;
    uns32b fno;
    long bytes;
    int keyframe;

    if ( ( avi == NULL ) ||
         trp_cast_uns32b( frame, &fno ) )
        return UNDEF;
    if ( AVI_read_video( avi, &vidbuf, &bytes, &keyframe, fno ) == -1 )
        return UNDEF;
    raw = trp_gc_malloc( sizeof( trp_raw_t ) );
    raw->tipo = TRP_RAW;
    raw->mode = 0;
    raw->unc_tipo = 0;
    raw->compression_level = 0;
    raw->len = bytes;
    raw->unc_len = 0;
    raw->data = vidbuf;
    return (trp_obj_t *)raw;
#endif
}

uns8b trp_avi_video_read_test( trp_obj_t *obj, trp_obj_t *raw, trp_obj_t *frame )
{
    return 1;

#if 0
    avi_t *avi = trp_avi_get( obj );
    char *vidbuf;
    uns32b fno;
    long bytes;
    int keyframe;

    if ( ( avi == NULL ) ||
         ( raw->tipo != TRP_RAW ) ||
         trp_cast_uns32b( frame, &fno ) )
        return 1;
    if ( ( vidbuf = ((trp_raw_t *)raw)->data ) == NULL )
        return 1;
    bytes = ((trp_raw_t *)raw)->len;
    return ( AVI_read_video( avi, &vidbuf, &bytes, &keyframe, fno ) == -1 ) ? 1 : 0;
#endif
}

trp_obj_t *trp_avi_parse_junk( trp_obj_t *obj, trp_obj_t *size )
{
    FILE *fp;
    trp_obj_t *res = UNDEF;
    uns32b ssize;
    uns32b n, k, fail, a, b, last;
    uns8b *buf, *p;

    if ( ( ( fp = trp_file_readable_fp( (trp_obj_t *)obj ) ) == NULL ) ||
         trp_cast_uns32b( size, &ssize ) )
        return UNDEF;
    if ( ssize == 0 )
        return UNDEF;

    if ( ( buf = malloc( ssize ) ) == NULL )
        return UNDEF;

    if ( fread( buf, ssize, 1, fp ) != 1 ) {
        free( buf );
        return UNDEF;
    }

    for ( n = 2 ; n <= ssize / 2 ; n++ ) {
        for ( k = 1, p = buf, fail = 0 ; k < ssize / n - 1 ; k++ ) {
            p += n;
            if ( strncmp( buf, p, n ) ) {
                fail = 1;
                break;
            }
            if ( k == 4 )
                break;
        }
        if ( !fail ) {
            a = 0;
            b = 0;
            last = 0xffffffff;
            for ( k = 0, p = buf ; k < n ; k++, p++ ) {
                if ( *p == 0 )
                    *p = ' ';
                else {
                    if ( ( *p < ' ' ) || ( ( *p >= 127 ) && ( *p <= 160 ) ) ) {
                        /*
                         forziamo l'uscita e ignoriamo i dati
                         */
                        a = 0;
                        b = 1;
                        break;
                    }
                    b++;
                    if ( ( *p >= ' ' ) && ( *p < 127 ) )
                        a++;
                }
                if ( *p != ' ' )
                    last = k;
            }
            /*
             rendiamo il risultato solo se il numero di
             caratteri stampabili e' almeno il 50% del totale
             */
            if ( ( a * 2 >= b ) && ( last != 0xffffffff ) ) {
                buf[ last + 1 ] = 0;
                res = trp_cord( buf );
            }
            break;
        }
    }
    free( buf );
    return res;
}

