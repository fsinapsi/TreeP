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

#include "./trpgtk_internal.h"

trp_obj_t *trp_gtk_menu_item_new( trp_obj_t *label )
{
    GtkWidget *w;

    if ( label ) {
        uns8b *p = trp_csprint( label );
        if ( strchr( p, '_' ) )
            w = gtk_menu_item_new_with_mnemonic( p );
        else
            w = gtk_menu_item_new_with_label( p );
        trp_csprint_free( p );
    } else
        w = gtk_menu_item_new();
    return trp_gtk_widget( w );
}

void trp_gtk_menu_item_set_submenu( trp_obj_t *obj, trp_obj_t *child )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_MENU_ITEM( o ) )
            if ( child ) {
                GtkWidget *c = trp_gtk_get_widget( child );
                if ( c )
                    if ( GTK_IS_WIDGET( c ) ) {
                        trp_gtk_list_append( &(((trp_gtk_t *)obj)->lw), child, NULL );
                        gtk_menu_item_set_submenu( (GtkMenuItem *)o, (GtkWidget *)c );
                    }
            } else {
                GtkWidget *c = gtk_menu_item_get_submenu( (GtkMenuItem *)o );
                if ( c ) {
                    trp_gtk_list_remove_by_widget( &(((trp_gtk_t *)obj)->lw), c, NULL );
                    gtk_menu_item_set_submenu( (GtkMenuItem *)o, NULL );
                }
            }
}

