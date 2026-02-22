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

#ifndef __trpsift__h
#define __trpsift__h

uns8b trp_sift_init();
trp_obj_t *trp_sift_features( trp_obj_t *pix );
trp_obj_t *trp_sift_match( trp_obj_t *obj1, trp_obj_t *obj2, trp_obj_t *threshold );
trp_obj_t *trp_sift_analyze( trp_obj_t *m );

#endif /* !__trpsift__h */
