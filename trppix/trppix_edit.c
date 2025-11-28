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

uns8b trp_pix_bgr( trp_obj_t *pix )
{
    trp_pix_color_t *c;
    uns32b i;
    uns8b t;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        t = c->red;
        c->red = c->blue;
        c->blue = t;
    }
    return 0;
}

uns8b trp_pix_noalpha( trp_obj_t *pix )
{
    trp_pix_color_t *c;
    uns32b i;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->alpha = 0xff;
    return 0;
}

uns8b trp_pix_gray( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;

    if ( c == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = TRP_PIX_RGB_TO_GRAY_C( c );
    return 0;
}

uns8b trp_pix_gray16( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;

    if ( c == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = ( ( ( ((uns32b)TRP_PIX_RGB_TO_GRAY_C( c )) << 1 ) + 17 ) / 34 ) * 17;
    return 0;
}

uns8b trp_pix_gray_maximize_range( trp_obj_t *pix, trp_obj_t *black )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;
    sig32b bl, p, pmin, pmax;

    if ( c == NULL )
        return 1;
    if ( black ) {
        if ( trp_cast_sig32b_range( black, &bl, 0, 0xff ) )
            return 1;
    } else
        bl = -1;
    pmin = 255;
    pmax = 0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        p = c->red = TRP_PIX_RGB_TO_GRAY_C( c );
        if ( pmin > p )
            pmin = p;
        if ( pmax < p )
            pmax = p;
    }
    if ( bl >= 0 )
        pmin = bl;
    pmax -= pmin;
    if ( pmax <= 0 )
        pmax = 1;
    c = ((trp_pix_t *)pix)->map.c;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        p = ( 255 * ( c->red - pmin ) ) / pmax;
        if ( p < 0 )
            p = 0;
        else if ( p > 255 )
            p = 255;
        c->red = c->green = c->blue = p;
    }
    return 0;
}

uns8b trp_pix_bw( trp_obj_t *pix, trp_obj_t *threshold )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;
    uns32b thre;

    if ( c == NULL )
        return 1;
    if ( threshold ) {
        if ( trp_cast_uns32b_range( threshold, &thre, 0, 256 ) )
            return 1;
    } else
        thre = 128;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = ( TRP_PIX_RGB_TO_GRAY_C( c ) < thre ) ? 0 : 0xff;
    return 0;
}

uns8b trp_pix_linear( trp_obj_t *pix, trp_obj_t *min1, trp_obj_t *max1, trp_obj_t *min2, trp_obj_t *max2 )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;
    double dmin1, dmax1, dmin2, dmax2, d;

    if ( c == NULL )
        return 1;
    if ( trp_cast_flt64b( min1, &dmin1 ) ||
         trp_cast_flt64b( max1, &dmax1 ) ||
         trp_cast_flt64b( min2, &dmin2 ) ||
         trp_cast_flt64b( max2, &dmax2 ) )
        return 1;
    if ( dmin1 == dmax1 )
        return 1;
    dmax1 = ( dmax2 - dmin2 ) / ( dmax1 - dmin1 );
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        d = ( (flt64b)( c->red ) - dmin1 ) * dmax1 + dmin2;
        if ( d <= 0.0 )
            c->red = 0;
        else if ( d >= 255.0 )
            c->red = 255;
        else
            c->red = (uns8b)( d + 0.5 );
        d = ( (flt64b)( c->green ) - dmin1 ) * dmax1 + dmin2;
        if ( d <= 0.0 )
            c->green = 0;
        else if ( d >= 255.0 )
            c->green = 255;
        else
            c->green = (uns8b)( d + 0.5 );
        d = ( (flt64b)( c->blue ) - dmin1 ) * dmax1 + dmin2;
        if ( d <= 0.0 )
            c->blue = 0;
        else if ( d >= 255.0 )
            c->blue = 255;
        else
            c->blue = (uns8b)( d + 0.5 );
    }
    return 0;
}

uns8b trp_pix_linear_colors( trp_obj_t *pix, trp_obj_t *min1, trp_obj_t *max1, trp_obj_t *min2, trp_obj_t *max2 )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;
    flt64b dmin1r, dmax1r, dmin2r, dmax2r, dmin1g, dmax1g, dmin2g, dmax2g, dmin1b, dmax1b, dmin2b, dmax2b, d;
    uns8b r, g, b, a;

    if ( c == NULL )
        return 1;
    if ( trp_pix_decode_color_uns8b( min1, NULL, &r, &g, &b, &a ) )
        return 1;
    dmin1r = (flt64b)r;
    dmin1g = (flt64b)g;
    dmin1b = (flt64b)b;
    if ( trp_pix_decode_color_uns8b( max1, NULL, &r, &g, &b, &a ) )
        return 1;
    dmax1r = (flt64b)r;
    dmax1g = (flt64b)g;
    dmax1b = (flt64b)b;
    if ( trp_pix_decode_color_uns8b( min2, NULL, &r, &g, &b, &a ) )
        return 1;
    dmin2r = (flt64b)r;
    dmin2g = (flt64b)g;
    dmin2b = (flt64b)b;
    if ( trp_pix_decode_color_uns8b( max2, NULL, &r, &g, &b, &a ) )
        return 1;
    dmax2r = (flt64b)r;
    dmax2g = (flt64b)g;
    dmax2b = (flt64b)b;
    if ( ( dmin1r == dmax1r ) || ( dmin1g == dmax1g ) || ( dmin1b == dmax1b ) )
        return 1;
    dmax1r = ( dmax2r - dmin2r ) / ( dmax1r - dmin1r );
    dmax1g = ( dmax2g - dmin2g ) / ( dmax1g - dmin1g );
    dmax1b = ( dmax2b - dmin2b ) / ( dmax1b - dmin1b );
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        d = ( (flt64b)( c->red ) - dmin1r ) * dmax1r + dmin2r;
        if ( d <= 0.0 )
            c->red = 0;
        else if ( d >= 255.0 )
            c->red = 255;
        else
            c->red = (uns8b)( d + 0.5 );
        d = ( (flt64b)( c->green ) - dmin1g ) * dmax1g + dmin2g;
        if ( d <= 0.0 )
            c->green = 0;
        else if ( d >= 255.0 )
            c->green = 255;
        else
            c->green = (uns8b)( d + 0.5 );
        d = ( (flt64b)( c->blue ) - dmin1b ) * dmax1b + dmin2b;
        if ( d <= 0.0 )
            c->blue = 0;
        else if ( d >= 255.0 )
            c->blue = 255;
        else
            c->blue = (uns8b)( d + 0.5 );
    }
    return 0;
}

uns8b trp_pix_negative( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b i;

    if ( c == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        c->red = ~( c->red );
        c->green = ~( c->green );
        c->blue = ~( c->blue );
    }
    return 0;
}

uns8b trp_pix_transparent( trp_obj_t *pix, trp_obj_t *color )
{
    trp_pix_color_t *c;
    uns32b i;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, pix, &r, &g, &b, &a ) )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h, c = ((trp_pix_t *)pix)->map.c ; i ; i--, c++ )
        c->alpha = ( ( c->red == r ) && ( c->green == g ) && ( c->blue == b ) ) ? 0 : 0xff;
    return 0;
}

uns8b trp_pix_clralpha( trp_obj_t *pix, trp_obj_t *color )
{
    trp_pix_color_t *c;
    uns32b i;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, pix, &r, &g, &b, &a ) )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h, c = ((trp_pix_t *)pix)->map.c ; i ; i--, c++ )
        if ( c->alpha == 0 ) {
            c->red = r;
            c->green = g;
            c->blue = b;
        }
    return 0;
}

uns8b trp_pix_setalpha( trp_obj_t *pix, trp_obj_t *pix_alpha )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix ), *ca = trp_pix_get_mapc( pix_alpha );
    uns32b w, h, i;
    uns8b r, g, b, a;

    if ( ( c == NULL ) || ( ca == NULL ) )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( ((trp_pix_t *)pix_alpha)->w != w ) ||
         ( ((trp_pix_t *)pix_alpha)->h != h ) )
        return 1;
    for ( i = w * h ; i ; i--, c++, ca++ )
        c->alpha = TRP_PIX_RGB_TO_GRAY_C( ca );
    return 0;
}

uns8b trp_pix_hflip( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns32b w, w2, i, j, k, l;

    if ( c == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    w2 = w >> 1;
    for ( i = ((trp_pix_t *)pix)->h ; i ; i--, c += w )
        for ( j = w2 ; j ; ) {
            k = w - j;
            j--;
            l = c[ j ].rgba;
            c[ j ].rgba = c[ k ].rgba;
            c[ k ].rgba = l;
        }
    return 0;
}

uns8b trp_pix_vflip( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix ), *p, *q;
    uns32b w, h, i, j, k;

    if ( c == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    for ( i = h >> 1 ; i ; ) {
        k = h - i;
        i--;
        p = c + w * i;
        q = c + w * k;
        for ( j = 0 ; j < w ; j++ ) {
            k = p[ j ].rgba;
            p[ j ].rgba = q[ j ].rgba;
            q[ j ].rgba = k;
        }
    }
    return 0;
}

uns8b trp_pix_snap_color( trp_obj_t *pix, trp_obj_t *src_color, trp_obj_t *thres, trp_obj_t *dst_color )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    flt64b th;
    uns32b i;
    uns8b sr, sg, sb, sa, dr, dg, db, da;

    if ( ( c == NULL ) ||
         trp_pix_decode_color_uns8b( src_color, NULL, &sr, &sg, &sb, &sa ) ||
         trp_cast_flt64b_range( thres, &th, 0.0, 255.0 ) )
        return 1;
    if ( dst_color ) {
        if ( trp_pix_decode_color_uns8b( dst_color, NULL, &dr, &dg, &db, &da ) )
            return 1;
    } else {
        dr = sr;
        dg = sg;
        db = sb;
        da = sa;
    }
    th = th * th * 3.0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        if ( pix_color_diff_color( sr, sg, sb, c ) <= th ) {
            c->red = dr;
            c->green = dg;
            c->blue = db;
            c->alpha = da;
        }
    return 0;
}

uns8b trp_pix_chroma_hold( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *thres )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    flt64b th;
    uns32b i;
    uns8b r, g, b, a;

    if ( ( c == NULL ) ||
         trp_pix_decode_color_uns8b( color, NULL, &r, &g, &b, &a ) ||
         trp_cast_flt64b_range( thres, &th, 0.0, 255.0 ) )
        return 1;
    th = th * th * 3.0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        if ( pix_color_diff_color( r, g, b, c ) > th )
            c->red = c->green = c->blue = TRP_PIX_RGB_TO_GRAY_C( c );
    return 0;
}

