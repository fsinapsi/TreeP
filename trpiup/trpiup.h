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

#ifndef __trpiup__h
#define __trpiup__h

#include <iup/iup.h>
#include <iup/iupkey.h>

#define K_csLEFT iup_XkeyCtrl(K_sLEFT)
#define K_csRIGHT iup_XkeyCtrl(K_sRIGHT)
#define K_csUP iup_XkeyCtrl(K_sUP)
#define K_csDOWN iup_XkeyCtrl(K_sDOWN)
#define K_csD iup_XkeyShift(K_cD)
#define K_csS iup_XkeyShift(K_cS)
#define K_csV iup_XkeyShift(K_cV)

uns8b trp_iup_init( int *argc, char ***argv );
void trp_iup_quit();
trp_obj_t *trp_iup_version();
trp_obj_t *trp_iup_version_date();
trp_obj_t *trp_iup_version_number();
uns8b trp_iup_main_loop();
uns8b trp_iup_loop_step();
uns8b trp_iup_loop_step_wait();
uns8b trp_iup_main_loop_level();
uns8b trp_iup_flush();
uns8b trp_iup_exit_loop();
uns8b trp_iup_set_str_global( trp_obj_t *name, trp_obj_t *value, ... );
uns8b trp_iup_set_attribute( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *ihvalue );
uns8b trp_iup_set_str_attribute( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value, ... );
uns8b trp_iup_set_int( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value );
uns8b trp_iup_set_double( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value );
uns8b trp_iup_set_attribute_handle( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *ih_named );
uns8b trp_iup_set_callback( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *cback );
uns8b trp_iup_popup( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y );
uns8b trp_iup_show( trp_obj_t *ih );
uns8b trp_iup_show_xy( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y );
uns8b trp_iup_hide( trp_obj_t *ih );
uns8b trp_iup_map( trp_obj_t *ih );
uns8b trp_iup_unmap( trp_obj_t *ih );
uns8b trp_iup_detach( trp_obj_t *ih );
uns8b trp_iup_refresh( trp_obj_t *ih );
uns8b trp_iup_refresh_children( trp_obj_t *ih );
uns8b trp_iup_update( trp_obj_t *ih );
uns8b trp_iup_update_children( trp_obj_t *ih );
uns8b trp_iup_redraw( trp_obj_t *ih );
uns8b trp_iup_redraw_children( trp_obj_t *ih );
uns8b trp_iup_set_focus( trp_obj_t *ih );
uns8b trp_iup_append( trp_obj_t *ih, trp_obj_t *new_child );
uns8b trp_iup_insert( trp_obj_t *ih, trp_obj_t *ref_child, trp_obj_t *new_child );
uns8b trp_iup_message( trp_obj_t *title, trp_obj_t *text, ... );
trp_obj_t *trp_iup_get_global( trp_obj_t *name );
trp_obj_t *trp_iup_get_attribute( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_str_attribute( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_int( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_int2( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_double( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_attribute_handle( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_get_child_count( trp_obj_t *ih );
trp_obj_t *trp_iup_get_focus();
trp_obj_t *trp_iup_timer();
trp_obj_t *trp_iup_fill();
trp_obj_t *trp_iup_dialog( trp_obj_t *child );
trp_obj_t *trp_iup_vbox( trp_obj_t *child, ... );
trp_obj_t *trp_iup_zbox( trp_obj_t *child, ... );
trp_obj_t *trp_iup_hbox( trp_obj_t *child, ... );
trp_obj_t *trp_iup_cbox( trp_obj_t *child, ... );
trp_obj_t *trp_iup_normalizer( trp_obj_t *child, ... );
trp_obj_t *trp_iup_tabs( trp_obj_t *child, ... );
trp_obj_t *trp_iup_sbox( trp_obj_t *child );
trp_obj_t *trp_iup_scroll_box( trp_obj_t *child );
trp_obj_t *trp_iup_frame( trp_obj_t *child );
trp_obj_t *trp_iup_radio( trp_obj_t *child );
trp_obj_t *trp_iup_expander( trp_obj_t *child );
trp_obj_t *trp_iup_split( trp_obj_t *child1, trp_obj_t *child2 );
trp_obj_t *trp_iup_label( trp_obj_t *title, ... );
trp_obj_t *trp_iup_flat_label( trp_obj_t *title, ... );
trp_obj_t *trp_iup_animated_label( trp_obj_t *animation );
trp_obj_t *trp_iup_link( trp_obj_t *url, trp_obj_t *title, ... );
trp_obj_t *trp_iup_button( trp_obj_t *title );
trp_obj_t *trp_iup_toggle( trp_obj_t *title );
trp_obj_t *trp_iup_val();
trp_obj_t *trp_iup_spin();
trp_obj_t *trp_iup_spinbox( trp_obj_t *child );
trp_obj_t *trp_iup_color_browser();
trp_obj_t *trp_iup_list();
trp_obj_t *trp_iup_flatlist();
trp_obj_t *trp_iup_text();
trp_obj_t *trp_iup_user();
trp_obj_t *trp_iup_clipboard();
trp_obj_t *trp_iup_progress_bar();
trp_obj_t *trp_iup_gauge();
trp_obj_t *trp_iup_item( trp_obj_t *title );
trp_obj_t *trp_iup_submenu( trp_obj_t *title, trp_obj_t *child );
trp_obj_t *trp_iup_separator();
trp_obj_t *trp_iup_menu( trp_obj_t *child, ... );
trp_obj_t *trp_iup_date_pick();
trp_obj_t *trp_iup_calendar();
trp_obj_t *trp_iup_image_rgba( trp_obj_t *pix );
trp_obj_t *trp_iup_file_dlg();
trp_obj_t *trp_iup_message_dlg();
trp_obj_t *trp_iup_color_dlg();
trp_obj_t *trp_iup_font_dlg();
trp_obj_t *trp_iup_progress_dlg();
trp_obj_t *trp_iup_get_brother( trp_obj_t *ih );
trp_obj_t *trp_iup_get_parent( trp_obj_t *ih );
trp_obj_t *trp_iup_get_dialog( trp_obj_t *ih );
trp_obj_t *trp_iup_get_dialog_child( trp_obj_t *ih, trp_obj_t *name );
trp_obj_t *trp_iup_text_convert_lin_col_to_pos( trp_obj_t *ih, trp_obj_t *lin, trp_obj_t *col );
trp_obj_t *trp_iup_text_convert_pos_to_lin_col( trp_obj_t *ih, trp_obj_t *pos );
trp_obj_t *trp_iup_convert_xy_to_pos( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y );
uns8b trp_iup_post_call( trp_obj_t *cback );

#endif /* !__trpiup__h */
