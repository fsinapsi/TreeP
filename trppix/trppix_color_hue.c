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

#define FP_BITS 18

static uns8b trp_pix_color_hue_low( trp_pix_t *pix, trp_obj_t *hue, trp_obj_t *saturation );

/*
    (c) Michael Niedermayer (mplayer)
*/

typedef struct {
    uns8b lutU[ 256 ][ 256 ];
    uns8b lutV[ 256 ][ 256 ];
} huesettings;

static void buildLut( huesettings *hs, int s, int c )
{
    int i, j , u, v, uout, vout;

    for ( i = 0 ; i < 256 ; i++ ) {
        for ( j = 0 ; j < 256 ; j++ ) {
            u = i - 128;
            v = j - 128;
            uout = (c*u - s*v + (1 << 15) + (128 << 16)) >> 16;
            vout = (s*u + c*v + (1 << 15) + (128 << 16)) >> 16;
            if(uout & (~0xFF)) uout = (~uout) >> 31;
            if(vout & (~0xFF)) vout = (~vout) >> 31;
            hs->lutU[i][j] = uout;
            hs->lutV[i][j] = vout;
        }
    }
}

static uns8b trp_pix_color_hue_low( trp_pix_t *pix, trp_obj_t *hue, trp_obj_t *saturation )
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
    huesettings *hs;
    trp_pix_color_t *c;
    flt64b fhue, fsat, ushift, vshift;
    int s, k, r, g, b, iy, ib, ir;
    uns32b n;

    if ( trp_cast_flt64b_range( hue, &fhue, -90.0, 90.0 ) ||
         trp_cast_flt64b_range( saturation, &fsat, -10.0, 10.0 ) )
        return 1;
    if ( ( hs = malloc( sizeof( huesettings ) ) ) == NULL )
        return 1;
    fhue = fhue * TRP_PI / 180.0;
    fsat = ( 10.0 + fsat ) / 10.0;
    s = (int)rint(sin(fhue) * (1<<16) * fsat);
    k = (int)rint(cos(fhue) * (1<<16) * fsat);
    buildLut( hs, s, k );
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    for ( c = trp_pix_get_mapc( pix ) ; n ; n--, c++ ) {
        r = c->red;
        g = c->green;
        b = c->blue;
        iy = (Y_R[r] + Y_G[g] + Y_B[b]) >> FP_BITS;
        ib = ( (Cb_R[r] + Cb_G[g] + Cb_B[b]) >> FP_BITS );
        ir = ( (Cr_R[r] + Cr_G[g] + Cr_B[b]) >> FP_BITS );
        ib = hs->lutU[ ib ][ ir ];
        ir = hs->lutV[ ib ][ ir ];
        r = (RGB_Y[iy] + R_Cr[ir]) >> FP_BITS;
        g = (RGB_Y[iy] + G_Cb[ib] + G_Cr[ir]) >> FP_BITS;
        b = (RGB_Y[iy] + B_Cb[ib]) >> FP_BITS;
        c->red = trp_pix_iclamp255( r );
        c->green = trp_pix_iclamp255( g );
        c->blue = trp_pix_iclamp255( b );
    }
    free( hs );
    return 0;
}

trp_obj_t *trp_pix_color_hue( trp_obj_t *pix, trp_obj_t *hue, trp_obj_t *saturation )
{
    trp_obj_t *res;

    if ( trp_pix_is_not_valid( pix ) )
        return UNDEF;
    res = trp_pix_clone( pix );
    if ( res != UNDEF )
        if ( trp_pix_color_hue_low( (trp_pix_t *)res, hue, saturation ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            res = UNDEF;
        }
    return res;
}

uns8b trp_pix_color_hue_test( trp_obj_t *pix, trp_obj_t *hue, trp_obj_t *saturation )
{
    if ( trp_pix_is_not_valid( pix ) )
        return 1;
    return trp_pix_color_hue_low( (trp_pix_t *)pix, hue, saturation );
}

