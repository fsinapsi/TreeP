/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

#ifndef __trpvlfeat__h
#define __trpvlfeat__h

uns8b trp_vl_init();
trp_obj_t *trp_vl_version();
trp_obj_t *trp_vl_configuration();
trp_obj_t *trp_vl_get_simd_enabled();
trp_obj_t *trp_vl_cpu_has_sse2();
trp_obj_t *trp_vl_cpu_has_sse3();
trp_obj_t *trp_vl_cpu_has_avx();
trp_obj_t *trp_vl_sift_new( trp_obj_t *w, trp_obj_t *h, trp_obj_t *octaves, trp_obj_t *levels, trp_obj_t *o_min );
uns8b trp_vl_sift_set_peak_thresh( trp_obj_t *f, trp_obj_t *val );
uns8b trp_vl_sift_set_edge_thresh( trp_obj_t *f, trp_obj_t *val );
uns8b trp_vl_sift_set_norm_thresh( trp_obj_t *f, trp_obj_t *val );
uns8b trp_vl_sift_set_magnif( trp_obj_t *f, trp_obj_t *val );
uns8b trp_vl_sift_set_window_size( trp_obj_t *f, trp_obj_t *val );
uns8b trp_vl_sift_process_first_octave( trp_obj_t *f, trp_obj_t *pix );
uns8b trp_vl_sift_process_next_octave( trp_obj_t *f );
uns8b trp_vl_sift_detect( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_octave_index( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_noctaves( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_octave_first( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_octave_width( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_octave_height( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_nlevels( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_nkeypoints( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_peak_thresh( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_edge_thresh( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_norm_thresh( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_magnif( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_window_size( trp_obj_t *f );
trp_obj_t *trp_vl_sift_get_gss( trp_obj_t *f, trp_obj_t *level );
uns8b trp_vl_sift_match_descr( trp_obj_t *f, trp_obj_t *pix );
trp_obj_t *trp_vl_sift_match( trp_obj_t *f1, trp_obj_t *f2, trp_obj_t *cmp, trp_obj_t *thr );

#endif /* !__trpvlfeat__h */
