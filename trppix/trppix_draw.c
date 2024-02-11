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
#define trp_pix_draw_linear(x,x1,x2,y1,y2) ((((x)-(x1))*((y2)-(y1)))/((x2)-(x1))+(y1))

static void trp_pix_draw_box_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x, double y, double w, double h );
static void trp_pix_draw_point_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x, double y );
static void trp_pix_draw_line_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x1, double y1, double x2, double y2, double l1, double l2 );

static void trp_pix_draw_box_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x, double y, double w, double h )
{
    trp_pix_color_t *pdst, *p;
    uns32b wdst = ((trp_pix_t *)dst)->w, hdst = ((trp_pix_t *)dst)->h, ww, hh, i;

    if ( x < 0.0 ) {
        if ( w <= -x )
            return;
        w -= -x;
        x = 0.0;
    }
    if ( y < 0.0 ) {
        if ( h <= -y )
            return;
        h -= -y;
        y = 0.0;
    }
    if ( ( x > (double)wdst - 1.0 ) || ( y > (double)hdst - 1.0 ) )
        return;
    if ( w > (double)wdst - x )
        w = (double)wdst - x;
    if ( h > (double)hdst - y )
        h = (double)hdst - y;
    ww = trp_pix_cast( w );
    hh = trp_pix_cast( h );
    pdst = ((trp_pix_t *)dst)->map.c + trp_pix_cast( x ) + wdst * trp_pix_cast( y );
    for ( ; hh ; hh--, pdst += wdst )
        for ( i = ww, p = pdst ; i ; i--, p++ ) {
            p->red = r;
            p->green = g;
            p->blue = b;
            p->alpha = a;
        }
}

static void trp_pix_draw_point_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x, double y )
{
    uns32b w = ((trp_pix_t *)dst)->w, h = ((trp_pix_t *)dst)->h;

    if ( ( x >= 0.0 ) && ( x <= (double)w - 1.0 ) &&
         ( y >= 0.0 ) && ( y <= (double)h - 1.0 ) ) {
        trp_pix_color_t *p = ((trp_pix_t *)dst)->map.c + trp_pix_cast( x ) + w * trp_pix_cast( y );
        p->red = r;
        p->green = g;
        p->blue = b;
        p->alpha = a;
    }
}

static void trp_pix_draw_line_basic( trp_obj_t *dst, uns8b r, uns8b g, uns8b b, uns8b a, double x1, double y1, double x2, double y2, double l1, double l2 )
{
    trp_pix_color_t *pdst, *p;
    uns32b w = ((trp_pix_t *)dst)->w, h = ((trp_pix_t *)dst)->h;

    if ( x1 < 0.0 ) {
        if ( x2 < 0.0 )
            return;
        y1 = trp_pix_draw_linear( 0.0, x1, x2, y1, y2 );
        x1 = 0.0;
    }
    if ( x1 > (double)w - 1.0 ) {
        if ( x2 > (double)w - 1.0 )
            return;
        y1 = trp_pix_draw_linear( (double)w - 1.0, x2, x1, y2, y1 );
        x1 = (double)w - 1.0;
    }
    if ( x2 < 0.0 ) {
        y2 = trp_pix_draw_linear( 0.0, x2, x1, y2, y1 );
        x2 = 0.0;
    }
    if ( x2 > (double)w - 1.0 ) {
        y2 = trp_pix_draw_linear( (double)w - 1.0, x1, x2, y1, y2 );
        x2 = (double)w - 1.0;
    }
    if ( y1 < 0.0 ) {
        if ( y2 < 0.0 )
            return;
        x1 = trp_pix_draw_linear( 0.0, y1, y2, x1, x2 );
        y1 = 0.0;
    }
    if ( y1 > (double)h - 1.0 ) {
        if ( y2 > (double)h - 1.0 )
            return;
        x1 = trp_pix_draw_linear( (double)h - 1.0, y2, y1, x2, x1 );
        y1 = (double)h - 1.0;
    }
    if ( y2 < 0.0 ) {
        x2 = trp_pix_draw_linear( 0.0, y2, y1, x2, x1 );
        y2 = 0.0;
    }
    if ( y2 > (double)h - 1.0 ) {
        x2 = trp_pix_draw_linear( (double)h - 1.0, y1, y2, x1, x2 );
        y2 = (double)h - 1.0;
    }
    pdst = ((trp_pix_t *)dst)->map.c;
    if ( trp_pix_dist( x1, x2 ) >= trp_pix_dist( y1, y2 ) ) {
        double x;
        if ( x1 == x2 ) {
            /*
             vale necessariamente y1 == y2;
             */
            trp_pix_draw_point_basic( dst, r, g, b, a, x1, y1 );
            return;
        }
        if ( x1 > x2 ) {
            x = x1;
            x1 = x2;
            x2 = x;
            x = y1;
            y1 = y2;
            y2 = x;
        }
        if ( l2 == 0.0 ) /* vale necessariamente: x1 < x2 */
            for ( x = x1 ; trp_pix_cast( x ) <= x2 ; x += 1.0 ) {
                p = pdst + trp_pix_cast( x ) + w * trp_pix_cast( trp_pix_draw_linear( x, x1, x2, y1, y2 ) );
                p->red = r;
                p->green = g;
                p->blue = b;
                p->alpha = a;
            }
        else {
            double d = ( y2 - y1 ) / ( x2 - x1 ), l = 0.0;
            d = sqrt( d * d + 1.0 );
            l2 += l1;
            if ( d >= l2 )
                l1 = l2 = d;
            for ( x = x1 ; trp_pix_cast( x ) <= x2 ; x += 1.0 ) {
                l += d;
                if ( l >= l2 )
                    l = 0.0;
                if ( l < l1 ) {
                    p = pdst + trp_pix_cast( x ) + w * trp_pix_cast( trp_pix_draw_linear( x, x1, x2, y1, y2 ) );
                    p->red = r;
                    p->green = g;
                    p->blue = b;
                    p->alpha = a;
                }
            }
        }
    } else {
        double y;
        if ( y1 > y2 ) {
            y = y1;
            y1 = y2;
            y2 = y;
            y = x1;
            x1 = x2;
            x2 = y;
        }
        if ( l2 == 0.0 ) /* vale necessariamente: y1 < y2 */
            for ( y = y1 ; trp_pix_cast( y ) <= y2 ; y += 1.0 ) {
                p = pdst + trp_pix_cast( trp_pix_draw_linear( y, y1, y2, x1, x2 ) ) + w * trp_pix_cast( y );
                p->red = r;
                p->green = g;
                p->blue = b;
                p->alpha = a;
            }
        else {
            double d = ( x2 - x1 ) / ( y2 - y1 ), l = 0.0;
            d = sqrt( d * d + 1.0 );
            l2 += l1;
            if ( d >= l2 )
                l1 = l2 = d;
            for ( y = y1 ; trp_pix_cast( y ) <= y2 ; y += 1.0 ) {
                l += d;
                if ( l >= l2 )
                    l = 0.0;
                if ( l < l1 ) {
                    p = pdst + trp_pix_cast( trp_pix_draw_linear( y, y1, y2, x1, x2 ) ) + w * trp_pix_cast( y );
                    p->red = r;
                    p->green = g;
                    p->blue = b;
                    p->alpha = a;
                }
            }
        }
    }
}

uns8b trp_pix_draw_pix( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src )
{
    trp_pix_color_t *pdst, *psrc;
    double xx, yy;
    uns32b xdst, ydst, wdst, hdst, wsrco, wsrc, hsrc;

    if ( ( dst == src ) || ( src->tipo != TRP_PIX ) || ( dst->tipo != TRP_PIX ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) )
        return 1;
    if ( ( ( pdst = ((trp_pix_t *)dst)->map.c ) == NULL ) ||
         ( ( psrc = ((trp_pix_t *)src)->map.c ) == NULL ) )
        return 1;
    wdst = ((trp_pix_t *)dst)->w;
    hdst = ((trp_pix_t *)dst)->h;
    if ( ( xx > (double)wdst - 1.0 ) || ( yy > (double)hdst - 1.0 ) )
        return 0;
    wsrco = wsrc = ((trp_pix_t *)src)->w;
    hsrc = ((trp_pix_t *)src)->h;
    if ( xx < 0.0 ) {
        xdst = trp_pix_cast( -xx );
        if ( wsrc <= xdst )
            return 0;
        psrc += xdst;
        wsrc -= xdst;
        xx = 0.0;
    }
    if ( yy < 0.0 ) {
        ydst = trp_pix_cast( -yy );
        if ( hsrc <= ydst )
            return 0;
        psrc += ( ydst * wsrco );
        hsrc -= ydst;
        yy = 0.0;
    }
    xdst = trp_pix_cast( xx );
    ydst = trp_pix_cast( yy );
    if ( wsrc > wdst - xdst )
        wsrc = wdst - xdst;
    if ( hsrc > hdst - ydst )
        hsrc = hdst - ydst;
    pdst += ( xdst + ydst * wdst );
    wsrc <<= 2;
    for ( ; hsrc ; hsrc--, pdst += wdst, psrc += wsrco )
        memcpy( pdst, psrc, wsrc );
    return 0;
}

uns8b trp_pix_draw_pix_alpha( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src )
{
    trp_pix_color_t *pdst, *psrc;
    double xx, yy;
    uns32b xdst, ydst, wdst, hdst, wsrco, wsrc, hsrc;
    uns8b alpha, nalpha;

    if ( ( dst == src ) || ( src->tipo != TRP_PIX ) || ( dst->tipo != TRP_PIX ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) )
        return 1;
    if ( ( ( pdst = ((trp_pix_t *)dst)->map.c ) == NULL ) ||
         ( ( psrc = ((trp_pix_t *)src)->map.c ) == NULL ) )
        return 1;
    wdst = ((trp_pix_t *)dst)->w;
    hdst = ((trp_pix_t *)dst)->h;
    if ( ( xx > (double)wdst - 1.0 ) || ( yy > (double)hdst - 1.0 ) )
        return 0;
    wsrco = wsrc = ((trp_pix_t *)src)->w;
    hsrc = ((trp_pix_t *)src)->h;
    if ( xx < 0.0 ) {
        xdst = trp_pix_cast( -xx );
        if ( wsrc <= xdst )
            return 0;
        psrc += xdst;
        wsrc -= xdst;
        xx = 0.0;
    }
    if ( yy < 0.0 ) {
        ydst = trp_pix_cast( -yy );
        if ( hsrc <= ydst )
            return 0;
        psrc += ( ydst * wsrco );
        hsrc -= ydst;
        yy = 0.0;
    }
    xdst = trp_pix_cast( xx );
    ydst = trp_pix_cast( yy );
    if ( wsrc > wdst - xdst )
        wsrc = wdst - xdst;
    if ( hsrc > hdst - ydst )
        hsrc = hdst - ydst;
    pdst += ( xdst + ydst * wdst );
    for ( ; hsrc ; hsrc--, pdst += wdst, psrc += wsrco )
        for ( xdst = 0 ; xdst < wsrc ; xdst++ ) {
            alpha = psrc[ xdst ].alpha;
            nalpha = ~alpha;
            pdst[ xdst ].red = ( (int)( pdst[ xdst ].red ) * (int)nalpha + (int)( psrc[ xdst ].red ) * (int)alpha ) / 255;
            pdst[ xdst ].green = ( (int)( pdst[ xdst ].green ) * (int)nalpha + (int)( psrc[ xdst ].green ) * (int)alpha ) / 255;
            pdst[ xdst ].blue = ( (int)( pdst[ xdst ].blue ) * (int)nalpha + (int)( psrc[ xdst ].blue ) * (int)alpha ) / 255;
            pdst[ xdst ].alpha = ( (int)( pdst[ xdst ].alpha ) * (int)nalpha + (int)alpha * (int)alpha ) / 255;
        }
    return 0;
}

uns8b trp_pix_draw_pix_odd_lines( trp_obj_t *dst, trp_obj_t *src )
{
    trp_pix_color_t *pdst, *psrc;
    uns32b w, h, ww, wwww;

    if ( ( dst == src ) || ( src->tipo != TRP_PIX ) || ( dst->tipo != TRP_PIX ) )
        return 1;
    if ( ( ( pdst = ((trp_pix_t *)dst)->map.c ) == NULL ) ||
         ( ( psrc = ((trp_pix_t *)src)->map.c ) == NULL ) )
        return 1;
    w = ((trp_pix_t *)dst)->w;
    h = ((trp_pix_t *)dst)->h;
    if ( ( ((trp_pix_t *)src)->w != w ) ||
         ( ((trp_pix_t *)src)->h != h ) )
        return 1;
    ww = w << 1;
    wwww = w << 2;
    pdst += w;
    psrc += w;
    for ( h = ( h - 1 ) >> 1 ; h ; h--, pdst += ww, psrc += ww )
        memcpy( pdst, psrc, wwww );
    return 0;
}

uns8b trp_pix_draw_box( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h, trp_obj_t *color )
{
    double xx, yy, ww, hh;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) ||
         trp_cast_double( w, &ww ) ||
         trp_cast_double( h, &hh ) )
        return 1;
    if ( ( ww <= 0.0 ) || ( hh <= 0.0 ) )
        return 1;
    trp_pix_draw_box_basic( dst, r, g, b, a, xx, yy, ww, hh );
    return 0;
}

uns8b trp_pix_draw_point( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *color )
{
    double xx, yy;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) )
        return 1;
    trp_pix_draw_point_basic( dst, r, g, b, a, xx, yy );
    return 0;
}

uns8b trp_pix_draw_line( trp_obj_t *dst, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *color )
{
    double xx1, yy1, xx2, yy2;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) ||
         trp_cast_double( x1, &xx1 ) ||
         trp_cast_double( y1, &yy1 ) ||
         trp_cast_double( x2, &xx2 ) ||
         trp_cast_double( y2, &yy2 ) )
        return 1;
    trp_pix_draw_line_basic( dst, r, g, b, a, xx1, yy1, xx2, yy2, 0.0, 0.0 );
    return 0;
}

uns8b trp_pix_draw_dashed_line( trp_obj_t *dst, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2,  trp_obj_t *l1, trp_obj_t *l2, trp_obj_t *color )
{
    double xx1, yy1, xx2, yy2, ll1, ll2;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) ||
         trp_cast_double( x1, &xx1 ) ||
         trp_cast_double( y1, &yy1 ) ||
         trp_cast_double( x2, &xx2 ) ||
         trp_cast_double( y2, &yy2 ) ||
         trp_cast_double( l1, &ll1 ) ||
         trp_cast_double( l2, &ll2 ) )
        return 1;
    if ( ( ll1 <= 0.0 ) || ( ll2 <= 0.0 ) )
        return 1;
    trp_pix_draw_line_basic( dst, r, g, b, a, xx1, yy1, xx2, yy2, ll1, ll2 );
    return 0;
}

uns8b trp_pix_draw_circle( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *rad, trp_obj_t *color )
{
    double xx, yy, rr, i, j, err;
    uns8b r, g, b, a;

    if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) ||
         trp_cast_double( rad, &rr ) )
        return 1;
    if ( rr < 0 )
        return 1;
    i = -rr;
    j = 0.0;
    err = 2.0 - 2.0 * rr;
    do {
        trp_pix_draw_point_basic( dst, r, g, b, a, xx - i, yy + j );
        trp_pix_draw_point_basic( dst, r, g, b, a, xx - j, yy - i );
        trp_pix_draw_point_basic( dst, r, g, b, a, xx + i, yy - j );
        trp_pix_draw_point_basic( dst, r, g, b, a, xx + j, yy + i );
        if ( err > i ) {
            i += 1.0;
            err += i * 2.0 + 1.0;
        } else {
            j += 1.0;
            err += j * 2.0 + 1.0;
        }
    } while ( i < 0 );
    return 0;
}

uns8b trp_pix_draw_grid( trp_obj_t *dst, trp_obj_t *size, trp_obj_t *color )
{
    trp_pix_color_t *c;
    uns32b ssize, xstart, ystart, w, h, i, j;

    if ( ( dst->tipo != TRP_PIX ) || trp_cast_uns32b_range( size, &ssize, 1, 0xffffffff ) )
        return 1;
    if ( ( c = ((trp_pix_t *)dst)->map.c ) == NULL )
        return 1;
    w = ((trp_pix_t *)dst)->w;
    h = ((trp_pix_t *)dst)->h;
    xstart = ( w - 1 - ( ( w - 1 + ssize ) / ssize - 1 ) * ssize ) >> 1;
    ystart = ( h - 1 - ( ( h - 1 + ssize ) / ssize - 1 ) * ssize ) >> 1;
    if ( color ) {
        trp_pix_color_t *d;
        uns8b r, g, b, a;

        if ( trp_pix_decode_color_uns8b( color, dst, &r, &g, &b, &a ) )
            return 1;
        for ( i = xstart ; i < w ; i += ssize )
            for ( j = 0, d = c + i ; j < h ; j++, d += w ) {
                d->red = r;
                d->green = g;
                d->blue = b;
                d->alpha = a;
            }
        for ( i = ystart ; i < h ; i += ssize )
            for ( j = 0, d = c + w * i ; j < w ; j++, d++ ) {
                d->red = r;
                d->green = g;
                d->blue = b;
                d->alpha = a;
            }
    } else {
        trp_pix_color_t *d;

        for ( i = xstart ; i < w ; i += ssize )
            for ( j = 0, d = c + i ; j < h ; j++, d += w ) {
                d->red = ~( d->red );
                d->green = ~( d->green );
                d->blue = ~( d->blue );
            }
        for ( i = ystart ; i < h ; i += ssize )
            for ( j = 0, d = c + w * i ; j < w ; j++, d++ ) {
                d->red = ~( d->red );
                d->green = ~( d->green );
                d->blue = ~( d->blue );
            }
    }
    return 0;
}

/*

uns8b trp_pix_draw_text( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *text, ... )
{
    uns32b xdst, ydst;
    uns8b *p;
    va_list args;

    if ( trp_cast_uns32b( x, &xdst ) ||
         trp_cast_uns32b( y, &ydst ) )
        return 1;
    if ( trp_pix_lock_and_context_set_image( pix ) )
        return 1;
    if ( ((trp_pix_t *)pix)->ft == NULL ) {
        trp_pix_unlock();
        return 1;
    }
    imlib_context_set_font( ((trp_pix_t *)pix)->ft );
    imlib_context_set_color( colorval( ((trp_pix_t *)pix)->color.red ),
                             colorval( ((trp_pix_t *)pix)->color.green ),
                             colorval( ((trp_pix_t *)pix)->color.blue ),
                             colorval( ((trp_pix_t *)pix)->color.alpha ) );
    va_start( args, text );
    p = trp_csprint_multi( text, args );
    va_end( args );
    imlib_text_draw( xdst, ydst, p );
    trp_csprint_free( p );
    trp_pix_unlock();
    return 0;
}

*/

