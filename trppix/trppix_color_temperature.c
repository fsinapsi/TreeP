/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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

#define FP_BITS 18

static uns8b trp_pix_color_temperature_adm_low( trp_pix_t *pix, trp_obj_t *temperature, trp_obj_t *angle );
static void trp_pix_color_temperature_temp2rgb( flt64b temp, flt64b *r, flt64b *g, flt64b *b );
static uns8b trp_pix_color_temperature_photodemon_low( trp_pix_t *pix, trp_obj_t *temperature, trp_obj_t *strength );

/*
    Algorithm: Copyright (C) 1999 Winston Chang
*/

static uns8b trp_pix_color_temperature_adm_low( trp_pix_t *pix, trp_obj_t *temperature, trp_obj_t *angle )
{
    extern int Y_R[];
    extern int Y_G[];
    extern int Y_B[];
    extern int Cb_R[];
    extern int Cb_G[];
    extern int Cb_B[];
    extern int Cr_R[];
    extern int Cr_G[];
    extern int Cr_B[];
    extern int RGB_Y[];
    extern int R_Cr[];
    extern int G_Cb[];
    extern int G_Cr[];
    extern int B_Cb[];
    trp_pix_color_t *c;
    flt64b temp, ang, ushift, vshift;
    int r, g, b, iy, ib, ir;
    uns32b n;

    if ( trp_cast_flt64b_range( temperature, &temp, -1.0, 1.0 ) )
        return 1;
    if ( angle ) {
        if ( trp_cast_flt64b_range( angle, &ang, 0.0, 180.0 ) )
            return 1;
    } else
        ang = 30.0;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    ang = ang * TRP_PI / 180.0;
    ushift = +100.0 * cos( ang ) * temp;
    vshift = -100.0 * sin( ang ) * temp;
    for ( c = trp_pix_get_mapc( pix ) ; n ; n--, c++ ) {
        r = c->red;
        g = c->green;
        b = c->blue;
        iy = (Y_R[r] + Y_G[g] + Y_B[b]) >> FP_BITS;
        ib = ( (Cb_R[r] + Cb_G[g] + Cb_B[b]) >> FP_BITS ) + (int)( ushift * ((flt64b)iy)/255.0 );
        ir = ( (Cr_R[r] + Cr_G[g] + Cr_B[b]) >> FP_BITS ) + (int)( vshift * ((flt64b)iy)/255.0 );
        ib = trp_pix_iclamp255( ib );
        ir = trp_pix_iclamp255( ir );
        r = (RGB_Y[iy] + R_Cr[ir]) >> FP_BITS;
        g = (RGB_Y[iy] + G_Cb[ib] + G_Cr[ir]) >> FP_BITS;
        b = (RGB_Y[iy] + B_Cb[ib]) >> FP_BITS;
        c->red = trp_pix_iclamp255( r );
        c->green = trp_pix_iclamp255( g );
        c->blue = trp_pix_iclamp255( b );
    }
    return 0;
}

/*
    Algorithm: Copyright (C) 2012 Tanner Helland
*/

static void trp_pix_color_temperature_temp2rgb( flt64b temp, flt64b *r, flt64b *g, flt64b *b )
{
    temp /= 100.0;
    *r = ( temp <= 66.0 ) ? 255.0 : 329.698727446 * pow( temp - 60.0, -0.1332047592 );
    *g = ( temp <= 66.0 ) ? 99.4708025861 * log( temp ) - 161.1195681661 : 288.1221695283 * pow( temp - 60.0, -0.0755148492 );
    *b = ( temp >= 66.0 ) ? 255.0 : ( ( temp <= 19.0 ) ? 0 : 138.5177312231 * log( temp - 10.0 ) - 305.0447927307 );
    trp_pix_fclamp_rgb( r, g, b );
}

static uns8b trp_pix_color_temperature_photodemon_low( trp_pix_t *pix, trp_obj_t *temperature, trp_obj_t *strength )
{
    trp_pix_color_t *c;
    flt64b temp, stre, d, tr, tg, tb, wr, wg, wb, lk, r, g, b, l;
    uns32b w, h, n;

    if ( trp_cast_flt64b_range( temperature, &temp, 1000.0, 40000.0 ) )
        return 1;
    if ( strength ) {
        if ( trp_cast_flt64b_range( strength, &stre, 0.0, 1.0 ) )
            return 1;
    } else
        stre = 0.58;
    trp_pix_color_temperature_temp2rgb( temp, &tr, &tg, &tb );
    tr *= stre;
    tg *= stre;
    tb *= stre;
    d = 1.0 + stre;
    wr = ( stre / d ) * TRP_PIX_WEIGHT_RED_F;
    wg = ( stre / d ) * TRP_PIX_WEIGHT_GREEN_F;
    wb = ( stre / d ) * TRP_PIX_WEIGHT_BLUE_F;
    lk = ( tr * TRP_PIX_WEIGHT_RED_F + tg * TRP_PIX_WEIGHT_GREEN_F + tb * TRP_PIX_WEIGHT_BLUE_F ) / d;
    w = pix->w;
    h = pix->h;
    for ( n = w * h, c = pix->map.c ; n ; n--, c++ ) {
        r = (flt64b)(c->red);
        g = (flt64b)(c->green);
        b = (flt64b)(c->blue);
        /*
         * l mantiene la luminosità originale
         */
        l = r * wr + g * wg + b * wb - lk;
        r = l + ( r + tr ) / d;
        g = l + ( g + tg ) / d;
        b = l + ( b + tb ) / d;
        trp_pix_fclamp_rgb( &r, &g, &b );
        c->red = (uns8b)( r + 0.5 );
        c->green = (uns8b)( g + 0.5 );
        c->blue = (uns8b)( b + 0.5 );
    }
    return 0;
}

trp_obj_t *trp_pix_color_temperature_adm( trp_obj_t *pix, trp_obj_t *temperature, trp_obj_t *angle )
{
    trp_obj_t *res;

    if ( trp_pix_is_not_valid( pix ) )
        return UNDEF;
    res = trp_pix_clone( pix );
    if ( res != UNDEF )
        if ( trp_pix_color_temperature_adm_low( (trp_pix_t *)res, temperature, angle ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            res = UNDEF;
        }
    return res;
}

uns8b trp_pix_color_temperature_adm_test( trp_obj_t *pix, trp_obj_t *temperature, trp_obj_t *angle )
{
    if ( trp_pix_is_not_valid( pix ) )
        return 1;
    return trp_pix_color_temperature_adm_low( (trp_pix_t *)pix, temperature, angle );
}

trp_obj_t *trp_pix_color_temperature_photodemon( trp_obj_t *pix, trp_obj_t *temperature, trp_obj_t *strength )
{
    trp_obj_t *res;

    if ( trp_pix_is_not_valid( pix ) )
        return UNDEF;
    res = trp_pix_clone( pix );
    if ( res != UNDEF )
        if ( trp_pix_color_temperature_photodemon_low( (trp_pix_t *)res, temperature, strength ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            res = UNDEF;
        }
    return res;
}

uns8b trp_pix_color_temperature_photodemon_test( trp_obj_t *pix, trp_obj_t *temperature, trp_obj_t *strength )
{
    if ( trp_pix_is_not_valid( pix ) )
        return 1;
    return trp_pix_color_temperature_photodemon_low( (trp_pix_t *)pix, temperature, strength );
}

