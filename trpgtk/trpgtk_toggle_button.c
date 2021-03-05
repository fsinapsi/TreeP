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

trp_obj_t *trp_gtk_toggle_button_new( trp_obj_t *label )
{
    GtkWidget *w;

    if ( label ) {
        uns8b *p = trp_csprint( label );
        if ( strchr( p, '_' ) )
            w = gtk_toggle_button_new_with_mnemonic( p );
        else
            w = gtk_toggle_button_new_with_label( p );
        trp_csprint_free( p );
    } else
        w = gtk_toggle_button_new();
    return trp_gtk_widget( w );
}

void trp_gtk_toggle_button_set_active( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            gtk_toggle_button_set_active( (GtkToggleButton *)o, BOOLVAL( on_off ) );
}

trp_obj_t *trp_gtk_toggle_button_get_active( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            res = VALBOOL( gtk_toggle_button_get_active( (GtkToggleButton *)o ) );
    return res;
}

void trp_gtk_toggle_button_set_mode( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            gtk_toggle_button_set_mode( (GtkToggleButton *)o, BOOLVAL( on_off ) );
}

trp_obj_t *trp_gtk_toggle_button_get_mode( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            res = VALBOOL( gtk_toggle_button_get_mode( (GtkToggleButton *)o ) );
    return res;
}

void trp_gtk_toggle_button_set_inconsistent( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            gtk_toggle_button_set_inconsistent( (GtkToggleButton *)o, BOOLVAL( on_off ) );
}

trp_obj_t *trp_gtk_toggle_button_get_inconsistent( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_TOGGLE_BUTTON( o ) )
            res = VALBOOL( gtk_toggle_button_get_inconsistent( (GtkToggleButton *)o ) );
    return res;
}

