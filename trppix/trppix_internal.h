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

#ifndef __trppix_internal__h
#define __trppix_internal__h

#include "../trp/trp.h"
#include "./trppix.h"
#include "./Epeg.h"

typedef union {
    struct {
        uns8b red;
        uns8b green;
        uns8b blue;
        uns8b alpha;
    };
    uns32b rgba;
} trp_pix_color_t;

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    uns32b w;
    uns32b h;
    union {
        uns8b *p;
        uns32b *t;
        trp_pix_color_t *c;
    } map;
    struct {
        uns16b red;
        uns16b green;
        uns16b blue;
        uns16b alpha;
    } color;
} trp_pix_t;

/* la somma deve essere 1000 */
#define TRP_PIX_WEIGHT_RED 299
#define TRP_PIX_WEIGHT_GREEN 587
#define TRP_PIX_WEIGHT_BLUE 114

#define TRP_PIX_WEIGHT_RED_F (((float)TRP_PIX_WEIGHT_RED)/1000.0)
#define TRP_PIX_WEIGHT_GREEN_F (((float)TRP_PIX_WEIGHT_GREEN)/1000.0)
#define TRP_PIX_WEIGHT_BLUE_F (((float)TRP_PIX_WEIGHT_BLUE)/1000.0)

#define TRP_PIX_RGB_TO_GRAY(r,g,b) ((uns8b)((((uns32b)(r))*TRP_PIX_WEIGHT_RED+ \
                                             ((uns32b)(g))*TRP_PIX_WEIGHT_GREEN+ \
                                             ((uns32b)(b))*TRP_PIX_WEIGHT_BLUE+500)/1000))
#define TRP_PIX_RGB_TO_GRAY_C(c) TRP_PIX_RGB_TO_GRAY((c)->red,(c)->green,(c)->blue)
#define TRP_PIX_RGB_TO_GRAY_01(r,g,b) (((double)TRP_PIX_RGB_TO_GRAY(r,g,b))/255.0)
#define TRP_PIX_RGB_TO_GRAY_01_C(c) TRP_PIX_RGB_TO_GRAY_01((c)->red,(c)->green,(c)->blue)

uns8b trp_pix_close( trp_pix_t *obj );
trp_obj_t *trp_pix_create_image_from_data( int must_copy, uns32b w, uns32b h, uns8b *data );
trp_obj_t *trp_pix_create_basic( uns32b w, uns32b h );
trp_obj_t *trp_pix_create_color( uns16b red, uns16b green, uns16b blue, uns16b alpha );
uns8b trp_pix_decode_color( trp_obj_t *obj, uns16b *red, uns16b *green, uns16b *blue, uns16b *alpha );
uns8b trp_pix_decode_color_uns8b( trp_obj_t *obj, trp_obj_t *pix, uns8b *red, uns8b *green, uns8b *blue, uns8b *alpha );
uns16b trp_pix_colors_type( trp_pix_t *pix, uns16b max_colors );
uns8b trp_pix_has_alpha_low( trp_pix_t *pix );
trp_obj_t *trp_pix_load_low( uns8b *cpath );
trp_obj_t *trp_pix_load_memory_low( uns8b *idata, uns32b isize );
void trp_pix_load_set_loader_svg( trp_obj_t *pix );
uns8b trp_pix_load_png( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_png_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_jpg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_jpg_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_pnm( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_pnm_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_gif( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_gif_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
trp_obj_t *trp_pix_load_gif_multiple( uns8b *cpath );
uns8b trp_pix_load_tga( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_tga_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_xpm( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_xpm_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_ptg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_ptg_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_scale_low( uns32b wi, uns32b hi, uns8b *idata, uns32b wo, uns32b ho, uns8b **odata );
uns8b trp_pix_rotate_low( trp_obj_t *pix, flt64b a, uns32b *wo, uns32b *ho, uns8b **data );
trp_obj_t *trp_pix_crop_low( trp_obj_t *pix, double xx, double yy, double ww, double hh );
void trp_pix_ss_444_to_420jpeg( uns8b *buf, uns32b width, uns32b height );
uns8b *trp_pix_trp2yuv( trp_obj_t *pix );
uns8b trp_pix_yuv2trp( uns8b *yuv, trp_obj_t *pix );
uns8b *trp_pix_get_map_low( trp_pix_t *obj );
#define trp_pix_get_mapp(o) trp_pix_get_map_low((trp_pix_t *)(o))
#define trp_pix_get_mapt(o) ((uns32b *)(trp_pix_get_map_low((trp_pix_t *)(o))))
#define trp_pix_get_mapc(o) ((trp_pix_color_t *)(trp_pix_get_map_low((trp_pix_t *)(o))))
#define trp_pix_is_not_valid(o) ((uns8b)(trp_pix_get_mapp(o)?0:1))
void trp_pix_fclamp( flt64b *val );
void trp_pix_fclamp_rgb( flt64b *r, flt64b *g, flt64b *b );
#define trp_pix_iclamp255(v) (((v)<0)?0:(((v)>255)?255:(v)))
trp_obj_t *trp_pix_clone( trp_obj_t *pix );
flt64b pix_color_diff( uns8b r1, uns8b g1, uns8b b1, uns8b r2, uns8b g2, uns8b b2 );
flt64b pix_color_diff_color( uns8b r, uns8b g, uns8b b, trp_pix_color_t *c );

#endif /* !__trppix_internal__h */
