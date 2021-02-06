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
    fprintf( stderr, "#message dialog distrutto\n" );
#endif
}

trp_obj_t *trp_gtk_message_dialog_new( trp_obj_t *parent, trp_obj_t *flags, trp_obj_t *type, trp_obj_t *buttons, trp_obj_t *msg, ...  )
{
    GtkWidget *w, *wp;
    uns8b *p;
    trp_obj_t *res;
    va_list args;

    if ( parent != UNDEF ) {
        wp = trp_gtk_get_widget( parent );
        if ( wp == NULL )
            return UNDEF;
        if ( !GTK_IS_WINDOW( wp ) )
            return UNDEF;
    } else
        wp = NULL;
    if ( ( flags->tipo != TRP_SIG64 ) ||
         ( type->tipo != TRP_SIG64 ) ||
         ( buttons->tipo != TRP_SIG64 ) )
        return UNDEF;
    va_start( args, msg );
    p = trp_csprint_multi( msg, args );
    va_end( args );
    w = gtk_message_dialog_new( (GtkWindow *)wp,
                                ((trp_sig64_t *)flags)->val,
                                ((trp_sig64_t *)type)->val,
                                ((trp_sig64_t *)buttons)->val,
                                "%s", p );
    trp_csprint_free( p );
    res = trp_gtk_widget( w );
#ifdef TRP_GTK_DEBUG
    fprintf(stderr, "#creato message dialog: %p\n", res );
#endif
    trp_gtk_list_append( &_trp_gtk_list, res, &_trp_gtk_mutex );
    g_signal_connect( (gpointer)w, "destroy",
                      (GCallback)trp_gtk_destroy,
                      (gpointer)res );
    return res;
}

