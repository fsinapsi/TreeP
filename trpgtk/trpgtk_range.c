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

trp_obj_t *trp_gtk_range_get_value( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_RANGE( o ) )
            res = trp_double( gtk_range_get_value( (GtkRange *)o ) );
    return res;
}

void trp_gtk_range_set_value( trp_obj_t *obj, trp_obj_t *val )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    double dval;

    if ( o && ( trp_cast_double( val, &dval ) == 0 ) )
        if ( GTK_IS_RANGE( o ) )
            gtk_range_set_value( (GtkRange *)o, dval );
}

trp_obj_t *trp_gtk_range_get_lower_stepper_sensitivity( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_RANGE( o ) )
            res = trp_sig64( gtk_range_get_lower_stepper_sensitivity( (GtkRange *)o ) );
    return res;
}

trp_obj_t *trp_gtk_range_get_upper_stepper_sensitivity( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_RANGE( o ) )
            res = trp_sig64( gtk_range_get_upper_stepper_sensitivity( (GtkRange *)o ) );
    return res;
}

