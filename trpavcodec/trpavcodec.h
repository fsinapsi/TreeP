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

#ifndef __trpavcodec__h
#define __trpavcodec__h

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

uns8b trp_av_init();
void trp_av_quit();
trp_obj_t *trp_av_format_version();
trp_obj_t *trp_av_codec_version();
trp_obj_t *trp_av_util_version();
trp_obj_t *trp_av_swscale_version();
trp_obj_t *trp_av_sws_context( trp_obj_t *wi, trp_obj_t *hi, trp_obj_t *wo, trp_obj_t *ho, trp_obj_t *alg );
uns8b trp_av_sws_scale( trp_obj_t *swsctx, trp_obj_t *pi, trp_obj_t *po );
trp_obj_t *trp_av_open_input_file( trp_obj_t *path, trp_obj_t *par );
uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix );
uns8b trp_av_skip_frame( trp_obj_t *fmtctx, trp_obj_t *n );
uns8b trp_av_seek_frame( trp_obj_t *fmtctx, trp_obj_t *ts );
trp_obj_t *trp_av_time_base( trp_obj_t *fmtctx );
trp_obj_t *trp_av_pts( trp_obj_t *fmtctx );
trp_obj_t *trp_av_index_entries( trp_obj_t *fmtctx );
trp_obj_t *trp_av_index_keyframe( trp_obj_t *fmtctx, trp_obj_t *frameno );

#endif /* !__trpavcodec__h */
