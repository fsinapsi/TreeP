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

trp_obj_t *trp_gtk_notebook_new()
{
    return trp_gtk_widget( gtk_notebook_new() );
}

trp_obj_t *trp_gtk_notebook_append_page( trp_obj_t *nb, trp_obj_t *obj, trp_obj_t *lb )
{
    GtkWidget *nn = trp_gtk_get_widget( nb );
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( nn && oo )
        if ( GTK_IS_NOTEBOOK( nn ) &&
             GTK_IS_WIDGET( oo ) ) {
            GtkWidget *tab_label;
            gint r;

            if ( lb ) {
                tab_label = trp_gtk_get_widget( lb );
                if ( tab_label == NULL )
                    return UNDEF;
                if ( !GTK_IS_WIDGET( tab_label ) )
                    return UNDEF;
            } else
                tab_label = NULL;
            r = gtk_notebook_append_page( (GtkNotebook *)nn, oo, tab_label );
            if ( r >= 0 ) {
                trp_gtk_list_append( &(((trp_gtk_t *)nb)->lw), obj, NULL );
                if ( lb )
                    trp_gtk_list_append( &(((trp_gtk_t *)nb)->lw), lb, NULL );
                res = trp_sig64( r );
            }
        }
    return res;
}

void trp_gtk_notebook_remove_page( trp_obj_t *nb, trp_obj_t *idx )
{
    GtkWidget *nn = trp_gtk_get_widget( nb );

    if ( nn )
        if ( GTK_IS_NOTEBOOK( nn ) ) {
            gint n = gtk_notebook_get_n_pages( (GtkNotebook *)nn );

            if ( n ) {
                GtkWidget *child = NULL;
                gint i;

                if ( idx )
                    if ( idx->tipo == TRP_SIG64 )
                        if ( ((trp_sig64_t *)idx)->val == -1 )
                            i = n - 1;
                        else if ( ( ((trp_sig64_t *)idx)->val >= 0 ) &&
                                  ( ((trp_sig64_t *)idx)->val < n ) )
                            i = ((trp_sig64_t *)idx)->val;
                        else
                            return;
                    else {
                        child = trp_gtk_get_widget( idx );
                        if ( child == NULL )
                            return;
                        if ( !GTK_IS_WIDGET( child ) )
                            return;
                        i = gtk_notebook_page_num( (GtkNotebook *)nn, child );
                        if ( i < 0 )
                            return;
                    }
                else
                    i = n - 1;
                if ( child == NULL )
                    child = gtk_notebook_get_nth_page( (GtkNotebook *)nn, i );
                if ( child ) {
                    trp_gtk_list_remove_by_widget( &(((trp_gtk_t *)nb)->lw), child, NULL );
                    if ( child = gtk_notebook_get_tab_label( (GtkNotebook *)nn, child ) )
                        trp_gtk_list_remove_by_widget( &(((trp_gtk_t *)nb)->lw), child, NULL );
                    gtk_notebook_remove_page( (GtkNotebook *)nn, i );
                }
            }
        }
}

void trp_gtk_notebook_set_scrollable( trp_obj_t *nb, trp_obj_t *on_off )
{
    GtkWidget *nn = trp_gtk_get_widget( nb );

    if ( nn && TRP_BOOLP( on_off ) )
        if ( GTK_IS_NOTEBOOK( nn ) )
            gtk_notebook_set_scrollable( (GtkNotebook *)nn, BOOLVAL( on_off ) );
}

trp_obj_t *trp_gtk_notebook_get_current_page( trp_obj_t *nb )
{
    GtkWidget *nn = trp_gtk_get_widget( nb );
    trp_obj_t *res = UNDEF;

    if ( nn )
        if ( GTK_IS_NOTEBOOK( nn ) )
            res = trp_sig64( gtk_notebook_get_current_page( (GtkNotebook *)nn ) );
    return res;
}

void trp_gtk_notebook_set_current_page( trp_obj_t *nb, trp_obj_t *idx )
{
    GtkWidget *nn = trp_gtk_get_widget( nb );

    if ( nn && ( idx->tipo == TRP_SIG64 ) )
        if ( GTK_IS_NOTEBOOK( nn ) )
            gtk_notebook_set_current_page( (GtkNotebook *)nn,
                                           (gint)( ((trp_sig64_t *)idx)->val ) );
}

