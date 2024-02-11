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

#ifndef __trpvid_internal__h
#define __trpvid_internal__h

#include "../trp/trp.h"
#include "./trpvid.h"

#define MAX_QSCALE_ASP 10
#define MAX_QSCALE_AVC 40
#define MAX_WARPING_POINTS 4
#define MAX_BFRAMES 8

typedef struct {
    uns32b size;
    uns8b typ;
    sig8b qscale;
} frameinfo_t;

typedef struct {
    int seq_parameter_set_id;
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int log2_max_frame_num_minus4;
    int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    int num_ref_frames_in_pic_order_cnt_cycle;
    int delta_pic_order_always_zero_flag;
    int num_ref_frames;
    int frame_mbs_only_flag;
    int vui_seq_parameters_aspect_ratio_idc;
    int vui_seq_parameters_sar_width;
    int vui_seq_parameters_sar_height;
    uns8b *scaling_list[ 12 ];
} sps_t;

typedef struct {
    int pic_parameter_set_id;
    int seq_parameter_set_id;
    int entropy_coding_mode_flag;
    int pic_order_present_flag;
    int num_slice_groups_minus1;
    int num_slice_group_map_units_minus1;
    int NumberBitsPerSliceGroupId;

    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    int second_chroma_qp_index_offset;
    int deblocking_filter_control_present_flag;
    int constrained_intra_pred_flag;
    int redundant_pic_cnt_present_flag;
    int transform_8x8_mode_flag;
    uns8b *scaling_list[ 12 ];
} pps_t;

/*
 sample to chunk (v. ISO/IEC 14496-12 8.18 Sample To Chunk Box)
 */

typedef struct {
    uns32b first_chunk;
    uns32b samples_per_chunk;
    uns32b sample_description_index;
} stc_t;

typedef struct {
    uns8b tipo;
    sig8b bitstream_type; /* 0 = undef, -1: errore, 1 = MPEG-4 ASP, 2 = MS MPEG4, 3 = H.264 */
    uns8b r;
    uns8b matroska;
    uns32b cnt;
    FILE *fp;
    uns8b *error;
    uns8b *buf;
    uns8b *intra_quant_matrix, *inter_quant_matrix;
    uns8b *matroska_codec_private;
    frameinfo_t *qscale;
    uns8b **userdata;
    uns32b userdata_cnt;
    uns32b buf_alloc;
    uns32b buf_size;
    uns32b buf_pos;
    uns32b matroska_size;
    uns32b cnt_vol, cnt_vop;
    uns32b missing_vol;
    uns32b coded;
    uns32b shape, time_inc_bits, quant_precision;
    uns32b newpred_enable, reduced_resolution_enable;
    uns32b width, height, par, par_w, par_h;
    uns32b packed, cnt_packed, interlaced, tff, alternate_scan;
    uns32b sprite_enable, sprite_warping_points, mpeg_quant, qpel;
    uns32b divx_version, divx_build;
    uns32b tmp_frame_size, tmp_frame_pos, tmp_frame_cnt, avg_frame_size;
    sig32b max_frame_size;
    sps_t **sps;
    pps_t **pps;
    uns32b sps_cnt;
    uns32b pps_cnt;
    int idr_flag;
    uns32b max_bframes, cnt_bframe, cnt_bframes[ MAX_BFRAMES + 1 ];
    uns32b cnt_warp_points_used[ MAX_WARPING_POINTS + 1 ];
    uns32b cnt_qscale[ MAX_QSCALE_AVC + 1 ][ 7 ];
    uns32b cnt_qscale_cnt[ 7 ];
    sig32b cnt_qscale_max[ 7 ];
    sig32b cnt_qscale_avg[ 7 ];
    uns32b cnt_qscale_var[ 7 ];
    uns32b mp4_sample_cnt, mp4_entry_cnt, mp4_chunk_cnt;
    uns32b *mp4_sample_size;
    stc_t *mp4_sample_to_chunk;
    uns64b *mp4_chunk_offset;
    int rl;
} trp_vid_t;

uns8b trp_vid_close( trp_vid_t *obj );
uns8b trp_vid_parse_msmpeg4( trp_vid_t *vid );
uns8b trp_vid_parse_mpeg4asp( trp_vid_t *vid );
uns8b trp_vid_parse_mpeg4avc( trp_vid_t *vid );
uns32b trp_vid_effective_qscale( sig32b qscale, sig8b bitstream_type );
void trp_vid_update_qscale( trp_vid_t *vid, sig8b bitstream_type, uns32b typ, sig32b qscale );
void trp_vid_store_userdata( trp_vid_t *vid, uns8b *src, uns32b size );
void trp_vid_calculate_max_avg_frame_size( trp_vid_t *vid );
uns8b trp_vid_check( trp_obj_t *obj, trp_vid_t **vid );

#endif /* !__trpvid_internal__h */
