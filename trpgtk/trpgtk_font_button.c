/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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

trp_obj_t *trp_gtk_font_button_new( trp_obj_t *font )
{
    GtkWidget *w;

    if ( font ) {
        uns8b *p = trp_csprint( font );
        w = gtk_font_button_new_with_font( p );
        trp_csprint_free( p );
    } else
        w = gtk_font_button_new();
    return trp_gtk_widget( w );
}

void trp_gtk_font_button_set_font_name( trp_obj_t *obj, trp_obj_t *font )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo )
        if ( GTK_IS_FONT_BUTTON( oo ) ) {
            uns8b *p = trp_csprint( font );
            gtk_font_button_set_font_name( (GtkFontButton *)oo, p );
            trp_csprint_free( p );
        }
}

trp_obj_t *trp_gtk_font_button_get_font_name( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_FONT_BUTTON( oo ) )
            res = trp_cord( gtk_font_button_get_font_name( (GtkFontButton *)oo ) );
    return res;
}

void trp_gtk_font_button_set_show_style( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FONT_BUTTON( o ) )
            gtk_font_button_set_show_style( (GtkFontButton *)o, BOOLVAL( on_off ) );
}

void trp_gtk_font_button_set_show_size( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FONT_BUTTON( o ) )
            gtk_font_button_set_show_size( (GtkFontButton *)o, BOOLVAL( on_off ) );
}

void trp_gtk_font_button_set_use_font( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FONT_BUTTON( o ) )
            gtk_font_button_set_use_font( (GtkFontButton *)o, BOOLVAL( on_off ) );
}

void trp_gtk_font_button_set_use_size( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FONT_BUTTON( o ) )
            gtk_font_button_set_use_size( (GtkFontButton *)o, BOOLVAL( on_off ) );
}

void trp_gtk_font_button_set_title( trp_obj_t *obj, trp_obj_t *title )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FONT_BUTTON( o ) ) {
            uns8b *p = trp_csprint( title );
            gtk_font_button_set_title( (GtkFontButton *)o, p );
            trp_csprint_free( p );
        }
}

