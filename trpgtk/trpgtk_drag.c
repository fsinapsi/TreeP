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

#include "./trpgtk_internal.h"

static GtkTargetEntry _trp_gtk_drag_table[] = {
    { "text/uri-list", 0, 3 },
};

static void trp_gtk_drag_received( GtkWidget *widget, GdkDragContext *dc,
                                   gint x, gint y,
                                   GtkSelectionData *d,
                                   guint info,
                                   guint t,
                                   trp_gtk_signal_t *s );

static void trp_gtk_drag_received( GtkWidget *widget, GdkDragContext *dc,
                                   gint x, gint y,
                                   GtkSelectionData *d,
                                   guint info,
                                   guint t,
                                   trp_gtk_signal_t *s )
{
    if ( info == 3 )
        (void)( s->net->f )( (trp_obj_t *)( s->udata ), trp_cord( d->data ) );
    gtk_drag_finish( dc, TRUE, FALSE, t );
}

void trp_gtk_drag_dest_set( trp_obj_t *obj, trp_obj_t *net, trp_obj_t *udata )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( net->tipo == TRP_NETPTR ) )
        if ( GTK_IS_WIDGET( o ) &&
             ( ((trp_netptr_t *)net)->nargs == 2 ) ) {
            trp_gtk_signal_t *s;

            gtk_drag_dest_set( (GtkWidget *)o, GTK_DEST_DEFAULT_ALL,
                               _trp_gtk_drag_table, 1,
                               (GdkDragAction)(GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_DEFAULT) );
            s = trp_gtk_signal( net, (trp_obj_t **)udata );
            trp_gtk_list_append( &(((trp_gtk_t *)obj)->ls), (trp_obj_t *)s, NULL );
            s->id = g_signal_connect( (gpointer)o, "drag_data_received",
                                      (GCallback)trp_gtk_drag_received, (gpointer)s );
        }
}

void trp_gtk_drag_dest_unset( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_WIDGET( o ) )
            gtk_drag_dest_unset( (GtkWidget *)o );
}

