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

#include "../trp/trp.h"
#include "./trpjbig2.h"
#include "../trppix/trppix_internal.h"
#include <stdint.h>
#include <jbig2.h>

static void trp_jbig2_error_callback( void *error_callback_data, const char *message, Jbig2Severity severity, uint32_t seg_idx );
static uns8b trp_pix_load_jbig2_low( Jbig2Options opt, uns8b *cpath, uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_jbig2( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_jbig2_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_jbig2_init()
{
    extern uns8bfun_t _trp_pix_load_jbig2;
    extern uns8bfun_t _trp_pix_load_jbig2_memory;

    _trp_pix_load_jbig2 = trp_pix_load_jbig2;
    _trp_pix_load_jbig2_memory = trp_pix_load_jbig2_memory;
    return 0;
}

static void trp_jbig2_error_callback( void *error_callback_data, const char *message, Jbig2Severity severity, uint32_t seg_idx )
{
}

static uns8b trp_pix_load_jbig2_low( Jbig2Options opt, uns8b *cpath, uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    Jbig2Ctx *ctx;
    Jbig2Image *image;
    uns32b *d;
    uns8b *p, *q;
    uns32b i, j;
    int cnt;

    if ( ( ctx = jbig2_ctx_new( NULL, opt, NULL, trp_jbig2_error_callback, NULL ) ) == NULL )
        return 1;
    if ( cpath ) {
        FILE *fp;
        uns8b buf[ 4096 ];

        if ( ( fp = trp_fopen( cpath, "rb" ) ) == NULL ) {
            jbig2_ctx_free( ctx );
            return 1;
        }
        while ( ( cnt = fread( buf, 1, sizeof( buf ), fp ) ) > 0 )
            if ( jbig2_data_in( ctx, buf, (size_t)cnt ) < 0 ) {
                fclose( fp );
                jbig2_ctx_free( ctx );
                return 1;
            }
        fclose( fp );
    } else {
        if ( jbig2_data_in( ctx, idata, (size_t)isize ) < 0 ) {
            jbig2_ctx_free( ctx );
            return 1;
        }
    }
    if ( jbig2_complete_page( ctx ) < 0 ) {
        jbig2_ctx_free( ctx );
        return 1;
    }
    if ( ( image = jbig2_page_out( ctx ) ) == NULL ) {
        jbig2_ctx_free( ctx );
        return 1;
    }
    *w = image->width;
    *h = image->height;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL ) {
        jbig2_ctx_free( ctx );
        return 1;
    }
    d = (uns32b *)( *data );
    p = image->data;
    for ( i = 0 ; i < *h ; i++, p += image->stride ) {
        for ( j = 0, q = p, cnt = 0 ; j < *w ; j++ ) {
#ifdef TRP_LITTLE_ENDIAN
            *d++ = ( ( *q & 0x80 ) ? 0xff000000 : 0xffffffff );
#else
            *d++ = ( ( *q & 0x80 ) ? 0x000000ff : 0xffffffff );
#endif
            if ( ++cnt == 8 ) {
                q++;
                cnt = 0;
            } else
                *q <<= 1;
        }
    }
    jbig2_release_page( ctx, image );
    jbig2_ctx_free( ctx );
    return 0;
}

static uns8b trp_pix_load_jbig2( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    if ( trp_pix_load_jbig2_low( (Jbig2Options)0, cpath, NULL, 0, w, h, data ) )
        if ( trp_pix_load_jbig2_low( (Jbig2Options)JBIG2_OPTIONS_EMBEDDED, cpath, NULL, 0, w, h, data ) )
            return 1;
    return 0;
}

static uns8b trp_pix_load_jbig2_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    if ( trp_pix_load_jbig2_low( (Jbig2Options)0, NULL, idata, isize, w, h, data ) )
        if ( trp_pix_load_jbig2_low( (Jbig2Options)JBIG2_OPTIONS_EMBEDDED, NULL, idata, isize, w, h, data ) )
            return 1;
    return 0;
}

