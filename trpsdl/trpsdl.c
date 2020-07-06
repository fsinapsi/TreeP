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

#include "../trp/trp.h"
#include "./trpsdl.h"
#include <SDL2/SDL.h>

typedef struct {
    uns8b *buf;
    uns32b len;
    uns32b vol;
} trp_sdl_audio_cback_t;

static void trp_sdl_wavplay_cback( void *userdata, uns8b *stream, int len );

uns8b trp_sdl_init()
{
    if ( SDL_Init( SDL_INIT_AUDIO ) < 0 ) {
        fprintf( stderr, "Initialization of SDL2 failed\n" );
        return 1;
    }
    return 0;
}

void trp_sdl_quit()
{
    SDL_Quit();
}

static void trp_sdl_wavplay_cback( void *userdata, uns8b *stream, int len )
{
    trp_sdl_audio_cback_t *a = (trp_sdl_audio_cback_t *)userdata;

    SDL_memset( stream, 0, len );
    if ( a->len ) {
        if ( len > a->len )
            len = a->len;
        SDL_MixAudio( stream, a->buf, len, a->vol );
        a->buf += len;
        a->len -= len;
    }
}

uns8b trp_sdl_playwav( trp_obj_t *path, trp_obj_t *volume )
{
    uns8b *cpath;
    trp_sdl_audio_cback_t a;
    SDL_AudioSpec wav_spec;
    uns32b wav_length;
    uns8b *wav_buffer;
    double vol;

    if ( volume ) {
        if ( trp_cast_double_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    cpath = trp_csprint( path );
    if( SDL_LoadWAV( cpath, &wav_spec, &wav_buffer, &wav_length ) == NULL ) {
        trp_csprint_free( cpath );
        return 1;
    }
    trp_csprint_free( cpath );
    a.buf = wav_buffer;
    a.len = wav_length;
    a.vol = (uns32b)( vol * (double)(SDL_MIX_MAXVOLUME) + 0.5 );
    wav_spec.callback = trp_sdl_wavplay_cback;
    wav_spec.userdata = (void *)( &a );
    if ( SDL_OpenAudio( &wav_spec, NULL ) < 0 )
        return 1;
    SDL_PauseAudio( 0 );
    while ( a.len )
        SDL_Delay( 50 );
    SDL_Delay( 350 );
    SDL_CloseAudio();
    SDL_FreeWAV( wav_buffer );
    return 0;
}



