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

static void trp_pix_get_box_stat( trp_pix_t *p, uns32b sx, uns32b sy, uns32b sw, uns32b sh, trp_pix_color_t *c, double *ds );
static uns8b trp_pix_trim_low( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold, uns32b *ox, uns32b *oy, uns32b *ow, uns32b *oh );

static void trp_pix_get_box_stat( trp_pix_t *p, uns32b sx, uns32b sy, uns32b sw, uns32b sh, trp_pix_color_t *c, double *ds )
{
    trp_pix_color_t *map;
    uns32b n = sw * sh, i, j;
    double r = 0.0, g = 0.0, b = 0.0, d;

    for ( i = 0, map = p->map.c + sx + sy * p->w ; i < sh ; i++, map += p->w )
        for ( j = 0 ; j < sw ; j++ ) {
            r += (double)( map[ j ].red );
            g += (double)( map[ j ].green );
            b += (double)( map[ j ].blue );
        }
    r /= (double)n;
    g /= (double)n;
    b /= (double)n;
    c->red = (uns8b)( r + 0.5 );
    c->green = (uns8b)( g + 0.5 );
    c->blue = (uns8b)( b + 0.5 );
    c->alpha = 0;
    for ( i = 0, *ds = 0, map = p->map.c + sx + sy * p->w ; i < sh ; i++, map += p->w )
        for ( j = 0 ; j < sw ; j++ ) {
            d = r - (double)( map[ j ].red );
            *ds += ( d * d );
            d = g - (double)( map[ j ].green );
            *ds += ( d * d );
            d = b - (double)( map[ j ].blue );
            *ds += ( d * d );
        }
    *ds = sqrt( *ds / (double)( 3 * n ) );
}

trp_obj_t *trp_pix_box_stat( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h )
{
    uns32b sx, sy, sw, sh;
    trp_pix_color_t c;
    double ds;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_rint_range( x, &sx, 0, 0xffff ) ||
         trp_cast_uns32b_rint_range( y, &sy, 0, 0xffff ) ||
         trp_cast_uns32b_rint_range( w, &sw, 1, 0xffff ) ||
         trp_cast_uns32b_rint_range( h, &sh, 1, 0xffff ) )
        return UNDEF;
    if ( ( sx + sw > ((trp_pix_t *)pix)->w ) ||
         ( sy + sh > ((trp_pix_t *)pix)->h ) )
        return UNDEF;
    trp_pix_get_box_stat( (trp_pix_t *)pix, sx, sy, sw, sh, &c, &ds );
    return trp_cons( trp_pix_create_color( 257 * (uns16b)(c.red),
                                           257 * (uns16b)(c.green),
                                           257 * (uns16b)(c.blue),
                                           257 * (uns16b)(c.alpha) ),
                     trp_double( ds ) );
}

trp_obj_t *trp_pix_is_empty( trp_obj_t *pix, trp_obj_t *threshold )
{
    trp_obj_t *res = TRP_TRUE;
    uns32b thr, n;
    trp_pix_color_t *map;

    if ( threshold ) {
        if ( trp_cast_uns32b_rint_range( threshold, &thr, 0, 0xff ) )
            return TRP_FALSE;
    } else {
        thr = 0xff;
    }
    if ( pix->tipo != TRP_PIX )
        return TRP_FALSE;
    if ( ( map = ((trp_pix_t *)pix)->map.c ) == NULL )
        return TRP_FALSE;
    for ( n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; n ; n--, map++ )
        if ( ( map->red < thr ) ||
             ( map->green < thr ) ||
             ( map->blue < thr ) ) {
            res = TRP_FALSE;
            break;
        }
    return res;
}

trp_obj_t *trp_pix_trim( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold )
{
    uns32b x, y, w, h;

    if ( trp_pix_trim_low( pix, color, threshold, &x, &y, &w, &h ) )
        return UNDEF;
    return trp_pix_crop( pix,
                         trp_sig64( (sig64b)x ), trp_sig64( (sig64b)y ),
                         trp_sig64( (sig64b)w ), trp_sig64( (sig64b)h ) );
}

trp_obj_t *trp_pix_trim_values( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold )
{
    uns32b x, y, w, h;

    if ( trp_pix_trim_low( pix, color, threshold, &x, &y, &w, &h ) )
        return UNDEF;
    return trp_cons( trp_sig64( (sig64b)x ),
                     trp_cons( trp_sig64( (sig64b)y ),
                               trp_cons( trp_sig64( (sig64b)w ),
                                         trp_cons( trp_sig64( (sig64b)h ), NIL ))));
}

#define trp_pix_trim_cond(val1,val2,thr) ((val1>=val2)?((val1-val2)>thr):((val2-val1)>thr))

static uns8b trp_pix_trim_low( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold, uns32b *ox, uns32b *oy, uns32b *ow, uns32b *oh )
{
    uns32b thr, w, h, x1, x2, y1, y2, i;
    trp_pix_color_t *map, *c;
    uns8b r, g, b, a;

    if ( color ) {
        if ( trp_pix_decode_color_uns8b( color, pix, &r, &g, &b, &a ) )
            return 1;
    } else {
        r = g = b = 0xff;
    }
    if ( threshold ) {
        if ( trp_cast_uns32b_rint_range( threshold, &thr, 0, 0xff ) )
            return 1;
    } else {
        thr = 0;
    }
    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( map = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    x1 = 0;
    x2 =  w - 1;
    y1 = 0;
    y2 =  h - 1;
    for ( ; ; x1++ ) {
        if ( x1 > x2 )
            return 1;
        for ( c = map + x1, i = y1 ; i <= y2 ; i++, c += w )
            if ( c->alpha )
                if ( trp_pix_trim_cond( c->red, r, thr ) ||
                     trp_pix_trim_cond( c->green, g, thr ) ||
                     trp_pix_trim_cond( c->blue, b, thr ) )
                    break;
        if ( i <= y2 )
            break;
    }
    for ( ; x2 > x1 ; x2-- ) {
        for ( c = map + x2, i = y1 ; i <= y2 ; i++, c += w )
            if ( c->alpha )
                if ( trp_pix_trim_cond( c->red, r, thr ) ||
                     trp_pix_trim_cond( c->green, g, thr ) ||
                     trp_pix_trim_cond( c->blue, b, thr ) )
                    break;
        if ( i <= y2 )
            break;
    }
    for ( ; ; y1++ ) {
        for ( c = map + x1 + w * y1, i = x1 ; i <= x2 ; i++, c++ )
            if ( c->alpha )
                if ( trp_pix_trim_cond( c->red, r, thr ) ||
                     trp_pix_trim_cond( c->green, g, thr ) ||
                     trp_pix_trim_cond( c->blue, b, thr ) )
                    break;
        if ( i <= x2 )
            break;
    }
    for ( ; y2 > y1 ; y2-- ) {
        for ( c = map + x1 + w * y2, i = x1 ; i <= x2 ; i++, c++ )
            if ( c->alpha )
                if ( trp_pix_trim_cond( c->red, r, thr ) ||
                     trp_pix_trim_cond( c->green, g, thr ) ||
                     trp_pix_trim_cond( c->blue, b, thr ) )
                    break;
        if ( i <= x2 )
            break;
    }
    *ox = x1;
    *oy = y1;
    *ow = x2 + 1 - x1;
    *oh = y2 + 1 - y1;
    return 0;
}

