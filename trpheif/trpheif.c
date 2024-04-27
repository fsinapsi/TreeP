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
#include "./trpheif.h"
#include <libheif/heif.h>
#include "../trppix/trppix_internal.h"

static uns8b trp_pix_load_heif_low( uns8b *cpath, uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_heif( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_heif_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_heif_init()
{
    extern uns8bfun_t _trp_pix_load_heif;
    extern uns8bfun_t _trp_pix_load_heif_memory;

    _trp_pix_load_heif = trp_pix_load_heif;
    _trp_pix_load_heif_memory = trp_pix_load_heif_memory;
    if ( heif_init( NULL ).code )
        return 1;
    return 0;
}

void trp_heif_quit()
{
    heif_deinit();
}

static uns8b trp_pix_load_heif_low( uns8b *cpath, uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    struct heif_context *ctx;
    struct heif_image_handle *handle;
    struct heif_image *img;
    uns8b *p;
    const uns8b *q;
    int stride;
    uns32b i, j;
    uns8b res = 1;

    if ( ( ctx = heif_context_alloc() ) == NULL )
        return 1;
    if ( cpath ) {
        if ( heif_context_read_from_file( ctx, cpath, NULL ).code )
            goto uscita1;
    } else {
        if ( heif_context_read_from_memory_without_copy( ctx, idata, isize, NULL ).code )
            goto uscita1;
    }
    if ( heif_context_get_primary_image_handle( ctx, &handle ).code )
        goto uscita1;
    if ( heif_decode_image( handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, NULL ).code )
        goto uscita2;
    *w = heif_image_handle_get_width( handle );
    *h = heif_image_handle_get_height( handle );
    j = *w << 2;
    if ( ( *data = malloc( *h * j ) ) == NULL )
        goto uscita3;
    p = *data;
    q = heif_image_get_plane_readonly( img, heif_channel_interleaved, &stride );
    for ( i = 0 ; i < *h ; i++, p += j, q += stride )
        memcpy( p, q, j );
    res = 0;
uscita3:
    heif_image_release( img );
uscita2:
    heif_image_handle_release( handle );
uscita1:
    heif_context_free( ctx );
    return res;
}

static uns8b trp_pix_load_heif( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_heif_low( cpath, NULL, 0, w, h, data );
}

static uns8b trp_pix_load_heif_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_heif_low( NULL, idata, isize, w, h, data );
}

