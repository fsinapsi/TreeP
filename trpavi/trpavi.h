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

#ifndef __trpavi__h
#define __trpavi__h

uns8b trp_avi_init();
trp_obj_t *trp_avi_open_input_file( trp_obj_t *path, trp_obj_t *getindex );
trp_obj_t *trp_avi_has_index( trp_obj_t *obj );
uns8b trp_avi_set_audio_track( trp_obj_t *obj, trp_obj_t *n );
trp_obj_t *trp_avi_audio_tracks( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_format( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_delay( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_chunks( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_fpos( trp_obj_t *obj, trp_obj_t *chunk );
trp_obj_t *trp_avi_audio_size( trp_obj_t *obj, trp_obj_t *chunk );
trp_obj_t *trp_avi_audio_streamsize( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_channels( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_frequency( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_vbr( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_samplerate( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_mp3rate( trp_obj_t *obj );
trp_obj_t *trp_avi_audio_padrate( trp_obj_t *obj );
trp_obj_t *trp_avi_video_compressor( trp_obj_t *obj );
trp_obj_t *trp_avi_video_delay( trp_obj_t *obj );
trp_obj_t *trp_avi_video_frames( trp_obj_t *obj );
trp_obj_t *trp_avi_video_keyframes( trp_obj_t *obj );
trp_obj_t *trp_avi_video_fpos( trp_obj_t *obj, trp_obj_t *frame );
trp_obj_t *trp_avi_video_size( trp_obj_t *obj, trp_obj_t *frame );
trp_obj_t *trp_avi_video_max_size( trp_obj_t *obj );
trp_obj_t *trp_avi_video_streamsize( trp_obj_t *obj );
trp_obj_t *trp_avi_video_framerate( trp_obj_t *obj );
trp_obj_t *trp_avi_video_bitrate( trp_obj_t *obj );
trp_obj_t *trp_avi_video_min_keyint( trp_obj_t *obj );
trp_obj_t *trp_avi_video_max_keyint( trp_obj_t *obj );
trp_obj_t *trp_avi_video_frame_is_keyframe( trp_obj_t *obj, trp_obj_t *frame );
trp_obj_t *trp_avi_video_read( trp_obj_t *obj, trp_obj_t *frame );
uns8b trp_avi_video_read_test( trp_obj_t *obj, trp_obj_t *raw, trp_obj_t *frame );
trp_obj_t *trp_avi_parse_junk( trp_obj_t *obj, trp_obj_t *size );

#endif /* !__trpavi__h */
