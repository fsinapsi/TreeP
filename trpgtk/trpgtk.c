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
#undef trp_gtk_width
#undef trp_gtk_height

static uns8b trp_gtk_print( trp_print_t *p, trp_gtk_t *obj );
static uns8b trp_gtk_close( trp_gtk_t *obj );
static trp_obj_t *trp_gtk_length( trp_gtk_t *obj );
static trp_obj_t *trp_gtk_nth( uns32b n, trp_gtk_t *obj );
static trp_obj_t *trp_gtk_width( trp_gtk_t *obj );
static trp_obj_t *trp_gtk_height( trp_gtk_t *obj );

uns8b trp_gtk_init( int *argc, char ***argv )
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_nth_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];
                                    
    if ( gtk_init_check( argc, argv ) == FALSE ) {
        fprintf( stderr, "Initialization of GTK failed\n" );
        return 1;
    }
    _trp_print_fun[ TRP_GTK ] = trp_gtk_print;
    _trp_close_fun[ TRP_GTK ] = trp_gtk_close;
    _trp_length_fun[ TRP_GTK ] = trp_gtk_length;
    _trp_nth_fun[ TRP_GTK ] = trp_gtk_nth;
    _trp_width_fun[ TRP_GTK ] = trp_gtk_width;
    _trp_height_fun[ TRP_GTK ] = trp_gtk_height;
    return 0;
}

static uns8b trp_gtk_print( trp_print_t *p, trp_gtk_t *obj )
{
    if ( trp_print_char_star( p, "#gtk" ) )
        return 1;
    if ( obj->w == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_gtk_close( trp_gtk_t *obj )
{
    if ( obj->w ) {
        if ( GTK_IS_WIDGET( obj->w ) )
            gtk_widget_destroy( (GtkWidget *)( obj->w ) );
        else {
            /*
             FIXME
             trattare eventuali altri casi...
             */
        }
        obj->w = NULL;
    }
    return 0;
}

static trp_obj_t *trp_gtk_length( trp_gtk_t *obj )
{
    GtkWidget *w = obj->w;
    if ( GTK_IS_NOTEBOOK( w ) )
        return trp_sig64( gtk_notebook_get_n_pages( (GtkNotebook *)w ) );
    if ( GTK_IS_ENTRY( w ) )
        return trp_sig64( ((GtkEntry *)w)->text_length );
    if ( GTK_IS_TEXT_VIEW( w ) )
        w = (GtkWidget *)gtk_text_view_get_buffer( (GtkTextView *)w );
    if ( GTK_IS_TEXT_BUFFER( w ) )
        return trp_sig64( gtk_text_buffer_get_char_count( (GtkTextBuffer *)w ) );
    return UNDEF;
}

static trp_obj_t *trp_gtk_nth( uns32b n, trp_gtk_t *obj )
{
    GtkWidget *w = obj->w;
    if ( GTK_IS_NOTEBOOK( w ) ) {
        trp_obj_t *res;

        if ( w = gtk_notebook_get_nth_page( (GtkNotebook *)w, n ) )
            if ( trp_gtk_list_find_by_widget( &(obj->lw), w, NULL, &res ) )
                res = UNDEF;
        return res;
    }
    return UNDEF;
}

static trp_obj_t *trp_gtk_width( trp_gtk_t *obj )
{
    GtkWidget *oo = obj->w;
    gint width, height;
    GtkRequisition requisition;

    if ( !GTK_IS_WIDGET( oo ) )
        return UNDEF;
    if ( oo->window )
        gdk_window_get_size( oo->window, &width, &height );
    else
        width = 0;
    gtk_widget_size_request( oo, &requisition );
    return trp_sig64( width > requisition.width ? width : requisition.width );
}

static trp_obj_t *trp_gtk_height( trp_gtk_t *obj )
{
    GtkWidget *oo = obj->w;
    gint width, height;
    GtkRequisition requisition;

    if ( !GTK_IS_WIDGET( oo ) )
        return UNDEF;
    if ( oo->window )
        gdk_window_get_size( oo->window, &width, &height );
    else
        height = 0;
    gtk_widget_size_request( oo, &requisition );
    return trp_sig64( height > requisition.height ? height : requisition.height );
}

trp_obj_t *trp_gtk_locale_to_utf8( trp_obj_t *obj )
{
    uns8b *p = trp_csprint( obj ), *q;
    trp_obj_t *res;

    q = g_locale_to_utf8( p, -1, NULL, NULL, NULL );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    res = trp_cord( q );
    free( q );
    return res;
}

trp_obj_t *trp_gtk_locale_from_utf8( trp_obj_t *obj )
{
    uns8b *p = trp_csprint( obj ), *q;
    trp_obj_t *res;

    q = g_locale_from_utf8( p, -1, NULL, NULL, NULL );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    res = trp_cord( q );
    free( q );
    return res;
}

trp_obj_t *trp_gtk_filename_to_utf8( trp_obj_t *obj )
{
    uns8b *p = trp_csprint( obj ), *q;
    trp_obj_t *res;

    q = g_filename_to_utf8( p, -1, NULL, NULL, NULL );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    res = trp_cord( q );
    free( q );
    return res;
}

trp_obj_t *trp_gtk_filename_display_name( trp_obj_t *obj )
{
    uns8b *p = trp_csprint( obj ), *q;
    trp_obj_t *res;

    q = g_filename_display_name( p );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    res = trp_cord( q );
    free( q );
    return res;
}

trp_obj_t *trp_gtk_filename_display_basename( trp_obj_t *obj )
{
    uns8b *p = trp_csprint( obj ), *q;
    trp_obj_t *res;

    q = g_filename_display_basename( p );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    res = trp_cord( q );
    free( q );
    return res;
}

trp_obj_t *trp_gtk_get_host_name()
{
    const uns8b *q = g_get_host_name();
    trp_obj_t *res;

    if ( q )
        res = trp_cord( q );
    else
        res = UNDEF;
    return res;
}

trp_obj_t *trp_gtk_get_user_name()
{
    const uns8b *q = g_get_user_name();
    trp_obj_t *res;

    if ( q )
        res = trp_cord( q );
    else
        res = UNDEF;
    return res;
}

trp_obj_t *trp_gtk_get_real_name()
{
    const uns8b *q = g_get_real_name();
    trp_obj_t *res;

    if ( q )
        res = trp_cord( q );
    else
        res = UNDEF;
    return res;
}



