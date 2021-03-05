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

trp_obj_t *trp_gtk_file_chooser_get_filename( trp_obj_t *obj )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( o )
        if ( GTK_IS_FILE_CHOOSER( o ) ) {
            uns8b *p = (uns8b *)gtk_file_chooser_get_filename( (GtkFileChooser *)o );

            if ( p ) {
                trp_convert_slash( p );
                res = trp_cord( p );
                g_free( p );
            }
        }
    return res;
}

void trp_gtk_file_chooser_set_filename( trp_obj_t *obj, trp_obj_t *path )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FILE_CHOOSER( o ) ) {
            uns8b *p = trp_csprint( path );
            gtk_file_chooser_set_filename( (GtkFileChooser *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_file_chooser_set_current_name( trp_obj_t *obj, trp_obj_t *path )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FILE_CHOOSER( o ) ) {
            uns8b *p = trp_csprint( path );
            gtk_file_chooser_set_current_name( (GtkFileChooser *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_file_chooser_set_current_folder( trp_obj_t *obj, trp_obj_t *path )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FILE_CHOOSER( o ) ) {
            uns8b *p = trp_csprint( path );
            gtk_file_chooser_set_current_folder( (GtkFileChooser *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_file_chooser_select_filename( trp_obj_t *obj, trp_obj_t *path )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o )
        if ( GTK_IS_FILE_CHOOSER( o ) ) {
            uns8b *p = trp_csprint( path );
            (void)gtk_file_chooser_select_filename( (GtkFileChooser *)o, p );
            trp_csprint_free( p );
        }
}

void trp_gtk_file_chooser_set_local_only( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FILE_CHOOSER( o ) )
            gtk_file_chooser_set_local_only( (GtkFileChooser *)o, BOOLVAL( on_off ) );
}

void trp_gtk_file_chooser_set_show_hidden( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FILE_CHOOSER( o ) )
            gtk_file_chooser_set_show_hidden( (GtkFileChooser *)o, BOOLVAL( on_off ) );
}

void trp_gtk_file_chooser_set_create_folders( trp_obj_t *obj, trp_obj_t *on_off )
{
    GtkWidget *o = trp_gtk_get_widget( obj );
    if ( o && TRP_BOOLP( on_off ) )
        if ( GTK_IS_FILE_CHOOSER( o ) )
            gtk_file_chooser_set_create_folders( (GtkFileChooser *)o, BOOLVAL( on_off ) );
}

