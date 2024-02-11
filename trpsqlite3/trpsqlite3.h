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

#ifndef __trpsqlite3__h
#define __trpsqlite3__h

uns8b trp_sqlite3_init();
void trp_sqlite3_quit();
trp_obj_t *trp_sqlite3_open( trp_obj_t *path );
trp_obj_t *trp_sqlite3_escape_strings( trp_obj_t *s, ... );
trp_obj_t *trp_sqlite3_exec( trp_obj_t *obj, trp_obj_t *query, ... );
trp_obj_t *trp_sqlite3_exec_raw( trp_obj_t *obj, trp_obj_t *query, ... );
uns8b trp_sqlite3_exec_data( trp_obj_t *obj, trp_obj_t *net, trp_obj_t *data, trp_obj_t *query, ... );
uns8b trp_sqlite3_exec_data_raw( trp_obj_t *obj, trp_obj_t *net, trp_obj_t *data, trp_obj_t *query, ... );
uns8b trp_sqlite3_begin( trp_obj_t *obj );
uns8b trp_sqlite3_begin_exclusive( trp_obj_t *obj );
uns8b trp_sqlite3_end( trp_obj_t *obj );
uns8b trp_sqlite3_rollback( trp_obj_t *obj );
trp_obj_t *trp_sqlite3_changes( trp_obj_t *obj );
trp_obj_t *trp_sqlite3_total_changes( trp_obj_t *obj );

#endif /* !__trpsqlite3__h */
