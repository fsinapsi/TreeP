/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

static uns8b trp_gtk_cback_internal( trp_gtk_signal_t *s );
static gboolean trp_gtk_idle_cback( trp_gtk_signal_t *s );
static gboolean trp_gtk_event_cback( GtkWidget *w, GdkEvent *e, trp_gtk_signal_t *s );
static void trp_gtk_signal_cback( GtkWidget *w, trp_gtk_signal_t *s );

static uns8b trp_gtk_cback_internal( trp_gtk_signal_t *s )
{
    uns8b res;

    switch ( s->net->nargs ) {
    case 0:
        res = (s->net->f)();
        break;
    case 1:
        res = (s->net->f)( s->udata[  0 ] );
        break;
    case 2:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ] );
        break;
    case 3:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ] );
        break;
    case 4:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ] );
        break;
    case 5:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ] );
        break;
    case 6:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ] );
        break;
    case 7:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ] );
        break;
    case 8:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ] );
        break;
    case 9:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ] );
        break;
    case 10:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ] );
        break;
    case 11:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ] );
        break;
    case 12:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ] );
        break;
    case 13:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ] );
        break;
    case 14:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ] );
        break;
    case 15:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ] );
        break;
    case 16:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ], s->udata[ 15 ] );
        break;
    case 17:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ], s->udata[ 15 ],
                           s->udata[ 16 ] );
        break;
    case 18:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ], s->udata[ 15 ],
                           s->udata[ 16 ], s->udata[ 17 ] );
        break;
    case 19:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ], s->udata[ 15 ],
                           s->udata[ 16 ], s->udata[ 17 ], s->udata[ 18 ] );
        break;
    case 20:
        res = (s->net->f)( s->udata[  0 ], s->udata[  1 ], s->udata[  2 ], s->udata[  3 ],
                           s->udata[  4 ], s->udata[  5 ], s->udata[  6 ], s->udata[  7 ],
                           s->udata[  8 ], s->udata[  9 ], s->udata[ 10 ], s->udata[ 11 ],
                           s->udata[ 12 ], s->udata[ 13 ], s->udata[ 14 ], s->udata[ 15 ],
                           s->udata[ 16 ], s->udata[ 17 ], s->udata[ 18 ], s->udata[ 19 ] );
        break;
    }
    return res;
}

static gboolean trp_gtk_idle_cback( trp_gtk_signal_t *s )
{
    return trp_gtk_cback_internal( s ) ? FALSE : TRUE;
}

static gboolean trp_gtk_event_cback( GtkWidget *w, GdkEvent *e, trp_gtk_signal_t *s )
{
    s->udata[ 0 ] = trp_sig64( e->type );
    switch ( e->type ) {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        s->udata[ 1 ] = trp_sig64( ((GdkEventButton *)e)->button );
        s->udata[ 2 ] = trp_math_rint( trp_double( ((GdkEventButton *)e)->x ) );
        s->udata[ 3 ] = trp_math_rint( trp_double( ((GdkEventButton *)e)->y ) );
        break;
    case GDK_KEY_PRESS:
        s->udata[ 1 ] = trp_sig64( ((GdkEventKey *)e)->state );
        s->udata[ 2 ] = trp_sig64( ((GdkEventKey *)e)->keyval );
        break;
    default:
        break;
    }
    return trp_gtk_cback_internal( s ) ? FALSE : TRUE;
}

static void trp_gtk_signal_cback( GtkWidget *w, trp_gtk_signal_t *s )
{
    trp_gtk_cback_internal( s );
}

trp_obj_t *trp_gtk_timeout_add( trp_obj_t *net, trp_obj_t *interval, ... )
{
    trp_obj_t *res = UNDEF;

    if ( net->tipo == TRP_NETPTR )
        if ( ((trp_netptr_t *)net)->nargs <= 20 ) {
            uns32b nargs;
            va_list args;

            va_start( args, interval );
            nargs = trp_nargs( args ) - 1;
            va_end( args );
            if ( ((trp_netptr_t *)net)->nargs == nargs ) {
                uns32b i;

                interval = trp_math_rint( trp_math_times( interval,
                                                          trp_sig64( 1000 ),
                                                          NULL ) );
                if ( !trp_cast_uns32b( interval, &i ) ) {
                    trp_gtk_signal_t *s;
                    trp_obj_t **u;

                    if ( nargs ) {
                        uns32b l;

                        u = trp_gc_malloc( sizeof( trp_obj_t * ) * nargs );
                        va_start( args, interval );
                        for ( l = 0 ; l < nargs ; l++ )
                            u[ l ] = va_arg( args, trp_obj_t * );
                        va_end( args );
                    } else
                        u = NULL;
                    s = trp_gtk_signal( net, u );
                    trp_gtk_list_append( &_trp_gtk_list, (trp_obj_t *)s, &_trp_gtk_mutex );
                    s->id = g_timeout_add( (guint)i, (GSourceFunc)trp_gtk_idle_cback, (gpointer)s );
                    res = trp_sig64( s->id );
                }
            }
        }
    return res;
}

trp_obj_t *trp_gtk_idle_add( trp_obj_t *net, ... )
{
    trp_obj_t *res = UNDEF;

    if ( net->tipo == TRP_NETPTR )
        if ( ((trp_netptr_t *)net)->nargs <= 20 ) {
            uns32b nargs;
            va_list args;

            va_start( args, net );
            nargs = trp_nargs( args ) - 1;
            va_end( args );
            if ( ((trp_netptr_t *)net)->nargs == nargs ) {
                trp_gtk_signal_t *s;
                trp_obj_t **u;

                if ( nargs ) {
                    uns32b l;

                    u = trp_gc_malloc( sizeof( trp_obj_t * ) * nargs );
                    va_start( args, net );
                    for ( l = 0 ; l < nargs ; l++ )
                        u[ l ] = va_arg( args, trp_obj_t * );
                    va_end( args );
                } else
                    u = NULL;
                s = trp_gtk_signal( net, u );
                trp_gtk_list_append( &_trp_gtk_list, (trp_obj_t *)s, &_trp_gtk_mutex );
                s->id = g_idle_add( (GSourceFunc)trp_gtk_idle_cback, (gpointer)s );
                res = trp_sig64( s->id );
            }
        }
    return res;
}

trp_obj_t *trp_gtk_signal_connect( trp_obj_t *obj, trp_obj_t *sig, trp_obj_t *net, ... )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo && ( net->tipo == TRP_NETPTR ) )
        if ( ((trp_netptr_t *)net)->nargs <= 20 )
            if ( GTK_IS_OBJECT( oo ) ) {
                uns8b *p = trp_csprint( sig );
                GCallback cback = NULL;
                uns32b nargs, shft = 0, l;
                va_list args;

                va_start( args, net );
                nargs = trp_nargs( args ) - 1;
                va_end( args );
                l = strlen( p );
                if ( l > 5 )
                    if ( strcmp( p + l - 5, "event" ) == 0 ) {
                        cback = (GCallback)trp_gtk_event_cback;
                        if ( strcmp( p, "button-press-event" ) == 0 )
                            shft = 4;
                        else if ( strcmp( p, "key-press-event" ) == 0 )
                            shft = 3;
                        else
                            shft = 1;
                    }
                if ( cback == NULL )
                    cback = (GCallback)trp_gtk_signal_cback;
                if ( ((trp_netptr_t *)net)->nargs == nargs + shft ) {
                    trp_gtk_signal_t *s;
                    trp_obj_t **u;

                    if ( nargs + shft ) {
                        u = trp_gc_malloc( sizeof( trp_obj_t * ) * ( nargs + shft ) );
                        va_start( args, net );
                        for ( l = 0 ; l < nargs ; l++ )
                            u[ l + shft ] = va_arg( args, trp_obj_t * );
                        va_end( args );
                    } else
                        u = NULL;
                    s = trp_gtk_signal( net, u );
                    trp_gtk_list_append( &(((trp_gtk_t *)obj)->ls), (trp_obj_t *)s, NULL );
                    s->id = g_signal_connect( (gpointer)oo, p, cback, (gpointer)s );
                    res = trp_sig64( s->id );
                }
                trp_csprint_free( p );
            }
    return res;
}

void trp_gtk_signal_handler_disconnect( trp_obj_t *obj, trp_obj_t *id )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo && ( id->tipo == TRP_SIG64 ) )
        if ( GTK_IS_OBJECT( oo ) ) {
            trp_obj_t *x, *prev = NULL;
            gulong sid = ((trp_sig64_t *)id)->val;

            for ( x = ((trp_gtk_t *)obj)->ls ; x ; x = ((trp_cons_t *)x)->cdr ) {
                if ( ((trp_gtk_signal_t *)(((trp_cons_t *)x)->car))->id == sid ) {
                    if ( prev )
                        ((trp_cons_t *)prev)->cdr = ((trp_cons_t *)x)->cdr;
                    else
                        ((trp_gtk_t *)obj)->ls = ((trp_cons_t *)x)->cdr;
                    trp_gc_free( x );
                    break;
                }
                prev = x;
            }
            g_signal_handler_disconnect( (gpointer)oo, sid );
        }
}

void trp_gtk_signal_emit_by_name( trp_obj_t *obj, trp_obj_t *name )
/*
 a g_signal_emit_by_name vanno passati altri parametri...
 per il momento non va usata...
 */
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo  ) {
        uns8b *cname = trp_csprint( name );
        g_signal_emit_by_name( (gpointer)oo, cname );
        trp_csprint_free( cname );
    }
}

