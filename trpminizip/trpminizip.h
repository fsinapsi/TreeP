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

#ifndef __trpminizip__h
#define __trpminizip__h

uns8b trp_minizip_zip( trp_obj_t *zip_path, trp_obj_t *src_path, trp_obj_t *str_path, trp_obj_t *level );
uns8b trp_minizip_unzip( trp_obj_t *zip_path, trp_obj_t *dst_path );

#endif /* !__trpminizip__h */
