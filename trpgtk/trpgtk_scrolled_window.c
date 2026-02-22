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

trp_obj_t *trp_gtk_scrolled_window_new( trp_obj_t *hadj, trp_obj_t *vadj )
{
    GtkWidget *w, *h_adj, *v_adj;
    trp_obj_t *res;

    if ( hadj == UNDEF )
        h_adj = NULL;
    else {
        h_adj = trp_gtk_get_widget( hadj );
        if ( h_adj == NULL )
            return UNDEF;
        if ( !GTK_IS_ADJUSTMENT( h_adj ) )
            return UNDEF;
    }
    if ( vadj == UNDEF )
        v_adj = NULL;
    else {
        v_adj = trp_gtk_get_widget( vadj );
        if ( v_adj == NULL )
            return UNDEF;
        if ( !GTK_IS_ADJUSTMENT( v_adj ) )
            return UNDEF;
    }
    res = trp_gtk_widget( gtk_scrolled_window_new( (GtkAdjustment *)h_adj,
                                                   (GtkAdjustment *)v_adj ) );
    if ( h_adj )
        trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), hadj, NULL );
    if ( v_adj )
        trp_gtk_list_append( &(((trp_gtk_t *)res)->lw), vadj, NULL );
    return res;
}

void trp_gtk_scrolled_window_add_with_viewport( trp_obj_t *obj, trp_obj_t *child )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    GtkWidget *cc = trp_gtk_get_widget( child );

    if ( oo && cc )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) &&
             GTK_IS_WIDGET( cc ) ) {
            trp_gtk_list_append( &(((trp_gtk_t *)obj)->lw), child, NULL );
            gtk_scrolled_window_add_with_viewport( (GtkScrolledWindow *)oo, (GtkWidget *)cc );
        }
}

void trp_gtk_scrolled_window_set_policy( trp_obj_t *obj, trp_obj_t *hpol, trp_obj_t *vpol )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    if ( oo && ( hpol->tipo == TRP_SIG64 ) && ( vpol->tipo == TRP_SIG64 ) )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) )
            gtk_scrolled_window_set_policy( (GtkScrolledWindow *)oo,
                                            ((trp_sig64_t *)hpol)->val,
                                            ((trp_sig64_t *)vpol)->val );
}

trp_obj_t *trp_gtk_scrolled_window_get_hadjustment( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) )
            res = trp_gtk_widget( gtk_scrolled_window_get_hadjustment( (GtkScrolledWindow *)oo ) );
    return res;
}

trp_obj_t *trp_gtk_scrolled_window_get_vadjustment( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) )
            res = trp_gtk_widget( gtk_scrolled_window_get_vadjustment( (GtkScrolledWindow *)oo ) );
    return res;
}

trp_obj_t *trp_gtk_scrolled_window_get_hscrollbar( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) )
            res = trp_gtk_widget( gtk_scrolled_window_get_hscrollbar( (GtkScrolledWindow *)oo ) );
    return res;
}

trp_obj_t *trp_gtk_scrolled_window_get_vscrollbar( trp_obj_t *obj )
{
    GtkWidget *oo = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( oo )
        if ( GTK_IS_SCROLLED_WINDOW( oo ) )
            res = trp_gtk_widget( gtk_scrolled_window_get_vscrollbar( (GtkScrolledWindow *)oo ) );
    return res;
}

