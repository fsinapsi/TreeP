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

#ifndef __trpmagic__h
#define __trpmagic__h

uns8b trp_magic_init();
void trp_magic_quit();
uns8b trp_magic_reinit( trp_obj_t *path );
trp_obj_t *trp_magic_available();
trp_obj_t *trp_magic_file( trp_obj_t *path );
trp_obj_t *trp_magic_buffer( trp_obj_t *raw, trp_obj_t *cnt );

#endif /* !__trpmagic__h */
