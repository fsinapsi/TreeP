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

trp_obj_t *trp_gtk_combo_box_new()
{
    return trp_gtk_widget( gtk_combo_box_new() );
}

trp_obj_t *trp_gtk_combo_box_get_active( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_COMBO_BOX( o ) )
            res = trp_sig64( gtk_combo_box_get_active( (GtkComboBox *)o ) );
    return res;
}

void trp_gtk_combo_box_set_active( trp_obj_t *obj, trp_obj_t *idx )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && ( idx->tipo == TRP_SIG64 ) )
        if ( GTK_IS_COMBO_BOX( o ) )
            gtk_combo_box_set_active( (GtkComboBox *)o, ((trp_sig64_t *)idx)->val );
}

