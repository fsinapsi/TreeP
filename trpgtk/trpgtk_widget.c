/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

static void trp_gtk_widget_generic( trp_obj_t *obj, voidfun_t f );

static void trp_gtk_widget_generic( trp_obj_t *obj, voidfun_t f )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_WIDGET( o ) )
            (f)( o );
}

void trp_gtk_widget_show( trp_obj_t *obj )
{
    trp_gtk_widget_generic( obj, gtk_widget_show );
}

void trp_gtk_widget_hide( trp_obj_t *obj )
{
    trp_gtk_widget_generic( obj, gtk_widget_hide );
}

void trp_gtk_widget_show_all( trp_obj_t *obj )
{
    trp_gtk_widget_generic( obj, gtk_widget_show_all );
}

void trp_gtk_widget_realize( trp_obj_t *obj )
{
    trp_gtk_widget_generic( obj, gtk_widget_realize );
}

void trp_gtk_widget_grab_focus( trp_obj_t *obj )
{
    trp_gtk_widget_generic( obj, gtk_widget_grab_focus );
}

void trp_gtk_widget_set_sensitive( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_WIDGET( o ) )
            gtk_widget_set_sensitive( (GtkWidget *)o, BOOLVAL( on_off ) );
}

void trp_gtk_widget_set_size_request( trp_obj_t *obj, trp_obj_t *width, trp_obj_t *height )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( width->tipo == TRP_SIG64 ) && ( height->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WIDGET( o ) )
            gtk_widget_set_size_request( (GtkWidget *)o,
                                         (gint)( ((trp_sig64_t *)width)->val ),
                                         (gint)( ((trp_sig64_t *)height)->val ) );
}

void trp_gtk_widget_modify_font( trp_obj_t *obj, trp_obj_t *font )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_WIDGET( o ) )
            if ( font ) {
                uns8b *p = trp_csprint( font );
                PangoFontDescription *desc;

                if ( desc = pango_font_description_from_string( p ) ) {
                    gtk_widget_modify_font( (GtkWidget *)o, desc );
                    pango_font_description_free( desc );
                }
                trp_csprint_free( p );
            } else
                gtk_widget_modify_font( (GtkWidget *)o, NULL );
}

void trp_gtk_widget_modify_fg( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( state->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WIDGET( o ) )
            if ( color ) {
                uns16b r, g, b, a;

                if ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) {
                    GdkColor c;

                    c.pixel = 0;
                    c.red = r;
                    c.green = g;
                    c.blue = b;
                    gtk_widget_modify_fg( (GtkWidget *)o, ((trp_sig64_t *)state)->val, &c );
                }
            } else
                gtk_widget_modify_fg( (GtkWidget *)o, ((trp_sig64_t *)state)->val, NULL );
}

void trp_gtk_widget_modify_bg( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( state->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WIDGET( o ) )
            if ( color ) {
                uns16b r, g, b, a;

                if ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) {
                    GdkColor c;

                    c.pixel = 0;
                    c.red = r;
                    c.green = g;
                    c.blue = b;
                    gtk_widget_modify_bg( (GtkWidget *)o, ((trp_sig64_t *)state)->val, &c );
                }
            } else
                gtk_widget_modify_bg( (GtkWidget *)o, ((trp_sig64_t *)state)->val, NULL );
}

void trp_gtk_widget_modify_text( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( state->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WIDGET( o ) )
            if ( color ) {
                uns16b r, g, b, a;

                if ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) {
                    GdkColor c;

                    c.pixel = 0;
                    c.red = r;
                    c.green = g;
                    c.blue = b;
                    gtk_widget_modify_text( (GtkWidget *)o, ((trp_sig64_t *)state)->val, &c );
                }
            } else
                gtk_widget_modify_text( (GtkWidget *)o, ((trp_sig64_t *)state)->val, NULL );
}

void trp_gtk_widget_modify_base( trp_obj_t *obj, trp_obj_t *state, trp_obj_t *color )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( state->tipo == TRP_SIG64 ) )
        if ( GTK_IS_WIDGET( o ) )
            if ( color ) {
                uns16b r, g, b, a;

                if ( !trp_pix_decode_color( color, &r, &g, &b, &a ) ) {
                    GdkColor c;

                    c.pixel = 0;
                    c.red = r;
                    c.green = g;
                    c.blue = b;
                    gtk_widget_modify_base( (GtkWidget *)o, ((trp_sig64_t *)state)->val, &c );
                }
            } else
                gtk_widget_modify_base( (GtkWidget *)o, ((trp_sig64_t *)state)->val, NULL );
}

void trp_gtk_widget_set_tooltip_text( trp_obj_t *obj, trp_obj_t *text )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_WIDGET( o ) ) {
            uns8b *p = trp_csprint( text );
            gtk_widget_set_tooltip_text( o, p );
            trp_csprint_free( p );
        }
}

