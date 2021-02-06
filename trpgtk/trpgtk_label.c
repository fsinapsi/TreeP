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

trp_obj_t *trp_gtk_label_new( trp_obj_t *obj, trp_obj_t *pattern )
{
    uns8b *p = trp_csprint( obj );
    GtkWidget *w;

    w = gtk_label_new( p );
    trp_csprint_free( p );
    if ( pattern ) {
        p = trp_csprint( pattern );
        gtk_label_set_pattern( (GtkLabel *)w, p );
        trp_csprint_free( p );
    }
    return trp_gtk_widget( w );
}

trp_obj_t *trp_gtk_label_get_text( trp_obj_t *label )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    trp_obj_t *res = UNDEF;
    if ( l )
        if ( GTK_IS_LABEL( l ) )
            res = trp_cord( gtk_label_get_text( (GtkLabel *)l ) );
    return res;
}

void trp_gtk_label_set_text( trp_obj_t *label, trp_obj_t *text, ... )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    if ( l )
        if ( GTK_IS_LABEL( l ) ) {
            uns8b *p;
            va_list args;

            va_start( args, text );
            p = trp_csprint_multi( text, args );
            va_end( args );
            gtk_label_set_text( (GtkLabel *)l, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_label_set_justify( trp_obj_t *label, trp_obj_t *j )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    if ( l && ( j->tipo == TRP_SIG64 ) )
        if ( GTK_IS_LABEL( l ) )
            gtk_label_set_justify( (GtkLabel *)l, (GtkJustification)( ((trp_sig64_t *)j)->val ) );
}

void trp_gtk_label_set_selectable( trp_obj_t *label, trp_obj_t *on_off )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    if ( l && TRP_BOOLP( on_off ) )
        if ( GTK_IS_LABEL( l ) )
            gtk_label_set_selectable( (GtkLabel *)l, BOOLVAL( on_off ) );
}

void trp_gtk_label_set_ellipsize( trp_obj_t *label, trp_obj_t *mode )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    if ( l && ( mode->tipo == TRP_SIG64 ) )
        if ( GTK_IS_LABEL( l ) )
            gtk_label_set_ellipsize( (GtkLabel *)l, (PangoEllipsizeMode)( ((trp_sig64_t *)mode)->val ) );
}

void trp_gtk_label_set_pattern( trp_obj_t *label, trp_obj_t *pattern )
{
    GtkWidget *l = trp_gtk_get_widget( label );
    if ( l )
        if ( GTK_IS_LABEL( l ) ) {
            uns8b *p = trp_csprint( pattern );
            gtk_label_set_pattern( (GtkLabel *)l, p );
            trp_csprint_free( p );
        }
}

