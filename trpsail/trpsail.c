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

#include "../trp/trp.h"
#include "./trpsail.h"
#include <sail/sail.h>
#include "../trppix/trppix_internal.h"

static uns8b trp_pix_load_sail_low( struct sail_image *image, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_sail( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_sail_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_sail_init()
{
    extern uns8bfun_t _trp_pix_load_sail;
    extern uns8bfun_t _trp_pix_load_sail_memory;

    _trp_pix_load_sail = trp_pix_load_sail;
    _trp_pix_load_sail_memory = trp_pix_load_sail_memory;
    sail_set_log_barrier( SAIL_LOG_LEVEL_SILENCE );
    return 0;
}

static uns8b trp_pix_load_sail_low( struct sail_image *image, uns32b *w, uns32b *h, uns8b **data )
{
    size_t size;
    uns8b res = 1;

    switch ( image->pixel_format ) {
        case SAIL_PIXEL_FORMAT_BPP24_RGB:
            if ( image->bytes_per_line == image->width * 3 ) {
                uns8b *c;
                trp_pix_color_t *d;

                *w = image->width;
                *h = image->height;
                size = ( ( *w * *h ) << 2 );
                if ( *data = malloc( size ) ) {
                    uns32b i;

                    for ( c = (uns8b *)( image->pixels ), d = (trp_pix_color_t *)( *data ), i = *w * *h ;
                          i ;
                          i--, d++ ) {
                        d->red = *c++;
                        d->green = *c++;
                        d->blue = *c++;
                        d->alpha = 0xff;
                    }
                    res = 0;
                }
            }
            break;
        case SAIL_PIXEL_FORMAT_BPP32_RGBA:
            if ( image->bytes_per_line == ( image->width << 2 ) ) {
                *w = image->width;
                *h = image->height;
                size = ( ( *w * *h ) << 2 );
                if ( *data = malloc( size ) ) {
                    memcpy( *data, image->pixels, size );
                    res = 0;
                }
            }
            break;
        default:
            /* fprintf( stderr, "### supporto: %s\n", sail_pixel_format_to_string( image->pixel_format ) ); */
            break;
    }
    sail_destroy_image( image );
    return res;
}

static uns8b trp_pix_load_sail( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    struct sail_image *image;

    if ( sail_load_from_file( cpath, &image ) != SAIL_OK )
        return 1;
    return trp_pix_load_sail_low( image, w, h, data );
}

static uns8b trp_pix_load_sail_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    struct sail_image *image;

    if ( sail_load_from_memory( idata, isize, &image ) != SAIL_OK )
        return 1;
    return trp_pix_load_sail_low( image, w, h, data );
}

