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

static trp_obj_t *trp_pix_top_bottom_field( uns8b bottom, trp_obj_t *pix );
static void trp_pix_colormod_set_table( uns8b tipo, uns8b *t, double v );
static void trp_pix_colormod_basic( uns8b tipo, trp_pix_color_t *map, uns32b w, uns32b h, double vr, double vg, double vb );

#define TRP_PIX_COLORS_TYPE_MAX 300

uns16b trp_pix_colors_type( trp_pix_t *pix )
{
    uns32b n = pix->w * pix->h, i;
    uns16b ss, j;
    trp_pix_color_t *map = pix->map.c, s[ TRP_PIX_COLORS_TYPE_MAX + 1 ];
    uns8b gray, bw;

    for ( gray = 1, bw = 1, i = 0 ; i < n ; i++ ) {
        if ( ( map[ i ].red != map[ i ].green ) ||
             ( map[ i ].green != map[ i ].blue ) ||
             ( map[ i ].alpha != 0xff ) ) {
            gray = 0;
            break;
        }
        if ( ( map[ i ].red != 0 ) && ( map[ i ].red != 0xff ) )
            bw = 0;
    }
    if ( gray )
        return 0xffff - bw;
    for ( ss = 0, i = 0 ; i < n ; i++ ) {
        for ( j = 0 ; j < ss ; j++ )
            if ( s[ j ].rgba == map[ i ].rgba )
                break;
        if ( j == ss ) {
            if ( ss == TRP_PIX_COLORS_TYPE_MAX )
                break;
            s[ ss++ ].rgba = map[ i ].rgba;
        }
    }
    return ss;
}

uns8b trp_pix_has_alpha( trp_pix_t *pix )
{
    uns32b n = pix->w * pix->h, i;
    trp_pix_color_t *map = pix->map.c;

    for ( i = 0 ; i < n ; i++ )
        if ( map[ i ].alpha != 0xff )
            return 1;
    return 0;
}

trp_obj_t *trp_pix_grayp( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return ( trp_pix_colors_type( (trp_pix_t *)pix ) & 0xfffe == 0xfffe ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_pix_bwp( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return ( trp_pix_colors_type( (trp_pix_t *)pix ) == 0xfffe ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_pix_point( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y )
{
    trp_pix_color_t *map;
    double xx, yy;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) )
        return UNDEF;
    if ( ( ((trp_pix_t *)pix)->map.p == NULL ) ||
         ( xx < 0.0 ) || ( yy < 0.0 ) ||
         ( xx > (double)( ((trp_pix_t *)pix)->w ) - 1.0 ) ||
         ( yy > (double)( ((trp_pix_t *)pix)->h ) - 1.0 ) )
        return UNDEF;
    map = ((trp_pix_t *)pix)->map.c + trp_pix_cast( xx ) + ((trp_pix_t *)pix)->w * trp_pix_cast( yy );
    return trp_pix_create_color( 257 * map->red, 257 * map->green,
                                 257 * map->blue, 257 * map->alpha );
}

trp_obj_t *trp_pix_color_count( trp_obj_t *pix, trp_obj_t *color )
{
    uns32b i, n;
    trp_pix_color_t c, *p;

    if ( trp_pix_decode_color_uns8b( color, pix, &( c.red ), &( c.green ),
                                     &( c.blue ), &( c.alpha ) ) )
        return UNDEF;

    for ( p = ((trp_pix_t *)pix)->map.c, i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h, n = 0 ; i ; )
        if ( p[ --i ].rgba == c.rgba )
            ++n;
    return trp_sig64( n );
}

static trp_obj_t *trp_pix_top_bottom_field( uns8b bottom, trp_obj_t *pix )
{
    trp_pix_color_t *p;
    uns32b w, h;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( p = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( h & 1 )
        return UNDEF;
    h >>= 1;
    if ( ( pix = trp_pix_create_basic( w, h ) ) != UNDEF ) {
        trp_pix_color_t *q = ((trp_pix_t *)pix)->map.c;
        uns32b w2 = w << 1, w4 = w << 2;
        if ( bottom )
            p += w;
        for ( ; h ; h--, q += w, p += w2 )
            memcpy( q, p, w4 );
    }
    return pix;
}

trp_obj_t *trp_pix_top_field( trp_obj_t *pix )
{
    return trp_pix_top_bottom_field( 0, pix );
}

trp_obj_t *trp_pix_bottom_field( trp_obj_t *pix )
{
    return trp_pix_top_bottom_field( 1, pix );
}

trp_obj_t *trp_pix_crop_low( trp_obj_t *pix, double xx, double yy, double ww, double hh )
{
    trp_pix_color_t *p;
    uns32b pw, ph, sx, sy, sw, sh, sw4;

    if ( ( p = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    pw = ((trp_pix_t *)pix)->w;
    ph = ((trp_pix_t *)pix)->h;
    if ( xx < 0.0 ) {
        ww += xx;
        xx = 0.0;
    }
    if ( yy < 0.0 ) {
        hh += yy;
        yy = 0.0;
    }
    if ( ( xx > (double)pw - 1.0 ) || ( yy > (double)ph - 1.0 ) ||
         ( ww < 1.0 ) || ( hh < 1.0 ) )
        return UNDEF;
    sx = trp_pix_cast( xx );
    sy = trp_pix_cast( yy );
    sw = trp_pix_cast( ww );
    sh = trp_pix_cast( hh );
    if ( sw > pw - sx )
        sw = pw - sx;
    if ( sh > ph - sy )
        sh = ph - sy;
    if ( ( pix = trp_pix_create_basic( sw, sh ) ) != UNDEF ) {
        trp_pix_color_t *q = ((trp_pix_t *)pix)->map.c;
        p += ( sx + pw * sy );
        sw4 = sw << 2;
        for ( ; sh ; sh--, q += sw, p += pw )
            memcpy( q, p, sw4 );
    }
    return pix;
}

trp_obj_t *trp_pix_crop( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h )
{
    double xx, yy, ww, hh;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_cast_double( x, &xx ) ||
         trp_cast_double( y, &yy ) ||
         trp_cast_double( w, &ww ) ||
         trp_cast_double( h, &hh ) )
        return UNDEF;
    return trp_pix_crop_low( pix, xx, yy, ww, hh );
}

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
    uns8b t;

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
    trp_pix_color_t *c;
    uns32b i;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = ( (uns32b)( c->red ) * 299 +
                                        (uns32b)( c->green ) * 587 +
                                        (uns32b)( c->blue ) * 114 + 500 ) / 1000;
    return 0;
}

uns8b trp_pix_gray16( trp_obj_t *pix )
{
    trp_pix_color_t *c;
    uns32b i;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = ( ( ( ( ( (uns32b)( c->red ) * 299 +
                                                (uns32b)( c->green ) * 587 +
                                                (uns32b)( c->blue ) * 114 + 500 ) / 1000 ) << 1 ) + 17 ) / 34 ) * 17;
    return 0;
}

uns8b trp_pix_gray_maximize_range( trp_obj_t *pix, trp_obj_t *black )
{
    trp_pix_color_t *c;
    uns32b i;
    sig32b bl, p, pmin, pmax;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    if ( black ) {
        if ( trp_cast_sig32b_range( black, &bl, 0, 0xff ) )
            return 1;
    } else
        bl = -1;
    pmin = 255;
    pmax = 0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        p = c->red = ( (uns32b)( c->red ) * 299 +
                       (uns32b)( c->green ) * 587 +
                       (uns32b)( c->blue ) * 114 + 500 ) / 1000;
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
        if ( p > 255 )
            p = 255;
        c->red = c->green = c->blue = p;
    }
    return 0;
}

uns8b trp_pix_bw( trp_obj_t *pix, trp_obj_t *threshold )
{
    trp_pix_color_t *c;
    uns32b i;
    uns32b thre;

    if ( threshold ) {
        if ( trp_cast_uns32b_range( threshold, &thre, 0, 256 ) )
            return 1;
    } else
        thre = 128;
    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        c->red = c->green = c->blue = ( ( ( (uns32b)( c->red ) * 299 +
                                            (uns32b)( c->green ) * 587 +
                                            (uns32b)( c->blue ) * 114 + 500 ) / 1000 ) < thre ) ? 0 : 0xff;
    return 0;
}

uns8b trp_pix_linear( trp_obj_t *pix, trp_obj_t *min1, trp_obj_t *max1, trp_obj_t *min2, trp_obj_t *max2 )
{
    trp_pix_color_t *c;
    uns32b i;
    double dmin1, dmax1, dmin2, dmax2, d;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    if ( trp_cast_double( min1, &dmin1 ) ||
         trp_cast_double( max1, &dmax1 ) ||
         trp_cast_double( min2, &dmin2 ) ||
         trp_cast_double( max2, &dmax2 ) )
        return 1;
    if ( dmin1 == dmax1 )
        return 1;
    dmax1 = ( dmax2 - dmin2 ) / ( dmax1 - dmin1 );
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ ) {
        d = ( (double)( c->red ) - dmin1 ) * dmax1 + dmin2;
        if ( d <= 0.0 )
            c->red = 0;
        else if ( d >= 255.0 )
            c->red = 255;
        else
            c->red = (uns8b)( d + 0.5 );
        d = ( (double)( c->green ) - dmin1 ) * dmax1 + dmin2;
        if ( d <= 0.0 )
            c->green = 0;
        else if ( d >= 255.0 )
            c->green = 255;
        else
            c->green = (uns8b)( d + 0.5 );
        d = ( (double)( c->blue ) - dmin1 ) * dmax1 + dmin2;
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
   trp_pix_color_t *c;
    uns32b i;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
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

uns8b trp_pix_hflip( trp_obj_t *pix )
{
    trp_pix_color_t *c;
    uns32b w, w2, i, j, k, l;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
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
    trp_pix_color_t *c, *p, *q;
    uns32b w, h, i, j, k;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
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

uns8b trp_pix_rotate_test_low( trp_obj_t *pix, sig64b a )
{
    trp_pix_color_t *c;
    uns32b w, h, i, j, n, m;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    switch ( ( a >= 0 ) ? ( a % 360 ) : ( 360 - (-a) % 360 ) ) {
    case 0:
        return 0;
    case 90:
        {
            trp_pix_color_t *d;

            n = ( w * h ) << 2;
            if ( ( d = malloc( n ) ) == NULL )
                return 1;
            j = w * h - 1;
            for ( i = 0 ; i <= j ; i++ )
                d[ ( w - 1 - i / h ) * h + ( i % h ) ].rgba = c[ ( i == j ) ? j : ( ( (uns64b)i * (uns64b)w ) % (uns64b)j ) ].rgba;
            memcpy( c, d, n );
            free( d );
        }
        ((trp_pix_t *)pix)->w = h;
        ((trp_pix_t *)pix)->h = w;
        return 0;
    case 180:
        for ( n = 0, i = 0 ; i < ( ( h + 1 ) >> 1 ) ; i++ )
            for ( j = 0 ; j < w ; j++, n++ ) {
                m = ( h - 1 - i ) * w + ( w - 1 - j );
                if ( n < m ) {
                    a = c[ n ].rgba;
                    c[ n ].rgba = c[ m ].rgba;
                    c[ m ].rgba = a;
                }
            }
        return 0;
    case 270:
        {
            trp_pix_color_t *d;

            n = ( w * h ) << 2;
            if ( ( d = malloc( n ) ) == NULL )
                return 1;
            j = w * h - 1;
            for ( i = 0 ; i <= j ; i++ )
                d[ i - ( ( i % h ) << 1 ) + h - 1 ].rgba = c[ ( i == j ) ? j : ( ( (uns64b)i * (uns64b)w ) % (uns64b)j ) ].rgba;
            memcpy( c, d, n );
            free( d );
        }
        ((trp_pix_t *)pix)->w = h;
        ((trp_pix_t *)pix)->h = w;
        return 0;
    }
    return 1;
}

uns8b trp_pix_rotate_test( trp_obj_t *pix, trp_obj_t *angle )
{
    sig64b a;

    if ( trp_cast_sig64b( angle, &a ) )
        return 1;
    return trp_pix_rotate_test_low( pix, a );
}

trp_obj_t *trp_pix_rotate_orthogonal( trp_obj_t *pix, sig64b a )
{
    trp_obj_t *res;
    uns32b w, h;

    if ( a % 90 )
        return UNDEF;
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( res = trp_pix_create_basic( w, h ) ) != UNDEF ) {
        memcpy( ((trp_pix_t *)res)->map.p, ((trp_pix_t *)pix)->map.p, ( w * h ) << 2 );
        trp_pix_rotate_test_low( res, a );
    }
    return res;
}

trp_obj_t *trp_pix_rotate( trp_obj_t *pix, trp_obj_t *angle )
{
    extern objfun_t _trp_pix_rotate_lept;
    extern objfun_t _trp_pix_rotate_cv;
    sig64b a;

    if ( trp_cast_sig64b( angle, &a ) == 0 ) {
        trp_obj_t *res = trp_pix_rotate_orthogonal( pix, a );
        if ( res != UNDEF )
            return res;
    }
    if ( _trp_pix_rotate_lept )
        return ( _trp_pix_rotate_lept )( pix, angle );
    if ( _trp_pix_rotate_cv )
        return ( _trp_pix_rotate_cv )( pix, angle, NULL );
    return UNDEF;
}

static void trp_pix_colormod_set_table( uns8b tipo, uns8b *t, double v )
{
    double td[ 256 ];
    int i;

    switch ( tipo ) {
    case 0: /* brightness */
        for ( i = 0 ; i < 256 ; i++ )
            td[ i ] = (double)i / 255.0 + v;
        break;
    case 1: /* contrast */
        for ( i = 0 ; i < 256 ; i++ )
            td[ i ] = ( (double)i / 255.0 - 0.5 ) * v + 0.5;
        break;
    case 2: /* gamma */
        v = 1.0 / v;
        for ( i = 0 ; i < 256 ; i++ )
            td[ i ] = pow( (double)i / 255.0, v );
        break;
    }
    for ( i = 0 ; i < 256 ; i++ ) {
        if ( td[ i ] < 0 )
            td[ i ] = 0;
        if ( td[ i ] > 1 )
            td[ i ] = 1;
        t[ i ] = (uns8b)( td[ i ] * 255.0 + 0.5 );
    }
}

static void trp_pix_colormod_basic( uns8b tipo, trp_pix_color_t *map, uns32b w, uns32b h, double vr, double vg, double vb )
{
    static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    static double oldvr[ 3 ] = { 0.0, 0.0, 0.0 }, oldvg[ 3 ] = { 0.0, 0.0, 0.0 }, oldvb[ 3 ] = { 0.0, 0.0, 0.0 };
    static uns8b tr[ 3 ][ 256 ], tg[ 3 ][ 256 ], tb[ 3 ][ 256 ];
    uns32b i;

    if ( ( ( tipo == 0 ) && ( vr == 0.0 ) && ( vg == 0.0 ) && ( vb == 0.0 ) ) ||
         ( ( ( tipo == 1 ) || ( tipo == 2 ) ) && ( vr == 1.0 ) && ( vg == 1.0 ) && ( vb == 1.0 ) ) )
        return;
    pthread_mutex_lock( &mut );
    if ( vr != oldvr[ tipo ] ) {
        trp_pix_colormod_set_table( tipo, tr[ tipo ], vr );
        oldvr[ tipo ] = vr;
    }
    if ( vg != oldvg[ tipo ] ) {
        trp_pix_colormod_set_table( tipo, tg[ tipo ], vg );
        oldvg[ tipo ] = vg;
    }
    if ( vb != oldvb[ tipo ] ) {
        trp_pix_colormod_set_table( tipo, tb[ tipo ], vb );
        oldvb[ tipo ] = vb;
    }
    if ( map )
        for ( i = w * h ; i ; i--, map++ ) {
            map->red = tr[ tipo ][ map->red ];
            map->green = tg[ tipo ][ map->green ];
            map->blue = tb[ tipo ][ map->blue ];
        }
    pthread_mutex_unlock( &mut );
}

void trp_pix_colormod_init()
{
    trp_pix_colormod_basic( 0, NULL, 0, 0, 1.0, 1.0, 1.0 );
    trp_pix_colormod_basic( 1, NULL, 0, 0, 1.0, 1.0, 1.0 );
    trp_pix_colormod_basic( 2, NULL, 0, 0, 1.0, 1.0, 1.0 );
}

uns8b trp_pix_brightness( trp_obj_t *pix, trp_obj_t *val )
{
    double v;

    if ( ( pix->tipo != TRP_PIX ) || trp_cast_double( val, &v ) )
        return 1;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return 1;
    trp_pix_colormod_basic( 0, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            v, v, v );
    return 0;
}

uns8b trp_pix_brightness_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b )
{
    double vr, vg, vb;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_cast_double( val_r, &vr ) ||
         trp_cast_double( val_g, &vg ) ||
         trp_cast_double( val_b, &vb ) )
        return 1;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return 1;
    trp_pix_colormod_basic( 0, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            vr, vg, vb );
    return 0;
}

uns8b trp_pix_contrast( trp_obj_t *pix, trp_obj_t *val )
{
    double v;

    if ( ( pix->tipo != TRP_PIX ) || trp_cast_double( val, &v ) )
        return 1;
    if ( ( ((trp_pix_t *)pix)->map.p == NULL ) || ( v < 0.0 ) )
        return 1;
    trp_pix_colormod_basic( 1, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            v, v, v );
    return 0;
}

uns8b trp_pix_contrast_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b )
{
    double vr, vg, vb;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_cast_double( val_r, &vr ) ||
         trp_cast_double( val_g, &vg ) ||
         trp_cast_double( val_b, &vb ) )
        return 1;
    if ( ( ((trp_pix_t *)pix)->map.p == NULL ) ||
         ( vr < 0.0 ) || ( vg < 0.0 ) || ( vb < 0.0 ) )
        return 1;
    trp_pix_colormod_basic( 1, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            vr, vg, vb );
    return 0;
}

uns8b trp_pix_gamma( trp_obj_t *pix, trp_obj_t *val )
{
    double v;

    if ( ( pix->tipo != TRP_PIX ) || trp_cast_double( val, &v ) )
        return 1;
    if ( ( ((trp_pix_t *)pix)->map.p == NULL ) || ( v <= 0.0 ) )
        return 1;
    trp_pix_colormod_basic( 2, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            v, v, v );
    return 0;
}

uns8b trp_pix_gamma_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b )
{
    double vr, vg, vb;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_cast_double( val_r, &vr ) ||
         trp_cast_double( val_g, &vg ) ||
         trp_cast_double( val_b, &vb ) )
        return 1;
    if ( ( ((trp_pix_t *)pix)->map.p == NULL ) ||
         ( vr <= 0.0 ) || ( vg <= 0.0 ) || ( vb <= 0.0 ) )
        return 1;
    trp_pix_colormod_basic( 2, ((trp_pix_t *)pix)->map.c,
                            ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                            vr, vg, vb );
    return 0;
}

uns8b trp_pix_snap_color( trp_obj_t *pix,  trp_obj_t *src_color,  trp_obj_t *thres, trp_obj_t *dst_color )
{
    uns32b i;
    trp_pix_color_t *c;
    double th, diff;
    uns16b j;
    uns8b sr, sg, sb, sa, dr, dg, db, da;

    if ( ( pix->tipo != TRP_PIX ) ||
         trp_pix_decode_color_uns8b( src_color, NULL, &sr, &sg, &sb, &sa ) ||
         trp_cast_double_range( thres, &th, 0.0, 255.0 ) )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
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
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; ) {
        i--;
        diff = 0.0;
        j = trp_pix_dist( c[ i ].red, sr );
        diff += (double)( j * j );
        j = trp_pix_dist( c[ i ].green, sg );
        diff += (double)( j * j );
        j = trp_pix_dist( c[ i ].blue, sb );
        diff += (double)( j * j );
        if ( diff <= th ) {
            c[ i ].red = dr;
            c[ i ].green = dg;
            c[ i ].blue = db;
            c[ i ].alpha = da;
        }
    }
    return 0;
}

uns8b trp_pix_scale_test( trp_obj_t *pix_i, trp_obj_t *pix_o )
{
    static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    static uns32b *minj = NULL;
    static uns32b prev_wi = 0, prev_hi = 0, prev_wo = 0, prev_ho = 0;
    uns64b tcol[ 4 ];
    uns8b *mapi, *mapo, *q, *r, *pre_r, *pre_pre_r;
    uns32b *maxj, *mini, *maxi, *pesominj, *pesomaxj, *pesomini, *pesomaxi;
    uns32b wi, hi, wo, ho;
    uns32b btpp = 4, btppwi, area, aream, peso, pesoy,
        pesointermedio, pesomassimo, x, y, i, j;

    if ( ( pix_i->tipo != TRP_PIX ) || ( pix_i->tipo != TRP_PIX ) )
        return 1;
    mapi = ((trp_pix_t *)pix_i)->map.p;
    mapo = ((trp_pix_t *)pix_o)->map.p;
    if ( ( mapi == NULL ) || ( mapo == NULL ) )
        return 1;
    wi = ((trp_pix_t *)pix_i)->w;
    hi = ((trp_pix_t *)pix_i)->h;
    wo = ((trp_pix_t *)pix_o)->w;
    ho = ((trp_pix_t *)pix_o)->h;
    pthread_mutex_lock( &mut );
    if ( ( wo != prev_wo ) || ( ho != prev_ho ) ) {
        if ( minj )
            minj = trp_gc_realloc( minj, ( sizeof( uns32b ) * ( wo + ho ) ) << 2 );
        else
            minj = trp_gc_malloc_atomic( ( sizeof( uns32b ) * ( wo + ho ) ) << 2 );
    }
    maxj = minj + ho;
    mini = maxj + ho;
    maxi = mini + wo;
    pesominj = maxi + wo;
    pesomaxj = pesominj + ho;
    pesomini = pesomaxj + ho;
    pesomaxi = pesomini + wo;
    area = wi * hi;
    aream = area >> 1;
    if ( ( wo != prev_wo ) || ( ho != prev_ho ) ||
         ( wi != prev_wi ) || ( hi != prev_hi ) ) {
        for ( y = 0 ; y < ho ; y++ ) {
            minj[ y ] = ( y * hi ) / ho;
            maxj[ y ] = ( ( y + 1 ) * hi + ho - 1 ) / ho - 1;
            pesominj[ y ] = ho + ho * minj[ y ] - y * hi;
            if ( minj[ y ] < maxj[ y ] )
                pesomaxj[ y ] = ( y + 1 ) * hi - ho * maxj[ y ];
            else
                pesomaxj[ y ] = pesominj[ y ] + ( y + 1 ) * hi - ho * maxj[ y ] - ho;
        }
        for ( x = 0 ; x < wo ; x++ ) {
            mini[ x ] = ( x * wi ) / wo;
            maxi[ x ] = ( ( x + 1 ) * wi + wo - 1 ) / wo - 1;
            pesomini[ x ] = wo + wo * mini[ x ] - x * wi;
            if ( mini[ x ] < maxi[ x ] )
                pesomaxi[ x ] = ( x + 1 ) * wi - wo * maxi[ x ];
            else
                pesomaxi[ x ] = pesomini[ x ] + ( x + 1 ) * wi - wo * maxi[ x ] - wo;
        }
        prev_wo = wo;
        prev_ho = ho;
        prev_wi = wi;
        prev_hi = hi;
    }
    pesomassimo = wo * ho;
    btppwi = btpp * wi;
    q = mapo;
    for ( y = 0 ; y < ho ; y++ ) {
        pre_pre_r = mapi + btpp * minj[ y ] * wi;
        for ( x = 0 ; x < wo ; x++ ) {
            tcol[ 0 ] = tcol[ 1 ] = tcol[ 2 ] = tcol[ 3 ] = aream;
            pesoy = pesominj[ y ];
            pesointermedio = wo * pesoy;
            pre_r = pre_pre_r + btpp * mini[ x ];
            for ( j = minj[ y ] ; ; j++ ) {
                if ( j == maxj[ y ] ) {
                    pesoy = pesomaxj[ y ];
                    pesointermedio = wo * pesoy;
                }
                peso = pesomini[ x ] * pesoy;
                r = pre_r;
                for ( i = mini[ x ] ; ; i++ ) {
                    if ( i == maxi[ x ] )
                        peso = pesomaxi[ x ] * pesoy;
                    tcol[ 0 ] += peso * (uns32b)( *r++ );
                    tcol[ 1 ] += peso * (uns32b)( *r++ );
                    tcol[ 2 ] += peso * (uns32b)( *r++ );
                    tcol[ 3 ] += peso * (uns32b)( *r++ );
                    if ( i == maxi[ x ] )
                        break;
                    peso = pesointermedio;
                }
                if ( j == maxj[ y ] )
                    break;
                pesoy = ho;
                pesointermedio = pesomassimo;
                pre_r += btppwi;
            }
            *q++ = tcol[ 0 ] / area;
            *q++ = tcol[ 1 ] / area;
            *q++ = tcol[ 2 ] / area;
            *q++ = tcol[ 3 ] / area;
        }
    }
    pthread_mutex_unlock( &mut );
    return 0;
}

trp_obj_t *trp_pix_scale( trp_obj_t *pix, trp_obj_t *w, trp_obj_t *h )
{
    trp_obj_t *res;

    if ( h )
        res = trp_pix_create( w, h );
    else {
        uns32b wi, hi, ww, hh;

        if ( ( pix->tipo != TRP_PIX ) || trp_cast_uns32b_rint_range( w, &ww, 1, 0xffff ) )
            return UNDEF;
        if ( ((trp_pix_t *)pix)->map.p == NULL )
            return UNDEF;
        wi = ((trp_pix_t *)pix)->w;
        hi = ((trp_pix_t *)pix)->h;
        if ( wi >= hi ) {
            hh = ( ww * hi + ( wi >> 1 ) ) / wi;
        } else {
            hh = ww;
            ww = ( hh * wi + ( hi >> 1 ) ) / hi;
        }
        res = trp_pix_create_basic( ww, hh );
    }
    if ( res != UNDEF )
        if ( trp_pix_scale_test( pix, res ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            trp_gc_free( res );
            res = UNDEF;
        }
    return res;
}

/*
 questa implementazione di trp_pix_mse e` corretta per immagini
 fino a una risoluzione massima di 11909 x 11909
 */

trp_obj_t *trp_pix_mse( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_color_t *map1, *map2;
    sig64b tot, j;
    uns32b w, h, n, i;

    if ( ( pix1->tipo != TRP_PIX ) || ( pix2->tipo != TRP_PIX ) )
        return UNDEF;
    if ( ( ( map1 = ((trp_pix_t *)pix1)->map.c ) == NULL ) ||
         ( ( map2 = ((trp_pix_t *)pix2)->map.c ) == NULL ) )
        return UNDEF;
    w = ((trp_pix_t *)pix1)->w;
    h = ((trp_pix_t *)pix1)->h;
    if ( ( w != ((trp_pix_t *)pix2)->w ) ||
         ( h != ((trp_pix_t *)pix2)->h ) )
        return UNDEF;
    n = w * h;
    for ( i = n, tot = 0 ; i ; ) {
        i--;
        j = ( ( (sig32b)( map1[ i ].red ) ) - ( (sig32b)( map2[ i ].red ) ) ) * 299 +
            ( ( (sig32b)( map1[ i ].green ) ) - ( (sig32b)( map2[ i ].green ) ) ) * 587 +
            ( ( (sig32b)( map1[ i ].blue ) ) - ( (sig32b)( map2[ i ].blue ) ) ) * 114;
        tot += j * j;
    }
    return trp_math_ratio( trp_sig64( tot ), trp_sig64( 65025000000LL * (sig64b)n ), NULL );
}

trp_obj_t *trp_pix_gray_histogram( trp_obj_t *pix )
{
    trp_array_t *obj;
    trp_pix_color_t *c;
    uns32b i, h[ 256 ];

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    for ( i = 0 ; i < 256 ; i++ )
        h[ i ] = 0;
    for ( i = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ; i ; i--, c++ )
        ++h[ ( (uns32b)( c->red ) * 299 +
               (uns32b)( c->green ) * 587 +
               (uns32b)( c->blue ) * 114 + 500 ) / 1000 ];
    obj = trp_gc_malloc( sizeof( trp_array_t ) );
    obj->tipo = TRP_ARRAY;
    obj->incr = 1;
    obj->len = 256;
    obj->data = trp_gc_malloc( sizeof( trp_obj_t * ) * 256 );
    for ( i = 0 ; i < 256 ; i++ )
        obj->data[ i ] = trp_sig64( (sig64b)( h[ i ] ) );
    return (trp_obj_t *)obj;
}

