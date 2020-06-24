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

#ifdef TRP_GTK_DEBUG
static void trp_gtk_finalize( void *obj, void *data );
static void trp_gtk_finalize_signal( void *obj, void *data );

static void trp_gtk_finalize( void *obj, void *data )
{
    fprintf( stderr, "#finalizzato (widget) %p\n", obj );
}

static void trp_gtk_finalize_signal( void *obj, void *data )
{
    fprintf( stderr, "#finalizzato (signal) %p\n", obj );
}
#endif

trp_obj_t *trp_gtk_widget( void *w )
{
#ifdef TRP_GTK_DEBUG
    trp_gtk_t *res = trp_gc_malloc_finalize( sizeof( trp_gtk_t ), trp_gtk_finalize );
#else
    trp_gtk_t *res = trp_gc_malloc( sizeof( trp_gtk_t ) );
#endif
    res->tipo = TRP_GTK;
    res->w = w;
    res->lw = NULL;
    res->ls = NULL;
    res->id = 0;
    return (trp_obj_t *)res;
}

GtkWidget *trp_gtk_get_widget( trp_obj_t *obj )
{
    if ( obj == NULL )
        return NULL;
    if ( obj->tipo != TRP_GTK )
        return NULL;
    return (GtkWidget *)( ((trp_gtk_t *)obj)->w );
}

trp_gtk_signal_t *trp_gtk_signal( trp_obj_t *net, trp_obj_t **udata )
{
#ifdef TRP_GTK_DEBUG
    trp_gtk_signal_t *s = trp_gc_malloc_finalize( sizeof( trp_gtk_signal_t ), trp_gtk_finalize_signal );
#else
    trp_gtk_signal_t *s = trp_gc_malloc( sizeof( trp_gtk_signal_t ) );
#endif
    s->net = (trp_netptr_t *)net;
    s->udata = udata;
    return s;
}

void trp_gtk_list_append( trp_obj_t **list, trp_obj_t *obj, pthread_mutex_t *mtx )
{
    if ( mtx )
        pthread_mutex_lock( mtx );
    *list = trp_cons( obj, *list );
    if ( mtx )
        pthread_mutex_unlock( mtx );
}

void trp_gtk_list_remove( trp_obj_t **list, trp_obj_t *obj, pthread_mutex_t *mtx )
{
    trp_obj_t *x, *prev = NULL;

    if ( mtx )
        pthread_mutex_lock( mtx );
    for ( x = *list ; x ; x = ((trp_cons_t *)x)->cdr ) {
        if ( ((trp_cons_t *)x)->car == obj ) {
            if ( prev )
                ((trp_cons_t *)prev)->cdr = ((trp_cons_t *)x)->cdr;
            else
                *list = ((trp_cons_t *)x)->cdr;
            trp_gc_free( x );
            break;
        }
        prev = x;
    }
    if ( mtx )
        pthread_mutex_unlock( mtx );
}

void trp_gtk_list_remove_by_widget( trp_obj_t **list, GtkWidget *w, pthread_mutex_t *mtx )
{
    trp_obj_t *x, *prev = NULL;

    if ( mtx )
        pthread_mutex_lock( mtx );
    for ( x = *list ; x ; x = ((trp_cons_t *)x)->cdr ) {
        if ( ((trp_gtk_t *)((trp_cons_t *)x)->car)->w == w ) {
            if ( prev )
                ((trp_cons_t *)prev)->cdr = ((trp_cons_t *)x)->cdr;
            else
                *list = ((trp_cons_t *)x)->cdr;
            trp_gc_free( x );
            break;
        }
        prev = x;
    }
    if ( mtx )
        pthread_mutex_unlock( mtx );
}

uns8b trp_gtk_list_find_by_widget( trp_obj_t **list, GtkWidget *w, pthread_mutex_t *mtx, trp_obj_t **obj )
{
    trp_obj_t *x;
    uns8b res = 1;

    if ( mtx )
        pthread_mutex_lock( mtx );
    for ( x = *list ; x ; x = ((trp_cons_t *)x)->cdr ) {
        if ( ((trp_gtk_t *)((trp_cons_t *)x)->car)->w == w ) {
            *obj = ((trp_cons_t *)x)->car;
            res = 0;
            break;
        }
    }
    if ( mtx )
        pthread_mutex_unlock( mtx );
    return res;
}



