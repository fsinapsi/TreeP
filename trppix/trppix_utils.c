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

