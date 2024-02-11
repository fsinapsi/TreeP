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

static trp_obj_t *trp_pix_top_bottom_field( uns8b bottom, trp_obj_t *pix );
static uns8b trp_pix_top_bottom_field_test( uns8b bottom, trp_obj_t *pix );
static void trp_pix_colormod_set_table( uns8b tipo, uns8b *t, double v );
static void trp_pix_colormod_basic( uns8b tipo, trp_pix_color_t *map, uns32b w, uns32b h, double vr, double vg, double vb );

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

static uns8b trp_pix_top_bottom_field_test( uns8b bottom, trp_obj_t *pix )
{
    trp_pix_color_t *p, *q;
    uns32b w, h, w2, w4;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( p = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    h >>= 1; /* accettiamo anche altezze dispari */
    q = p + w;
    w2 = w << 1;
    w4 = w << 2;
    if ( bottom ) {
        trp_pix_color_t *r = p;
        p = q;
        q = r;
    }
    for ( ; h ; h--, p += w2, q += w2 )
        memcpy( q, p, w4 );
    return 0;
}

uns8b trp_pix_top_field_test( trp_obj_t *pix )
{
    return trp_pix_top_bottom_field_test( 0, pix );
}

uns8b trp_pix_bottom_field_test( trp_obj_t *pix )
{
    return trp_pix_top_bottom_field_test( 1, pix );
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

/*
 questa implementazione di trp_pix_mse è corretta per immagini
 fino a una risoluzione massima di 11909 x 11909
 */

trp_obj_t *trp_pix_mse( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_color_t *map1 = trp_pix_get_mapc( pix1 ), *map2 = trp_pix_get_mapc( pix2 );
    sig64b tot, j;
    uns32b w, h, n, i;

    if ( ( map1 == NULL ) || ( map2 == NULL ) )
        return UNDEF;
    w = ((trp_pix_t *)pix1)->w;
    h = ((trp_pix_t *)pix1)->h;
    if ( ( w != ((trp_pix_t *)pix2)->w ) ||
         ( h != ((trp_pix_t *)pix2)->h ) )
        return UNDEF;
    n = w * h;
    for ( i = n, tot = 0 ; i ; ) {
        i--;
        j = ( ( (sig32b)( map1[ i ].red ) ) - ( (sig32b)( map2[ i ].red ) ) ) * TRP_PIX_WEIGHT_RED +
            ( ( (sig32b)( map1[ i ].green ) ) - ( (sig32b)( map2[ i ].green ) ) ) * TRP_PIX_WEIGHT_GREEN +
            ( ( (sig32b)( map1[ i ].blue ) ) - ( (sig32b)( map2[ i ].blue ) ) ) * TRP_PIX_WEIGHT_BLUE;
        tot += j * j;
    }
    return trp_math_ratio( trp_sig64( tot ), trp_sig64( 65025000000LL * (sig64b)n ), NULL );
}

