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

#ifndef __trprsvg__h
#define __trprsvg__h

uns8b trp_rsvg_init();
trp_obj_t *trp_rsvg_load( trp_obj_t *src, trp_obj_t *mult );
trp_obj_t *trp_rsvg_load_size( trp_obj_t *src, trp_obj_t *target_width, trp_obj_t *target_height );
trp_obj_t *trp_rsvg_width( trp_obj_t *src );
trp_obj_t *trp_rsvg_height( trp_obj_t *src );

#endif /* !__trprsvg__h */
