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

static pthread_mutex_t _trp_gtk_mutex = PTHREAD_MUTEX_INITIALIZER;
static trp_obj_t *_trp_gtk_list = NULL;

static void trp_gtk_destroy( GtkWidget *w, trp_obj_t *udata );

static void trp_gtk_destroy( GtkWidget *w, trp_obj_t *udata )
{
    trp_gtk_list_remove( &_trp_gtk_list, udata, &_trp_gtk_mutex );
#ifdef TRP_GTK_DEBUG
    fprintf( stderr, "#file chooser distrutto\n" );
#endif
}

trp_obj_t *trp_gtk_file_chooser_dialog_new( trp_obj_t *title, trp_obj_t *action, trp_obj_t *parent )
{
    GtkWidget *w;
    uns8b *p;
    trp_obj_t *res;

    if ( action->tipo != TRP_SIG64 )
        return UNDEF;
    if ( parent ) {
        w = trp_gtk_get_widget( parent );
        if ( w == NULL )
            return UNDEF;
        if ( !GTK_IS_WINDOW( w ) )
            return UNDEF;
    } else
        w = NULL;
    p = trp_csprint( title );
    w = gtk_file_chooser_dialog_new( p, (GtkWindow *)w,
                                     (GtkFileChooserAction)( ((trp_sig64_t *)action)->val ),
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                     NULL );
    trp_csprint_free( p );
    res = trp_gtk_widget( w );
#ifdef TRP_GTK_DEBUG
    fprintf(stderr, "#creato file chooser dialog: %p\n", res );
#endif
    trp_gtk_list_append( &_trp_gtk_list, res, &_trp_gtk_mutex );
    g_signal_connect( (gpointer)w, "destroy",
                      (GCallback)trp_gtk_destroy,
                      (gpointer)res );
    return res;
}

