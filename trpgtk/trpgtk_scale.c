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

trp_obj_t *trp_gtk_hscale_new( trp_obj_t *adj )
{
    GtkWidget *o = trp_gtk_get_widget( adj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_ADJUSTMENT( o ) ) {
            res = trp_gtk_widget( gtk_hscale_new( (GtkAdjustment *)o ) );
            trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), adj, NULL );
        }
    return res;
}

trp_obj_t *trp_gtk_vscale_new( trp_obj_t *adj )
{
    GtkWidget *o = trp_gtk_get_widget( adj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_ADJUSTMENT( o ) ) {
            res = trp_gtk_widget( gtk_vscale_new( (GtkAdjustment *)o ) );
            trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), adj, NULL );
        }
    return res;
}

trp_obj_t *trp_gtk_hscale_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step )
{
    double dmin, dmax, dstep;

    if ( trp_cast_double( min, &dmin ) ||
         trp_cast_double( max, &dmax ) ||
         trp_cast_double( step, &dstep ) )
        return UNDEF;
    if ( dmin > dmax )
        return UNDEF;
    return trp_gtk_widget( gtk_hscale_new_with_range( dmin, dmax, dstep ) );
}

trp_obj_t *trp_gtk_vscale_new_with_range( trp_obj_t *min, trp_obj_t *max, trp_obj_t *step )
{
    double dmin, dmax, dstep;

    if ( trp_cast_double( min, &dmin ) ||
         trp_cast_double( max, &dmax ) ||
         trp_cast_double( step, &dstep ) )
        return UNDEF;
    if ( dmin > dmax )
        return UNDEF;
    return trp_gtk_widget( gtk_vscale_new_with_range( dmin, dmax, dstep ) );
}

void trp_gtk_scale_set_draw_value( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_SCALE( o ) )
            gtk_scale_set_draw_value( (GtkScale *)o, BOOLVAL( on_off ) );
}

