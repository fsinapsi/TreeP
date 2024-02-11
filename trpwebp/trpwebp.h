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

#ifndef __trpwebp__h
#define __trpwebp__h

uns8b trp_webp_init();
uns8b trp_webp_save( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *quality );
trp_obj_t *trp_webp_save_memory( trp_obj_t *pix, trp_obj_t *quality );

#endif /* !__trpwebp__h */
