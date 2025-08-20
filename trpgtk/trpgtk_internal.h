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

#ifndef __trpgtk_internal__h
#define __trpgtk_internal__h

#include "../trp/trp.h"
#include "./trpgtk.h"

#undef TRP_GTK_DEBUG

#define BOOLVAL(x) (((x)==TRP_TRUE)?TRUE:FALSE)
#define VALBOOL(x) (((x)==TRUE)?TRP_TRUE:TRP_FALSE)

typedef struct {
    uns8b tipo;
    void *w;
    trp_obj_t *lw;
    trp_obj_t *ls;
    gulong id;
} trp_gtk_t;

typedef struct {
    trp_netptr_t *net;
    trp_obj_t **udata;
    gulong id;
} trp_gtk_signal_t;

trp_obj_t *trp_gtk_widget( void *w );
GtkWidget *trp_gtk_get_widget( trp_obj_t *obj );
trp_gtk_signal_t *trp_gtk_signal( trp_obj_t *net, trp_obj_t **udata );
void trp_gtk_list_append( trp_obj_t **list, trp_obj_t *obj, pthread_mutex_t *mtx );
void trp_gtk_list_remove( trp_obj_t **list, trp_obj_t *obj, pthread_mutex_t *mtx );
void trp_gtk_list_remove_by_widget( trp_obj_t **list, GtkWidget *w, pthread_mutex_t *mtx );
uns8b trp_gtk_list_find_by_widget( trp_obj_t **list, GtkWidget *w, pthread_mutex_t *mtx, trp_obj_t **obj );

#endif /* !__trpgtk_internal__h */
