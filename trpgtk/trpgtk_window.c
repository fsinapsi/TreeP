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
    fprintf( stderr, "#finestra distrutta\n" );
#endif
}

trp_obj_t *trp_gtk_window_new( trp_obj_t *wtype )
{
    GtkWidget *w;
    trp_obj_t *res;
    GtkWindowType wt;

    if ( wtype ) {
        if ( wtype->tipo != TRP_SIG64 )
            return UNDEF;
        wt = ((trp_sig64_t *)wtype)->val;
    } else
        wt = GTK_WINDOW_TOPLEVEL;
    w = gtk_window_new( wt );
    res = trp_gtk_widget( w );
#ifdef TRP_GTK_DEBUG
    fprintf(stderr, "#creata finestra: %p\n", res );
#endif
    trp_gtk_list_append( &_trp_gtk_list, res, &_trp_gtk_mutex );
    g_signal_connect( (gpointer)w, "destroy",
                      (GCallback)trp_gtk_destroy,
                      (gpointer)res );
    return res;
}

void trp_gtk_window_set_title( trp_obj_t *w, trp_obj_t *obj, ... )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win )
        if ( GTK_IS_WINDOW( win ) ) {
            uns8b *p ;
            va_list args;

            va_start( args, obj );
            p = trp_csprint_multi( obj, args );
            va_end( args );
            gtk_window_set_title( (GtkWindow *)win, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_window_set_transient_for( trp_obj_t *w, trp_obj_t *parent )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    GtkWidget *winp = trp_gtk_get_widget( parent );
    if ( win && winp )
        if ( GTK_IS_WINDOW( win ) && GTK_IS_WINDOW( winp ) )
            gtk_window_set_transient_for( (GtkWindow *)win, (GtkWindow *)winp );
}

void trp_gtk_window_set_modal( trp_obj_t *w, trp_obj_t *obj )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win && TRP_BOOLP( obj ) )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_set_modal( (GtkWindow *)win, BOOLVAL( obj ) );
}

void trp_gtk_window_set_resizable( trp_obj_t *w, trp_obj_t *obj )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win && TRP_BOOLP( obj ) )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_set_resizable( (GtkWindow *)win, BOOLVAL( obj ) );
}

void trp_gtk_window_set_default_size( trp_obj_t *w, trp_obj_t *width, trp_obj_t *height )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win && ( width->tipo == TRP_SIG64 ) && ( height->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_set_default_size( (GtkWindow *)win,
                                         (gint)( ((trp_sig64_t *)width)->val ),
                                         (gint)( ((trp_sig64_t *)height)->val ) );
}

void trp_gtk_window_set_position( trp_obj_t *w, trp_obj_t *pos )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win && ( pos->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_set_position( (GtkWindow *)win, (GtkWindowPosition)( ((trp_sig64_t *)pos)->val ) );
}

void trp_gtk_window_maximize( trp_obj_t *w )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_maximize( (GtkWindow *)win );
}

void trp_gtk_window_unmaximize( trp_obj_t *w )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_unmaximize( (GtkWindow *)win );
}

void trp_gtk_window_fullscreen( trp_obj_t *w )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_fullscreen( (GtkWindow *)win );
}

void trp_gtk_window_unfullscreen( trp_obj_t *w )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_unfullscreen( (GtkWindow *)win );
}

trp_obj_t *trp_gtk_window_get_position( trp_obj_t *w )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    trp_obj_t *res = UNDEF;

    if ( win )
        if ( GTK_IS_WINDOW( win ) ) {
            gint root_x, root_y;

            gtk_window_get_position( (GtkWindow *)win, &root_x, &root_y );
            res = trp_cons( trp_sig64( root_x ), trp_sig64( root_y ) );
        }
    return res;
}

void trp_gtk_window_move( trp_obj_t *w, trp_obj_t *x, trp_obj_t *y )
{
    GtkWidget *win = trp_gtk_get_widget( w );
    if ( win && ( x->tipo == TRP_SIG64 ) && ( y->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WINDOW( win ) )
            gtk_window_move( (GtkWindow *)win, ((trp_sig64_t *)x)->val, ((trp_sig64_t *)y)->val );
}

