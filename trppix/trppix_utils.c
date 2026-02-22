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

#include "./trppix_internal.h"

uns8b *trp_pix_get_map_low( trp_pix_t *obj )
{
    if ( obj->tipo != TRP_PIX )
        return NULL;
    return obj->map.p;
}

void trp_pix_fclamp( flt64b *val )
{
    if ( *val < 0.0 )
        *val = 0.0;
    else if ( *val > 255.0 )
        *val = 255.0;
}

void trp_pix_fclamp_rgb( flt64b *r, flt64b *g, flt64b *b )
{
    trp_pix_fclamp( r );
    trp_pix_fclamp( g );
    trp_pix_fclamp( b );
}

trp_obj_t *trp_pix_clone( trp_obj_t *pix )
{
    trp_pix_crop_low( pix, 0.0, 0.0, (flt64b)( ((trp_pix_t *)pix)->w ), (flt64b)( ((trp_pix_t *)pix)->h ) );
}

flt64b pix_color_diff( uns8b r1, uns8b g1, uns8b b1, uns8b r2, uns8b g2, uns8b b2 )
{
    flt64b diff = 0.0;
    uns16b i;

    i = TRP_ABSDIFF( r1, r2 );
    diff += (flt64b)( i * i );
    i = TRP_ABSDIFF( g1, g2 );
    diff += (flt64b)( i * i );
    i = TRP_ABSDIFF( b1, b2 );
    diff += (flt64b)( i * i );
    return diff;
}

flt64b pix_color_diff_color( uns8b r, uns8b g, uns8b b, trp_pix_color_t *c )
{
    return pix_color_diff( r, g, b, c->red, c->green, c->blue );
}

