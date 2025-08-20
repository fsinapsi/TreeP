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

trp_obj_t *trp_gtk_frame_new( trp_obj_t *label )
{
    GtkWidget *w;
    uns8b *p = label ? trp_csprint( label ) : NULL;

    w = gtk_frame_new( p );
    if ( p )
        trp_csprint_free( p );
    return trp_gtk_widget( w );
}

void trp_gtk_frame_set_label( trp_obj_t *obj, trp_obj_t *label )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FRAME( o ) ) {
            uns8b *p = label ? trp_csprint( label ) : NULL;

            gtk_frame_set_label( (GtkFrame *)o, p );
            if ( p )
                trp_csprint_free( p );
        }
}

