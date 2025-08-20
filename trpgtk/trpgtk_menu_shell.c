/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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

void trp_gtk_menu_shell_append( trp_obj_t *obj, trp_obj_t *child )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    GtkWidget *c = trp_gtk_get_widget( child );
    if ( o && c )
        if ( GTK_IS_MENU_SHELL( o ) &&
             GTK_IS_MENU_ITEM( c ) ) {
            trp_gtk_list_append( &(((trp_gtk_t *)obj)->lw), child, NULL );
            gtk_menu_shell_append( (GtkMenuShell *)o, (GtkWidget *)c );
        }
}

