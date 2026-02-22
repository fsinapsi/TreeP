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

void trp_gtk_container_add( trp_obj_t *cont, trp_obj_t *obj )
{
    GtkWidget *cc = trp_gtk_get_widget( cont );
    GtkWidget *oo = trp_gtk_get_widget( obj );

    if ( cc && oo )
        if ( GTK_IS_CONTAINER( cc ) &&
             GTK_IS_WIDGET( oo ) ) {
            trp_gtk_list_append( &(((trp_gtk_t *)cont)->lw), obj, NULL );
            gtk_container_add( (GtkContainer *)cc, oo );
        }
}

void trp_gtk_container_remove( trp_obj_t *cont, trp_obj_t *obj )
{
    GtkWidget *cc = trp_gtk_get_widget( cont );
    if ( cc )
        if ( GTK_IS_CONTAINER( cc ) ) {
            GtkWidget *oo;

            if ( obj )
                oo = trp_gtk_get_widget( obj );
            else {
                /*
                 se obj == NULL, rimuoviamo il primo figlio
                 (ammesso che il contenitore ne abbia almeno uno)
                 */
                GList *l = gtk_container_get_children( (GtkContainer *)cc );
                if ( l == NULL )
                    oo = NULL;
                else {
                    oo = (GtkWidget *)( l->data );
                    g_list_free( l );
                }
            }
            if ( oo ) {
                if ( obj )
                    trp_gtk_list_remove( &(((trp_gtk_t *)cont)->lw), obj, NULL );
                else
                    trp_gtk_list_remove_by_widget( &(((trp_gtk_t *)cont)->lw), oo, NULL );
                gtk_container_remove( (GtkContainer *)cc, oo );
            }
        }
}

void trp_gtk_container_set_border_width( trp_obj_t *cont, trp_obj_t *obj )
{
    GtkWidget *cc = trp_gtk_get_widget( cont );

    if ( cc )
        if ( GTK_IS_CONTAINER( cc ) ) {
            uns32b oo;

            if ( !trp_cast_uns32b( obj, &oo ) )
                gtk_container_set_border_width( (GtkContainer *)cc, (guint)oo );
        }
}

