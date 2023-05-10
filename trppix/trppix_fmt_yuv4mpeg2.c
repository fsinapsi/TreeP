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

void trp_pix_ss_444_to_420jpeg( uns8b *buf, uns32b width, uns32b height );
uns8b *trp_pix_pix2yuv( trp_obj_t *pix );

uns8b trp_pix_save_yuv4mpeg2_init( trp_obj_t *width, trp_obj_t *height, trp_obj_t *framerate, trp_obj_t *aspect_ratio, trp_obj_t *f )
{
    FILE *fp;
    uns32b w, h, fn, fd, an, ad, n;
    uns8b buf[ 256 ];

    if ( f == NULL )
        f = trp_stdout();
    if ( ( ( fp = trp_file_writable_fp( f ) ) == NULL ) ||
         trp_cast_uns32b_range( width, &w, 1, 0xffff ) ||
         trp_cast_uns32b_range( height, &h, 1, 0xffff ) ||
         trp_cast_uns32b_range( trp_math_num( framerate ), &fn, 1, 0xffff ) ||
         trp_cast_uns32b_range( trp_math_den( framerate ), &fd, 1, 0xffff ) ||
         trp_cast_uns32b_range( trp_math_num( aspect_ratio ), &an, 1, 0xffff ) ||
         trp_cast_uns32b_range( trp_math_den( aspect_ratio ), &ad, 1, 0xffff ) )
        return 1;
    if ( ( w & 1 ) || ( h & 1 ) )
        return 1;
    sprintf( buf, "YUV4MPEG2 W%u H%u F%u:%u Ip A%u:%u\n",
             w, h, fn, fd, an, ad );
    n = strlen( buf );
    return ( trp_file_write_chars( fp, buf, n ) == n ) ? 0 : 1;
}

uns8b trp_pix_save_yuv4mpeg2( trp_obj_t *pix, trp_obj_t *f )
{
    FILE *fp;
    uns8b *p, *cb, *cr;
    uns32b w, h, n;

    if ( f == NULL )
        f = trp_stdout();
    if ( ( fp = trp_file_writable_fp( f ) ) == NULL )
        return 1;
    if ( ( p = trp_pix_pix2yuv( pix ) ) == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    n = w * h;
    cb = p + n;
    cr = cb + n;
    trp_pix_ss_444_to_420jpeg( cb, w, h );
    trp_pix_ss_444_to_420jpeg( cr, w, h );
    if ( trp_file_write_chars( fp, "FRAME\n", 6 ) != 6 ) {
        free( p );
        return 1;
    }
    if ( trp_file_write_chars( fp, p, n ) != n ) {
        free( p );
        return 1;
    }
    n >>= 2;
    if ( trp_file_write_chars( fp, cb, n ) != n ) {
        free( p );
        return 1;
    }
    if ( trp_file_write_chars( fp, cr, n ) != n ) {
        free( p );
        return 1;
    }
    free( p );
    return 0;
}

