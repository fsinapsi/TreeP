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

typedef struct {
    objfun_t fun;
    trp_obj_t *udata;
    trp_obj_t *th;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uns8b run;
} trp_sdl_audio_play_t;

static uns8b trp_sdl_audio_init();
static uns8b trp_sdl_video_init();
static void SDLCALL trp_sdl_audio_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total );
static void trp_sdl_audio_play_cback_signal( trp_sdl_audio_play_t *a );

static uns8b _trp_sdl_audio_init = 0;
static uns8b _trp_sdl_video_init = 0;

uns8b trp_sdl_init()
{
    return 0;
}

void trp_sdl_quit()
{
    if ( _trp_sdl_audio_init )
        SDL_QuitSubSystem( SDL_INIT_AUDIO );
    if ( _trp_sdl_video_init )
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
    SDL_Quit();
}

static uns8b trp_sdl_audio_init()
{
    if ( _trp_sdl_audio_init == 0 ) {
        if ( !SDL_InitSubSystem( SDL_INIT_AUDIO ) )
            return 1;
        _trp_sdl_audio_init = 1;
    }
    return 0;
}

static uns8b trp_sdl_video_init()
{
    if ( _trp_sdl_video_init == 0 ) {
        if ( !SDL_InitSubSystem( SDL_INIT_VIDEO ) )
            return 1;
        _trp_sdl_video_init = 1;
    }
    return 0;
}

uns8b trp_sdl_enable_screen_saver()
{
    if ( trp_sdl_video_init() )
        return 1;
    return ( SDL_EnableScreenSaver() == true ) ? 0 : 1;
}

uns8b trp_sdl_disable_screen_saver()
{
    if ( trp_sdl_video_init() )
        return 1;
    return ( SDL_DisableScreenSaver() == true ) ? 0 : 1;
}

static void SDLCALL trp_sdl_audio_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total )
{
    trp_sdl_audio_play_t *a = (trp_sdl_audio_play_t *)userdata;
    trp_obj_t *raw;

    if ( a->run ) {
        if ( a->th == NULL ) {
            if ( trp_thread_register_my_thread() ) {
                a->run = 0;
                trp_sdl_audio_play_cback_signal( a );
                return;
            }
            a->th = trp_thread_self();
        }
        raw = (a->fun)( a->udata, trp_sig64( (sig64b)len ) );
        if ( raw->tipo == TRP_RAW ) {
            SDL_PutAudioStreamData( stream, ((trp_raw_t *)raw)->data, ((trp_raw_t *)raw)->len );
        } else {
            a->run = 0;
            trp_sdl_audio_play_cback_signal( a );
        }
    }
}

static void trp_sdl_audio_play_cback_signal( trp_sdl_audio_play_t *a )
{
    pthread_mutex_lock( &( a->mutex ) );
    pthread_cond_signal( &( a->cond ) );
    pthread_mutex_unlock( &( a->mutex ) );
}

uns8b trp_sdl_audio_play( trp_obj_t *funptr, trp_obj_t *udata, trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps, trp_obj_t *volume )
{
    SDL_AudioStream *stream;
    trp_sdl_audio_play_t a;
    SDL_AudioSpec audiospec;
    uns32b i;
    flt64b vol;

    if ( trp_sdl_audio_init() )
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
    a.th = NULL;
    pthread_mutex_init( &( a.mutex ), NULL );
    pthread_cond_init( &( a.cond ), NULL );
    a.run = 1;
    pthread_mutex_lock( &( a.mutex ) );
    stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec, trp_sdl_audio_play_cback, (void *)( &a ) );
    SDL_SetAudioStreamGain( stream, (float)vol );
    SDL_ResumeAudioDevice( SDL_GetAudioStreamDevice( stream ) );
    pthread_cond_wait( &( a.cond ), &( a.mutex ) );
    pthread_mutex_unlock( &( a.mutex ) );
    SDL_FlushAudioStream( stream );
    SDL_DestroyAudioStream( stream );
    return a.th ? 0 : 1;
}

