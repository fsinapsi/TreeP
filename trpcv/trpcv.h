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

#ifndef __trpcv__h
#define __trpcv__h

#include <cv.h>

uns8b trp_cv_init();
trp_obj_t *trp_cv_version();
trp_obj_t *trp_cv_pix_load( trp_obj_t *path );
uns8b trp_cv_pix_gray( trp_obj_t *pix );
uns8b trp_cv_pix_smooth( trp_obj_t *pix, trp_obj_t *type, trp_obj_t *size1, trp_obj_t *size2, trp_obj_t *sigma1, trp_obj_t *sigma2 );
trp_obj_t *trp_cv_pix_rotate( trp_obj_t *pix, trp_obj_t *angle, trp_obj_t *flags );
trp_obj_t *trp_cv_get_affine_transform( trp_obj_t *sx1, trp_obj_t *sy1, trp_obj_t *dx1, trp_obj_t *dy1, trp_obj_t *sx2, trp_obj_t *sy2, trp_obj_t *dx2, trp_obj_t *dy2, trp_obj_t *sx3, trp_obj_t *sy3, trp_obj_t *dx3, trp_obj_t *dy3 );
uns8b trp_cv_pix_warp_affine( trp_obj_t *pixi, trp_obj_t *pixo, trp_obj_t *aff_mat, trp_obj_t *flags );
trp_obj_t *trp_cv_sift_features( trp_obj_t *pix );
trp_obj_t *trp_cv_sift_features_draw( trp_obj_t *pix );
trp_obj_t *trp_cv_sift_match( trp_obj_t *o1, trp_obj_t *o2, trp_obj_t *thr );

#endif /* !__trpcv__h */
