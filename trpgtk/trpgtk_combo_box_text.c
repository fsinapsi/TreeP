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

#include "./trpgtk_internal.h"

#if (GTK_MAJOR_VERSION > 2) || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 24)

trp_obj_t *trp_gtk_combo_box_text_new()
{
    return trp_gtk_widget( gtk_combo_box_text_new() );
}

void trp_gtk_combo_box_text_append_text( trp_obj_t *obj, trp_obj_t *text )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_COMBO_BOX_TEXT( o ) ) {
            uns8b *p = trp_csprint( text );
            gtk_combo_box_text_append_text( (GtkComboBoxText *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_combo_box_text_remove( trp_obj_t *obj, trp_obj_t *pos )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( pos->tipo == TRP_SIG64 ) )
        if ( GTK_IS_COMBO_BOX_TEXT( o ) )
            gtk_combo_box_text_remove( (GtkComboBoxText *)o, ((trp_sig64_t *)pos)->val );
}

#else

trp_obj_t *trp_gtk_combo_box_text_new()
{
    return trp_gtk_widget( gtk_combo_box_new_text() );
}

void trp_gtk_combo_box_text_append_text( trp_obj_t *obj, trp_obj_t *text )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_COMBO_BOX( o ) ) {
            uns8b *p = trp_csprint( text );
            gtk_combo_box_append_text( (GtkComboBox *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_combo_box_text_remove( trp_obj_t *obj, trp_obj_t *pos )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( pos->tipo == TRP_SIG64 ) )
        if ( GTK_IS_COMBO_BOX( o ) )
            gtk_combo_box_remove_text( (GtkComboBox *)o, ((trp_sig64_t *)pos)->val );
}

#endif



