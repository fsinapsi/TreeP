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

#ifndef __trpmgl__h
#define __trpmgl__h

uns8b trp_mgl_init();
trp_obj_t *trp_mgl_create_graph_zb( trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_mgl_create_graph_ps( trp_obj_t *w, trp_obj_t *h );
trp_obj_t *trp_mgl_create_graph_idtf();
trp_obj_t *trp_mgl_create_data();
trp_obj_t *trp_mgl_create_data_size( trp_obj_t *nx, trp_obj_t *ny, trp_obj_t *nz );
trp_obj_t *trp_mgl_create_data_file( trp_obj_t *path );
trp_obj_t *trp_mgl_create_parser();
uns8b trp_mgl_write_bmp( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_jpg( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_png( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_png_solid( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_eps( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_svg( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_idtf( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_write_gif( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr );
uns8b trp_mgl_update( trp_obj_t *mgl );
uns8b trp_mgl_box( trp_obj_t *mgl, trp_obj_t *ticks );
uns8b trp_mgl_set_light( trp_obj_t *mgl, trp_obj_t *enable );
uns8b trp_mgl_rotate( trp_obj_t *mgl, trp_obj_t *tetx, trp_obj_t *tety, trp_obj_t *tetz );
uns8b trp_mgl_data_modify( trp_obj_t *mgd, trp_obj_t *eq, trp_obj_t *dim );
uns8b trp_mgl_plot( trp_obj_t *mgl, trp_obj_t *mgd, trp_obj_t *pen );
uns8b trp_mgl_surf( trp_obj_t *mgl, trp_obj_t *mgd, trp_obj_t *sch );

#endif /* !__trpmgl__h */
