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

#ifndef __trpcairo__h
#define __trpcairo__h

#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <cairo-ft.h>
#include FT_SFNT_NAMES_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include FT_TYPE1_TABLES_H

uns8b trp_cairo_init();
void trp_cairo_quit();
trp_obj_t *trp_cairo_svg_surface_create_for_stream( trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_cairo_svg_surface_create_from_svg( trp_obj_t *src );
trp_obj_t *trp_cairo_pdf_surface_create( trp_obj_t *path, trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_cairo_flush_and_close_raw( trp_obj_t *obj );
trp_obj_t *trp_cairo_flush_and_close_string( trp_obj_t *obj );
trp_obj_t *trp_cairo_get_matrix( trp_obj_t *obj );
trp_obj_t *trp_cairo_user_to_device( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
trp_obj_t *trp_cairo_user_to_device_distance( trp_obj_t *obj, trp_obj_t *dx, trp_obj_t *dy );
trp_obj_t *trp_cairo_device_to_user( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
trp_obj_t *trp_cairo_device_to_user_distance( trp_obj_t *obj, trp_obj_t *dx, trp_obj_t *dy );
trp_obj_t *trp_cairo_get_ft_family_name( trp_obj_t *obj );
trp_obj_t *trp_cairo_get_ft_style_name( trp_obj_t *obj );
trp_obj_t *trp_cairo_get_ft_postscript_name( trp_obj_t *obj );
trp_obj_t *trp_cairo_get_ft_available_chars( trp_obj_t *obj );
trp_obj_t *trp_cairo_toy_font_face_get_family( trp_obj_t *obj );
trp_obj_t *trp_cairo_font_extents( trp_obj_t *obj );
trp_obj_t *trp_cairo_text_extents( trp_obj_t *obj, trp_obj_t *s, ...  );
uns8b trp_cairo_surface_flush( trp_obj_t *obj );
uns8b trp_cairo_save( trp_obj_t *obj );
uns8b trp_cairo_restore( trp_obj_t *obj );
uns8b trp_cairo_copy_page( trp_obj_t *obj );
uns8b trp_cairo_show_page( trp_obj_t *obj );
uns8b trp_cairo_translate( trp_obj_t *obj, trp_obj_t *tx, trp_obj_t *ty );
uns8b trp_cairo_scale( trp_obj_t *obj, trp_obj_t *sx, trp_obj_t *sy );
uns8b trp_cairo_rotate( trp_obj_t *obj, trp_obj_t *angle );
uns8b trp_cairo_transform( trp_obj_t *obj, trp_obj_t *xx, trp_obj_t *xy, trp_obj_t *x0, trp_obj_t *yx, trp_obj_t *yy, trp_obj_t *y0 );
uns8b trp_cairo_set_matrix( trp_obj_t *obj, trp_obj_t *xx, trp_obj_t *xy, trp_obj_t *x0, trp_obj_t *yx, trp_obj_t *yy, trp_obj_t *y0 );
uns8b trp_cairo_identity_matrix( trp_obj_t *obj );
uns8b trp_cairo_set_antialias( trp_obj_t *obj, trp_obj_t *antialias );
uns8b trp_cairo_select_font_face( trp_obj_t *obj, trp_obj_t *family, trp_obj_t *slant, trp_obj_t *weight );
uns8b trp_cairo_set_font_face_ft( trp_obj_t *obj, trp_obj_t *path );
uns8b trp_cairo_set_font_size( trp_obj_t *obj, trp_obj_t *size );
uns8b trp_cairo_set_line_width( trp_obj_t *obj, trp_obj_t *width );
uns8b trp_cairo_set_line_cap( trp_obj_t *obj, trp_obj_t *line_cap );
uns8b trp_cairo_set_line_join( trp_obj_t *obj, trp_obj_t *line_cap );
uns8b trp_cairo_set_fill_rule( trp_obj_t *obj, trp_obj_t *fill_rule );
uns8b trp_cairo_set_miter_limit( trp_obj_t *obj, trp_obj_t *limit );
uns8b trp_cairo_set_source_rgba( trp_obj_t *obj, trp_obj_t *color );
uns8b trp_cairo_set_source_surface( trp_obj_t *obj, trp_obj_t *src, trp_obj_t *x, trp_obj_t *y, trp_obj_t *alpha );
uns8b trp_cairo_new_path( trp_obj_t *obj );
uns8b trp_cairo_close_path( trp_obj_t *obj );
uns8b trp_cairo_stroke( trp_obj_t *obj );
uns8b trp_cairo_stroke_preserve( trp_obj_t *obj );
uns8b trp_cairo_fill( trp_obj_t *obj );
uns8b trp_cairo_fill_preserve( trp_obj_t *obj );
uns8b trp_cairo_clip( trp_obj_t *obj );
uns8b trp_cairo_clip_preserve( trp_obj_t *obj );
uns8b trp_cairo_reset_clip( trp_obj_t *obj );
uns8b trp_cairo_paint( trp_obj_t *obj );
uns8b trp_cairo_paint_with_alpha( trp_obj_t *obj, trp_obj_t *alpha );
uns8b trp_cairo_move_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
uns8b trp_cairo_rel_move_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
uns8b trp_cairo_line_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
uns8b trp_cairo_rel_line_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y );
uns8b trp_cairo_curve_to( trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *x3, trp_obj_t *y3 );
uns8b trp_cairo_rel_curve_to( trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *x3, trp_obj_t *y3 );
uns8b trp_cairo_rectangle( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h );
uns8b trp_cairo_arc( trp_obj_t *obj, trp_obj_t *xc, trp_obj_t *yc, trp_obj_t *radius, trp_obj_t *angle1, trp_obj_t *angle2 );
uns8b trp_cairo_arc_negative( trp_obj_t *obj, trp_obj_t *xc, trp_obj_t *yc, trp_obj_t *radius, trp_obj_t *angle1, trp_obj_t *angle2 );
uns8b trp_cairo_show_text( trp_obj_t *obj, trp_obj_t *s, ... );

#endif /* !__trpcairo__h */
