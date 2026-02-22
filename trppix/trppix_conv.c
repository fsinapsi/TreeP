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

int Y_R[256];
int Y_G[256];
int Y_B[256];
int Cb_R[256];
int Cb_G[256];
int Cb_B[256];
int Cr_R[256];
int Cr_G[256];
int Cr_B[256];
int RGB_Y[256];
int R_Cr[256];
int G_Cb[256];
int G_Cr[256];
int B_Cb[256];

static int myround( flt64b n );

static int myround( flt64b n )
{
    if ( n >= 0 )
        return (int)( n + 0.5 );
    else
        return (int)( n - 0.5 );
}

void trp_pix_conv_init()
{
    int i;

    /*
     * Q_Z[i] =   (coefficient * i
     *             * (Q-excursion) / (Z-excursion) * fixed-point-factor)
     *
     * to one of each, add the following:
     *             + (fixed-point-factor / 2)         --- for rounding later
     *             + (Q-offset * fixed-point-factor)  --- to add the offset
     *
     */
    for (i = 0; i < 256; i++) {
        Y_R[i] = myround(0.299 * (flt64b)i
                         * 219.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Y_G[i] = myround(0.587 * (flt64b)i
                         * 219.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Y_B[i] = myround((0.114 * (flt64b)i
                          * 219.0 / 255.0 * (flt64b)(1<<FP_BITS))
                         + (flt64b)(1<<(FP_BITS-1))
                         + (16.0 * (flt64b)(1<<FP_BITS)));

        Cb_R[i] = myround(-0.168736 * (flt64b)i
                          * 224.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Cb_G[i] = myround(-0.331264 * (flt64b)i
                          * 224.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Cb_B[i] = myround((0.500 * (flt64b)i
                           * 224.0 / 255.0 * (flt64b)(1<<FP_BITS))
                          + (flt64b)(1<<(FP_BITS-1))
                          + (128.0 * (flt64b)(1<<FP_BITS)));

        Cr_R[i] = myround(0.500 * (flt64b)i
                          * 224.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Cr_G[i] = myround(-0.418688 * (flt64b)i
                          * 224.0 / 255.0 * (flt64b)(1<<FP_BITS));
        Cr_B[i] = myround((-0.081312 * (flt64b)i
                           * 224.0 / 255.0 * (flt64b)(1<<FP_BITS))
                          + (flt64b)(1<<(FP_BITS-1))
                          + (128.0 * (flt64b)(1<<FP_BITS)));
    }
    /*
     * Q_Z[i] =   (coefficient * i
     *             * (Q-excursion) / (Z-excursion) * fixed-point-factor)
     *
     * to one of each, add the following:
     *             + (fixed-point-factor / 2)         --- for rounding later
     *             + (Q-offset * fixed-point-factor)  --- to add the offset
     *
     */

    /* clip Y values under 16 */
    for (i = 0; i < 16; i++) {
        RGB_Y[i] = myround((1.0 * (flt64b)(16)
                            * 255.0 / 219.0 * (flt64b)(1<<FP_BITS))
                           + (flt64b)(1<<(FP_BITS-1)));
    }
    for (i = 16; i < 236; i++) {
        RGB_Y[i] = myround((1.0 * (flt64b)(i - 16)
                            * 255.0 / 219.0 * (flt64b)(1<<FP_BITS))
                           + (flt64b)(1<<(FP_BITS-1)));
    }
    /* clip Y values above 235 */
    for (i = 236; i < 256; i++) {
        RGB_Y[i] = myround((1.0 * (flt64b)(235)
                            * 255.0 / 219.0 * (flt64b)(1<<FP_BITS))
                           + (flt64b)(1<<(FP_BITS-1)));
    }

    /* clip Cb/Cr values below 16 */
    for (i = 0; i < 16; i++) {
        R_Cr[i] = myround(1.402 * (flt64b)(-112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cr[i] = myround(-0.714136 * (flt64b)(-112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cb[i] = myround(-0.344136 * (flt64b)(-112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        B_Cb[i] = myround(1.772 * (flt64b)(-112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
    }
    for (i = 16; i < 241; i++) {
        R_Cr[i] = myround(1.402 * (flt64b)(i - 128)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cr[i] = myround(-0.714136 * (flt64b)(i - 128)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cb[i] = myround(-0.344136 * (flt64b)(i - 128)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        B_Cb[i] = myround(1.772 * (flt64b)(i - 128)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
    }
    /* clip Cb/Cr values above 240 */
    for (i = 241; i < 256; i++) {
        R_Cr[i] = myround(1.402 * (flt64b)(112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cr[i] = myround(-0.714136 * (flt64b)(112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        G_Cb[i] = myround(-0.344136 * (flt64b)(i - 128)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
        B_Cb[i] = myround(1.772 * (flt64b)(112)
                          * 255.0 / 224.0 * (flt64b)(1<<FP_BITS));
    }
}

void trp_pix_ss_444_to_420jpeg( uns8b *buf, uns32b width, uns32b height )
{
    uns8b *in0 = buf, *in1 = buf + width, *out = buf;
    int x, y;

    for ( y = 0 ; y < height ; y += 2, in0 += width, in1 += width )
        for ( x = 0 ; x < width ; x += 2, in0 += 2, in1 += 2 )
            *out++ = ( in0[0] + in0[1] + in1[0] + in1[1] ) >> 2;
}

/*

uns8b trp_pix_ss_420jpeg_to_444( uns8b *buffer, uns32b width, uns32b height, uns8b *saveme )
{
    uns1 *inm, *in0, *inp, *out0, *out1;
    uns1 cmm, cm0, cmp, c0m, c00, c0p, cpm, cp0, cpp;
    int x, y;

    memcpy( saveme, buffer, width );
    in0 = buffer + (width * height / 4) - 2;
    inm = in0 - width/2;
    inp = in0 + width/2;
    out1 = buffer + (width * height) - 1;
    out0 = out1 - width;
    for (y = height; y > 0; y -= 2) {
        if (y == 2) {
            in0 = saveme + width/2 - 1;
            inp = in0 + width/2;
        }
        for (x = width; x > 0; x -= 2) {
            cmm = ((x == 2) || (y == 2)) ? BLANK_CRB : inm[0];
            cm0 = (y == 2) ? BLANK_CRB : inm[1];
            cmp = ((x == width) || (y == 2)) ? BLANK_CRB : inm[2];
            c0m = (x == 2) ? BLANK_CRB : in0[0];
            c00 = in0[1];
            c0p = (x == width) ? BLANK_CRB : in0[2];
            cpm = ((x == 2) || (y == height)) ? BLANK_CRB : inp[0];
            cp0 = (y == height) ? BLANK_CRB : inp[1];
            cpp = ((x == width) || (y == height)) ? BLANK_CRB : inp[2];
            inm--;
            in0--;
            inp--;
            *(out1--) = (1*cpp + 3*(cp0+c0p) + 9*c00 + 8) >> 4;
            *(out1--) = (1*cpm + 3*(cp0+c0m) + 9*c00 + 8) >> 4;
            *(out0--) = (1*cmp + 3*(cm0+c0p) + 9*c00 + 8) >> 4;
            *(out0--) = (1*cmm + 3*(cm0+c0m) + 9*c00 + 8) >> 4;
        }
        out1 -= width;
        out0 -= width;
    }
    return 0;
}

*/

uns8b *trp_pix_trp2yuv( trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns8b *buf;
    uns32b n;

    if ( c == NULL )
        return NULL;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    if ( buf = malloc( 3 * n ) ) {
        uns8b *p, *cb, *cr;
        int r, g, b;

        p = buf;
        cb = p + n;
        cr = cb + n;
        for ( ; n ; n--, c++ ) {
            r = c->red;
            g = c->green;
            b = c->blue;
            *p++ = (Y_R[r] + Y_G[g] + Y_B[b]) >> FP_BITS;
            *cb++ = (Cb_R[r] + Cb_G[g] + Cb_B[b]) >> FP_BITS;
            *cr++ = (Cr_R[r] + Cr_G[g] + Cr_B[b]) >> FP_BITS;
        }
    }
    return buf;
}

uns8b trp_pix_yuv2trp( uns8b *yuv, trp_obj_t *pix )
{
    trp_pix_color_t *c = trp_pix_get_mapc( pix );
    uns8b *cb, *cr;
    int r, g, b;
    uns32b n;

    if ( c == NULL )
        return 1;
    n = ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h;
    cb = yuv + n;
    cr = cb + n;
    for ( ; n ; n--, c++, yuv++, cb++, cr++ ) {
        r = (RGB_Y[*yuv] + R_Cr[*cr]) >> FP_BITS;
        g = (RGB_Y[*yuv] + G_Cb[*cb] + G_Cr[*cr]) >> FP_BITS;
        b = (RGB_Y[*yuv] + B_Cb[*cb]) >> FP_BITS;
        c->red = (r < 0) ? 0 : (r > 255) ? 255 : r;
        c->green = (g < 0) ? 0 : (g > 255) ? 255 : g;
        c->blue = (b < 0) ? 0 : (b > 255) ? 255 : b;
    }
    return 0;
}

