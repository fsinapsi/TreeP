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

#ifndef __trppix__h
#define __trppix__h

uns8b trp_pix_init();
void trp_pix_quit();
trp_obj_t *trp_pix_color( trp_obj_t *red, trp_obj_t *green, trp_obj_t *blue, trp_obj_t *alpha );
trp_obj_t *trp_pix_color_red( trp_obj_t *obj );
trp_obj_t *trp_pix_color_green( trp_obj_t *obj );
trp_obj_t *trp_pix_color_blue( trp_obj_t *obj );
trp_obj_t *trp_pix_color_alpha( trp_obj_t *obj );
trp_obj_t *trp_pix_create( trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_info( trp_obj_t *path );
trp_obj_t *trp_pix_load( trp_obj_t *path );
trp_obj_t *trp_pix_load_multiple( trp_obj_t *path );
trp_obj_t *trp_pix_load_memory( trp_obj_t *raw, trp_obj_t *cnt );
trp_obj_t *trp_pix_load_memory_ext( trp_obj_t *raw, trp_obj_t *w, trp_obj_t *h, trp_obj_t *cnt );
trp_obj_t *trp_pix_load_thumbnail( trp_obj_t *path, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_load_thumbnail_memory( trp_obj_t *raw, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_load_thumbnail_memory_ext( trp_obj_t *raw, trp_obj_t *sw, trp_obj_t *sh, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_image_format( trp_obj_t *obj );
uns8b trp_pix_save_png( trp_obj_t *pix, trp_obj_t *path );
uns8b trp_pix_save_jpg( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *quality );
trp_obj_t *trp_pix_save_jpg_memory( trp_obj_t *pix, trp_obj_t *quality );
uns8b trp_pix_save_pnm( trp_obj_t *pix, trp_obj_t *path );
uns8b trp_pix_save_pnm_noalpha( trp_obj_t *pix, trp_obj_t *path );
uns8b trp_pix_save_gif( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *transp, trp_obj_t *delay );
uns8b trp_pix_save_yuv4mpeg2_init( trp_obj_t *width, trp_obj_t *height, trp_obj_t *framerate, trp_obj_t *aspect_ratio, trp_obj_t *f );
uns8b trp_pix_save_yuv4mpeg2( trp_obj_t *pix, trp_obj_t *f );
uns8b trp_pix_set_color( trp_obj_t *pix, trp_obj_t *color );
trp_obj_t *trp_pix_get_color( trp_obj_t *pix );
trp_obj_t *trp_pix_get_luminance( trp_obj_t *pix );
trp_obj_t *trp_pix_get_contrast( trp_obj_t *pix );
uns8b trp_pix_draw_pix( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src );
uns8b trp_pix_draw_pix_alpha( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src );
uns8b trp_pix_draw_pix_odd_lines( trp_obj_t *dst, trp_obj_t *src );
uns8b trp_pix_draw_box( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h, trp_obj_t *color );
uns8b trp_pix_draw_point( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *color );
uns8b trp_pix_draw_line( trp_obj_t *dst, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *color );
uns8b trp_pix_draw_dashed_line( trp_obj_t *dst, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2,  trp_obj_t *l1, trp_obj_t *l2, trp_obj_t *color );
uns8b trp_pix_draw_circle( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *rad, trp_obj_t *color );
uns8b trp_pix_draw_grid( trp_obj_t *dst, trp_obj_t *size, trp_obj_t *color );
trp_obj_t *trp_pix_grayp( trp_obj_t *pix );
trp_obj_t *trp_pix_bwp( trp_obj_t *pix );
trp_obj_t *trp_pix_point( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y );
trp_obj_t *trp_pix_color_count( trp_obj_t *pix, trp_obj_t *color );
trp_obj_t *trp_pix_top_field( trp_obj_t *pix );
trp_obj_t *trp_pix_bottom_field( trp_obj_t *pix );
trp_obj_t *trp_pix_crop( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h );
uns8b trp_pix_bgr( trp_obj_t *pix );
uns8b trp_pix_noalpha( trp_obj_t *pix );
uns8b trp_pix_gray( trp_obj_t *pix );
uns8b trp_pix_gray16( trp_obj_t *pix );
uns8b trp_pix_gray_maximize_range( trp_obj_t *pix, trp_obj_t *black );
uns8b trp_pix_bw( trp_obj_t *pix, trp_obj_t *threshold );
uns8b trp_pix_linear( trp_obj_t *pix, trp_obj_t *min1, trp_obj_t *max1, trp_obj_t *min2, trp_obj_t *max2 );
uns8b trp_pix_negative( trp_obj_t *pix );
uns8b trp_pix_transparent( trp_obj_t *pix, trp_obj_t *color );
uns8b trp_pix_clralpha( trp_obj_t *pix, trp_obj_t *color );
uns8b trp_pix_hflip( trp_obj_t *pix );
uns8b trp_pix_vflip( trp_obj_t *pix );
uns8b trp_pix_rotate_test( trp_obj_t *pix, trp_obj_t *angle );
trp_obj_t *trp_pix_rotate( trp_obj_t *pix, trp_obj_t *angle );
uns8b trp_pix_brightness( trp_obj_t *pix, trp_obj_t *val );
uns8b trp_pix_brightness_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b );
uns8b trp_pix_contrast( trp_obj_t *pix, trp_obj_t *val );
uns8b trp_pix_contrast_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b );
uns8b trp_pix_gamma( trp_obj_t *pix, trp_obj_t *val );
uns8b trp_pix_gamma_rgb( trp_obj_t *pix, trp_obj_t *val_r, trp_obj_t *val_g, trp_obj_t *val_b );
uns8b trp_pix_snap_color( trp_obj_t *pix,  trp_obj_t *src_color,  trp_obj_t *thres, trp_obj_t *dst_color );
uns8b trp_pix_scale_test( trp_obj_t *pix_i, trp_obj_t *pix_o );
trp_obj_t *trp_pix_scale( trp_obj_t *pix, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_mse( trp_obj_t *pix1, trp_obj_t *pix2 );
trp_obj_t *trp_pix_gray_histogram( trp_obj_t *pix );
trp_obj_t *trp_pix_text( trp_obj_t *s, ... );
trp_obj_t *trp_pix_ssim( trp_obj_t *pix1, trp_obj_t *pix2, trp_obj_t *weights );
trp_obj_t *trp_pix_ssim_linear( trp_obj_t *pix1, trp_obj_t *pix2 );
trp_obj_t *trp_pix_ssim_gaussian( trp_obj_t *pix1, trp_obj_t *pix2 );
trp_obj_t *trp_pix_scd( trp_obj_t *pix, trp_obj_t *ref, trp_obj_t *dimblock, trp_obj_t *radius );
trp_obj_t *trp_pix_scd_histogram( trp_obj_t *pix, trp_obj_t *ref );
trp_obj_t *trp_pix_box_stat( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_pix_is_empty( trp_obj_t *pix, trp_obj_t *threshold );
trp_obj_t *trp_pix_trim( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold );
trp_obj_t *trp_pix_trim_values( trp_obj_t *pix, trp_obj_t *color, trp_obj_t *threshold );
/*
uns8b trp_pix_draw_text( trp_obj_t *pix, trp_obj_t *x, trp_obj_t *y, trp_obj_t *text, ... );
*/

#endif /* !__trppix__h */
