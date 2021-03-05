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
#include "../trppix/trppix_internal.h"

void trp_gtk_color_selection_set_current_color( trp_obj_t *obj, trp_obj_t *color )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    uns16b r, g, b, a;

    if ( oo && ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) ) {
        if ( GTK_IS_COLOR_SELECTION_DIALOG( oo ) )
            oo = ((GtkColorSelectionDialog *)oo)->colorsel;
        if ( GTK_IS_COLOR_SELECTION( oo ) ) {
            GdkColor c;

            c.pixel = 0;
            c.red = r;
            c.green = g;
            c.blue = b;
            gtk_color_selection_set_current_color( (GtkColorSelection *)oo, &c );
            gtk_color_selection_set_current_alpha( (GtkColorSelection *)oo, a );
        }
    }
}

trp_obj_t *trp_gtk_color_selection_get_current_color( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo ) {
        if ( GTK_IS_COLOR_SELECTION_DIALOG( oo ) )
            oo = ((GtkColorSelectionDialog *)oo)->colorsel;
        if ( GTK_IS_COLOR_SELECTION( oo ) ) {
            GdkColor c;

            gtk_color_selection_get_current_color( (GtkColorSelection *)oo, &c );
            res = trp_pix_create_color( c.red, c.green, c.blue, gtk_color_selection_get_current_alpha( (GtkColorSelection *)oo ) );
        }
    }
    return res;
}

void trp_gtk_color_selection_set_has_opacity_control( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo && TRP_BOOLP( on_off ) ) {
        if ( GTK_IS_COLOR_SELECTION_DIALOG( oo ) )
            oo = ((GtkColorSelectionDialog *)oo)->colorsel;
        if ( GTK_IS_COLOR_SELECTION( oo ) )
            gtk_color_selection_set_has_opacity_control( (GtkColorSelection *)oo, BOOLVAL( on_off ) );
    }
}

