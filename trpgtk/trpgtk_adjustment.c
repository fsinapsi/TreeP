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

trp_obj_t *trp_gtk_adjustment_new( trp_obj_t *value, trp_obj_t *lower, trp_obj_t *upper, trp_obj_t *step_incr, trp_obj_t *page_incr, trp_obj_t *page_size )
{
    double dvalue, dlower, dupper, dstep_incr, dpage_incr, dpage_size;

    if ( trp_cast_double( value, &dvalue ) ||
         trp_cast_double( lower, &dlower ) ||
         trp_cast_double( upper, &dupper ) ||
         trp_cast_double( step_incr, &dstep_incr ) ||
         trp_cast_double( page_incr, &dpage_incr ) ||
         trp_cast_double( page_size, &dpage_size ) )
        return UNDEF;
    return trp_gtk_widget( gtk_adjustment_new( dvalue, dlower, dupper, dstep_incr, dpage_incr, dpage_size ) );
}

trp_obj_t *trp_gtk_adjustment_get_value( trp_obj_t *obj )
{
    GtkWidget *w = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( w ) {
        if ( GTK_IS_SCROLLED_WINDOW( w ) )
            w = (GtkWidget *)gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)w );
        if ( GTK_IS_ADJUSTMENT( w ) )
            res = trp_double( gtk_adjustment_get_value( (GtkAdjustment *)w ) );
    }
    return res;
}

void trp_gtk_adjustment_set_value( trp_obj_t *obj, trp_obj_t *value )
{
    GtkWidget *w = trp_gtk_get_widget( obj );
    double dvalue;

    if ( w && ( !trp_cast_double( value, &dvalue ) ) ) {
        if ( GTK_IS_SCROLLED_WINDOW( w ) )
            w = (GtkWidget *)gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)w );
        if ( GTK_IS_ADJUSTMENT( w ) )
            gtk_adjustment_set_value( (GtkAdjustment *)w, dvalue );
    }
}

trp_obj_t *trp_gtk_adjustment_get_lower( trp_obj_t *obj )
{
    GtkWidget *w = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( w )
        if ( GTK_IS_SCROLLED_WINDOW( w ) )
            w = (GtkWidget *)gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)w );
        if ( GTK_IS_ADJUSTMENT( w ) )
            res = trp_double( gtk_adjustment_get_lower( (GtkAdjustment *)w ) );
    return res;
}

trp_obj_t *trp_gtk_adjustment_get_upper( trp_obj_t *obj )
{
    GtkWidget *w = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( w )
        if ( GTK_IS_SCROLLED_WINDOW( w ) )
            w = (GtkWidget *)gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)w );
        if ( GTK_IS_ADJUSTMENT( w ) )
            res = trp_double( gtk_adjustment_get_upper( (GtkAdjustment *)w ) );
    return res;
}

