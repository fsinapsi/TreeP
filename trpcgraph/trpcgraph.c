/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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
#include "./trpcgraph.h"
#include "../trppix/trppix_internal.h"
#include <gvc.h>
#include <cgraph.h>

// static uns8b trp_vl_print( trp_print_t *p, trp_vlfeat_t *obj );
static trp_obj_t *trp_ag_dot2pix_low( uns8b flags, trp_obj_t *s );

uns8b trp_ag_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

//    _trp_print_fun[ TRP_VLFEAT ] = trp_vl_print;
    return 0;
}

static trp_obj_t *trp_ag_dot2pix_low( uns8b flags, trp_obj_t *s )
{
    Agraph_t *g;
    GVC_t *gvc;
    trp_raw_t *raw;
    uns8b *res, *data;
    unsigned len;
    uns32b w, h;
    uns8b i;

    res = trp_csprint( s );
    g = agmemread( res );
    trp_csprint_free( res );
    if ( g == NULL )
        return UNDEF;
    gvc = gvContext();
    gvLayout( gvc, g, "dot" );
    gvRenderData( gvc, g, "png", (char **)(&res), &len );
    gvFreeLayout( gvc, g );
    agclose( g );
    gvFreeContext( gvc );
    if ( len == 0 ) {
        gvFreeRenderData( (char *)res );
        return UNDEF;
    }
    raw = trp_gc_malloc( sizeof( trp_raw_t ) );
    raw->tipo = TRP_RAW;
    raw->mode = 0;
    raw->unc_tipo = 0;
    raw->compression_level = 0;
    raw->len = len;
    raw->unc_len = 0;
    raw->data = res;
    i = trp_pix_load_png_memory( raw, &w, &h, &data );
    trp_gc_free( raw );
    gvFreeRenderData( (char *)res );
    if ( i )
        return UNDEF;
    if ( flags & 1 ) {
        trp_pix_color_t *c;
        uns32b j;

        for ( j = w * h, c = (trp_pix_color_t *)data ; j ; j--, c++ )
            if ( ( c->red == c->green ) && ( c->green == c->blue ) ) {
                c->alpha = ~( c->red );
                c->red = 0;
                c->green = 0;
                c->blue = 0;
            }
    }
    return trp_pix_create_image_from_data( 0, w, h, data );
}

trp_obj_t *trp_ag_dot2pix( trp_obj_t *s )
{
    return trp_ag_dot2pix_low( 0, s );
}

trp_obj_t *trp_ag_dot2pix_transparent( trp_obj_t *s )
{
    return trp_ag_dot2pix_low( 1, s );
}

