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

#include "./trppix_internal.h"

uns8b trp_pix_load_pnm( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    /*
     FIXME
     */
    return 1;
}

uns8b trp_pix_load_pnm_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    /*
     FIXME
     */
    return 1;
}

uns8b trp_pix_save_pnm( trp_obj_t *pix, trp_obj_t *path )
{
    /*
     FIXME
     */
    return 1;
}

uns8b trp_pix_save_pnm_noalpha( trp_obj_t *pix, trp_obj_t *path )
{
    FILE *fp;
    uns32b w, h, bpl, i;
    trp_pix_color_t *c;
    uns8b *cpath, buf[ 30 ], *p, *q;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    cpath = trp_csprint( path );
    fp = trp_fopen( cpath, "wb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    sprintf( buf, "P6\n%d %d\n255\n", (int)w, (int)h );
    i = strlen( buf );
    if ( trp_file_write_chars( fp, buf, i ) != i ) {
        fclose( fp );
        return 1;
    }
    bpl = w * 3;
    p = trp_gc_malloc_atomic( bpl );
    for ( ; h ; h-- ) {
        for ( q = p, i = 0 ; i < w ; i++, c++ ) {
            *q++ = c->red;
            *q++ = c->green;
            *q++ = c->blue;
        }
        if ( trp_file_write_chars( fp, p, bpl ) != bpl ) {
            trp_gc_free( p );
            fclose( fp );
            return 1;
        }
    }
    trp_gc_free( p );
    fclose( fp );
    return 0;
}

