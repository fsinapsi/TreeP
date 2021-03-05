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

#ifndef __trpsuf__h
#define __trpsuf__h

uns8b trp_suf_init();
trp_obj_t *trp_suf_sais( trp_obj_t *s );
trp_obj_t *trp_suf_sa( trp_obj_t *suf, trp_obj_t *idx );
trp_obj_t *trp_suf_lcp( trp_obj_t *suf, trp_obj_t *idx );
trp_obj_t *trp_suf_search( trp_obj_t *suf, trp_obj_t *pattern );
trp_obj_t *trp_suf_lcs( trp_obj_t *s, ... );
trp_obj_t *trp_suf_lcs_alt( trp_obj_t *s, ... );
trp_obj_t *trp_suf_lcs_k( trp_obj_t *k, trp_obj_t *s, ... );
trp_obj_t *trp_suf_lcs_k_alt( trp_obj_t *k, trp_obj_t *s, ... );

#endif /* !__trpsuf__h */
