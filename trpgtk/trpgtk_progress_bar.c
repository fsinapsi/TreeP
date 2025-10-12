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

trp_obj_t *trp_gtk_progress_bar_new()
{
    return trp_gtk_widget( gtk_progress_bar_new() );
}

void trp_gtk_progress_bar_pulse( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo )
        if ( GTK_IS_PROGRESS_BAR( oo ) )
            gtk_progress_bar_pulse( (GtkProgressBar *)oo );
}

void trp_gtk_progress_bar_set_text( trp_obj_t *obj, trp_obj_t *msg, ... )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo )
        if ( GTK_IS_PROGRESS_BAR( oo ) ) {
            uns8b *p ;
            va_list args;

            va_start( args, msg );
            p = trp_csprint_multi( msg, args );
            va_end( args );
            gtk_progress_bar_set_text( (GtkProgressBar *)oo, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_progress_bar_set_fraction( trp_obj_t *obj, trp_obj_t *percent )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    double p;

    if ( oo && ( !trp_cast_flt64b( percent, &p ) ) )
        if ( GTK_IS_PROGRESS_BAR( oo ) )
            gtk_progress_bar_set_fraction( (GtkProgressBar *)oo, p );
}

void trp_gtk_progress_bar_set_pulse_step( trp_obj_t *obj, trp_obj_t *percent )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    double p;

    if ( oo && ( !trp_cast_flt64b( percent, &p ) ) )
        if ( GTK_IS_PROGRESS_BAR( oo ) )
            gtk_progress_bar_set_pulse_step( (GtkProgressBar *)oo, p );
}

