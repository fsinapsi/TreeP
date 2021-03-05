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

#ifndef __trpgtk__h
#define __trpgtk__h

#define GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pango/pango.h>

//extern const guint gtk_major_version;
//extern const guint gtk_minor_version;
//extern const guint gtk_micro_version;

uns8b trp_gtk_init( int *argc, char ***argv );
trp_obj_t *trp_gtk_locale_to_utf8( trp_obj_t *obj );
trp_obj_t *trp_gtk_locale_from_utf8( trp_obj_t *obj );
trp_obj_t *trp_gtk_filename_to_utf8( trp_obj_t *obj );
trp_obj_t *trp_gtk_filename_display_name( trp_obj_t *obj );
trp_obj_t *trp_gtk_filename_display_basename( trp_obj_t *obj );
trp_obj_t *trp_gtk_get_host_name();
trp_obj_t *trp_gtk_get_user_name();
trp_obj_t *trp_gtk_get_real_name();

trp_obj_t *trp_gtk_timeout_add( trp_obj_t *net, trp_obj_t *interval, ... );
trp_obj_t *trp_gtk_idle_add( trp_obj_t *net, ... );
trp_obj_t *trp_gtk_signal_connect( trp_obj_t *obj, trp_obj_t *sig, trp_obj_t *net, ... );
void trp_gtk_signal_handler_disconnect( trp_obj_t *obj, trp_obj_t *id );
void trp_gtk_signal_emit_by_name( trp_obj_t *obj, trp_obj_t *name );

void trp_gtk_widget_show( trp_obj_t *obj );
void trp_gtk_widget_hide( trp_obj_t *obj );
void trp_gtk_widget_show_all( trp_obj_t *obj );
void trp_gtk_widget_realize( trp_obj_t *obj );
void trp_gtk_widget_grab_focus( trp_obj_t *obj );
void trp_gtk_widget_set_sensitive( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_widget_set_size_request( trp_obj_t *obj, trp_obj_t *width, trp_obj_t *height );
void trp_gtk_widget_modify_font( trp_obj_t *obj, trp_obj_t *font );
void trp_gtk_widget_modify_fg( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color );
void trp_gtk_widget_modify_bg( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color );
void trp_gtk_widget_modify_text( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color );
void trp_gtk_widget_modify_base( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color );
void trp_gtk_widget_set_tooltip_text( trp_obj_t *obj, trp_obj_t *text );

void trp_gtk_container_add( trp_obj_t *cont, trp_obj_t *obj );
void trp_gtk_container_remove( trp_obj_t *cont, trp_obj_t *obj );
void trp_gtk_container_set_border_width( trp_obj_t *cont, trp_obj_t *obj );

trp_obj_t *trp_gtk_window_new( trp_obj_t *wtype );
void trp_gtk_window_set_title( trp_obj_t *w, trp_obj_t *obj, ... );
void trp_gtk_window_set_transient_for( trp_obj_t *w, trp_obj_t *parent );
void trp_gtk_window_set_modal( trp_obj_t *w, trp_obj_t *obj );
void trp_gtk_window_set_resizable( trp_obj_t *w, trp_obj_t *obj );
void trp_gtk_window_set_default_size( trp_obj_t *w, trp_obj_t *width, trp_obj_t *height );
void trp_gtk_window_set_position( trp_obj_t *w, trp_obj_t *pos );
void trp_gtk_window_maximize( trp_obj_t *w );
void trp_gtk_window_unmaximize( trp_obj_t *w );
void trp_gtk_window_fullscreen( trp_obj_t *w );
void trp_gtk_window_unfullscreen( trp_obj_t *w );
trp_obj_t *trp_gtk_window_get_position( trp_obj_t *w );
void trp_gtk_window_move( trp_obj_t *w, trp_obj_t *x, trp_obj_t *y );

trp_obj_t *trp_gtk_hbox_new( trp_obj_t *homogeneous, trp_obj_t *space );
trp_obj_t *trp_gtk_vbox_new( trp_obj_t *homogeneous, trp_obj_t *space );
void trp_gtk_box_pack_start( trp_obj_t *box, trp_obj_t *obj, trp_obj_t *expand, trp_obj_t *fill, trp_obj_t *padding );
void trp_gtk_box_pack_end( trp_obj_t *box, trp_obj_t *obj, trp_obj_t *expand, trp_obj_t *fill, trp_obj_t *padding );

trp_obj_t *trp_gtk_label_new( trp_obj_t *obj, trp_obj_t *pattern );
trp_obj_t *trp_gtk_label_get_text( trp_obj_t *label );
void trp_gtk_label_set_text( trp_obj_t *label, trp_obj_t *text, ... );
void trp_gtk_label_set_justify( trp_obj_t *label, trp_obj_t *j );
void trp_gtk_label_set_selectable( trp_obj_t *label, trp_obj_t *on_off );
void trp_gtk_label_set_ellipsize( trp_obj_t *label, trp_obj_t *mode );
void trp_gtk_label_set_pattern( trp_obj_t *label, trp_obj_t *pattern );

trp_obj_t *trp_gtk_button_new( trp_obj_t *label );
void trp_gtk_button_clicked( trp_obj_t *obj );

trp_obj_t *trp_gtk_toggle_button_new( trp_obj_t *label );
void trp_gtk_toggle_button_set_active( trp_obj_t *obj, trp_obj_t *on_off );
trp_obj_t *trp_gtk_toggle_button_get_active( trp_obj_t *obj );
void trp_gtk_toggle_button_set_mode( trp_obj_t *obj, trp_obj_t *on_off );
trp_obj_t *trp_gtk_toggle_button_get_mode( trp_obj_t *obj );
void trp_gtk_toggle_button_set_inconsistent( trp_obj_t *obj, trp_obj_t *on_off );
trp_obj_t *trp_gtk_toggle_button_get_inconsistent( trp_obj_t *obj );

trp_obj_t *trp_gtk_check_button_new( trp_obj_t *label );

trp_obj_t *trp_gtk_file_chooser_get_filename( trp_obj_t *obj );
void trp_gtk_file_chooser_set_filename( trp_obj_t *obj, trp_obj_t *path );
void trp_gtk_file_chooser_set_current_name( trp_obj_t *obj, trp_obj_t *path );
void trp_gtk_file_chooser_set_current_folder( trp_obj_t *obj, trp_obj_t *path );
void trp_gtk_file_chooser_select_filename( trp_obj_t *obj, trp_obj_t *path );
void trp_gtk_file_chooser_set_local_only( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_file_chooser_set_show_hidden( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_file_chooser_set_create_folders( trp_obj_t *obj, trp_obj_t *on_off );

trp_obj_t *trp_gtk_file_chooser_dialog_new( trp_obj_t *title, trp_obj_t *action, trp_obj_t *parent );

trp_obj_t *trp_gtk_file_chooser_widget_new( trp_obj_t *action );

trp_obj_t *trp_gtk_color_button_new( trp_obj_t *color );
void trp_gtk_color_button_set_color( trp_obj_t *obj, trp_obj_t *color );
trp_obj_t *trp_gtk_color_button_get_color( trp_obj_t *obj );
void trp_gtk_color_button_set_use_alpha( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_color_button_set_title( trp_obj_t *obj, trp_obj_t *title );

void trp_gtk_color_selection_set_current_color( trp_obj_t *obj, trp_obj_t *color );
trp_obj_t *trp_gtk_color_selection_get_current_color( trp_obj_t *obj );
void trp_gtk_color_selection_set_has_opacity_control( trp_obj_t *obj, trp_obj_t *on_off );

trp_obj_t *trp_gtk_color_selection_dialog_new( trp_obj_t *title, trp_obj_t *parent );

trp_obj_t *trp_gtk_event_box_new();

trp_obj_t *trp_gtk_notebook_new();
trp_obj_t *trp_gtk_notebook_append_page( trp_obj_t *nb, trp_obj_t *obj, trp_obj_t *lb );
void trp_gtk_notebook_remove_page( trp_obj_t *nb, trp_obj_t *idx );
void trp_gtk_notebook_set_scrollable( trp_obj_t *nb, trp_obj_t *on_off );
trp_obj_t *trp_gtk_notebook_get_current_page( trp_obj_t *nb );
void trp_gtk_notebook_set_current_page( trp_obj_t *nb, trp_obj_t *idx );

trp_obj_t *trp_gtk_image_new( trp_obj_t *width, trp_obj_t *height );
trp_obj_t *trp_gtk_image_new_from_pixbuf( trp_obj_t *pix );
trp_obj_t *trp_gtk_image_new_from_file( trp_obj_t *path );
void trp_gtk_image_set_from_pixbuf( trp_obj_t *obj, trp_obj_t *pix );
trp_obj_t *trp_gtk_image_get_image( trp_obj_t *obj );

trp_obj_t *trp_gtk_adjustment_new( trp_obj_t *value, trp_obj_t *lower, trp_obj_t *upper, trp_obj_t *step_incr, trp_obj_t *page_incr, trp_obj_t *page_size );
trp_obj_t *trp_gtk_adjustment_get_value( trp_obj_t *obj );
void trp_gtk_adjustment_set_value( trp_obj_t *obj, trp_obj_t *value );
trp_obj_t *trp_gtk_adjustment_get_lower( trp_obj_t *obj );
trp_obj_t *trp_gtk_adjustment_get_upper( trp_obj_t *obj );

trp_obj_t *trp_gtk_scrolled_window_new( trp_obj_t *hadj, trp_obj_t *vadj );
void trp_gtk_scrolled_window_add_with_viewport( trp_obj_t *obj, trp_obj_t *child );
void trp_gtk_scrolled_window_set_policy( trp_obj_t *obj, trp_obj_t *hpol, trp_obj_t *vpol );
trp_obj_t *trp_gtk_scrolled_window_get_hadjustment( trp_obj_t *obj );
trp_obj_t *trp_gtk_scrolled_window_get_vadjustment( trp_obj_t *obj );
trp_obj_t *trp_gtk_scrolled_window_get_hscrollbar( trp_obj_t *obj );
trp_obj_t *trp_gtk_scrolled_window_get_vscrollbar( trp_obj_t *obj );

trp_obj_t *trp_gtk_dialog_run( trp_obj_t *obj );

void trp_gtk_editable_set_editable( trp_obj_t *obj, trp_obj_t *on_off );

trp_obj_t *trp_gtk_entry_new( trp_obj_t *str );
trp_obj_t *trp_gtk_entry_get_text( trp_obj_t *obj );
void trp_gtk_entry_set_text( trp_obj_t *obj, trp_obj_t *str );
void trp_gtk_entry_set_max_length( trp_obj_t *obj, trp_obj_t *n );
void trp_gtk_entry_set_visibility( trp_obj_t *obj, trp_obj_t *on_off );

trp_obj_t *trp_gtk_spin_button_new( trp_obj_t *adj, trp_obj_t *climb_rate, trp_obj_t *digits );
trp_obj_t *trp_gtk_spin_button_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step );
void trp_gtk_spin_button_set_value( trp_obj_t *obj, trp_obj_t *val );
trp_obj_t *trp_gtk_spin_button_get_value( trp_obj_t *obj );
void trp_gtk_spin_button_set_numeric( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_spin_button_set_wrap( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_spin_button_set_range( trp_obj_t *obj, trp_obj_t *min, trp_obj_t *max );
void trp_gtk_spin_button_update( trp_obj_t *obj );

trp_obj_t *trp_gtk_hscale_new( trp_obj_t *adj );
trp_obj_t *trp_gtk_vscale_new( trp_obj_t *adj );
trp_obj_t *trp_gtk_hscale_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step );
trp_obj_t *trp_gtk_vscale_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step );
void trp_gtk_scale_set_draw_value( trp_obj_t *obj, trp_obj_t *on_off );

trp_obj_t *trp_gtk_range_get_value( trp_obj_t *obj );
void trp_gtk_range_set_value( trp_obj_t *obj, trp_obj_t *val );
trp_obj_t *trp_gtk_range_get_lower_stepper_sensitivity( trp_obj_t *obj );
trp_obj_t *trp_gtk_range_get_upper_stepper_sensitivity( trp_obj_t *obj );

trp_obj_t *trp_gtk_combo_box_new();
trp_obj_t *trp_gtk_combo_box_get_active( trp_obj_t *obj );
void trp_gtk_combo_box_set_active( trp_obj_t *obj, trp_obj_t *idx );

trp_obj_t *trp_gtk_combo_box_text_new();
void trp_gtk_combo_box_text_append_text( trp_obj_t *obj, trp_obj_t *text );
void trp_gtk_combo_box_text_remove( trp_obj_t *obj, trp_obj_t *pos );

trp_obj_t *trp_gtk_text_view_new( trp_obj_t *buffer );
void trp_gtk_text_view_set_editable( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_text_view_set_wrap_mode( trp_obj_t *obj, trp_obj_t *mode );
void trp_gtk_text_view_set_cursor_visible( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_text_view_set_overwrite( trp_obj_t *obj, trp_obj_t *on_off );

void trp_gtk_text_buffer_append( trp_obj_t *obj, ... );
void trp_gtk_text_buffer_append_color( trp_obj_t *obj, trp_obj_t *color, ... );
trp_obj_t *trp_gtk_text_buffer_get_text( trp_obj_t *obj, trp_obj_t *include_hidden_chars );
void trp_gtk_text_buffer_set_text( trp_obj_t *obj, ... );
void trp_gtk_text_buffer_set_text_color( trp_obj_t *obj, trp_obj_t *color, ... );
void trp_gtk_text_buffer_select_all( trp_obj_t *obj );

trp_obj_t *trp_gtk_progress_bar_new();
void trp_gtk_progress_bar_pulse( trp_obj_t *obj );
void trp_gtk_progress_bar_set_text( trp_obj_t *obj, trp_obj_t *msg, ... );
void trp_gtk_progress_bar_set_fraction( trp_obj_t *obj, trp_obj_t *percent );
void trp_gtk_progress_bar_set_pulse_step( trp_obj_t *obj, trp_obj_t *percent );

trp_obj_t *trp_gtk_font_button_new( trp_obj_t *font );
void trp_gtk_font_button_set_font_name( trp_obj_t *obj, trp_obj_t *font );
trp_obj_t *trp_gtk_font_button_get_font_name( trp_obj_t *obj );
void trp_gtk_font_button_set_show_style( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_font_button_set_show_size( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_font_button_set_use_font( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_font_button_set_use_size( trp_obj_t *obj, trp_obj_t *on_off );
void trp_gtk_font_button_set_title( trp_obj_t *obj, trp_obj_t *title );

trp_obj_t *trp_gtk_hseparator_new();
trp_obj_t *trp_gtk_vseparator_new();

trp_obj_t *trp_gtk_frame_new( trp_obj_t *label );
void trp_gtk_frame_set_label( trp_obj_t *obj, trp_obj_t *label );

trp_obj_t *trp_gtk_menu_item_new( trp_obj_t *label );
void trp_gtk_menu_item_set_submenu( trp_obj_t *obj, trp_obj_t *child );

trp_obj_t *trp_gtk_menu_bar_new();

trp_obj_t *trp_gtk_menu_new();

void trp_gtk_menu_shell_append( trp_obj_t *obj, trp_obj_t *child );

trp_obj_t *trp_gtk_tree_view_new();

trp_obj_t *trp_gtk_message_dialog_new( trp_obj_t *parent, trp_obj_t *flags, trp_obj_t *type, trp_obj_t *buttons, trp_obj_t *msg, ...  );

trp_obj_t *trp_gtk_calendar_new();
void trp_gtk_calendar_set_date( trp_obj_t *obj, trp_obj_t *date );
trp_obj_t *trp_gtk_calendar_get_date( trp_obj_t *obj );

void trp_gtk_drag_dest_set( trp_obj_t *obj, trp_obj_t *net, trp_obj_t *udata );
void trp_gtk_drag_dest_unset( trp_obj_t *obj );

void trp_gtk_graph_qscale( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt );
void trp_gtk_graph_size( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt, trp_obj_t *mag, trp_obj_t *avg_int );
void trp_gtk_graph_vbvfill( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt, trp_obj_t *vbv_size, trp_obj_t *vbv_init, trp_obj_t *vbv_rate, trp_obj_t *fps );

#define trp_gtk_main()                 gtk_main()
#define trp_gtk_main_quit()            gtk_main_quit()
#define trp_gtk_main_iteration()       gtk_main_iteration()
#define trp_gtk_major_version()        trp_sig64(gtk_major_version)
#define trp_gtk_minor_version()        trp_sig64(gtk_minor_version)
#define trp_gtk_micro_version()        trp_sig64(gtk_micro_version)
#define trp_gtk_width(o)               ((o)?trp_width(o):trp_sig64(gdk_screen_width()))
#define trp_gtk_height(o)              ((o)?trp_height(o):trp_sig64(gdk_screen_height()))
#define trp_gtk_depth()                trp_sig64(gdk_visual_get_system()->depth)
#define trp_gtk_events_pending()       ((gtk_events_pending()==TRUE)?TRP_TRUE:TRP_FALSE)
#define trp_gtk_get_default_language() trp_cord(pango_language_to_string(gtk_get_default_language()))

#endif /* !__trpgtk__h */
