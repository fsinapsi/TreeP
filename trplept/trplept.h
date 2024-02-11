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

#ifndef __trplept__h
#define __trplept__h

#include <leptonica/allheaders.h>

uns8b trp_lept_init();
trp_obj_t *trp_lept_version();
trp_obj_t *trp_lept_cversion();
trp_obj_t *trp_lept_pix_find_skew( trp_obj_t *pix );
uns8b trp_lept_pix_dither_to_binary( trp_obj_t *pix );
uns8b trp_lept_pix_unsharp_masking( trp_obj_t *pix, trp_obj_t *halfwidth, trp_obj_t *fract );
uns8b trp_lept_pix_close_gray( trp_obj_t *pix, trp_obj_t *hsize, trp_obj_t *vsize );
uns8b trp_lept_pix_blockconv( trp_obj_t *pix, trp_obj_t *wc, trp_obj_t *hc );
uns8b trp_lept_pix_subtract( trp_obj_t *pixd, trp_obj_t *pixs );
uns8b trp_lept_pix_abs_difference( trp_obj_t *pixd, trp_obj_t *pixs );
uns8b trp_lept_pix_blend( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src, trp_obj_t *fract, trp_obj_t *trans_color );
trp_obj_t *trp_lept_pix_count_conn_comp( trp_obj_t *pix, trp_obj_t *connectivity );

#endif /* !__trplept__h */
