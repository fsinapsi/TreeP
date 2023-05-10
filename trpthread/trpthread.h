/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

#ifndef __trpthread__h
#define __trpthread__h

uns8b trp_thread_init();
void trp_thread_quit();
trp_obj_t *trp_thread_create( trp_obj_t *net, ... );
uns8b trp_thread_join( trp_obj_t *th );
trp_obj_t *trp_thread_self();
trp_obj_t *trp_thread_stopped( trp_obj_t *th );
trp_obj_t *trp_thread_cur();
trp_obj_t *trp_thread_max();
trp_obj_t *trp_thread_list();
uns8b trp_thread_send( uns8b flags, trp_obj_t *bmax, trp_obj_t *obj, trp_obj_t *th );
uns8b trp_thread_receive( uns8b flags, trp_obj_t **obj, trp_obj_t **mitt, trp_obj_t *th, ... );
trp_obj_t *trp_thread_case( trp_obj_t *obj, ... );

#endif /* !__trpthread__h */
