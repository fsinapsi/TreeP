/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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

enum {
    TRP_PIX_PNG = 0,
    TRP_PIX_JPG,
    TRP_PIX_PNM,
    TRP_PIX_GIF,
    TRP_PIX_MAX /* lasciarlo sempre per ultimo */
};

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

uns8b trp_pix_close( trp_pix_t *obj );
trp_obj_t *trp_pix_create_image_from_data( int must_copy, uns32b w, uns32b h, uns8b *data );
trp_obj_t *trp_pix_create_basic( uns32b w, uns32b h );
trp_obj_t *trp_pix_create_color( uns16b red, uns16b green, uns16b blue, uns16b alpha );
uns8b trp_pix_decode_color( trp_obj_t *obj, uns16b *red, uns16b *green, uns16b *blue, uns16b *alpha );
uns8b trp_pix_decode_color_uns8b( trp_obj_t *obj, trp_obj_t *pix, uns8b *red, uns8b *green, uns8b *blue, uns8b *alpha );
trp_obj_t *trp_pix_load_basic( uns8b *cpath );
uns8b trp_pix_info_png( uns8b *cpath, uns32b *w, uns32b *h );
uns8b trp_pix_load_png( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_load_png_memory( trp_raw_t *raw, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_info_jpg( uns8b *cpath, uns32b *w, uns32b *h );
uns8b trp_pix_load_jpg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_info_pnm( uns8b *cpath, uns32b *w, uns32b *h );
uns8b trp_pix_load_pnm( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
uns8b trp_pix_info_gif( uns8b *cpath, uns32b *w, uns32b *h );
uns8b trp_pix_load_gif( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
trp_obj_t *trp_pix_load_gif_multiple( uns8b *cpath );
trp_obj_t *trp_pix_crop_low( trp_obj_t *pix, double xx, double yy, double ww, double hh );
uns8b trp_pix_rotate_test_low( trp_obj_t *pix, sig64b a );
trp_obj_t *trp_pix_rotate_orthogonal( trp_obj_t *pix, sig64b a );

#endif /* !__trppix_internal__h */
