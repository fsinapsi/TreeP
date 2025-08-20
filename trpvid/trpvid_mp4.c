/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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

#ifdef TRP_LITTLE_ENDIAN
#define mp4norm16(n) trp_swap_endian16((uns16b)(n))
#define mp4norm32(n) trp_swap_endian32((uns32b)(n))
#define mp4norm64(n) trp_swap_endian64((uns64b)(n))
#else /* TRP_LITTLE_ENDIAN */
#define mp4norm16(n) ((uns16b)(n))
#define mp4norm32(n) ((uns32b)(n))
#define mp4norm64(n) ((uns64b)(n))
#endif /* TRP_LITTLE_ENDIAN */

uns8b trp_vid_mp4_load_sample_size( trp_obj_t *obj, trp_obj_t *s_size, trp_obj_t *s_cnt, trp_obj_t *compact )
{
    trp_vid_t *vid;
    uns32b sample_size, sample_cnt, i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( s_size, &sample_size ) ||
         trp_cast_uns32b( s_cnt, &sample_cnt ) ||
         ( !TRP_BOOLP( compact ) ) )
        return 1;
    if ( ( sample_cnt == 0 ) ||
         ( vid->mp4_sample_size ) )
        return 1;
    vid->mp4_sample_size = trp_malloc( 4 * sample_cnt );
    if ( compact == TRP_FALSE ) {
        if ( sample_size ) {
            for ( i = 0 ; i < sample_cnt ; i++ )
                vid->mp4_sample_size[ i ] = sample_size;
        } else {
            if ( fread( vid->mp4_sample_size, 4, sample_cnt, vid->fp ) != sample_cnt ) {
                free( vid->mp4_sample_size );
                vid->mp4_sample_size = NULL;
                return 1;
            }
            for ( i = 0 ; i < sample_cnt ; i++ )
                vid->mp4_sample_size[ i ] = mp4norm32( vid->mp4_sample_size[ i ] );
        }
    } else {
        uns8b *tmp1, *p;
        uns16b *tmp2;

        /*
         in questo caso, sample_size e' field_size, e deve valere uno tra: 4, 8, 16
         */
        if ( ( sample_size != 4 ) && ( sample_size != 8 ) && ( sample_size != 16 ) ) {
            free( vid->mp4_sample_size );
            vid->mp4_sample_size = NULL;
            return 1;
        }
        i = ( sample_size * sample_cnt + 4 ) / 8;
        tmp1 = trp_gc_malloc( i );
        if ( fread( tmp1, 1, i, vid->fp ) != i ) {
            trp_gc_free( tmp1 );
            free( vid->mp4_sample_size );
            vid->mp4_sample_size = NULL;
            return 1;
        }
        switch ( sample_size ) {
        case 4:
            for ( p = tmp1, i = 0 ; i < sample_cnt ; i++ ) {
                if ( ( i % 2 ) == 0 ) {
                    vid->mp4_sample_size[ i ] = ( *p ) >> 4;
                } else {
                    vid->mp4_sample_size[ i ] = ( *p ) & 15;
                    p++;
                }
            }
            break;
        case 8:
            for ( i = 0 ; i < sample_cnt ; i++ )
                vid->mp4_sample_size[ i ] = tmp1[ i ];
            break;
        case 16:
            for ( tmp2 = (uns16b *)tmp1, i = 0 ; i < sample_cnt ; i++ )
                vid->mp4_sample_size[ i ] = mp4norm16( tmp2[ i ] );
            break;
        }
        trp_gc_free( tmp1 );
    }
    vid->mp4_sample_cnt = sample_cnt;
    return 0;
}

uns8b trp_vid_mp4_load_sample_to_chunk( trp_obj_t *obj, trp_obj_t *e_cnt )
{
    trp_vid_t *vid;
    uns32b entry_cnt, i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( e_cnt, &entry_cnt ) )
        return 1;
    if ( ( entry_cnt == 0 ) ||
         ( vid->mp4_sample_to_chunk ) )
        return 1;
    vid->mp4_sample_to_chunk = trp_malloc( 12 * entry_cnt );
    if ( fread( vid->mp4_sample_to_chunk, 12, entry_cnt, vid->fp ) != entry_cnt ) {
        free( vid->mp4_sample_to_chunk );
        vid->mp4_sample_to_chunk = NULL;
        return 1;
    }
    for ( i = 0 ; i < entry_cnt ; i++ ) {
        vid->mp4_sample_to_chunk[ i ].first_chunk = mp4norm32( vid->mp4_sample_to_chunk[ i ].first_chunk );
        vid->mp4_sample_to_chunk[ i ].samples_per_chunk = mp4norm32( vid->mp4_sample_to_chunk[ i ].samples_per_chunk );
        vid->mp4_sample_to_chunk[ i ].sample_description_index = mp4norm32( vid->mp4_sample_to_chunk[ i ].sample_description_index );
    }
    vid->mp4_entry_cnt = entry_cnt;
    return 0;
}

uns8b trp_vid_mp4_load_chunk_offset( trp_obj_t *obj, trp_obj_t *c_cnt, trp_obj_t *f_size )
{
    trp_vid_t *vid;
    uns32b *tmp;
    uns32b chunk_cnt, field_size, i;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( c_cnt, &chunk_cnt ) ||
         trp_cast_uns32b( f_size, &field_size ) )
        return 1;
    if ( ( chunk_cnt == 0 ) ||
         ( vid->mp4_chunk_offset ) ||
         ( ( field_size != 32 ) && ( field_size != 64 ) ) )
        return 1;
    vid->mp4_chunk_offset = trp_malloc( 8 * chunk_cnt );
    if ( field_size == 32 ) {
        tmp = trp_gc_malloc( 4 * chunk_cnt );
        if ( fread( tmp, 4, chunk_cnt, vid->fp ) != chunk_cnt ) {
            trp_gc_free( tmp );
            free( vid->mp4_chunk_offset );
            vid->mp4_chunk_offset = NULL;
            return 1;
        }
        for ( i = 0 ; i < chunk_cnt ; i++ )
            vid->mp4_chunk_offset[ i ] = mp4norm32( tmp[ i ] );
        trp_gc_free( tmp );
    } else { /* field_size == 64 */
        if ( fread( vid->mp4_chunk_offset, 8, chunk_cnt, vid->fp ) != chunk_cnt ) {
            free( vid->mp4_chunk_offset );
            vid->mp4_chunk_offset = NULL;
            return 1;
        }
        for ( i = 0 ; i < chunk_cnt ; i++ )
            vid->mp4_chunk_offset[ i ] = mp4norm64( vid->mp4_chunk_offset[ i ] );
    }
    vid->mp4_chunk_cnt = chunk_cnt;
    return 0;
}

trp_obj_t *trp_vid_mp4_track_size( trp_obj_t *obj )
{
    trp_vid_t *vid;
    uns32b i;
    sig64b tot = 0;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    for ( i = 0 ; i < vid->mp4_sample_cnt ; i++ )
        tot = tot + vid->mp4_sample_size[ i ];
    return trp_sig64( tot );
}

trp_obj_t *trp_vid_mp4_sample_size( trp_obj_t *obj, trp_obj_t *spl )
{
    trp_vid_t *vid;
    uns32b sample;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( spl, &sample ) )
        return UNDEF;
    if ( sample >= vid->mp4_sample_cnt )
        return UNDEF;
    return trp_sig64( vid->mp4_sample_size[ sample ] );
}

trp_obj_t *trp_vid_mp4_sample_offset( trp_obj_t *obj, trp_obj_t *spl )
{
    trp_vid_t *vid;
    uns32b chunk, sample, sample0, entry = 0, cnt;
    sig64b res;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( spl, &sample ) )
        return UNDEF;
    if ( ( sample >= vid->mp4_sample_cnt ) ||
         ( vid->mp4_sample_to_chunk == NULL ) )
        return UNDEF;
    sample0 = sample;
    for ( ; ; entry++ ) {
        if ( entry + 1 == vid->mp4_entry_cnt )
            break;

        cnt = vid->mp4_sample_to_chunk[ entry + 1 ].first_chunk -
            vid->mp4_sample_to_chunk[ entry ].first_chunk;

        if ( sample < cnt * vid->mp4_sample_to_chunk[ entry ].samples_per_chunk )
            break;

        sample -= ( cnt * vid->mp4_sample_to_chunk[ entry ].samples_per_chunk );
    }
    chunk = ( vid->mp4_sample_to_chunk[ entry ].first_chunk +
              sample / vid->mp4_sample_to_chunk[ entry ].samples_per_chunk ) - 1;
    sample = sample % vid->mp4_sample_to_chunk[ entry ].samples_per_chunk;
    sample0 -= sample;

    if ( chunk >= vid->mp4_chunk_cnt )
        return UNDEF;

    res = vid->mp4_chunk_offset[ chunk ];

    for ( ; sample ; sample-- )
        res = res + vid->mp4_sample_size[ sample0++ ];

    return trp_sig64( res );
}

