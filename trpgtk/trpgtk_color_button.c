/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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

trp_obj_t *trp_gtk_color_button_new( trp_obj_t *color )
{
    GtkWidget *w;

    if ( color ) {
        uns16b r, g, b, a;
        GdkColor c;

        if ( trp_pix_decode_color( color, &r, &g, &b, &a ) )
            return UNDEF;
        c.pixel = 0;
        c.red = r;
        c.green = g;
        c.blue = b;
        w = gtk_color_button_new_with_color( &c );
    } else
        w = gtk_color_button_new();
    return trp_gtk_widget( w );
}

void trp_gtk_color_button_set_color( trp_obj_t *obj, trp_obj_t *color )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    uns16b r, g, b, a;

    if ( oo && ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) )
        if ( GTK_IS_COLOR_BUTTON( oo ) ) {
            GdkColor c;

            c.pixel = 0;
            c.red = r;
            c.green = g;
            c.blue = b;
            gtk_color_button_set_color( (GtkColorButton *)oo, &c );
            gtk_color_button_set_alpha( (GtkColorButton *)oo, a );
        }
}

trp_obj_t *trp_gtk_color_button_get_color( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_COLOR_BUTTON( oo ) ) {
            GdkColor c;

            gtk_color_button_get_color( (GtkColorButton *)oo, &c );
            res = trp_pix_create_color( c.red, c.green, c.blue, gtk_color_button_get_alpha( (GtkColorButton *)oo ) );
        }
    return res;
}

void trp_gtk_color_button_set_use_alpha( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_COLOR_BUTTON( o ) )
            gtk_color_button_set_use_alpha( (GtkColorButton *)o, BOOLVAL( on_off ) );
}

void trp_gtk_color_button_set_title( trp_obj_t *obj, trp_obj_t *title )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_COLOR_BUTTON( o ) ) {
            uns8b *p = trp_csprint( title );
            gtk_color_button_set_title( (GtkColorButton *)o, p );
            trp_csprint_free( p );
        }
}

