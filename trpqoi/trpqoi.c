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
#include "./trpqoi.h"
#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "qoi.h"
#include "../trppix/trppix_internal.h"
#include <sys/stat.h>

static uns8b trp_pix_load_qoi( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_qoi_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_qoi_init()
{
    extern uns8bfun_t _trp_pix_load_qoi;
    extern uns8bfun_t _trp_pix_load_qoi_memory;

    _trp_pix_load_qoi = trp_pix_load_qoi;
    _trp_pix_load_qoi_memory = trp_pix_load_qoi_memory;
    return 0;
}

static uns8b trp_pix_load_qoi( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *idata, *hd;
    size_t isize;
    FILE *fp;
    qoi_desc desc;
    uns32b header_magic;
    int p;
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
    if ( isize < QOI_HEADER_SIZE )
        return 1;
    fp = trp_fopen( cpath, "rb" );
    if ( fp == NULL )
        return 1;
    if ( ( idata = malloc( isize ) ) == NULL ) {
        (void)fclose( fp );
        return 1;
    }
    if ( fread( idata, QOI_HEADER_SIZE, 1, fp ) != 1 ) {
        free( idata );
        (void)fclose( fp );
        return 1;
    }
    hd = idata;
    p = 0;
    header_magic = qoi_read_32( hd, &p );
    *w = qoi_read_32( hd, &p );
    *h = qoi_read_32( hd, &p );
    if ( ( header_magic != QOI_MAGIC ) || ( *h >= QOI_PIXELS_MAX / *w ) ) {
        free( idata );
        (void)fclose( fp );
        return 1;
    }
    if ( isize > QOI_HEADER_SIZE )
        if ( fread( idata + QOI_HEADER_SIZE, isize - QOI_HEADER_SIZE, 1, fp ) != 1 ) {
            free( idata );
            (void)fclose( fp );
            return 1;
        }
    (void)fclose( fp );
    *data = (uns8b *)qoi_decode( idata, isize, &desc, 4 );
    free( idata );
    return *data ? 0 : 1;
}

static uns8b trp_pix_load_qoi_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *hd;
    qoi_desc desc;
    uns32b header_magic;
    int p;

    if ( isize < QOI_HEADER_SIZE )
        return 1;
    hd = idata;
    p = 0;
    header_magic = qoi_read_32( hd, &p );
    *w = qoi_read_32( hd, &p );
    *h = qoi_read_32( hd, &p );
    if ( ( header_magic != QOI_MAGIC ) || ( *h >= QOI_PIXELS_MAX / *w ) )
        return 1;
    *data = (uns8b *)qoi_decode( idata, isize, &desc, 4 );
    return *data ? 0 : 1;
}

uns8b trp_qoi_save( trp_obj_t *pix, trp_obj_t *path )
{
    uns8b *pi, *po;
    int out_len;
    qoi_desc desc;
    uns8b res = 1;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( pi = ((trp_pix_t *)pix)->map.p ) == NULL )
        return 1;
    desc.width = ((trp_pix_t *)pix)->w;
    desc.height = ((trp_pix_t *)pix)->h;
    desc.channels = 4;
    desc.colorspace = 1;
    po = qoi_encode( pi, &desc, &out_len );
    if ( po ) {
        uns8b *cpath = trp_csprint( path );
        FILE *fp;

        fp = trp_fopen( cpath, "w+b" );
        trp_csprint_free( cpath );
        if ( fp ) {
            if ( fwrite( po, out_len, 1, fp ) == 1 )
                res = 0;
            fclose( fp );
            if ( res )
                trp_remove( path );
        }
        free( po );
    }
    return res;
}

trp_obj_t *trp_qoi_save_memory( trp_obj_t *pix )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    trp_obj_t *raw = UNDEF;
    uns8b *pi, *po;
    int out_len;
    qoi_desc desc;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( pi = ((trp_pix_t *)pix)->map.p ) == NULL )
        return UNDEF;
    desc.width = ((trp_pix_t *)pix)->w;
    desc.height = ((trp_pix_t *)pix)->h;
    desc.channels = 4;
    desc.colorspace = 1;
    po = qoi_encode( pi, &desc, &out_len );
    if ( po ) {
        raw = trp_raw_internal( out_len, 0 );
        memcpy( ((trp_raw_t *)raw)->data, po, out_len );
        free( po );
    }
    return raw;
}

