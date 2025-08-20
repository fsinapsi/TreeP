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

#ifndef __trpaud__h
#define __trpaud__h

uns8b trp_aud_init();
trp_obj_t *trp_aud_create( trp_obj_t *f, trp_obj_t *forced_codec );
uns8b trp_aud_parse_aac_header( trp_obj_t *aud, trp_obj_t *len );
uns8b trp_aud_parse( trp_obj_t *aud, trp_obj_t *len, trp_obj_t *stripped );
uns8b trp_aud_parse_step( trp_obj_t *aud );
uns8b trp_aud_fpout_begin( trp_obj_t *aud, trp_obj_t *f, trp_obj_t *skip_garbage );
uns8b trp_aud_fpout_end( trp_obj_t *aud );
trp_obj_t *trp_aud_codec( trp_obj_t *aud );
trp_obj_t *trp_aud_version( trp_obj_t *aud );
trp_obj_t *trp_aud_layer( trp_obj_t *aud );
trp_obj_t *trp_aud_frames( trp_obj_t *aud );
trp_obj_t *trp_aud_duration( trp_obj_t *aud );
trp_obj_t *trp_aud_vbr( trp_obj_t *aud );
trp_obj_t *trp_aud_padding( trp_obj_t *aud );
trp_obj_t *trp_aud_bitrate( trp_obj_t *aud );
trp_obj_t *trp_aud_frequency( trp_obj_t *aud );
trp_obj_t *trp_aud_emphasis( trp_obj_t *aud );
trp_obj_t *trp_aud_mode( trp_obj_t *aud );
trp_obj_t *trp_aud_initial_skip( trp_obj_t *aud );
trp_obj_t *trp_aud_internal_skip( trp_obj_t *aud );
trp_obj_t *trp_aud_buf_act( trp_obj_t *aud );
trp_obj_t *trp_aud_tot_read( trp_obj_t *aud );
trp_obj_t *trp_aud_encoder( trp_obj_t *aud );
trp_obj_t *trp_aud_splitted( trp_obj_t *aud );

#endif /* !__trpaud__h */
