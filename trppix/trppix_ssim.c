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

typedef struct {
    uns32b w, h, win_w, win_h, x, y, win_cnt;
    trp_pix_color_t *map1, *map2;
    double *weights;
    double c1, c2;
    double ssim_r, ssim_g, ssim_b;
    voidfun_t calculate;
    uns32b pmur1[ 8 ], pmur2[ 8 ], pmug1[ 8 ], pmug2[ 8 ], pmub1[ 8 ], pmub2[ 8 ];
    uns32b pzir1[ 8 ], pzir2[ 8 ], pzig1[ 8 ], pzig2[ 8 ], pzib1[ 8 ], pzib2[ 8 ];
    uns32b pzir12[ 8 ], pzig12[ 8 ], pzib12[ 8 ];
} trp_pix_ssim_t;

static double _ssim_linear_8x8[] = {
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625,
    0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625, 0.015625
};

static double _ssim_gaussian_11x11[] = {
    0.000001, 0.000007, 0.000037, 0.000112, 0.000219, 0.000273, 0.000219, 0.000112, 0.000037, 0.000007, 0.000001,
    0.000007, 0.000057, 0.000273, 0.000831, 0.001618, 0.002021, 0.001618, 0.000831, 0.000273, 0.000057, 0.000007,
    0.000037, 0.000273, 0.001296, 0.003937, 0.007668, 0.009576, 0.007668, 0.003937, 0.001296, 0.000273, 0.000037,
    0.000112, 0.000831, 0.003937, 0.011959, 0.023294, 0.029091, 0.023294, 0.011959, 0.003937, 0.000831, 0.000112,
    0.000219, 0.001618, 0.007668, 0.023294, 0.045371, 0.056661, 0.045371, 0.023294, 0.007668, 0.001618, 0.000219,
    0.000273, 0.002021, 0.009576, 0.029091, 0.056661, 0.070808, 0.056661, 0.029091, 0.009576, 0.002021, 0.000273,
    0.000219, 0.001618, 0.007668, 0.023294, 0.045371, 0.056661, 0.045371, 0.023294, 0.007668, 0.001618, 0.000219,
    0.000112, 0.000831, 0.003937, 0.011959, 0.023294, 0.029091, 0.023294, 0.011959, 0.003937, 0.000831, 0.000112,
    0.000037, 0.000273, 0.001296, 0.003937, 0.007668, 0.009576, 0.007668, 0.003937, 0.001296, 0.000273, 0.000037,
    0.000007, 0.000057, 0.000273, 0.000831, 0.001618, 0.002021, 0.001618, 0.000831, 0.000273, 0.000057, 0.000007,
    0.000001, 0.000007, 0.000037, 0.000112, 0.000219, 0.000273, 0.000219, 0.000112, 0.000037, 0.000007, 0.000001
};

static void trp_pix_ssim_normalize_weights( trp_pix_ssim_t *s );
static int trp_pix_ssim_next( trp_pix_ssim_t *s );
static void trp_pix_ssim_calculate( trp_pix_ssim_t *s );
static void trp_pix_ssim_calculate_linear_8x8( trp_pix_ssim_t *s );
static void trp_pix_ssim_calculate_gaussian_11x11( trp_pix_ssim_t *s );
static trp_obj_t *trp_pix_ssim_index_basic( trp_obj_t *pix1, trp_obj_t *pix2, trp_pix_ssim_t *s );

static void trp_pix_ssim_normalize_weights( trp_pix_ssim_t *s )
{
    uns32b i;
    double tot = 0, *p;

    for ( i = s->win_w * s->win_h, p = s->weights ; i ; i-- )
        tot += *p++;
    for ( i = s->win_w * s->win_h, p = s->weights ; i ; i-- )
        *p++ /= tot;
}

/*
 sposta alla prossima finestra
 */

static int trp_pix_ssim_next( trp_pix_ssim_t *s )
{
    if ( s->x + s->win_w < s->w ) {
        s->x++;
        s->win_cnt++;
        (s->map1)++;
        (s->map2)++;
        return 1;
    }
    if ( s->y + s->win_h < s->h ) {
        s->y++;
        s->x = 0;
        s->win_cnt++;
        s->map1 += s->win_w;
        s->map2 += s->win_w;
        return 1;
    }
    return 0;
}

/*
 calcola l'indice della finestra corrente
 */

static void trp_pix_ssim_calculate( trp_pix_ssim_t *s )
{
    uns32b i, j;
    trp_pix_color_t *map1, *map2, *p1, *p2;
    double *weights, t1, t2, t12, w;
    double dmur1 = 0.0, dmur2 = 0.0, dmug1 = 0.0, dmug2 = 0.0, dmub1 = 0.0, dmub2 = 0.0;
    double dzir1 = 0.0, dzir2 = 0.0, dzig1 = 0.0, dzig2 = 0.0, dzib1 = 0.0, dzib2 = 0.0;
    double dzir12 = 0.0, dzig12 = 0.0, dzib12 = 0.0;

    weights = s->weights;
    map1 = s->map1;
    map2 = s->map2;

    for ( i = 0 ; i < s->win_h ; i++, map1 += s->w, map2 += s->w )
        for ( j = 0, p1 = map1, p2 = map2 ; j < s->win_w ; j++, p1++, p2++ ) {
            w = *weights++;
            t1 = (double)( p1->red );
            t2 = (double)( p2->red );
            dmur1 += w * t1;
            dmur2 += w * t2;
            dzir1 += w * t1 * t1;
            dzir2 += w * t2 * t2;
            dzir12 += w * t1 * t2;
            t1 = (double)( p1->green );
            t2 = (double)( p2->green );
            dmug1 += w * t1;
            dmug2 += w * t2;
            dzig1 += w * t1 * t1;
            dzig2 += w * t2 * t2;
            dzig12 += w * t1 * t2;
            t1 = (double)( p1->blue );
            t2 = (double)( p2->blue );
            dmub1 += w * t1;
            dmub2 += w * t2;
            dzib1 += w * t1 * t1;
            dzib2 += w * t2 * t2;
            dzib12 += w * t1 * t2;
        }

    t1 = dmur1 * dmur1;
    t2 = dmur2 * dmur2;
    t12 = dmur1 * dmur2;
    dzir1 -= t1;
    dzir2 -= t2;
    dzir12 -= t12;

    s->ssim_r +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dzir12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dzir1 + dzir2 + s->c2 ) );

    t1 = dmug1 * dmug1;
    t2 = dmug2 * dmug2;
    t12 = dmug1 * dmug2;
    dzig1 -= t1;
    dzig2 -= t2;
    dzig12 -= t12;

    s->ssim_g +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dzig12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dzig1 + dzig2 + s->c2 ) );

    t1 = dmub1 * dmub1;
    t2 = dmub2 * dmub2;
    t12 = dmub1 * dmub2;
    dzib1 -= t1;
    dzib2 -= t2;
    dzib12 -= t12;

    s->ssim_b +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dzib12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dzib1 + dzib2 + s->c2 ) );
}

/*
 versione ottimizzata per matrice lineare 8x8
 */

static void trp_pix_ssim_calculate_linear_8x8( trp_pix_ssim_t *s )
{
    uns32b i, j, x1, x2;
    uns32b mur1 = 0, mur2 = 0, mug1 = 0, mug2 = 0, mub1 = 0, mub2 = 0;
    uns32b zir1 = 0, zir2 = 0, zig1 = 0, zig2 = 0, zib1 = 0, zib2 = 0;
    uns32b zir12 = 0, zig12 = 0, zib12 = 0;
    trp_pix_color_t *row1, *row2, *p1, *p2;
    double t1, t2, t12;
    double dmu1, dmu2, dsi1, dsi2, dsi12;

    if ( s->x == 0 ) {
        for ( i = 0, row1 = s->map1, row2 = s->map2 ;
              i < 8 ;
              i++, row1++, row2++ ) {
            s->pmur1[ i ] = 0;
            s->pmur2[ i ] = 0;
            s->pmug1[ i ] = 0;
            s->pmug2[ i ] = 0;
            s->pmub1[ i ] = 0;
            s->pmub2[ i ] = 0;
            s->pzir1[ i ] = 0;
            s->pzir2[ i ] = 0;
            s->pzig1[ i ] = 0;
            s->pzig2[ i ] = 0;
            s->pzib1[ i ] = 0;
            s->pzib2[ i ] = 0;
            s->pzir12[ i ] = 0;
            s->pzig12[ i ] = 0;
            s->pzib12[ i ] = 0;
            for ( j = 0, p1 = row1, p2 = row2 ;
                  j < 8 ;
                  j++, p1 += s->w, p2 += s->w ) {
                x1 = p1->red;
                x2 = p2->red;
                s->pmur1[ i ] += x1;
                s->pmur2[ i ] += x2;
                s->pzir1[ i ] += x1 * x1;
                s->pzir2[ i ] += x2 * x2;
                s->pzir12[ i ] += x1 * x2;
                x1 = p1->green;
                x2 = p2->green;
                s->pmug1[ i ] += x1;
                s->pmug2[ i ] += x2;
                s->pzig1[ i ] += x1 * x1;
                s->pzig2[ i ] += x2 * x2;
                s->pzig12[ i ] += x1 * x2;
                x1 = p1->blue;
                x2 = p2->blue;
                s->pmub1[ i ] += x1;
                s->pmub2[ i ] += x2;
                s->pzib1[ i ] += x1 * x1;
                s->pzib2[ i ] += x2 * x2;
                s->pzib12[ i ] += x1 * x2;
            }
            mur1 += s->pmur1[ i ];
            mur2 += s->pmur2[ i ];
            mug1 += s->pmug1[ i ];
            mug2 += s->pmug2[ i ];
            mub1 += s->pmub1[ i ];
            mub2 += s->pmub2[ i ];
            zir1 += s->pzir1[ i ];
            zir2 += s->pzir2[ i ];
            zig1 += s->pzig1[ i ];
            zig2 += s->pzig2[ i ];
            zib1 += s->pzib1[ i ];
            zib2 += s->pzib2[ i ];
            zir12 += s->pzir12[ i ];
            zig12 += s->pzig12[ i ];
            zib12 += s->pzib12[ i ];
        }
    } else {
        /*
         s->x > 0 : si ottimizza!
         mi resta solo da calcolare i dati della colonna ( s->x - 1 ) % 8
         */
        i = ( s->x - 1 ) % 8;
        s->pmur1[ i ] = 0;
        s->pmur2[ i ] = 0;
        s->pmug1[ i ] = 0;
        s->pmug2[ i ] = 0;
        s->pmub1[ i ] = 0;
        s->pmub2[ i ] = 0;
        s->pzir1[ i ] = 0;
        s->pzir2[ i ] = 0;
        s->pzig1[ i ] = 0;
        s->pzig2[ i ] = 0;
        s->pzib1[ i ] = 0;
        s->pzib2[ i ] = 0;
        s->pzir12[ i ] = 0;
        s->pzig12[ i ] = 0;
        s->pzib12[ i ] = 0;
        for ( j = 0, p1 = s->map1 + 7, p2 = s->map2 + 7 ;
              j < 8 ;
              j++, p1 += s->w, p2 += s->w ) {
            x1 = p1->red;
            x2 = p2->red;
            s->pmur1[ i ] += x1;
            s->pmur2[ i ] += x2;
            s->pzir1[ i ] += x1 * x1;
            s->pzir2[ i ] += x2 * x2;
            s->pzir12[ i ] += x1 * x2;
            x1 = p1->green;
            x2 = p2->green;
            s->pmug1[ i ] += x1;
            s->pmug2[ i ] += x2;
            s->pzig1[ i ] += x1 * x1;
            s->pzig2[ i ] += x2 * x2;
            s->pzig12[ i ] += x1 * x2;
            x1 = p1->blue;
            x2 = p2->blue;
            s->pmub1[ i ] += x1;
            s->pmub2[ i ] += x2;
            s->pzib1[ i ] += x1 * x1;
            s->pzib2[ i ] += x2 * x2;
            s->pzib12[ i ] += x1 * x2;
        }
        for ( i = 0 ; i < 8 ; i++ ) {
            mur1 += s->pmur1[ i ];
            mur2 += s->pmur2[ i ];
            mug1 += s->pmug1[ i ];
            mug2 += s->pmug2[ i ];
            mub1 += s->pmub1[ i ];
            mub2 += s->pmub2[ i ];
            zir1 += s->pzir1[ i ];
            zir2 += s->pzir2[ i ];
            zig1 += s->pzig1[ i ];
            zig2 += s->pzig2[ i ];
            zib1 += s->pzib1[ i ];
            zib2 += s->pzib2[ i ];
            zir12 += s->pzir12[ i ];
            zig12 += s->pzig12[ i ];
            zib12 += s->pzib12[ i ];
        }
    }

    dmu1 = (double)mur1 / 64.0;
    dmu2 = (double)mur2 / 64.0;

    t1 = dmu1 * dmu1;
    t2 = dmu2 * dmu2;
    t12 = dmu1 * dmu2;
    dsi1 = (double)zir1 / 64.0 - t1;
    dsi2 = (double)zir2 / 64.0 - t2;
    dsi12 = (double)zir12 / 64.0 - t12;

    s->ssim_r +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dsi12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dsi1 + dsi2 + s->c2 ) );

    dmu1 = (double)mug1 / 64.0;
    dmu2 = (double)mug2 / 64.0;

    t1 = dmu1 * dmu1;
    t2 = dmu2 * dmu2;
    t12 = dmu1 * dmu2;
    dsi1 = (double)zig1 / 64.0 - t1;
    dsi2 = (double)zig2 / 64.0 - t2;
    dsi12 = (double)zig12 / 64.0 - t12;

    s->ssim_g +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dsi12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dsi1 + dsi2 + s->c2 ) );

    dmu1 = (double)mub1 / 64.0;
    dmu2 = (double)mub2 / 64.0;

    t1 = dmu1 * dmu1;
    t2 = dmu2 * dmu2;
    t12 = dmu1 * dmu2;
    dsi1 = (double)zib1 / 64.0 - t1;
    dsi2 = (double)zib2 / 64.0 - t2;
    dsi12 = (double)zib12 / 64.0 - t12;

    s->ssim_b +=
        ( ( 2.0 * t12 + s->c1 ) * ( 2.0 * dsi12 + s->c2 ) ) /
        ( ( t1 + t2 + s->c1 ) * ( dsi1 + dsi2 + s->c2 ) );
}

/*
 versione ottimizzata per matrice gaussiana 11x11
 */

static void trp_pix_ssim_calculate_gaussian_11x11( trp_pix_ssim_t *s )
{
    /*
     FIXME
     da fare
     */
    trp_pix_ssim_calculate( s );
}

static trp_obj_t *trp_pix_ssim_index_basic( trp_obj_t *pix1, trp_obj_t *pix2, trp_pix_ssim_t *s )
{
    if ( ( pix1->tipo != TRP_PIX ) || ( pix2->tipo != TRP_PIX ) )
        return UNDEF;
    if ( ( ( s->map1 = ((trp_pix_t *)pix1)->map.c ) == NULL ) ||
         ( ( s->map2 = ((trp_pix_t *)pix2)->map.c ) == NULL ) )
        return UNDEF;

    s->w = ((trp_pix_t *)pix1)->w;
    s->h = ((trp_pix_t *)pix1)->h;

    if ( ( s->w != ((trp_pix_t *)pix2)->w ) ||
         ( s->h != ((trp_pix_t *)pix2)->h ) ||
         ( s->w < s->win_w ) || ( s->h < s->win_h ) )
        return UNDEF;

    s->x = 0;
    s->y = 0;
    s->win_cnt = 1;

    s->c1 = 0.01 * 255.0;
    s->c1 *= s->c1;
    s->c2 = 0.03 * 255.0;
    s->c2 *= s->c2;

    s->ssim_r = 0.0;
    s->ssim_g = 0.0;
    s->ssim_b = 0.0;

    do ( s->calculate )( s );
    while ( trp_pix_ssim_next( s ) );

    s->ssim_r /= (double)( s->win_cnt );
    s->ssim_g /= (double)( s->win_cnt );
    s->ssim_b /= (double)( s->win_cnt );

    return trp_double( 0.299 * s->ssim_r +
                       0.587 * s->ssim_g +
                       0.114 * s->ssim_b );
}

trp_obj_t *trp_pix_ssim( trp_obj_t *pix1, trp_obj_t *pix2, trp_obj_t *weights )
{
    trp_obj_t *res;
    double *p, w;
    uns32b i, j;
    trp_pix_ssim_t s;
    uns8b allzero = 1;

    if ( weights == NULL )
        return trp_pix_ssim_linear( pix1, pix2 );
    if ( weights->tipo != TRP_ARRAY )
        return UNDEF;
    if ( ( s.win_h = ((trp_array_t *)weights)->len ) == 0 )
        return UNDEF;
    if ( ((trp_array_t *)weights)->data[ 0 ]->tipo != TRP_ARRAY )
        return UNDEF;
    if ( ( s.win_w = ((trp_array_t *)(((trp_array_t *)weights)->data[ 0 ]))->len ) == 0 )
        return UNDEF;
    p = s.weights = trp_gc_malloc_atomic( s.win_w * s.win_h * sizeof( double ) );
    for ( i = 0 ; i < s.win_h ; i++ ) {
        res = ((trp_array_t *)weights)->data[ i ];
        if ( res->tipo != TRP_ARRAY ) {
            trp_gc_free( s.weights );
            return UNDEF;
        }
        if ( ((trp_array_t *)res)->len != s.win_w ) {
            trp_gc_free( s.weights );
            return UNDEF;
        }
        for ( j = 0 ; j < s.win_w ; j++ ) {
            if ( trp_cast_double( ((trp_array_t *)res)->data[ j ], &w ) ) {
                trp_gc_free( s.weights );
                return UNDEF;
            }
            if ( w < 0 ) {
                trp_gc_free( s.weights );
                return UNDEF;
            }
            if ( w > 0 )
                allzero = 0;
            *p++ = w;
        }
    }
    if ( allzero ) {
        trp_gc_free( s.weights );
        return UNDEF;
    }
    s.calculate = trp_pix_ssim_calculate;
    trp_pix_ssim_normalize_weights( &s );
    res = trp_pix_ssim_index_basic( pix1, pix2, &s );
    trp_gc_free( s.weights );
    return res;
}

trp_obj_t *trp_pix_ssim_linear( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_ssim_t s;

    s.win_w = s.win_h = 8;
    s.weights = _ssim_linear_8x8;
    s.calculate = trp_pix_ssim_calculate_linear_8x8;
    return trp_pix_ssim_index_basic( pix1, pix2, &s );
}

trp_obj_t *trp_pix_ssim_gaussian( trp_obj_t *pix1, trp_obj_t *pix2 )
{
    trp_pix_ssim_t s;

    s.win_w = s.win_h = 11;
    s.weights = _ssim_gaussian_11x11;
    s.calculate = trp_pix_ssim_calculate_gaussian_11x11;
    return trp_pix_ssim_index_basic( pix1, pix2, &s );
}

