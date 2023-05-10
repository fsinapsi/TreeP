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

#include "../trp/trp.h"
#include "./trprsvg.h"
#include "../trppix/trppix_internal.h"
#include <librsvg/rsvg.h>

#define TRP_SVG_TARGET_WIDTH 720.0
#define TRP_SVG_TARGET_HEIGHT 460.0

static uns8b trp_pix_load_rsvg_low( uns8b target_type, flt64b target_width, flt64b target_height,
                                    RsvgHandle *handle, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_rsvg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_rsvg_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
static trp_obj_t *trp_rsvg_load_low( uns8b target_type, flt64b target_width, flt64b target_height, trp_obj_t *src );
static trp_obj_t *trp_rsvg_width_height_low( uns8b width, trp_obj_t *src );

uns8b trp_rsvg_init()
{
    extern uns8bfun_t _trp_pix_load_rsvg;
    extern uns8bfun_t _trp_pix_load_rsvg_memory;

    _trp_pix_load_rsvg = trp_pix_load_rsvg;
    _trp_pix_load_rsvg_memory = trp_pix_load_rsvg_memory;
    return 0;
}

/*
 * target_type:
 * 0: use target_width/height
 * 1: target_width/height are multiplicative factors
 */

static uns8b trp_pix_load_rsvg_low( uns8b target_type, flt64b target_width, flt64b target_height,
                                    RsvgHandle *handle, uns32b *w, uns32b *h, uns8b **data )
{
    cairo_surface_t *surface;
    cairo_t *cr;
    gdouble fw, fh;
    RsvgRectangle viewport;
    uns8b res = 1;

    if ( handle == NULL )
        return 1;
    if ( !rsvg_handle_get_intrinsic_size_in_pixels( handle, &fw, &fh ) ) {
        fw = TRP_SVG_TARGET_WIDTH;
        fh = TRP_SVG_TARGET_HEIGHT;
    }
    switch ( target_type ) {
        case 0:
            if ( fw/fh >= target_width/target_height ) {
                fh = target_width / (fw/fh);
                fw = target_width;
            } else {
                fw = target_height * (fw/fh);
                fh = target_height;
            }
            break;
        case 1:
            fw *= target_width;
            fh *= target_height;
            break;
    }
    *w = (uns32b)( ceil( fw ) );
    *h = (uns32b)( ceil( fh ) );
    surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, *w, *h );
    cr = cairo_create( surface );
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = *w;
    viewport.height = *h;
    if ( rsvg_handle_render_document( handle, cr, &viewport, NULL ) ) {
        trp_pix_color_t *s;

        cairo_surface_flush( surface );
        if ( s = (trp_pix_color_t *)cairo_image_surface_get_data( surface ) ) {
            if ( *data = malloc( ( *w * *h ) << 2 ) ) {
                trp_pix_color_t *d = (trp_pix_color_t *)( *data );
                flt64b m;
                uns32b i;

                for ( i = *w * *h ; i ; ) {
                    i--;
                    m = 255.0 / ((flt64b)( s[ i ].alpha ));
                    d[ i ].red = (uns8b)(((flt64b)( s[ i ].blue )) * m);
                    d[ i ].green = (uns8b)(((flt64b)( s[ i ].green )) * m);
                    d[ i ].blue = (uns8b)(((flt64b)( s[ i ].red )) * m);
                    d[ i ].alpha = s[ i ].alpha;
                }
                res = 0;
            }
        }
    }
    cairo_destroy( cr );
    cairo_surface_destroy( surface );
    g_object_unref( handle );
    return res;
}

static uns8b trp_pix_load_rsvg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_rsvg_low( 0, TRP_SVG_TARGET_WIDTH, TRP_SVG_TARGET_HEIGHT,
                                  rsvg_handle_new_from_file( cpath, NULL ), w, h, data );
}

static uns8b trp_pix_load_rsvg_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_rsvg_low( 0, TRP_SVG_TARGET_WIDTH, TRP_SVG_TARGET_HEIGHT,
                                  rsvg_handle_new_from_data( idata, isize, NULL ), w, h, data );
}

static trp_obj_t *trp_rsvg_load_low( uns8b target_type, flt64b target_width, flt64b target_height, trp_obj_t *src )
{
    RsvgHandle *handle = NULL;
    uns8b *data;
    uns32b w, h;

    switch ( src->tipo ) {
        case TRP_CORD:
            {
                uns8b *cpath = trp_csprint( src );
                handle = rsvg_handle_new_from_file( cpath, NULL );
                trp_csprint_free( cpath );
            }
            break;
        case TRP_RAW:
            handle = rsvg_handle_new_from_data( ((trp_raw_t *)src)->data, ((trp_raw_t *)src)->len, NULL );
            break;
    }
    if ( trp_pix_load_rsvg_low( target_type, target_width, target_height,
                                handle,
                                &w, &h, &data ) )
        return UNDEF;
    src = trp_pix_create_image_from_data( 0, w, h, data );
    trp_pix_load_set_loader_svg( src );
    return src;
}

trp_obj_t *trp_rsvg_load( trp_obj_t *src, trp_obj_t *mult )
{
    flt64b t;

    if ( mult ) {
        if ( trp_cast_flt64b( mult, &t ) )
            return UNDEF;
    } else
        t = 1.0;
    return trp_rsvg_load_low( 1, t, t, src );
}

trp_obj_t *trp_rsvg_load_size( trp_obj_t *src, trp_obj_t *target_width, trp_obj_t *target_height )
{
    flt64b tw, th;

    if ( trp_cast_flt64b( target_width, &tw ) || trp_cast_flt64b( target_height, &th ) )
        return UNDEF;
    return trp_rsvg_load_low( 0, tw, th, src );
}

static trp_obj_t *trp_rsvg_width_height_low( uns8b width, trp_obj_t *src )
{
    RsvgHandle *handle = NULL;
    gdouble fw, fh;

    switch ( src->tipo ) {
        case TRP_CORD:
            {
                uns8b *cpath = trp_csprint( src );
                handle = rsvg_handle_new_from_file( cpath, NULL );
                trp_csprint_free( cpath );
            }
            break;
        case TRP_RAW:
            handle = rsvg_handle_new_from_data( ((trp_raw_t *)src)->data, ((trp_raw_t *)src)->len, NULL );
            break;
    }
    if ( handle == NULL )
        return UNDEF;
    if ( !rsvg_handle_get_intrinsic_size_in_pixels( handle, &fw, &fh ) ) {
        fw = TRP_SVG_TARGET_WIDTH;
        fh = TRP_SVG_TARGET_HEIGHT;
    }
    g_object_unref( handle );
    return trp_double( width ? fw : fh );
}

trp_obj_t *trp_rsvg_width( trp_obj_t *src )
{
    return trp_rsvg_width_height_low( 1, src );
}

trp_obj_t *trp_rsvg_height( trp_obj_t *src )
{
    return trp_rsvg_width_height_low( 0, src );
}

