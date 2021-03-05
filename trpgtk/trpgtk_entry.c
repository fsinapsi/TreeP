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

trp_obj_t *trp_gtk_entry_new( trp_obj_t *str )
{
    GtkWidget *e = gtk_entry_new();

    if ( str ) {
        uns8b *p = trp_csprint( str );
        gtk_entry_set_text( (GtkEntry *)e, p );
        trp_csprint_free( p );
    }
    return trp_gtk_widget( e );
}

trp_obj_t *trp_gtk_entry_get_text( trp_obj_t *obj )
{
    GtkWidget *e = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( e )
        if ( GTK_IS_ENTRY( e ) )
            res = trp_cord( gtk_entry_get_text( (GtkEntry *)e ) );
    return res;
}

void trp_gtk_entry_set_text( trp_obj_t *obj, trp_obj_t *str )
{
    GtkWidget *e = trp_gtk_get_widget( obj );
    if ( e )
        if ( GTK_IS_ENTRY( e ) ) {
            uns8b *p = trp_csprint( str );
            gtk_entry_set_text( (GtkEntry *)e, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_entry_set_max_length( trp_obj_t *obj, trp_obj_t *n )
{
    GtkWidget *e = trp_gtk_get_widget( obj );
    if ( e && ( n->tipo == TRP_SIG64 ) )
        if ( GTK_IS_ENTRY( e ) )
            gtk_entry_set_max_length( (GtkEntry *)e, ((trp_sig64_t *)n)->val );
}

void trp_gtk_entry_set_visibility( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *e = trp_gtk_get_widget( obj );
    if ( e && TRP_BOOLP( on_off ) )
        if ( GTK_IS_ENTRY( e ) )
            gtk_entry_set_visibility( (GtkEntry *)e, BOOLVAL( on_off ) );
}

