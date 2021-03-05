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

static pthread_mutex_t _trp_gtk_mutex = PTHREAD_MUTEX_INITIALIZER;
static trp_obj_t *_trp_gtk_list = NULL;

static void trp_gtk_destroy( GtkWidget *w, trp_obj_t *udata );

static void trp_gtk_destroy( GtkWidget *w, trp_obj_t *udata )
{
    trp_gtk_list_remove( &_trp_gtk_list, udata, &_trp_gtk_mutex );
#ifdef TRP_GTK_DEBUG
    fprintf( stderr, "#color selection distrutto\n" );
#endif
}

trp_obj_t *trp_gtk_color_selection_dialog_new( trp_obj_t *title, trp_obj_t *parent )
{
    GtkWidget *w, *wp;
    uns8b *p;
    trp_obj_t *res;

    if ( parent ) {
        wp = trp_gtk_get_widget( parent );
        if ( wp == NULL )
            return UNDEF;
        if ( !GTK_IS_WINDOW( wp ) )
            return UNDEF;
    } else
        wp = NULL;
    p = trp_csprint( title );
    w = gtk_color_selection_dialog_new( p );
    trp_csprint_free( p );
    if ( wp )
        gtk_window_set_transient_for( (GtkWindow *)w, (GtkWindow *)wp );
    res = trp_gtk_widget( w );
#ifdef TRP_GTK_DEBUG
    fprintf(stderr, "#creato color selection dialog: %p\n", res );
#endif
    trp_gtk_list_append( &_trp_gtk_list, res, &_trp_gtk_mutex );
    g_signal_connect( (gpointer)w, "destroy",
                      (GCallback)trp_gtk_destroy,
                      (gpointer)res );
    return res;
}

