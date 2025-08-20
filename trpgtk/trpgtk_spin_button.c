/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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

trp_obj_t *trp_gtk_spin_button_new( trp_obj_t *adj, trp_obj_t *climb_rate, trp_obj_t *digits )
{
    GtkWidget *oo = trp_gtk_get_widget( adj );
    trp_obj_t *res = UNDEF;
    double cr;
    uns32b d;

    if ( oo &&
         ( !trp_cast_double( climb_rate, &cr ) ) &&
         ( !trp_cast_uns32b( digits, &d ) ) )
        if ( GTK_IS_ADJUSTMENT( oo ) ) {
            res = trp_gtk_widget( gtk_spin_button_new( (GtkAdjustment *)oo, cr, d ) );
            trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), adj, NULL );
        }
    return res;
}

trp_obj_t *trp_gtk_spin_button_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step )
{
    double dmin, dmax, dstep;

    if ( trp_cast_double( min, &dmin ) ||
         trp_cast_double( max, &dmax ) ||
         trp_cast_double( step, &dstep ) )
        return UNDEF;
    if ( dmin > dmax )
        return UNDEF;
    return trp_gtk_widget( gtk_spin_button_new_with_range( dmin, dmax, dstep ) );
}

void trp_gtk_spin_button_set_value( trp_obj_t *obj, trp_obj_t *val )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    double v;

    if ( oo && ( !trp_cast_double( val, &v ) ) )
        if ( GTK_IS_SPIN_BUTTON( oo ) )
            gtk_spin_button_set_value( (GtkSpinButton *)oo, v );
}

trp_obj_t *trp_gtk_spin_button_get_value( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_SPIN_BUTTON( oo ) ) {
            uns32b digits = gtk_spin_button_get_digits( (GtkSpinButton *)oo );
            for ( res = UNO ; digits ; digits-- )
                res = trp_math_times( res, DIECI, NULL );
            res = trp_math_ratio( trp_math_rint( trp_math_times( trp_double( gtk_spin_button_get_value( (GtkSpinButton *)oo ) ),
                                                                 res,
                                                                 NULL ) ),
                                  res,
                                  NULL );
        }
    return res;
}

void trp_gtk_spin_button_set_numeric( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo && TRP_BOOLP( on_off ) )
        if ( GTK_IS_SPIN_BUTTON( oo ) )
            gtk_spin_button_set_numeric( (GtkSpinButton *)oo, BOOLVAL( on_off ) );
}

void trp_gtk_spin_button_set_wrap( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo && TRP_BOOLP( on_off ) )
        if ( GTK_IS_SPIN_BUTTON( oo ) )
            gtk_spin_button_set_wrap( (GtkSpinButton *)oo, BOOLVAL( on_off ) );
}

void trp_gtk_spin_button_set_range( trp_obj_t *obj, trp_obj_t *min, trp_obj_t *max )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    double dmin, dmax;

    if ( oo && ( !trp_cast_double( min, &dmin ) ) && ( !trp_cast_double( max, &dmax ) ) )
        if ( GTK_IS_SPIN_BUTTON( oo ) )
            gtk_spin_button_set_range( (GtkSpinButton *)oo, dmin, dmax );
}

void trp_gtk_spin_button_update( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo )
        if ( GTK_IS_SPIN_BUTTON( oo ) )
            gtk_spin_button_update( (GtkSpinButton *)oo );
}

