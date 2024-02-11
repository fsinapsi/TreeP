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

trp_obj_t *trp_pix_scd_histogram( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_color_t *c1, *c2;
    uns32b n1, n2, i;
    uns32b h1[ 64 ], h2[ 64 ];

    if ( ( pix1->tipo != TRP_PIX ) || ( pix2->tipo != TRP_PIX ) )
        return UNDEF;
    if ( ( ( c1 = ((trp_pix_t *)pix1)->map.c ) == NULL ) ||
         ( ( c2 = ((trp_pix_t *)pix2)->map.c ) == NULL ) )
        return UNDEF;
    n1 = ((trp_pix_t *)pix1)->w * ((trp_pix_t *)pix1)->h;
    n2 = ((trp_pix_t *)pix2)->w * ((trp_pix_t *)pix2)->h;
    memset( h1, 0, sizeof( h1 ) );
    memset( h2, 0, sizeof( h2 ) );
    if ( n1 == n2 ) {
        for ( i = n1 ; i ; i--, c1++, c2++ ) {
            h1[ trp_pix_hist_val( c1 ) ]++;
            h2[ trp_pix_hist_val( c2 ) ]++;
        }
    } else {
        for ( i = n1 ; i ; i--, c1++ )
            h1[ trp_pix_hist_val( c1 ) ]++;
        for ( i = n2 ; i ; i--, c2++ )
            h2[ trp_pix_hist_val( c2 ) ]++;
    }
    n2 += n1;
    n1 = 0;
    for ( i = 0 ; i < 64 ; i++ )
        n1 += trp_pix_dist( h1[ i ], h2[ i ] );
    return trp_math_ratio( trp_sig64( n1 ), trp_sig64( n2 ), NULL );
}

uns8b trp_pix_scd_histogram_set( trp_obj_t *pix, trp_obj_t *raw )
{
    trp_pix_color_t *c;
    uns32b n;
    uns32b *h;

    if ( ( pix->tipo != TRP_PIX ) || ( raw->tipo != TRP_RAW ) )
        return 1;
    if ( ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL ) ||
         ( ((trp_raw_t *)raw)->len != 256 ) )
        return 1;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    h = (uns32b *)( ((trp_raw_t *)raw)->data );
    memset( h, 0, 256 );
    for ( ; n ; n--, c++ )
        h[ trp_pix_hist_val( c ) ]++;
    return 0;
}

trp_obj_t *trp_pix_scd_histogram_dist( trp_obj_t *raw1, trp_obj_t *raw2 )
{
    uns32b i, n, k;
    uns32b *h1, *h2;

    if ( ( raw1->tipo != TRP_RAW ) || ( raw2->tipo != TRP_RAW ) )
        return UNDEF;
    if ( ( ((trp_raw_t *)raw1)->len != 256 ) ||
         ( ((trp_raw_t *)raw2)->len != 256 ) )
        return UNDEF;
    h1 = (uns32b *)( ((trp_raw_t *)raw1)->data );
    h2 = (uns32b *)( ((trp_raw_t *)raw2)->data );
    for ( i = 0, n = 0, k = 0 ; i < 64 ; i++ ) {
        n += h1[ i ];
        n += h2[ i ];
        k += trp_pix_dist( h1[ i ], h2[ i ] );
    }
    return trp_math_ratio( trp_sig64( k ), trp_sig64( n ), NULL );
}

