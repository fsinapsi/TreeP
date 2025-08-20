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

#ifndef __trpstr__h
#define __trpstr__h

trp_obj_t *trp_str_clean( trp_obj_t *s, ... );
trp_obj_t *trp_str_uniform_blanks( trp_obj_t *s, ... );
trp_obj_t *trp_str_brackets_are_balanced( trp_obj_t *s, ... );
trp_obj_t *trp_str_decode_html_entities_utf8( trp_obj_t *s, ... );
trp_obj_t *trp_str_encode_html_entities( trp_obj_t *s, ... );
trp_obj_t *trp_str_decode_url( trp_obj_t *s, ... );
trp_obj_t *trp_str_encode_url( trp_obj_t *s, ... );
trp_obj_t *trp_str_json_unescape( trp_obj_t *s, ... );
trp_obj_t *trp_str_json_escape( trp_obj_t *s, ... );
trp_obj_t *trp_str_fields( trp_obj_t *s, trp_obj_t *sep );

#endif /* !__trpstr__h */
