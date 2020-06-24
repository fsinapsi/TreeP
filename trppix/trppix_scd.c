/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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

#define trp_pix_cast(x) ((uns32b)((x)+0.5))
#define trp_pix_dist(x,y) (((x)>=(y))?((x)-(y)):((y)-(x)))

static uns32b trp_pix_sadb( trp_pix_color_t *c, trp_pix_color_t *ref, uns32b w, int b );
static uns32b trp_pix_sadb_min( trp_pix_color_t *c, trp_pix_color_t *ref, int x, int y, uns32b w, uns32b h, int b, int r );
static trp_obj_t *trp_pix_scd_basic( trp_pix_color_t *c, trp_pix_color_t *ref, uns32b w, uns32b h, int b, int r );

static uns32b trp_pix_sadb( trp_pix_color_t *c, trp_pix_color_t *ref, uns32b w, int b )
{
    uns32b res = 0;
    int i, j;

    for ( j = b ; j ; j--, c += w, ref += w )
        for ( i = b ; i ; ) {
            i--;
            res += trp_pix_dist( c[ i ].red, ref[ i ].red ) +
                trp_pix_dist( c[ i ].green, ref[ i ].green ) +
                trp_pix_dist( c[ i ].blue, ref[ i ].blue );
        }
    return res;
}

static uns32b trp_pix_sadb_min( trp_pix_color_t *c, trp_pix_color_t *ref, int x, int y, uns32b w, uns32b h, int b, int r )
{
    trp_pix_color_t *d;
    uns32b smin = 0xffffffff, s;
    int i, j, imax;

    for ( j = ( y >= r ) ? -r : -y ; j <= 0 ; j++ ) {
        imax = r + j;
        i = -imax;
        if ( x + i < 0 )
            i = -x;
        if ( x + imax > w - b )
            imax = w - b - x;
        d = ref + ( x + i ) + w * ( y + j );
        for ( ; i <= imax ; i++, d++ ) {
            s = trp_pix_sadb( c, d, w, b );
            if ( s < smin )
                smin = s;
        }
    }
    for ( j = ( h - b >= y + r ) ? r : h - b - y ; j > 0 ; j-- ) {
        imax = r - j;
        i = -imax;
        if ( x + i < 0 )
            i = -x;
        if ( x + imax > w - b )
            imax = w - b - x;
        d = ref + ( x + i ) + w * ( y + j );
        for ( ; i <= imax ; i++, d++ ) {
            s = trp_pix_sadb( c, d, w, b );
            if ( s < smin )
                smin = s;
        }
    }
    return smin;
}

static trp_obj_t *trp_pix_scd_basic( trp_pix_color_t *c, trp_pix_color_t *ref, uns32b w, uns32b h, int b, int r )
{
    trp_pix_color_t *cc;
    uns32b ww = w * b, n = 0, d = 0;
    int x, y, nx, ny;

    if ( ( w < b ) || ( h < b ) )
        return UNDEF;
    for ( y = 0 ; ; y = ny, c += ww ) {
        ny = y + b;
        if ( ny > h ) {
            c -= w * ( ny - h );
            y = h - b;
            ny = h;
        }
        for ( x = 0, cc = c ; ; x = nx, cc += b ) {
            nx = x + b;
            if ( nx > w ) {
                cc -= ( nx - w );
                x = w - b;
                nx = w;
            }
            n += trp_pix_sadb_min( cc, ref, x, y, w, h, b, r );
            d++;
            if ( nx == w )
                break;
        }
        if ( ny == h )
            break;
    }
    return trp_math_ratio( trp_sig64( n ), trp_sig64( d * b * b * 765 ), NULL );
}

trp_obj_t *trp_pix_scd( trp_obj_t *pix, trp_obj_t *ref, trp_obj_t *dimblock, trp_obj_t *radius )
{
    trp_pix_color_t *c, *r;
    uns32b bb, rr, w, h;

    if ( ( pix->tipo != TRP_PIX ) || ( ref->tipo != TRP_PIX ) ||
         trp_cast_uns32b_range( dimblock, &bb, 1, 0xffffffff ) ||
         trp_cast_uns32b( radius, &rr ) )
        return UNDEF;
    if ( ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL ) ||
         ( ( r = ((trp_pix_t *)ref)->map.c ) == NULL ) )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( w != ((trp_pix_t *)ref)->w ) ||
         ( h != ((trp_pix_t *)ref)->h ) )
        return UNDEF;
    return trp_pix_scd_basic( c, r, w, h, (int)bb, (int)rr );
}

#define trp_pix_hist_val(c) ((19595*(uns32b)((c)->red)+38470*(uns32b)((c)->green)+7471*(uns32b)((c)->blue))>>18)

trp_obj_t *trp_pix_scd_histogram( trp_obj_t *pix, trp_obj_t *ref )
{
    trp_pix_color_t *c, *r;
    uns32b i, j, nc, nr, hc[ 64 ], hr[ 64 ];

    if ( ( pix->tipo != TRP_PIX ) || ( ref->tipo != TRP_PIX ) )
        return UNDEF;
    if ( ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL ) ||
         ( ( r = ((trp_pix_t *)ref)->map.c ) == NULL ) )
        return UNDEF;
    nc = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    nr = ((trp_pix_t *)ref)->w * ((trp_pix_t *)ref)->h;
    memset( hc, 0, sizeof( hc ) );
    memset( hr, 0, sizeof( hr ) );
    if ( nc == nr ) {
        for ( i = nc ; i ; i--, c++, r++ ) {
            hc[ trp_pix_hist_val( c ) ]++;
            hr[ trp_pix_hist_val( r ) ]++;
        }
    } else {
        for ( i = nc ; i ; i--, c++ )
            hc[ trp_pix_hist_val( c ) ]++;
        for ( i = nr ; i ; i--, r++ )
            hr[ trp_pix_hist_val( r ) ]++;
    }
    for ( i = 0, j = 0 ; i < 64 ; i++ )
        j += trp_pix_dist( hc[ i ], hr[ i ] );
    return trp_math_ratio( trp_sig64( j ), trp_sig64( nc + nr ), NULL );
}



