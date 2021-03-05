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

#ifndef __trpsdl__h
#define __trpsdl__h

uns8b trp_sdl_init();
void trp_sdl_quit();
uns8b trp_sdl_playwav( trp_obj_t *path, trp_obj_t *volume );
uns8b trp_sdl_playwav_memory( trp_obj_t *raw, trp_obj_t *volume );

#endif /* !__trpsdl__h */
