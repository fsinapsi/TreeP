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

#ifndef __trpdbf__h
#define __trpdbf__h

uns8b trp_dbf_init();
trp_obj_t *trp_dbf_open( trp_obj_t *path );
trp_obj_t *trp_dbf_get_string_version( trp_obj_t *db );
trp_obj_t *trp_dbf_create_from_dbf( trp_obj_t *path, trp_obj_t *db );
trp_obj_t *trp_dbf_column_name( trp_obj_t *db, trp_obj_t *idx );
trp_obj_t *trp_dbf_column_size( trp_obj_t *db, trp_obj_t *idx );
trp_obj_t *trp_dbf_column_type( trp_obj_t *db, trp_obj_t *idx );
trp_obj_t *trp_dbf_column_decimals( trp_obj_t *db, trp_obj_t *idx );
trp_obj_t *trp_dbf_get_date( trp_obj_t *db );
uns8b trp_dbf_copy_record( trp_obj_t *db_dst, trp_obj_t *db_src );

#endif /* !__trpdbf__h */
