/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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

#ifndef __trpvid__h
#define __trpvid__h

uns8b trp_vid_init();
trp_obj_t *trp_vid_create( trp_obj_t *f );
uns8b trp_vid_parse( trp_obj_t *vid, trp_obj_t *size, trp_obj_t *stripped );
uns8b trp_vid_parse_matroska( trp_obj_t *vid, trp_obj_t *size );
trp_obj_t *trp_vid_bitstream_type( trp_obj_t *obj );
trp_obj_t *trp_vid_error( trp_obj_t *obj );
trp_obj_t *trp_vid_userdata( trp_obj_t *obj );
trp_obj_t *trp_vid_missing_vol( trp_obj_t *obj );
trp_obj_t *trp_vid_cnt_vol( trp_obj_t *obj );
trp_obj_t *trp_vid_cnt_vop( trp_obj_t *obj );
trp_obj_t *trp_vid_max_frame_size( trp_obj_t *obj );
trp_obj_t *trp_vid_avg_frame_size( trp_obj_t *obj );
trp_obj_t *trp_vid_cnt_size_frame( trp_obj_t *obj, trp_obj_t *fno );
trp_obj_t *trp_vid_cnt_type_frame( trp_obj_t *obj, trp_obj_t *fno );
trp_obj_t *trp_vid_cnt_qscale_frame( trp_obj_t *obj, trp_obj_t *fno );
trp_obj_t *trp_vid_cnt_qscale( trp_obj_t *obj, trp_obj_t *qs, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb );
trp_obj_t *trp_vid_cnt_qscale_cnt( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb );
trp_obj_t *trp_vid_cnt_qscale_max( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb );
trp_obj_t *trp_vid_cnt_qscale_avg( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb );
trp_obj_t *trp_vid_cnt_qscale_var( trp_obj_t *obj, trp_obj_t *ttyp, trp_obj_t *tosa, trp_obj_t *tosb );
trp_obj_t *trp_vid_par( trp_obj_t *obj, trp_obj_t *sps_idx );
trp_obj_t *trp_vid_par_w( trp_obj_t *obj, trp_obj_t *sps_idx );
trp_obj_t *trp_vid_par_h( trp_obj_t *obj, trp_obj_t *sps_idx );
trp_obj_t *trp_vid_packed( trp_obj_t *obj );
trp_obj_t *trp_vid_interlaced( trp_obj_t *obj );
trp_obj_t *trp_vid_tff( trp_obj_t *obj );
trp_obj_t *trp_vid_alternate_scan( trp_obj_t *obj );
trp_obj_t *trp_vid_sprite_enable( trp_obj_t *obj );
trp_obj_t *trp_vid_sprite_warping_points( trp_obj_t *obj );
trp_obj_t *trp_vid_mpeg_quant( trp_obj_t *obj );
trp_obj_t *trp_vid_quant_matrix( trp_obj_t *obj, trp_obj_t *nmtx, trp_obj_t *idx, trp_obj_t *sps_idx );
trp_obj_t *trp_vid_qpel( trp_obj_t *obj );
trp_obj_t *trp_vid_max_bframes( trp_obj_t *obj );
trp_obj_t *trp_vid_cnt_bframes( trp_obj_t *obj, trp_obj_t *n );
trp_obj_t *trp_vid_cnt_warp_points_used( trp_obj_t *obj, trp_obj_t *points );
trp_obj_t *trp_vid_time_inc_bits( trp_obj_t *obj );
trp_obj_t *trp_vid_sps_cnt( trp_obj_t *obj );
trp_obj_t *trp_vid_sps_id( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_pps_cnt( trp_obj_t *obj );
trp_obj_t *trp_vid_pps_id( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_pps_sps_id( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_num_ref_frames( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_profile_idc( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_level_idc( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_chroma_format_idc( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_entropy_coding_mode( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_weighted_pred( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_weighted_bipred_idc( trp_obj_t *obj, trp_obj_t *idx );
trp_obj_t *trp_vid_transform_8x8_mode_flag( trp_obj_t *obj, trp_obj_t *idx );
uns8b trp_vid_mp4_load_sample_size( trp_obj_t *obj, trp_obj_t *s_size, trp_obj_t *s_cnt, trp_obj_t *compact );
uns8b trp_vid_mp4_load_sample_to_chunk( trp_obj_t *obj, trp_obj_t *e_cnt );
uns8b trp_vid_mp4_load_chunk_offset( trp_obj_t *obj, trp_obj_t *c_cnt, trp_obj_t *f_size );
trp_obj_t *trp_vid_mp4_track_size( trp_obj_t *obj );
trp_obj_t *trp_vid_mp4_sample_size( trp_obj_t *obj, trp_obj_t *spl );
trp_obj_t *trp_vid_mp4_sample_offset( trp_obj_t *obj, trp_obj_t *spl );
trp_obj_t *trp_vid_qscale_correction_a( trp_obj_t *obj );
trp_obj_t *trp_vid_qscale_correction_b( trp_obj_t *obj );
trp_obj_t *trp_vid_search_next( trp_obj_t *obj, trp_obj_t *f_cnt,
                                trp_obj_t *s_min, trp_obj_t *s_max,
                                trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                trp_obj_t *ttyp );
trp_obj_t *trp_vid_search_prev( trp_obj_t *obj, trp_obj_t *f_cnt,
                                trp_obj_t *s_min, trp_obj_t *s_max,
                                trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                trp_obj_t *ttyp );

#endif /* !__trpvid__h */
