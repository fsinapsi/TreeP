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
#include <SDL3/SDL.h>

typedef struct {
    uns8b *buf;
    uns32b len;
} trp_sdl_audio_cback_t;

typedef struct {
    uns8b  chunk_id[ 4 ];
    uns32b chunk_size;
    uns16b audio_format;
    uns16b num_channels;
    uns32b sample_rate;
    uns32b byte_rate;
    uns16b block_align;
    uns16b bits_per_sample;
} trp_sdl_wave_header;

static uns8b trp_sdl_check();
static uns8b trp_sdl_raw2audiospec( trp_raw_t *raw, SDL_AudioSpec *audiospec, uns8b **buf, uns32b *len );
static void trp_sdl_play( SDL_AudioSpec *audiospec, trp_sdl_audio_cback_t *a, flt64b vol );
static void SDLCALL trp_sdl_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total_amount );

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

static uns8b trp_sdl_raw2audiospec( trp_raw_t *raw, SDL_AudioSpec *audiospec, uns8b **buf, uns32b *len )
{
    uns8b *p;
    trp_sdl_wave_header *h;
    uns32b maxlen;

    if ( raw->tipo != TRP_RAW )
        return 1;
    if ( raw->len < 44 )
        return 1;
    p = raw->data;
    h = (trp_sdl_wave_header *)p;
    if ( strncmp( h->chunk_id, "RIFF", 4 ) )
        return 1;
    p += 8;
    h = (trp_sdl_wave_header *)p;
    if ( strncmp( h->chunk_id, "WAVE", 4 ) )
        return 1;
    p += 4;
    h = (trp_sdl_wave_header *)p;
    if ( strncmp( h->chunk_id, "fmt ", 4 ) )
        return 1;
    memset( audiospec, 0, sizeof( SDL_AudioSpec ) );
    audiospec->format = h->bits_per_sample;
    if ( h->bits_per_sample > 8 )
        audiospec->format |= SDL_AUDIO_MASK_SIGNED;
    audiospec->freq = h->sample_rate;
    audiospec->channels = h->num_channels;
    for ( ; ; ) {
        p += h->chunk_size + 8;
        if ( ( p - raw->data ) + 8 > raw->len )
            return 1;
        h = (trp_sdl_wave_header *)p;
        if ( strncmp( h->chunk_id, "data", 4 ) == 0 )
            break;
    }
    p += 8;
    maxlen = raw->len - ( p - raw->data );
    *len = h->chunk_size;
    if ( *len > maxlen )
        *len = maxlen;
    *buf = p;
    /*
     printf( "freq = %d, channels = %d, maxlen = %d, len = %d\n", audiospec->freq, audiospec->channels, maxlen, *len );
     */
    return 0;
}

static void trp_sdl_play( SDL_AudioSpec *audiospec, trp_sdl_audio_cback_t *a, flt64b vol )
{
    SDL_AudioStream *stream;

    stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, audiospec, trp_sdl_play_cback, (void *)a );
    SDL_SetAudioStreamGain( stream, (float)vol );
    SDL_ResumeAudioDevice( SDL_GetAudioStreamDevice( stream ) );
    while ( a->len )
        SDL_Delay( 50 );
    SDL_Delay( 350 );
    SDL_DestroyAudioStream( stream );
}

static void SDLCALL trp_sdl_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total_amount )
{
    trp_sdl_audio_cback_t *a = (trp_sdl_audio_cback_t *)userdata;

    if ( a->len ) {
        if ( len > a->len )
            len = a->len;
        SDL_PutAudioStreamData( stream, a->buf, len );
        a->buf += len;
        a->len -= len;
    }
}

uns8b trp_sdl_playwav( trp_obj_t *path, trp_obj_t *volume )
{
    uns8b *cpath;
    uns8b *buf;
    trp_sdl_audio_cback_t a;
    SDL_AudioSpec audiospec;
    flt64b vol;

    if ( trp_sdl_check() )
        return 1;
    if ( volume ) {
        if ( trp_cast_flt64b_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    cpath = trp_csprint( path );
    if( !SDL_LoadWAV( cpath, &audiospec, &( a.buf ), &( a.len ) ) ) {
        trp_csprint_free( cpath );
        return 1;
    }
    trp_csprint_free( cpath );
    buf = a.buf;
    trp_sdl_play( &audiospec, &a, vol );
    SDL_free( buf );
    return 0;
}

uns8b trp_sdl_playwav_memory( trp_obj_t *raw, trp_obj_t *volume )
{
    trp_sdl_audio_cback_t a;
    SDL_AudioSpec audiospec;
    flt64b vol;

    if ( trp_sdl_check() )
        return 1;
    if ( trp_sdl_raw2audiospec( (trp_raw_t *)raw, &audiospec, &( a.buf ), &( a.len ) ) )
        return 1;
    if ( volume ) {
        if ( trp_cast_flt64b_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    trp_sdl_play( &audiospec, &a, vol );
    return 0;
}

