/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

#ifndef __trpavcodec__h
#define __trpavcodec__h

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

uns8b trp_av_init();
void trp_av_quit();
trp_obj_t *trp_av_format_version();
trp_obj_t *trp_av_codec_version();
trp_obj_t *trp_av_util_version();
trp_obj_t *trp_av_swscale_version();
trp_obj_t *trp_av_avcodec_version();
trp_obj_t *trp_av_avcodec_configuration();
trp_obj_t *trp_av_avcodec_license();
trp_obj_t *trp_av_avcodec_list();
trp_obj_t *trp_av_sws_context( trp_obj_t *wi, trp_obj_t *hi, trp_obj_t *wo, trp_obj_t *ho, trp_obj_t *alg );
uns8b trp_av_sws_scale( trp_obj_t *swsctx, trp_obj_t *pi, trp_obj_t *po );
trp_obj_t *trp_av_avformat_open_input( trp_obj_t *path, trp_obj_t *par );
trp_obj_t *trp_av_avformat_open_input_cuvid( trp_obj_t *path, trp_obj_t *par );
uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix, trp_obj_t *frameno );
uns8b trp_av_skip_frame( trp_obj_t *fmtctx, trp_obj_t *n );
uns8b trp_av_read_scd_histogram_set( trp_obj_t *fmtctx, trp_obj_t *raw );
uns8b trp_av_rewind( trp_obj_t *fmtctx );
trp_obj_t *trp_av_is_frame_recoverable( trp_obj_t *fmtctx );
trp_obj_t *trp_av_path( trp_obj_t *fmtctx );
trp_obj_t *trp_av_nb_streams( trp_obj_t *fmtctx );
trp_obj_t *trp_av_video_stream_idx( trp_obj_t *fmtctx );
trp_obj_t *trp_av_nb_frames( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_sample_aspect_ratio( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_avg_frame_rate( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_r_frame_rate( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_video_frame_rate( trp_obj_t *fmtctx );
trp_obj_t *trp_av_codec_type( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_codec_id( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_codec_name( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_time_base( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_start_time( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_duration( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_metadata( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_frameno2ts( trp_obj_t *fmtctx, trp_obj_t *frameno );
trp_obj_t *trp_av_frameno( trp_obj_t *fmtctx );
trp_obj_t *trp_av_ts( trp_obj_t *fmtctx );
trp_obj_t *trp_av_nearest_keyframe( trp_obj_t *fmtctx, trp_obj_t *frameno, trp_obj_t *max_diff );
trp_obj_t *trp_av_get_buf_size( trp_obj_t *fmtctx );
trp_obj_t *trp_av_get_buf_content( trp_obj_t *fmtctx );
uns8b trp_av_set_buf_size( trp_obj_t *fmtctx, trp_obj_t *bufsize );
trp_obj_t *trp_av_first_ts( trp_obj_t *fmtctx, trp_obj_t *streamno );
trp_obj_t *trp_av_get_filter( trp_obj_t *fmtctx );
uns8b trp_av_set_filter( trp_obj_t *fmtctx, trp_obj_t *descr );
uns8b trp_av_set_filter_rows( trp_obj_t *fmtctx, trp_obj_t *mode );

#endif /* !__trpavcodec__h */
