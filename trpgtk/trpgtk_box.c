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

trp_obj_t *trp_gtk_hbox_new( trp_obj_t *homogeneous, trp_obj_t *space )
{
    uns32b sp;

    if ( ( !TRP_BOOLP( homogeneous ) ) ||
         trp_cast_uns32b( space, &sp ) )
        return UNDEF;
    return trp_gtk_widget( gtk_hbox_new( BOOLVAL( homogeneous ), sp ) );
}

trp_obj_t *trp_gtk_vbox_new( trp_obj_t *homogeneous, trp_obj_t *space )
{
    uns32b sp;

    if ( ( !TRP_BOOLP( homogeneous ) ) ||
         trp_cast_uns32b( space, &sp ) )
        return UNDEF;
    return trp_gtk_widget( gtk_vbox_new( BOOLVAL( homogeneous ), sp ) );
}

void trp_gtk_box_pack_start( trp_obj_t *box, trp_obj_t *obj, trp_obj_t *expand, trp_obj_t *fill, trp_obj_t *padding )
{
    GtkWidget *bb = trp_gtk_get_widget( box );
    GtkWidget *oo = trp_gtk_get_widget( obj );
    uns32b pd;

    if ( ( !trp_cast_uns32b( padding, &pd ) ) &&
         bb && oo &&
         TRP_BOOLP( expand ) &&
         TRP_BOOLP( fill ) )
        if ( GTK_IS_BOX( bb ) &&
             GTK_IS_WIDGET( oo ) ) {
            trp_gtk_list_append( &(((trp_gtk_t *)box)->lw), obj, NULL );
            gtk_box_pack_start( (GtkBox *)bb, oo, BOOLVAL( expand ), BOOLVAL( fill ), pd );
        }
}

void trp_gtk_box_pack_end( trp_obj_t *box, trp_obj_t *obj, trp_obj_t *expand, trp_obj_t *fill, trp_obj_t *padding )
{
    GtkWidget *bb = trp_gtk_get_widget( box );
    GtkWidget *oo = trp_gtk_get_widget( obj );
    uns32b pd;

    if ( ( !trp_cast_uns32b( padding, &pd ) ) &&
         bb && oo &&
         TRP_BOOLP( expand ) &&
         TRP_BOOLP( fill ) )
        if ( GTK_IS_BOX( bb ) &&
             GTK_IS_WIDGET( oo ) ) {
            trp_gtk_list_append( &(((trp_gtk_t *)box)->lw), obj, NULL );
            gtk_box_pack_end( (GtkBox *)bb, oo, BOOLVAL( expand ), BOOLVAL( fill ), pd );
        }
}

