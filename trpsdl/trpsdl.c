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
#include "../trppix/trppix_internal.h"

#define TRP_SDL_AUDIO_STREAM 0
#define TRP_SDL_WINDOW 1
#define TRP_SDL_TEXTURE 2
#define TRP_SDL_EVENT 3

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    union {
        void *handle;
        SDL_AudioStream *audio_stream;
        SDL_Window *window;
        SDL_Texture *texture;
        SDL_Event *event;
    };
    union {
        void *aux_handle;
        SDL_Renderer *renderer;
    };
} trp_sdl_t;

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
static uns8b trp_sdl_print( trp_print_t *p, trp_sdl_t *obj );
static uns8b trp_sdl_close( trp_sdl_t *obj );
static uns8b trp_sdl_close_basic( uns8b flags, trp_sdl_t *obj );
static void trp_sdl_finalize( void *obj, void *data );
static trp_obj_t *trp_sdl_create( uns8b sottotipo, void *handle, void *aux_handle );
static SDL_AudioStream *trp_sdl_audio_stream( trp_obj_t *obj );
static SDL_Window *trp_sdl_window( trp_obj_t *obj );
static SDL_Renderer *trp_sdl_renderer( trp_obj_t *obj );
static SDL_Texture *trp_sdl_texture( trp_obj_t *obj );
static SDL_Event *trp_sdl_event( trp_obj_t *obj );
static SDL_Surface *trp_sdl_pix2surface( trp_obj_t *pix );
static void SDLCALL trp_sdl_audio_play_cback( void *userdata, SDL_AudioStream *stream, int len, int total );
static void trp_sdl_audio_play_cback_signal( trp_sdl_audio_play_t *a );

static uns8b _trp_sdl_audio_init = 0;
static uns8b _trp_sdl_video_init = 0;

uns8b trp_sdl_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];

    _trp_print_fun[ TRP_SDL ] = trp_sdl_print;
    _trp_close_fun[ TRP_SDL ] = trp_sdl_close;
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

static uns8b trp_sdl_print( trp_print_t *p, trp_sdl_t *obj )
{
    if ( trp_print_char_star( p, "#sdl" ) )
        return 1;
    switch ( obj->sottotipo ) {
        case TRP_SDL_AUDIO_STREAM:
            if ( trp_print_char_star( p, " audio stream" ) )
                return 1;
            break;
        case TRP_SDL_WINDOW:
            if ( trp_print_char_star( p, " window" ) )
                return 1;
            break;
        case TRP_SDL_TEXTURE:
            if ( trp_print_char_star( p, " texture" ) )
                return 1;
            break;
        case TRP_SDL_EVENT:
            if ( trp_print_char_star( p, " event" ) )
                return 1;
            break;
    }
    if ( obj->handle == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_sdl_close( trp_sdl_t *obj )
{
    return trp_sdl_close_basic( 1, obj );
}

static uns8b trp_sdl_close_basic( uns8b flags, trp_sdl_t *obj )
{
    if ( obj->handle ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        switch ( obj->sottotipo ) {
            case TRP_SDL_AUDIO_STREAM:
                SDL_DestroyAudioStream( obj->audio_stream );
                break;
            case TRP_SDL_WINDOW:
                if ( obj->renderer )
                    SDL_DestroyRenderer( obj->renderer );
                SDL_DestroyWindow( obj->window );
                break;
            case TRP_SDL_TEXTURE:
                SDL_DestroyTexture( obj->texture );
                break;
            case TRP_SDL_EVENT:
                free( obj->handle );
                break;
        }
        obj->handle = NULL;
        obj->aux_handle = NULL;
    }
    return 0;
}

static void trp_sdl_finalize( void *obj, void *data )
{
    trp_sdl_close_basic( 0, (trp_sdl_t *)obj );
}

static trp_obj_t *trp_sdl_create( uns8b sottotipo, void *handle, void *aux_handle )
{
    trp_sdl_t *obj = trp_gc_malloc_atomic_finalize( sizeof( trp_sdl_t ), trp_sdl_finalize );

    obj->tipo = TRP_SDL;
    obj->sottotipo = sottotipo;
    obj->handle = handle;
    obj->aux_handle = aux_handle;
    return (trp_obj_t *)obj;
}

static SDL_AudioStream *trp_sdl_audio_stream( trp_obj_t *obj )
{
    if ( trp_sdl_audio_init() )
        return NULL;
    if ( obj->tipo != TRP_SDL )
        return NULL;
    if ( ((trp_sdl_t *)obj)->sottotipo != TRP_SDL_AUDIO_STREAM )
        return NULL;
    return ((trp_sdl_t *)obj)->audio_stream;
}

static SDL_Window *trp_sdl_window( trp_obj_t *obj )
{
    if ( trp_sdl_video_init() )
        return NULL;
    if ( obj->tipo != TRP_SDL )
        return NULL;
    if ( ((trp_sdl_t *)obj)->sottotipo != TRP_SDL_WINDOW )
        return NULL;
    return ((trp_sdl_t *)obj)->window;
}

static SDL_Renderer *trp_sdl_renderer( trp_obj_t *obj )
{
    if ( trp_sdl_video_init() )
        return NULL;
    if ( obj->tipo != TRP_SDL )
        return NULL;
    if ( ((trp_sdl_t *)obj)->sottotipo != TRP_SDL_WINDOW )
        return NULL;
    return ((trp_sdl_t *)obj)->renderer;
}

static SDL_Texture *trp_sdl_texture( trp_obj_t *obj )
{
    if ( trp_sdl_video_init() )
        return NULL;
    if ( obj->tipo != TRP_SDL )
        return NULL;
    if ( ((trp_sdl_t *)obj)->sottotipo != TRP_SDL_TEXTURE )
        return NULL;
    return ((trp_sdl_t *)obj)->texture;
}

static SDL_Event *trp_sdl_event( trp_obj_t *obj )
{
    if ( trp_sdl_video_init() )
        return NULL;
    if ( obj->tipo != TRP_SDL )
        return NULL;
    if ( ((trp_sdl_t *)obj)->sottotipo != TRP_SDL_EVENT )
        return NULL;
    return ((trp_sdl_t *)obj)->event;
}

static SDL_Surface *trp_sdl_pix2surface( trp_obj_t *pix )
{
    uns8b *p = trp_pix_get_mapp( pix ), *q;
    SDL_Surface *s;
    uns32b w, h, n;

    if ( trp_sdl_video_init() || ( p == NULL ) )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    n = w * h * 4;
    if ( ( q = malloc( n ) ) == NULL )
        return NULL;
    s = SDL_CreateSurfaceFrom( w, h, SDL_PIXELFORMAT_RGBA32, (void *)q, w << 2 );
    if ( s == NULL ) {
        free( q );
        return NULL;
    }
    memcpy( q, p, n );
    return s;
}

uns8b trp_sdl_enable_screen_saver()
{
    if ( trp_sdl_video_init() )
        return 1;
    if ( SDL_ScreenSaverEnabled() == true )
        return 0;
    return ( SDL_EnableScreenSaver() == true ) ? 0 : 1;
}

uns8b trp_sdl_disable_screen_saver()
{
    if ( trp_sdl_video_init() )
        return 1;
    if ( SDL_ScreenSaverEnabled() == false )
        return 0;
    return ( SDL_DisableScreenSaver() == true ) ? 0 : 1;
}

trp_obj_t *trp_sdl_open_audio_stream( trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps )
{
    SDL_AudioStream *stream;
    SDL_AudioSpec audiospec;
    uns32b i;

    if ( trp_sdl_audio_init() )
        return UNDEF;
    memset( &audiospec, 0, sizeof( SDL_AudioSpec ) );
    if ( trp_cast_uns32b( ch, &i ) )
        return UNDEF;
    audiospec.channels = i;
    if ( trp_cast_uns32b( freq, &i ) )
        return UNDEF;
    audiospec.freq = i;
    if ( trp_cast_uns32b( bps, &i ) )
        return UNDEF;
    audiospec.format = i;
    if ( audiospec.format > 8 )
        audiospec.format |= SDL_AUDIO_MASK_SIGNED;
    stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec, NULL, NULL );
    if ( stream == NULL )
        return UNDEF;
    return trp_sdl_create( TRP_SDL_AUDIO_STREAM, (void *)stream, NULL );
}

uns8b trp_sdl_pause_audio_stream_device( trp_obj_t *stream )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );

    if ( s == NULL )
        return 1;
    return ( SDL_PauseAudioStreamDevice( s ) == true ) ? 0 : 1;
}

uns8b trp_sdl_resume_audio_stream_device( trp_obj_t *stream )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );

    if ( s == NULL )
        return 1;
    return ( SDL_ResumeAudioStreamDevice( s ) == true ) ? 0 : 1;
}

uns8b trp_sdl_set_audio_stream_gain( trp_obj_t *stream, trp_obj_t *gain )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );
    flt64b g;

    if ( ( s == NULL ) || trp_cast_flt64b_range( gain, &g, 0.0, 1.0 ) )
        return 1;
    return ( SDL_SetAudioStreamGain( s, (float)g ) == true ) ? 0 : 1;
}

uns8b trp_sdl_flush_audio_stream( trp_obj_t *stream )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );

    if ( s == NULL )
        return 1;
    return ( SDL_FlushAudioStream( s ) == true ) ? 0 : 1;
}

uns8b trp_sdl_clear_audio_stream( trp_obj_t *stream )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );

    if ( s == NULL )
        return 1;
    return ( SDL_ClearAudioStream( s ) == true ) ? 0 : 1;
}

uns8b trp_sdl_put_audio_stream_data( trp_obj_t *stream, trp_obj_t *raw, trp_obj_t *len )
{
    SDL_AudioStream *s = trp_sdl_audio_stream( stream );
    uns32b l;

    if ( ( s == NULL ) || ( raw->tipo != TRP_RAW ) )
        return 1;
    if ( len ) {
        if ( trp_cast_uns32b( len, &l ) )
            return 1;
        if ( l > ((trp_raw_t *)raw)->len )
            return 1;
    } else
        l = ((trp_raw_t *)raw)->len;
    if ( l == 0 )
        return 0;
    return ( SDL_PutAudioStreamData( s, ((trp_raw_t *)raw)->data, l ) == true ) ? 0 : 1;
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
            trp_thread_unregister_my_thread();
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

uns8b trp_sdl_audio_play( trp_obj_t *funptr, trp_obj_t *udata, trp_obj_t *ch, trp_obj_t *freq, trp_obj_t *bps, trp_obj_t *gain )
{
    SDL_AudioStream *stream;
    trp_sdl_audio_play_t a;
    SDL_AudioSpec audiospec;
    uns32b i;
    flt64b g;

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
    if ( gain ) {
        if ( trp_cast_flt64b_range( gain, &g, 0.0, 1.0 ) )
            return 1;
    } else
        g = 1.0;
    a.fun = ((trp_funptr_t *)funptr)->f;
    a.udata = udata;
    a.th = NULL;
    pthread_mutex_init( &( a.mutex ), NULL );
    pthread_cond_init( &( a.cond ), NULL );
    a.run = 1;
    pthread_mutex_lock( &( a.mutex ) );
    stream = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec, trp_sdl_audio_play_cback, (void *)( &a ) );
    SDL_SetAudioStreamGain( stream, (float)g );
    SDL_ResumeAudioStreamDevice( stream );
    pthread_cond_wait( &( a.cond ), &( a.mutex ) );
    pthread_mutex_unlock( &( a.mutex ) );
    SDL_FlushAudioStream( stream );
    /*
     * FIXME
     * sembra che SDL_FlushAudioStream non sia sufficiente a garantire che la riproduzione audio sia terminata
     */
    SDL_Delay( 100 );
    SDL_DestroyAudioStream( stream );
    return a.th ? 0 : 1;
}

trp_obj_t *trp_sdl_create_window_and_renderer( trp_obj_t *title, trp_obj_t *width, trp_obj_t *height, trp_obj_t *flags )
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    uns8b *tit;
    uns32b w, h, fl;

    if ( trp_sdl_video_init() )
        return UNDEF;
    if ( trp_cast_uns32b( width, &w ) || trp_cast_uns32b( height, &h ) || trp_cast_uns32b( flags, &fl ) )
        return UNDEF;
    tit = trp_csprint( title );
    if ( SDL_CreateWindowAndRenderer( tit, w, h, fl, &window, &renderer ) == false ) {
        trp_csprint_free( tit );
        return UNDEF;
    }
    trp_csprint_free( tit );
    return trp_sdl_create( TRP_SDL_WINDOW, (void *)window, (void *)renderer );
}

trp_obj_t *trp_sdl_get_display_for_window( trp_obj_t *window )
{
    SDL_Window *w = trp_sdl_window( window );

    if ( w == NULL )
        return UNDEF;
    return trp_sig64( SDL_GetDisplayForWindow( w ) );
}

trp_obj_t *trp_sdl_get_desktop_display_size( trp_obj_t *display_id )
{
    const SDL_DisplayMode *dm;
    uns32b id;

    if ( trp_cast_uns32b( display_id, &id ) )
        return UNDEF;
    dm = SDL_GetDesktopDisplayMode( (SDL_DisplayID)id );
    if ( dm == NULL )
        return UNDEF;
    return trp_cons( trp_sig64( dm->w ), trp_sig64( dm->h ) );
}

trp_obj_t *trp_sdl_get_current_display_size( trp_obj_t *display_id )
{
    const SDL_DisplayMode *dm;
    uns32b id;

    if ( trp_cast_uns32b( display_id, &id ) )
        return UNDEF;
    dm = SDL_GetCurrentDisplayMode( (SDL_DisplayID)id );
    if ( dm == NULL )
        return UNDEF;
    return trp_cons( trp_sig64( dm->w ), trp_sig64( dm->h ) );
}

trp_obj_t *trp_sdl_get_window_size( trp_obj_t *window )
{
    SDL_Window *w = trp_sdl_window( window );
    int ww, hh;

    if ( w == NULL )
        return UNDEF;
    if ( SDL_GetWindowSize( w, &ww, &hh ) == false )
        return UNDEF;
    return trp_cons( trp_sig64( ww ), trp_sig64( hh ) );
}

trp_obj_t *trp_sdl_get_render_scale( trp_obj_t *window )
{
    SDL_Renderer *r = trp_sdl_renderer( window );
    flt32b scaleX, scaleY;

    if ( r == NULL )
        return UNDEF;
    if ( SDL_GetRenderScale( r, &scaleX, &scaleY ) == false )
        return UNDEF;
    return trp_cons( trp_double( (flt64b)scaleX ), trp_double( (flt64b)scaleY ) );
}

uns8b trp_sdl_set_window_position( trp_obj_t *window, trp_obj_t *x, trp_obj_t *y )
{
    SDL_Window *w = trp_sdl_window( window );
    sig32b xx, yy;

    if ( ( w == NULL ) || trp_cast_sig32b( x, &xx ) || trp_cast_sig32b( y, &yy ) )
        return 1;
    return ( SDL_SetWindowPosition( w, xx, yy ) == true ) ? 0 : 1;
}

uns8b trp_sdl_set_window_size( trp_obj_t *window, trp_obj_t *width, trp_obj_t *height )
{
    SDL_Window *w = trp_sdl_window( window );
    uns32b ww, hh;

    if ( ( w == NULL ) || trp_cast_uns32b( width, &ww ) || trp_cast_uns32b( height, &hh ) )
        return 1;
    return ( SDL_SetWindowSize( w, ww, hh ) == true ) ? 0 : 1;
}

uns8b trp_sdl_set_window_icon( trp_obj_t *window, trp_obj_t *pix )
{
    SDL_Window *w = trp_sdl_window( window );
    SDL_Surface *s;
    bool res;

    if ( w == NULL )
        return 1;
    s = trp_sdl_pix2surface( pix );
    if ( s == NULL )
        return 1;
    res = SDL_SetWindowIcon( w, s );
    SDL_DestroySurface( s );
    return ( res == true ) ? 0 : 1;
}

uns8b trp_sdl_set_window_fullscreen( trp_obj_t *window, trp_obj_t *fs )
{
    SDL_Window *w = trp_sdl_window( window );

    if ( ( w == NULL ) || ( ( fs != TRP_TRUE ) && ( fs != TRP_FALSE ) ) )
        return 1;
    return ( ( SDL_SetWindowFullscreen( w, ( fs == TRP_TRUE ) ? true : false ) ) == true ) ? 0 : 1;
}

uns8b trp_sdl_set_window_aspect_ratio( trp_obj_t *window, trp_obj_t *min_aspect, trp_obj_t *max_aspect )
{
    SDL_Window *w = trp_sdl_window( window );
    flt64b asp_min, asp_max;

    if ( ( w == NULL ) ||
         trp_cast_flt64b_range( min_aspect, &asp_min, 0.01, 100.0 ) ||
         trp_cast_flt64b_range( max_aspect, &asp_max, 0.01, 100.0 ) )
        return 1;
    return ( SDL_SetWindowAspectRatio( w, (float)asp_min, (float)asp_max ) == true ) ? 0 : 1;
}

uns8b trp_sdl_set_render_draw_color( trp_obj_t *window, trp_obj_t *color )
{
    SDL_Renderer *r = trp_sdl_renderer( window );
    uns8b red, green, blue, alpha;

    if ( ( r == NULL ) || trp_pix_decode_color_uns8b( color, NULL, &red, &green, &blue, &alpha ) )
        return 1;
    return ( SDL_SetRenderDrawColor( r, red, green, blue, alpha ) ) ? 0 : 1;
}

trp_obj_t *trp_sdl_create_texture( trp_obj_t *window, trp_obj_t *access, trp_obj_t *width, trp_obj_t *height )
{
    SDL_Renderer *r = trp_sdl_renderer( window );
    SDL_Texture *t;
    uns32b acc, w, h;

    if ( ( r == NULL ) || trp_cast_uns32b( access, &acc ) ||
         trp_cast_uns32b( width, &w ) || trp_cast_uns32b( height, &h ) )
        return UNDEF;
    t = SDL_CreateTexture( r, SDL_PIXELFORMAT_RGBA32, acc, w, h );
    if ( t == NULL )
        return UNDEF;
    return trp_sdl_create( TRP_SDL_TEXTURE, (void *)t, NULL );
}

trp_obj_t *trp_sdl_create_texture_from_surface( trp_obj_t *window, trp_obj_t *pix )
{
    SDL_Renderer *r = trp_sdl_renderer( window );
    SDL_Surface *s;
    SDL_Texture *t;

    if ( r == NULL )
        return UNDEF;
    s = trp_sdl_pix2surface( pix );
    if ( s == NULL )
        return UNDEF;
    t = SDL_CreateTextureFromSurface( r, s );
    SDL_DestroySurface( s );
    if ( t == NULL )
        return UNDEF;
    return trp_sdl_create( TRP_SDL_TEXTURE, (void *)t, NULL );
}

uns8b trp_sdl_render_clear( trp_obj_t *window )
{
    SDL_Renderer *r = trp_sdl_renderer( window );

    if ( r == NULL )
        return 1;
    return ( SDL_RenderClear( r ) == true ) ? 0 : 1;
}

uns8b trp_sdl_render_texture( trp_obj_t *window, trp_obj_t *texture )
{
    SDL_Renderer *r = trp_sdl_renderer( window );
    SDL_Texture *t = trp_sdl_texture( texture );

    if ( ( r == NULL ) || ( t == NULL ) )
        return 1;
    return ( SDL_RenderTexture( r, t, NULL, NULL ) == true ) ? 0 : 1;
}

uns8b trp_sdl_render_present( trp_obj_t *window )
{
    SDL_Renderer *r = trp_sdl_renderer( window );

    if ( r == NULL )
        return 1;
    return ( SDL_RenderPresent( r ) == true ) ? 0 : 1;
}

uns8b trp_sdl_update_texture( trp_obj_t *texture, trp_obj_t *pix )
{
    SDL_Texture *t = trp_sdl_texture( texture );
    uns8b *p = trp_pix_get_mapp( pix ), *q;
    float fw, fh;
    uns32b w, h;
    int pitch;

    if ( ( t == NULL ) || ( p == NULL ) )
        return 1;
    if ( SDL_GetTextureSize( t, &fw, &fh ) == false )
        return 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( w != (uns32b)( fw + 0.5 ) ) || ( h != (uns32b)( fh + 0.5 ) ) )
        return 1;
    if ( SDL_LockTexture( t, NULL, (void **)( &q ), &pitch ) == false )
        return 1;
    w <<= 2;
    if ( w != pitch ) {
        SDL_UnlockTexture( t );
        return 1;
    }
    memcpy( q, p, w * h );
    SDL_UnlockTexture( t );
    return 0;
}

trp_obj_t *trp_sdl_create_event()
{
    SDL_Event *e = malloc( sizeof( SDL_Event ) );

    if ( e == NULL )
        return UNDEF;
    return trp_sdl_create( TRP_SDL_EVENT, (void *)e, NULL );
}

uns8b trp_sdl_poll_event( trp_obj_t *event )
{
    SDL_Event *e = trp_sdl_event( event );

    if ( e == NULL )
        return 1;
    return ( SDL_PollEvent( e ) == true ) ? 0 : 1;
}

trp_obj_t *trp_sdl_event_type( trp_obj_t *event )
{
    SDL_Event *e = trp_sdl_event( event );

    if ( e == NULL )
        return UNDEF;
    return trp_sig64( e->type );
}

trp_obj_t *trp_sdl_event_key_scancode( trp_obj_t *event )
{
    SDL_Event *e = trp_sdl_event( event );

    if ( e == NULL )
        return UNDEF;
    if ( ( e->type != SDL_EVENT_KEY_DOWN ) && ( e->type != SDL_EVENT_KEY_UP ) )
        return UNDEF;
    return trp_sig64( e->key.scancode  );
}

trp_obj_t *trp_sdl_get_ticks()
{
    return trp_uns64( SDL_GetTicks() );
}

trp_obj_t *trp_sdl_get_ticks_ns()
{
    return trp_uns64( SDL_GetTicksNS() );
}

uns8b trp_sdl_delay( trp_obj_t *ms )
{
    uns32b n;

    if ( trp_cast_uns32b( ms, &n ) )
        return 1;
    SDL_Delay( n );
    return 0;
}

uns8b trp_sdl_delay_ns( trp_obj_t *ns )
{
    sig64b n;

    if ( trp_cast_sig64b( ns, &n ) )
        return 1;
    if ( n <= 0 )
        return 0;
    SDL_DelayNS( (uns64b)n );
    return 0;
}

uns8b trp_sdl_delay_precise( trp_obj_t *ns )
{
    sig64b n;

    if ( trp_cast_sig64b( ns, &n ) )
        return 1;
    if ( n <= 0 )
        return 0;
    SDL_DelayPrecise( (uns64b)n );
}

