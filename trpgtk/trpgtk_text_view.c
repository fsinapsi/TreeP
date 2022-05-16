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

#include "./trpgtk_internal.h"

trp_obj_t *trp_gtk_text_view_new( trp_obj_t *buffer )
{
    trp_obj_t *res = UNDEF;

    if ( buffer ) {
        GtkWidget *buf = trp_gtk_get_widget( buffer );

        if ( buf )
            if ( GTK_IS_TEXT_BUFFER( buf ) ) {
                res = trp_gtk_widget( gtk_text_view_new_with_buffer( (GtkTextBuffer *)buf ) );
                trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), buffer, NULL );
            }
    } else
        res = trp_gtk_widget( gtk_text_view_new() );
    return res;
}

void trp_gtk_text_view_set_editable( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TEXT_VIEW( o ) )
            gtk_text_view_set_editable( (GtkTextView *)o, BOOLVAL( on_off ) );
}

void trp_gtk_text_view_set_wrap_mode( trp_obj_t *obj, trp_obj_t *mode )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( mode->tipo == TRP_SIG64 ) )
        if ( GTK_IS_TEXT_VIEW( o ) )
            gtk_text_view_set_wrap_mode( (GtkTextView *)o, ((trp_sig64_t *)mode)->val );
}

void trp_gtk_text_view_set_cursor_visible( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TEXT_VIEW( o ) )
            gtk_text_view_set_cursor_visible( (GtkTextView *)o, BOOLVAL( on_off ) );
}

void trp_gtk_text_view_set_overwrite( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TEXT_VIEW( o ) )
            gtk_text_view_set_overwrite( (GtkTextView *)o, BOOLVAL( on_off ) );
}

