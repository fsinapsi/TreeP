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

typedef struct {
    uns8b  riff_chunk_id[ 4 ];
    uns32b riff_chunk_size;
    uns8b  riff_format[ 4 ];
    uns8b  chunk_id[ 4 ];
    uns32b chunk_size;
    uns16b audio_format;
    uns16b num_channels;
    uns32b sample_rate;
    uns32b byte_rate;
    uns16b block_align;
    uns16b bits_per_sample;
    uns8b  data_id[ 4 ];
    uns32b data_size;
} trp_sdl_wave_header;

static uns8b trp_sdl_raw2audiospec( trp_raw_t *raw, SDL_AudioSpec *wav_spec, uns32b *len );
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

static uns8b trp_sdl_raw2audiospec( trp_raw_t *raw, SDL_AudioSpec *wav_spec, uns32b *len )
{
    trp_sdl_wave_header *h;

    if ( raw->tipo != TRP_RAW )
        return 1;
    if ( raw->len < sizeof( trp_sdl_wave_header ) )
        return 1;
    h = (trp_sdl_wave_header *)( raw->data );
    if ( strncmp( h->riff_chunk_id, "RIFF", 4 ) ||
         strncmp( h->riff_format, "WAVE", 4 ) ||
         strncmp( h->chunk_id, "fmt ", 4 ) ||
         strncmp( h->data_id, "data", 4 ) )
        return 1;
    memset( wav_spec, 0, sizeof( SDL_AudioSpec ) );
    wav_spec->freq = h->sample_rate;
    wav_spec->format = AUDIO_S16LSB;
    wav_spec->channels = h->num_channels;
    wav_spec->samples = 4096;
    *len = h->data_size;
    if ( *len > raw->len - sizeof( trp_sdl_wave_header ) )
        *len = raw->len - sizeof( trp_sdl_wave_header );
    return 0;
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
    double vol;

    if ( volume ) {
        if ( trp_cast_double_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    cpath = trp_csprint( path );
    if( SDL_LoadWAV( cpath, &wav_spec, &( a.buf ), &( a.len ) ) == NULL ) {
        trp_csprint_free( cpath );
        return 1;
    }
    trp_csprint_free( cpath );
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
    SDL_FreeWAV( a.buf );
    return 0;
}

uns8b trp_sdl_playwav_memory( trp_obj_t *raw, trp_obj_t *volume )
{
    trp_sdl_audio_cback_t a;
    SDL_AudioSpec wav_spec;
    double vol;

    if ( trp_sdl_raw2audiospec( (trp_raw_t *)raw, &wav_spec, &( a.len ) ) )
        return 1;
    if ( volume ) {
        if ( trp_cast_double_range( volume, &vol, 0.0, 1.0 ) )
            return 1;
    } else
        vol = 1.0;
    a.buf = ( (trp_raw_t *)raw )->data + sizeof( trp_sdl_wave_header );
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
    return 0;
}



