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

#include "../trp/trp.h"
#include "./trpsdl.h"
#include "../trpthread/trpthread.h"
#include <SDL3/SDL.h>

static uns8b trp_sdl_check();
static void SDLCALL trp_sdl_audio_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total_amount );

uns8b trp_sdl_init()
{
    return 0;
}

void trp_sdl_quit()
{
    if ( !trp_sdl_check() )
        SDL_QuitSubSystem( SDL_INIT_AUDIO );
}

static uns8b trp_sdl_check()
{
    static uns8b toinit = 1;

    if ( toinit ) {
        if ( !SDL_InitSubSystem( SDL_INIT_AUDIO ) )
            return 1;
        toinit = 0;
    }
    return 0;
}

typedef struct {
    objfun_t fun;
    trp_obj_t *udata;
    uns8b run;
} trp_sdl_audio_play_t;

static void SDLCALL trp_sdl_audio_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total_amount )
{
    trp_sdl_audio_play_t *a = (trp_sdl_audio_play_t *)userdata;
    trp_obj_t *raw;

    if ( a->run ) {
        if ( a->run == 2 ) {
            if ( trp_thread_register_my_thread() ) {
                a->run = 0;
                return;
            }
            a->run = 1;
        }
        raw = (a->fun)( a->udata, trp_sig64( (sig64b)len ) );
        if ( raw->tipo == TRP_RAW ) {
            SDL_PutAudioStreamData( stream, ((trp_raw_t *)raw)->data, ((trp_raw_t *)raw)->len );
        } else {
            a->run = 0;
            trp_thread_unregister_my_thread();
        }
    }
}

uns8b trp_sdl_audio_play( trp_obj_t *funptr, trp_obj_t *udata, trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps, trp_obj_t *volume )
{
    SDL_AudioStream *stream;
    trp_sdl_audio_play_t a;
    SDL_AudioSpec audiospec;
    uns32b i;
    flt64b vol;

    if ( trp_sdl_check() )
        return 1;
    if ( funptr->tipo != TRP_FUNPTR )
        return 1;
    if ( ((trp_funptr_t *)funptr)->nargs != 2 )
        return 1;
    memset( &audiospec, 0, sizeof( SDL_AudioSpec ) );
    if ( trp_cast_uns32b( ch, &i ) )
        return 1;
    audiospec.channels = i;
    if ( trp_cast_uns32b( freq, &i ) )
        return 1;
    audiospec.freq = i;
    if ( trp_cast_uns32b( bps, &i ) )
        return 1;
    audiospec.format = i;
    if ( audiospec.format > 8 )
        audiospec.format |= SDL_AUDIO_MASK_SIGNED;
    if ( volume ) {
        if ( trp_cast_flt64b_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    a.fun = ((trp_funptr_t *)funptr)->f;
    a.udata = udata;
    a.run = 2;
    stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec, trp_sdl_audio_play_cback, (void *)( &a ) );
    SDL_SetAudioStreamGain( stream, (float)vol );
    SDL_ResumeAudioDevice( SDL_GetAudioStreamDevice( stream ) );
    while ( a.run )
        SDL_Delay( 50 );
    SDL_Delay( 100 );
    SDL_DestroyAudioStream( stream );
    return 0;
}

