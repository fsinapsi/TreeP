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
#include "./trpwebp.h"
#include "webp/decode.h"
#include "webp/encode.h"
#include "../trppix/trppix_internal.h"
#include <sys/stat.h>

static uns8b trp_pix_load_webp( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_webp_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_webp_init()
{
    extern uns8bfun_t _trp_pix_load_webp;
    extern uns8bfun_t _trp_pix_load_webp_memory;

    _trp_pix_load_webp = trp_pix_load_webp;
    _trp_pix_load_webp_memory = trp_pix_load_webp_memory;
    return 0;
}

#define TRP_WEBP_HEADER_MAX_SIZE 512

static uns8b trp_pix_load_webp( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *idata;
    size_t isize, osize, n;
    int width, height;
    FILE *fp;
#ifdef MINGW
    wchar_t *wpath;
    struct _stati64 st;

    wpath = trp_utf8_to_wc( cpath );
    if ( wpath == NULL )
        return 1;
    if ( _wstati64( wpath, &st ) ) {
        trp_gc_free( wpath );
        return 1;
    }
    trp_gc_free( wpath );
#else
    struct stat st;

    if ( lstat( cpath, &st ) )
        return 1;
#endif
    isize = st.st_size;
    fp = trp_fopen( cpath, "rb" );
    if ( fp == NULL )
        return 1;
    if ( ( idata = malloc( isize ) ) == NULL ) {
        (void)fclose( fp );
        return 1;
    }
    n = TRP_MIN( TRP_WEBP_HEADER_MAX_SIZE, isize );
    if ( fread( idata, n, 1, fp ) != 1 ) {
        free( idata );
        (void)fclose( fp );
        return 1;
    }
    if ( WebPGetInfo( idata, n, &width, &height ) == 0 ) {
        (void)fclose( fp );
        free( idata );
        return 1;
    }
    if ( n < isize )
        if ( fread( idata + n, isize - n, 1, fp ) != 1 ) {
            free( idata );
            (void)fclose( fp );
            return 1;
        }
    (void)fclose( fp );
    *w = (uns32b)width;
    *h = (uns32b)height;
    osize = ( ( *w * *h ) << 2 );
    if ( ( *data = malloc( osize ) ) == NULL ) {
        free( idata );
        return 1;
    }
    if ( WebPDecodeRGBAInto( idata, isize, *data, osize, *w << 2 ) == NULL ) {
        free( *data );
        free( idata );
        return 1;
    }
    free( idata );
    return 0;
}

static uns8b trp_pix_load_webp_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    size_t osize;
    int width, height;

    if ( WebPGetInfo( idata, isize, &width, &height ) == 0 )
        return 1;
    *w = (uns32b)width;
    *h = (uns32b)height;
    osize = ( ( *w * *h ) << 2 );
    if ( ( *data = malloc( osize ) ) == NULL )
        return 1;
    if ( WebPDecodeRGBAInto( idata, isize, *data, osize, *w << 2 ) == NULL ) {
        free( *data );
        return 1;
    }
    return 0;
}

uns8b trp_webp_save( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *quality )
{
    uns8b *pi, *po;
    size_t sz;
    uns32b w, h;
    double quality_factor;
    uns8b res = 1;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( pi = ((trp_pix_t *)pix)->map.p ) == NULL )
        return 1;
    if ( quality ) {
        if ( trp_cast_double_range( quality, &quality_factor, 0.0, 100.0 ) )
            return 1;
    } else
        quality_factor = 100.0;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    sz = WebPEncodeRGBA( pi, w, h, w << 2, quality_factor, &po );
    if ( sz ) {
        uns8b *cpath = trp_csprint( path );
        FILE *fp;

        fp = trp_fopen( cpath, "w+b" );
        trp_csprint_free( cpath );
        if ( fp ) {
            if ( fwrite( po, sz, 1, fp ) == 1 )
                res = 0;
            fclose( fp );
            if ( res )
                trp_remove( path );
        }
        WebPFree( po );
    }
    return res;
}

trp_obj_t *trp_webp_save_memory( trp_obj_t *pix, trp_obj_t *quality )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    trp_obj_t *raw = UNDEF;
    uns8b *pi, *po;
    size_t sz;
    uns32b w, h;
    double quality_factor;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( pi = ((trp_pix_t *)pix)->map.p ) == NULL )
        return UNDEF;
    if ( quality ) {
        if ( trp_cast_double_range( quality, &quality_factor, 0.0, 100.0 ) )
            return UNDEF;
    } else
        quality_factor = 100.0;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    sz = WebPEncodeRGBA( pi, w, h, w << 2, quality_factor, &po );
    if ( sz ) {
        raw = trp_raw_internal( sz, 0 );
        memcpy( ((trp_raw_t *)raw)->data, po, sz );
        WebPFree( po );
    }
    return raw;
}

