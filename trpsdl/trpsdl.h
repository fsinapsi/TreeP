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

#ifndef __trpsdl__h
#define __trpsdl__h

#include <SDL3/SDL.h>

uns8b trp_sdl_init();
void trp_sdl_quit();
uns8b trp_sdl_enable_screen_saver();
uns8b trp_sdl_disable_screen_saver();
trp_obj_t *trp_sdl_open_audio_stream( trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps );
uns8b trp_sdl_pause_audio_stream_device( trp_obj_t *stream );
uns8b trp_sdl_resume_audio_stream_device( trp_obj_t *stream );
uns8b trp_sdl_set_audio_stream_gain( trp_obj_t *stream, trp_obj_t *gain );
uns8b trp_sdl_flush_audio_stream( trp_obj_t *stream );
uns8b trp_sdl_clear_audio_stream( trp_obj_t *stream );
uns8b trp_sdl_put_audio_stream_data( trp_obj_t *stream, trp_obj_t *raw, trp_obj_t *len );
uns8b trp_sdl_audio_play( trp_obj_t *funptr, trp_obj_t *udata, trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps, trp_obj_t *gain );
trp_obj_t *trp_sdl_create_window_and_renderer( trp_obj_t *title, trp_obj_t *width, trp_obj_t *height, trp_obj_t *flags );
trp_obj_t *trp_sdl_get_display_for_window( trp_obj_t *window );
trp_obj_t *trp_sdl_get_desktop_display_size( trp_obj_t *display_id );
trp_obj_t *trp_sdl_get_current_display_size( trp_obj_t *display_id );
trp_obj_t *trp_sdl_get_window_size( trp_obj_t *window );
trp_obj_t *trp_sdl_get_render_scale( trp_obj_t *window );
uns8b trp_sdl_set_window_position( trp_obj_t *window, trp_obj_t *x, trp_obj_t *y );
uns8b trp_sdl_set_window_size( trp_obj_t *window, trp_obj_t *width, trp_obj_t *height );
uns8b trp_sdl_set_window_icon( trp_obj_t *window, trp_obj_t *pix );
uns8b trp_sdl_set_window_fullscreen( trp_obj_t *window, trp_obj_t *fs );
uns8b trp_sdl_set_window_aspect_ratio( trp_obj_t *window, trp_obj_t *min_aspect, trp_obj_t *max_aspect );
uns8b trp_sdl_set_render_draw_color( trp_obj_t *window, trp_obj_t *color );
trp_obj_t *trp_sdl_create_texture( trp_obj_t *window, trp_obj_t *access, trp_obj_t *width, trp_obj_t *height );
trp_obj_t *trp_sdl_create_texture_from_surface( trp_obj_t *window, trp_obj_t *pix );
uns8b trp_sdl_render_clear( trp_obj_t *window );
uns8b trp_sdl_render_texture( trp_obj_t *window, trp_obj_t *texture );
uns8b trp_sdl_render_present( trp_obj_t *window );
uns8b trp_sdl_update_texture( trp_obj_t *texture, trp_obj_t *pix );
trp_obj_t *trp_sdl_create_event();
uns8b trp_sdl_poll_event( trp_obj_t *event );
trp_obj_t *trp_sdl_event_type( trp_obj_t *event );
trp_obj_t *trp_sdl_event_key_scancode( trp_obj_t *event );
trp_obj_t *trp_sdl_get_ticks();
trp_obj_t *trp_sdl_get_ticks_ns();
uns8b trp_sdl_delay( trp_obj_t *ms );
uns8b trp_sdl_delay_ns( trp_obj_t *ns );
uns8b trp_sdl_delay_precise( trp_obj_t *ns );

#endif /* !__trpsdl__h */
