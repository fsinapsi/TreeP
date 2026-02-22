/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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
#include "../trppix/trppix_internal.h"
#include "./trpavcodec.h"
#include <libavutil/imgutils.h>

#if 1
#define TRP_AV_LOCK(fmtctx) if(((trp_avcodec_t *)(fmtctx))->accel)pthread_mutex_lock(&_trp_av_mutex)
#define TRP_AV_UNLOCK(fmtctx) if(((trp_avcodec_t *)(fmtctx))->accel)pthread_mutex_unlock(&_trp_av_mutex)
#else
#define TRP_AV_LOCK(fmtctx)
#define TRP_AV_UNLOCK(fmtctx)
#endif

#if 0
#define PRINT_BEGIN(msg,frameno) trp_av_print_begin(msg,frameno)
#define PRINT_END(msg) trp_av_print_end(msg)
#define PRINT_BUF(fmtctx) trp_av_print_buf((trp_avcodec_t *)(fmtctx))
#else
#define PRINT_BEGIN(msg,frameno)
#define PRINT_END(msg)
#define PRINT_BUF(fmtctx)
#endif

#define TRP_AV_FILTERS_ENABLED

typedef struct {
    AVFrame *frame;
    uns32b   fmin;
    uns32b   fmax;
    uns32b   skip;
    uns8b    status;
} trp_avcodec_buf_t;

typedef struct {
#ifdef TRP_AV_FILTERS_ENABLED
    AVFilterContext      *buffersink_ctx;
    AVFilterContext      *buffersrc_ctx;
    AVFilterGraph        *filter_graph;
#endif
    trp_obj_t            *descr;
} trp_avcodec_filter_t;

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    struct SwsContext    *sws_ctx;
    uns32b wi;
    uns32b hi;
    uns32b wo;
    uns32b ho;
} trp_avcodec_sws_t;

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    AVFormatContext      *fmt_ctx;
    AVCodecContext       *avctx;
    AVPacket             *packet;
    struct SwsContext    *sws_ctx;
    AVBufferRef          *hw_device_ref;
    AVFrame              *hw_frame;
    trp_avcodec_filter_t *filter;
    trp_avcodec_buf_t    *cbuf;
    trp_obj_t            *path;
    trp_obj_t            *video_time_base;
    trp_obj_t            *original_video_frame_rate;
    trp_obj_t            *video_frame_rate;
    trp_obj_t            *original_dar;
    trp_obj_t            *dar;
    trp_obj_t            *video_duration;
    trp_obj_t            *first_ts;
    trp_obj_t            *second_ts;
    trp_obj_t            *gamma;
    trp_obj_t            *contrast;
    trp_obj_t            *hue;
    trp_obj_t            *temperature;
    uns8b                *last_error_fun;
    enum AVPixelFormat    hw_pix_fmt;
    int                   last_error;
    uns32b                buf_max;
    uns32b                buf_cur;
    uns32b                video_stream_idx;
    uns8b                 accel;
    uns8b                 filter_rows;
    uns8b                 hflip;
    uns8b                 vflip;
    uns8b                 ignore_invalid_data;
    uns8b                 debug;
} trp_avcodec_t;

struct AVDictionary {
    int count;
    AVDictionaryEntry *elems;
};

static pthread_mutex_t _trp_av_mutex;

#define TRP_AV_BUFSIZE 2 /* almeno 2 */
#define TRP_AV_FRAMENO_UNDEF 0xffffffff

#define TRP_AV_ACCEL_NONE   0
#define TRP_AV_ACCEL_QSV    1
#define TRP_AV_ACCEL_AMF    2
#define TRP_AV_ACCEL_CUVID  3

#define trp_av_equal(x,y) (trp_equal(x,y)==TRP_TRUE)
#define trp_av_notequal(x,y) (trp_equal(x,y)==TRP_FALSE)
#define trp_av_less(x,y) (trp_less(x,y)==TRP_TRUE)
#define trp_av_greater(x,y) (trp_less(y,x)==TRP_TRUE)
#define trp_av_less_or_equal(x,y) (trp_less(y,x)==TRP_FALSE)
#define trp_av_greater_or_equal(x,y) (trp_less(x,y)==TRP_FALSE)
#define trp_av_plus(x,y) trp_cat(x,y,NULL)
#define trp_av_minus(x,y) trp_math_minus(x,y,NULL)
#define trp_av_times(x,y) trp_math_times(x,y,NULL)
#define trp_av_ratio(x,y) trp_math_ratio(x,y,NULL)

#define trp_av_next_buf_idx(fmtctx,idx) (((idx)+1)%((trp_avcodec_t *)(fmtctx))->buf_max)
#define trp_av_prev_buf_idx(fmtctx,idx) (((idx)?(idx):((trp_avcodec_t *)(fmtctx))->buf_max)-1)
#define trp_av_next_buf_cur(fmtctx) trp_av_next_buf_idx(fmtctx,((trp_avcodec_t *)(fmtctx))->buf_cur)
#define trp_av_prev_buf_cur(fmtctx) trp_av_prev_buf_idx(fmtctx,((trp_avcodec_t *)(fmtctx))->buf_cur)

static void trp_av_log_default_callback( void *ptr, int level, const char *fmt, va_list vl );
static uns8b trp_av_print( trp_print_t *p, trp_avcodec_t *obj );
static uns8b trp_av_close( trp_avcodec_t *obj );
static uns8b trp_av_close_basic( uns8b flags, trp_avcodec_t *obj );
static void trp_av_finalize( void *obj, void *data );
static void trp_av_close_filter( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_width( trp_avcodec_t *obj );
static trp_obj_t *trp_av_height( trp_avcodec_t *obj );
static trp_obj_t *trp_av_length( trp_avcodec_t *obj );

static trp_obj_t *trp_av_codec_name_list( int (*x)(const AVCodec *) );

static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx );
static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_sws_t *swsctx );

static trp_obj_t *trp_av_rational( struct AVRational *r );
static trp_obj_t *trp_av_ts_sig64_to_trp( trp_avcodec_t *fmtctx, sig64b ts );
static sig64b trp_av_ts_trp_to_sig64( trp_avcodec_t *fmtctx, trp_obj_t *ts );
static trp_obj_t *trp_av_pts( trp_avcodec_t *fmtctx, AVFrame *frame );
static trp_obj_t *trp_av_error( int errnum );
static void trp_av_frame_unref( trp_avcodec_buf_t *cbuf, uns32b idx );
static void trp_av_buf_insert_blank( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_frameno2ts( trp_avcodec_t *fmtctx, uns32b frameno );
static uns32b trp_av_ts2frameno( trp_avcodec_t *fmtctx, trp_obj_t *ts );
static trp_obj_t *trp_av_get_frame_rate( trp_avcodec_t *fmtctx );
static void trp_av_set_frame_rate( trp_avcodec_t *fmtctx, trp_obj_t *framerate );
static uns32b trp_av_last_frameno( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_last_pts( trp_avcodec_t *fmtctx );

static uns8b trp_av_read_frame_no_seek( trp_avcodec_t *fmtctx, uns32b bufidx, uns32b nframes, uns32b expected_frameno, trp_obj_t **ts );
static uns8b trp_av_read_frame_and_fix_buf( trp_avcodec_t *fmtctx, uns32b nframes, uns32b expected_frameno );
static uns8b trp_av_seek_and_read_frame( trp_avcodec_t *fmtctx, uns32b frameno, int flags );
static uns8b trp_av_read_frame_frameno( trp_avcodec_t *fmtctx, uns32b frameno, AVFrame **frame );

static trp_obj_t *trp_av_avformat_open_input_low( uns8b flags, trp_obj_t *path, trp_obj_t *hwaccel );
static void trp_av_read_frame_free( AVFrame *frameo, AVFrame *filt_rows, AVFrame *filt0_frame, AVFrame *filt1_frame );
static trp_obj_t *trp_av_metadata_low( AVDictionary *metadata );

static void trp_av_print_begin( uns8b *msg, uns32b frameno );
static void trp_av_print_end( uns8b *msg );
static void trp_av_print_buf( trp_avcodec_t *fmtctx );

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

uns8b trp_av_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];
    extern objfun_t _trp_length_fun[];
    pthread_mutexattr_t attr;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &_trp_av_mutex, &attr );
    pthread_mutexattr_destroy( &attr );
    _trp_print_fun[ TRP_AVCODEC ] = trp_av_print;
    _trp_close_fun[ TRP_AVCODEC ] = trp_av_close;
    _trp_width_fun[ TRP_AVCODEC ] = trp_av_width;
    _trp_height_fun[ TRP_AVCODEC ] = trp_av_height;
    _trp_length_fun[ TRP_AVCODEC ] = trp_av_length;
    av_log_set_callback( trp_av_log_default_callback );
    return 0;
}

void trp_av_quit()
{
    pthread_mutex_destroy( &_trp_av_mutex );
}

static void trp_av_log_default_callback( void *ptr, int level, const char *fmt, va_list vl )
{
}

static uns8b trp_av_print( trp_print_t *p, trp_avcodec_t *obj )
{
    if ( trp_print_char_star( p, ( obj->sottotipo == 0 ) ? "#sws context" : ( ( obj->sottotipo == 1 ) ? "#fmt context" : "#avcodec" ) ) )
        return 1;
    if ( obj->fmt_ctx == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_av_close( trp_avcodec_t *obj )
{
    return trp_av_close_basic( 1, obj );
}

static uns8b trp_av_close_basic( uns8b flags, trp_avcodec_t *obj )
{
    uns32b cnt;

    if ( obj->fmt_ctx ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        switch ( obj->sottotipo ) {
            case 0:
                sws_freeContext( ((trp_avcodec_sws_t *)obj)->sws_ctx );
                break;
            case 1:
                avformat_close_input( &obj->fmt_ctx );
                avcodec_free_context( &obj->avctx );
                av_packet_free( &obj->packet );
                if ( obj->sws_ctx )
                    sws_freeContext( obj->sws_ctx );
                if ( obj->hw_device_ref )
                    av_buffer_unref( &obj->hw_device_ref );
                if ( obj->hw_frame )
                    av_frame_free( &obj->hw_frame );
                trp_av_close_filter( obj );
                for ( cnt = 0 ; cnt < obj->buf_max ; cnt++ ) {
                    trp_av_frame_unref( obj->cbuf, cnt );
                    av_frame_free( &( obj->cbuf[ cnt ].frame ) );
                }
                trp_gc_free( obj->cbuf );
                obj->path = NULL;
                obj->video_time_base = NULL;
                obj->original_video_frame_rate = NULL;
                obj->video_frame_rate = NULL;
                obj->original_dar = NULL;
                obj->dar = NULL;
                obj->video_duration = NULL;
                obj->first_ts = NULL;
                obj->second_ts = NULL;
                obj->gamma = NULL;
                obj->contrast = NULL;
                obj->hue = NULL;
                obj->temperature = NULL;
                break;
        }
        obj->fmt_ctx = NULL;
    }
    return 0;
}

static void trp_av_finalize( void *obj, void *data )
{
    trp_av_close_basic( 0, (trp_avcodec_t *)obj );
}

static void trp_av_close_filter( trp_avcodec_t *fmtctx )
{
#ifdef TRP_AV_FILTERS_ENABLED
    if ( fmtctx->filter ) {
        avfilter_graph_free( &fmtctx->filter->filter_graph );
        fmtctx->filter = NULL;
    }
#endif
}

static trp_obj_t *trp_av_width( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->fmt_ctx )
        switch ( obj->sottotipo ) {
        case 1:
            res = trp_sig64( obj->fmt_ctx->streams[ obj->video_stream_idx ]->codecpar->width );
            break;
        }
    return res;
}

static trp_obj_t *trp_av_height( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->fmt_ctx )
        switch ( obj->sottotipo ) {
        case 1:
            res = trp_sig64( obj->fmt_ctx->streams[ obj->video_stream_idx ]->codecpar->height );
            break;
        }
    return res;
}

static trp_obj_t *trp_av_length( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->fmt_ctx )
        switch ( obj->sottotipo ) {
        case 1:
            res = trp_sig64( obj->fmt_ctx->nb_streams );
            break;
        }
    return res;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_format_version()
{
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             LIBAVFORMAT_VERSION_MAJOR,
             LIBAVFORMAT_VERSION_MINOR,
             LIBAVFORMAT_VERSION_MICRO );
    return trp_cord( buf );
}

trp_obj_t *trp_av_codec_version()
{
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             LIBAVCODEC_VERSION_MAJOR,
             LIBAVCODEC_VERSION_MINOR,
             LIBAVCODEC_VERSION_MICRO );
    return trp_cord( buf );
}

trp_obj_t *trp_av_util_version()
{
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             LIBAVUTIL_VERSION_MAJOR,
             LIBAVUTIL_VERSION_MINOR,
             LIBAVUTIL_VERSION_MICRO );
    return trp_cord( buf );
}

trp_obj_t *trp_av_swscale_version()
{
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             LIBSWSCALE_VERSION_MAJOR,
             LIBSWSCALE_VERSION_MINOR,
             LIBSWSCALE_VERSION_MICRO );
    return trp_cord( buf );
}

trp_obj_t *trp_av_avcodec_version()
{
    int v = avcodec_version();
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             AV_VERSION_MAJOR( v ),
             AV_VERSION_MINOR( v ),
             AV_VERSION_MICRO( v ) );
    return trp_cord( buf );
}

trp_obj_t *trp_av_avcodec_configuration()
{
    return trp_cord( avcodec_configuration() );
}

trp_obj_t *trp_av_avcodec_license()
{
    return trp_cord( avcodec_license() );
}

trp_obj_t *trp_av_avcodec_list()
{
    trp_obj_t *l = NIL;
    const AVCodecDescriptor *desc = NULL;

    while ( ( desc = avcodec_descriptor_next( desc ) ) ) {
        l = trp_cons( trp_list( trp_sig64( desc->type ),
                                trp_cord( desc->name ),
                                trp_cord( desc->long_name ),
                                NULL ),
                      l );
    }
    return l;
}

static trp_obj_t *trp_av_codec_name_list( int (*x)(const AVCodec *) )
{
    trp_obj_t *res = NIL;
    void *i = NULL;
    const AVCodec *p;

    while ( ( p = av_codec_iterate( &i ) ) ) {
        if ( x( p ) )
            res = trp_cons( trp_cord( p->name ), res );
    }
    return res;
}

trp_obj_t *trp_av_decoder_name_list()
{
    return trp_av_codec_name_list( av_codec_is_decoder );
}

trp_obj_t *trp_av_encoder_name_list()
{
    return trp_av_codec_name_list( av_codec_is_encoder );
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx )
{
    if ( fmtctx->tipo == TRP_AVCODEC )
        if ( fmtctx->sottotipo == 1 )
            return fmtctx->fmt_ctx;
    return NULL;
}

static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_sws_t *swsctx )
{
    if ( swsctx->tipo == TRP_AVCODEC )
        if ( swsctx->sottotipo == 0 )
            return swsctx->sws_ctx;
    return NULL;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

static trp_obj_t *trp_av_rational( struct AVRational *r )
{
    return trp_av_ratio( trp_sig64( r->num ), trp_sig64( r->den ) );
}

static trp_obj_t *trp_av_ts_sig64_to_trp( trp_avcodec_t *fmtctx, sig64b ts )
{
    if ( ts == AV_NOPTS_VALUE )
        return UNDEF;
    return trp_av_times( trp_sig64( ts ), fmtctx->video_time_base );
}

static sig64b trp_av_ts_trp_to_sig64( trp_avcodec_t *fmtctx, trp_obj_t *ts )
{
    if ( ts == UNDEF )
        return AV_NOPTS_VALUE;
    return ( (trp_sig64_t *)( trp_math_rint( trp_av_ratio( ts, fmtctx->video_time_base ) ) ) )->val;
}

static trp_obj_t *trp_av_pts( trp_avcodec_t *fmtctx, AVFrame *frame )
{
    if ( frame->pkt_dts == AV_NOPTS_VALUE )
        return UNDEF;
    return trp_av_ts_sig64_to_trp( fmtctx, frame->pkt_dts );
}

static trp_obj_t *trp_av_error( int errnum )
{
    uns8b msg[ AV_ERROR_MAX_STRING_SIZE ];

    if ( errnum == 0 )
        return trp_cord( "no error" );
    av_make_error_string( msg, AV_ERROR_MAX_STRING_SIZE, errnum );
    return trp_cord( msg );
}

static void trp_av_frame_unref( trp_avcodec_buf_t *cbuf, uns32b idx )
{
    if ( cbuf[ idx ].status ) {
        av_frame_unref( cbuf[ idx ].frame );
        cbuf[ idx ].status = 0;
        cbuf[ idx ].fmin = cbuf[ idx ].fmax = TRP_AV_FRAMENO_UNDEF;
        cbuf[ idx ].skip = 0;
    }
}

static void trp_av_buf_insert_blank( trp_avcodec_t *fmtctx )
{
    fmtctx->buf_cur = trp_av_next_buf_cur( fmtctx );
    trp_av_frame_unref( fmtctx->cbuf, fmtctx->buf_cur );
}

static trp_obj_t *trp_av_frameno2ts( trp_avcodec_t *fmtctx, uns32b frameno )
{
    trp_obj_t *ts;

    if ( frameno == 0 )
        ts = fmtctx->first_ts;
    else
        ts = trp_av_plus( fmtctx->second_ts, trp_av_times( trp_sig64( frameno - 1 ), fmtctx->video_duration ) );
    return ts;
}

static uns32b trp_av_ts2frameno( trp_avcodec_t *fmtctx, trp_obj_t *ts )
{
    uns32b frameno;

    if ( ts == UNDEF )
        return TRP_AV_FRAMENO_UNDEF;
    if ( trp_av_less( ts, fmtctx->second_ts ) ) {
        frameno = ( trp_av_less( ts, trp_av_ratio( trp_av_plus( fmtctx->first_ts, fmtctx->second_ts ), trp_sig64( 2 ) ) ) ) ? 0 : 1;
    } else {
        frameno = 1 + (uns32b)( ((trp_sig64_t *)trp_math_rint( trp_av_ratio(
                                trp_av_minus( ts, fmtctx->second_ts ),
                                fmtctx->video_duration ) ) )->val );
    }
    return frameno;
}

static trp_obj_t *trp_av_get_frame_rate( trp_avcodec_t *fmtctx )
{
    trp_obj_t *avg_fr = trp_av_rational( &( fmtctx->fmt_ctx->streams[ fmtctx->video_stream_idx ]->avg_frame_rate ) );
    trp_obj_t *r_fr = trp_av_rational( &( fmtctx->fmt_ctx->streams[ fmtctx->video_stream_idx ]->r_frame_rate ) );

    if ( avg_fr != UNDEF )
        return avg_fr;
    if ( r_fr != UNDEF )
        return r_fr;
    return trp_sig64( 24 );
}

static void trp_av_set_frame_rate( trp_avcodec_t *fmtctx, trp_obj_t *framerate )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    uns32b idx;

    fmtctx->video_frame_rate = framerate;
    fmtctx->video_duration = trp_av_ratio( UNO, framerate );
    for ( idx = 0 ; idx < fmtctx->buf_max ; idx++ )
        trp_av_frame_unref( cbuf, idx );
}

static uns32b trp_av_last_frameno( trp_avcodec_t *fmtctx )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;

    if ( cbuf[ fmtctx->buf_cur ].status == 0 )
        return TRP_AV_FRAMENO_UNDEF;
    return cbuf[ fmtctx->buf_cur ].fmax;
}

static trp_obj_t *trp_av_last_pts( trp_avcodec_t *fmtctx )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;

    if ( cbuf[ fmtctx->buf_cur ].status == 0 )
        return UNDEF;
    return trp_av_pts( fmtctx, cbuf[ fmtctx->buf_cur ].frame );
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

static uns8b trp_av_read_frame_no_seek( trp_avcodec_t *fmtctx, uns32b bufidx, uns32b nframes, uns32b expected_frameno, trp_obj_t **ts )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( fmtctx );
    AVCodecContext *avctx = fmtctx->avctx;
    AVPacket *packet = fmtctx->packet;
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    AVFrame *frame = cbuf[ bufidx ].frame, *read_frame;
    sig64b pkt_dts = AV_NOPTS_VALUE;
    uns32b video_stream_idx = fmtctx->video_stream_idx;
    uns32b read = 0;
    int must_read = 0, res;

    TRP_AV_LOCK( fmtctx );
    PRINT_BEGIN( "trp_av_read_frame_no_seek", expected_frameno );
    read_frame = fmtctx->accel ? fmtctx->hw_frame : frame;
    trp_av_frame_unref( cbuf, bufidx );
    for ( ; ; ) {
        if ( must_read ) {
            res = av_read_frame( fmt_ctx, packet );
            if ( res < 0 ) {
                res = avcodec_send_packet( avctx, NULL );
            } else {
                if ( packet->stream_index != video_stream_idx ) {
                    av_packet_unref( packet );
                    continue;
                }
                res = avcodec_send_packet( avctx, packet );
                pkt_dts = packet->dts;
                av_packet_unref( packet );
            }
            if ( ( res == AVERROR_INVALIDDATA ) && fmtctx->ignore_invalid_data ) {
                fmtctx->last_error_fun = "avcodec_send_packet - ignored";
                fmtctx->last_error = res;
                continue;
            }
            if ( res < 0 ) {
                fmtctx->last_error_fun = "avcodec_send_packet";
                goto error;
            }
        }
        res = avcodec_receive_frame( avctx, read_frame );
        if ( res == AVERROR(EAGAIN) ) {
            must_read++;
            continue;
        }
        if ( res < 0 ) {
            fmtctx->last_error_fun = "avcodec_receive_frame";
            goto error;
        }
        if ( ++read == nframes )
            break;
        av_frame_unref( read_frame );
        must_read = 0;
    }
    if ( fmtctx->debug ) {
        if ( must_read == 0 ) {
            fprintf( stderr, "must_read = %d\n", must_read );
        }
    }
    if ( fmtctx->accel ) {
        fmtctx->hw_pix_fmt = fmtctx->hw_frame->format;
        res = av_hwframe_transfer_data( frame, fmtctx->hw_frame, 0 );
        av_frame_unref( fmtctx->hw_frame );
        if ( res < 0 ) {
            av_frame_unref( frame );
            fmtctx->last_error_fun = "av_hwframe_transfer_data";
            goto error;
        }
    }
    cbuf[ bufidx ].status = 1;
    if ( frame->pkt_dts == AV_NOPTS_VALUE )
        frame->pkt_dts = pkt_dts;
    if ( ( frame->pkt_dts == AV_NOPTS_VALUE ) ||
         ( ( must_read == 0 ) && ( frame->pkt_dts < frame->pts ) ) )
        frame->pkt_dts = frame->pts;
    if ( frame->pkt_dts == AV_NOPTS_VALUE ) {
        switch ( expected_frameno ) {
            case 0:
                frame->pkt_dts = 0;
                break;
            case 1:
                frame->pkt_dts = trp_av_ts_trp_to_sig64( fmtctx, fmtctx->video_duration );
                break;
            default:
                break;
        }
        fmtctx->last_error_fun = "AV_NOPTS_VALUE";
        fmtctx->last_error = -1;
    } else {
        fmtctx->last_error_fun = NULL;
        fmtctx->last_error = 0;
    }
    *ts = trp_av_pts( fmtctx, frame );
    PRINT_END( "trp_av_read_frame_no_seek (success)" );
    TRP_AV_UNLOCK( fmtctx );
    return 0;
error:
    fmtctx->last_error = res;
    PRINT_END( "trp_av_read_frame_no_seek (error)" );
    TRP_AV_UNLOCK( fmtctx );
    return 1;
}

static uns8b trp_av_read_frame_and_fix_buf( trp_avcodec_t *fmtctx, uns32b nframes, uns32b expected_frameno )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    trp_obj_t *ts;
    uns32b idx = trp_av_next_buf_cur( fmtctx );
    uns32b prev_idx = fmtctx->buf_cur;
    uns32b frameno, cnt, dup, skp;

    TRP_AV_LOCK( fmtctx );
    PRINT_BEGIN( "trp_av_read_frame_and_fix_buf", expected_frameno );
    for ( cnt = 0, skp = 0 ; ; nframes = 1, expected_frameno = TRP_AV_FRAMENO_UNDEF ) {
        if ( trp_av_read_frame_no_seek( fmtctx, idx, nframes, expected_frameno, &ts ) )
            goto error;
        if ( ts == UNDEF ) {
            if ( fmtctx->debug )
                fprintf( stderr, "skipping frame (AV_NOPTS_VALUE)\n" );
            cnt += nframes;
            if ( cnt >= 10 )
                goto error;
            if ( cbuf[ prev_idx ].status )
                cbuf[ prev_idx ].skip++;
        } else {
            frameno = trp_av_ts2frameno( fmtctx, ts );
            if ( cbuf[ prev_idx ].status == 0 )
                break;
            if ( frameno > cbuf[ prev_idx ].fmax ) {
                dup = frameno - cbuf[ prev_idx ].fmax - 1;
                if ( dup ) {
                    if ( fmtctx->debug )
                        fprintf( stderr, "duplicate frames (%u)\n", dup );
                    cbuf[ prev_idx ].fmax = frameno - 1;
                }
                break;
            }
            skp++;
            cbuf[ prev_idx ].skip++;
        }
    }
    if ( skp )
        if ( fmtctx->debug )
            fprintf( stderr, "skipping frames (%u)\n", skp );
    cbuf[ idx ].fmin = cbuf[ idx ].fmax = frameno;
    fmtctx->buf_cur = idx;
//    PRINT_BUF( fmtctx );
    PRINT_END( "trp_av_read_frame_and_fix_buf (success)" );
    TRP_AV_UNLOCK( fmtctx );
    return 0;
error:
    trp_av_buf_insert_blank( fmtctx );
//    PRINT_BUF( fmtctx );
    PRINT_END( "trp_av_read_frame_and_fix_buf (error)" );
    TRP_AV_UNLOCK( fmtctx );
    return 1;
}

static uns8b trp_av_seek_and_read_frame( trp_avcodec_t *fmtctx, uns32b frameno, int flags )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( fmtctx );
    trp_obj_t *ts;
    sig64b tts;
    uns32b nframes;
    int res;

    TRP_AV_LOCK( fmtctx );
    PRINT_BEGIN( "trp_av_seek_and_read_frame", frameno );
    switch ( fmtctx->accel ) {
        case TRP_AV_ACCEL_NONE:
            nframes = 1;
            break;
        default:
            nframes = 6;
            frameno = ( frameno > 5 ) ? frameno - 5 : 0;
            break;
    }
    if ( frameno < 5 ) {
        nframes = 1;
        frameno = 0;
        ts = ZERO;
    } else {
        ts = trp_av_frameno2ts( fmtctx, frameno );
    }
    tts = trp_av_ts_trp_to_sig64( fmtctx, ts );
    trp_av_buf_insert_blank( fmtctx );
    avcodec_flush_buffers( fmtctx->avctx );
    res = av_seek_frame( fmt_ctx, fmtctx->video_stream_idx, tts, flags );
    if ( res < 0 ) {
        res = av_seek_frame( fmt_ctx, fmtctx->video_stream_idx, tts, 1 - flags );
        if ( res < 0 ) {
            /*
            printf( "av_seek_frame failed: ts = " );
            trp_print( ts, NULL );
            printf( " -> %lld (flags=%d)\n", tts, flags );
            */
            fmtctx->last_error_fun = "av_seek_frame";
            fmtctx->last_error = res;
            goto error;
        }
    }
    if ( trp_av_read_frame_and_fix_buf( fmtctx, nframes, frameno ) )
        goto error;
    PRINT_END( "trp_av_seek_and_read_frame (success)" );
    TRP_AV_UNLOCK( fmtctx );
    return 0;
error:
    PRINT_END( "trp_av_seek_and_read_frame (error)" );
    TRP_AV_UNLOCK( fmtctx );
    return 1;
}

static uns8b trp_av_read_frame_frameno( trp_avcodec_t *fmtctx, uns32b frameno, AVFrame **frame )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    uns32b idx = trp_av_next_buf_cur( fmtctx );

    TRP_AV_LOCK( fmtctx );
//    printf( "richiesta lettura del frame %u\n", frameno );
    PRINT_BEGIN( "trp_av_read_frame_frameno", frameno );
//    PRINT_BUF( fmtctx );

    for ( ; ; idx = trp_av_next_buf_idx( fmtctx, idx ) ) {
        if ( cbuf[ idx ].status ) {
            if ( ( frameno >= cbuf[ idx ].fmin ) && ( frameno <= cbuf[ idx ].fmax ) )
                goto success;
        }
        if ( idx == fmtctx->buf_cur )
            break;
    }
    if ( ( cbuf[ fmtctx->buf_cur ].fmin > frameno ) ||
         ( cbuf[ fmtctx->buf_cur ].fmax + 70 < frameno ) ) {
        for ( idx = frameno ; ; idx = ( idx > 50 ) ? idx - 50 : 0 ) {
            if ( trp_av_seek_and_read_frame( fmtctx, idx, AVSEEK_FLAG_BACKWARD ) )
                goto error2;
            if ( cbuf[ fmtctx->buf_cur ].fmin <= frameno )
                break;
            if ( idx == 0 )
                goto error1;
        }
    }
    for ( ; ; ) {
        idx = fmtctx->buf_cur;
        if ( ( frameno >= cbuf[ idx ].fmin ) && ( frameno <= cbuf[ idx ].fmax ) )
            break;
        if ( frameno < cbuf[ idx ].fmin )
            goto error1;
        if ( trp_av_read_frame_and_fix_buf( fmtctx, 1, frameno ) )
            goto error2;
        if ( ( frameno >= cbuf[ idx ].fmin ) && ( frameno <= cbuf[ idx ].fmax ) )
            break;
    }
success:
    *frame = fmtctx->cbuf[ idx ].frame;
    PRINT_END( "trp_av_read_frame_frameno (success)" );
    TRP_AV_UNLOCK( fmtctx );
    return 0;
error1:
    trp_av_buf_insert_blank( fmtctx );
    fmtctx->last_error_fun = "error in seeking";
    fmtctx->last_error = -1;
error2:
    PRINT_END( "trp_av_read_frame_frameno (error)" );
    TRP_AV_UNLOCK( fmtctx );
    return 1;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_avformat_open_input( trp_obj_t *path, trp_obj_t *hwaccel )
{
    return trp_av_avformat_open_input_low( 0, path, hwaccel );
}

trp_obj_t *trp_av_avformat_open_input_failure_cause( trp_obj_t *path, trp_obj_t *hwaccel )
{
    return trp_av_avformat_open_input_low( 1, path, hwaccel );
}

static trp_obj_t *trp_av_avformat_open_input_low( uns8b flags, trp_obj_t *path, trp_obj_t *hwaccel )
{
    trp_avcodec_t *obj;
    trp_obj_t *failure_cause = UNDEF;
    uns8b *cpath;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecParameters *origin_par;
    const AVCodec *codec;
    AVCodecContext *avctx = NULL;
    AVBufferRef *hw_device_ref = NULL;
    AVFrame *hw_frame = NULL;
    AVPacket *packet;
    enum AVHWDeviceType hw_type;
    uns32b video_stream_idx, cnt;
    int res;
    uns8b accel;

    if ( hwaccel ) {
        if ( trp_cast_uns32b_range( hwaccel, &cnt, TRP_AV_ACCEL_NONE, TRP_AV_ACCEL_CUVID ) ) {
            if ( flags & 1 )
                failure_cause = trp_cons( trp_cord( "accel" ), trp_cord( "invalid parameter" ) );
            return failure_cause;
        }
        accel = (uns8b)cnt;
    } else
        accel = TRP_AV_ACCEL_NONE;
    cpath = trp_csprint( path );
    res = avformat_open_input( &fmt_ctx, cpath, NULL, NULL );
    trp_csprint_free( cpath );
    if ( res ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avformat_open_input" ), trp_av_error( res ) );
        return failure_cause;
    }
    res = avformat_find_stream_info( fmt_ctx, NULL );
    if ( res < 0 ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avformat_find_stream_info" ), trp_av_error( res ) );
        goto error1;
    }
    res = av_find_best_stream( fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0 );
    if ( res < 0 ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "av_find_best_stream" ), trp_av_error( res ) );
        goto error1;
    }
    video_stream_idx = res;
    origin_par = fmt_ctx->streams[ video_stream_idx ]->codecpar;
    switch ( accel ) {
        case TRP_AV_ACCEL_NONE:
            codec = avcodec_find_decoder( origin_par->codec_id );
            break;
        case TRP_AV_ACCEL_QSV:
            hw_type = AV_HWDEVICE_TYPE_QSV;
            switch ( origin_par->codec_id ) {
                case AV_CODEC_ID_AV1:
                    codec = avcodec_find_decoder_by_name( "av1_qsv" );
                    break;
                case AV_CODEC_ID_H264:
                    codec = avcodec_find_decoder_by_name( "h264_qsv" );
                    break;
                case AV_CODEC_ID_HEVC:
                    codec = avcodec_find_decoder_by_name( "hevc_qsv" );
                    break;
                case AV_CODEC_ID_MJPEG:
                    codec = avcodec_find_decoder_by_name( "mjpeg_qsv" );
                    break;
                case AV_CODEC_ID_MPEG2VIDEO:
                    codec = avcodec_find_decoder_by_name( "mpeg2_qsv" );
                    break;
                case AV_CODEC_ID_VC1:
                    codec = avcodec_find_decoder_by_name( "vc1_qsv" );
                    break;
                case AV_CODEC_ID_VP8:
                    codec = avcodec_find_decoder_by_name( "vp8_qsv" );
                    break;
                case AV_CODEC_ID_VP9:
                    codec = avcodec_find_decoder_by_name( "vp9_qsv" );
                    break;
                case AV_CODEC_ID_VVC:
                    codec = avcodec_find_decoder_by_name( "vvc_qsv" );
                    break;
                default:
                    codec = avcodec_find_decoder( origin_par->codec_id );
                    accel = TRP_AV_ACCEL_NONE;
                    break;
            }
            break;
        case TRP_AV_ACCEL_AMF:
            hw_type = AV_HWDEVICE_TYPE_AMF;
            switch ( origin_par->codec_id ) {
                case AV_CODEC_ID_AV1:
                    codec = avcodec_find_decoder_by_name( "av1_amf" );
                    break;
                case AV_CODEC_ID_H264:
                    codec = avcodec_find_decoder_by_name( "h264_amf" );
                    break;
                case AV_CODEC_ID_HEVC:
                    codec = avcodec_find_decoder_by_name( "hevc_amf" );
                    break;
                case AV_CODEC_ID_VP9:
                    codec = avcodec_find_decoder_by_name( "vp9_amf" );
                    break;
                default:
                    codec = avcodec_find_decoder( origin_par->codec_id );
                    accel = TRP_AV_ACCEL_NONE;
                    break;
            }
            break;
        case TRP_AV_ACCEL_CUVID:
            hw_type = AV_HWDEVICE_TYPE_CUDA;
            switch ( origin_par->codec_id ) {
                case AV_CODEC_ID_H264:
                    codec = avcodec_find_decoder_by_name( "h264_cuvid" );
                    break;
                case AV_CODEC_ID_HEVC:
                    codec = avcodec_find_decoder_by_name( "hevc_cuvid" );
                    break;
                case AV_CODEC_ID_MPEG1VIDEO:
                    codec = avcodec_find_decoder_by_name( "mpeg1_cuvid" );
                    break;
                case AV_CODEC_ID_MPEG2VIDEO:
                    codec = avcodec_find_decoder_by_name( "mpeg2_cuvid" );
                    break;
                case AV_CODEC_ID_MPEG4:
                    codec = avcodec_find_decoder_by_name( "mpeg4_cuvid" );
                    break;
                case AV_CODEC_ID_VP8:
                    codec = avcodec_find_decoder_by_name( "vp8_cuvid" );
                    break;
                case AV_CODEC_ID_VP9:
                    codec = avcodec_find_decoder_by_name( "vp9_cuvid" );
                    break;
                case AV_CODEC_ID_VC1:
                    codec = avcodec_find_decoder_by_name( "vc1_cuvid" );
                    break;
                case AV_CODEC_ID_MJPEG:
                    codec = avcodec_find_decoder_by_name( "mjpeg_cuvid" );
                    break;
                default:
                    codec = avcodec_find_decoder( origin_par->codec_id );
                    accel = TRP_AV_ACCEL_NONE;
                    break;
            }
            break;
    }
    if ( codec == NULL ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avcodec_find_decoder" ), trp_cord( "decoder not found" ) );
        goto error1;
    }
    avctx = avcodec_alloc_context3( codec );
    if ( avctx == NULL ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avcodec_alloc_context3" ), trp_av_error( AVERROR(ENOMEM) ) );
        goto error1;
    }
    res = avcodec_parameters_to_context( avctx, origin_par );
    if ( res < 0 ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avcodec_parameters_to_context" ), trp_av_error( res ) );
        goto error1;
    }

    if ( accel ) {
        res = av_hwdevice_ctx_create( &hw_device_ref, hw_type, NULL, NULL, 0 );
        if ( res < 0 ) {
            if ( flags & 1 )
                failure_cause = trp_cons( trp_cord( "av_hwdevice_ctx_create" ), trp_av_error( res ) );
            goto error1;
        }
        if ( ( hw_frame = av_frame_alloc() ) == NULL ) {
            if ( flags & 1 )
                failure_cause = trp_cons( trp_cord( "av_frame_alloc" ), trp_av_error( AVERROR(ENOMEM) ) );
            goto error1;
        }
        avctx->hw_device_ctx = av_buffer_ref( hw_device_ref );
    }

    res = avcodec_open2( avctx, codec, NULL );
    if ( res < 0 ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "avcodec_open2" ), trp_av_error( res ) );
        goto error1;
    }
    packet = av_packet_alloc();
    if ( packet == NULL ) {
        if ( flags & 1 )
            failure_cause = trp_cons( trp_cord( "av_packet_alloc" ), trp_av_error( AVERROR(ENOMEM) ) );
        goto error1;
    }
    obj = trp_gc_malloc_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 1;
    obj->fmt_ctx = fmt_ctx;
    obj->path = path;
    obj->avctx = avctx;
    obj->packet = packet;
    obj->sws_ctx = NULL;
    obj->hw_device_ref = hw_device_ref;
    obj->hw_frame = hw_frame;
    obj->hw_pix_fmt = AV_PIX_FMT_NONE;
    obj->filter = NULL;
    obj->filter_rows = 0;
    obj->hflip = 0;
    obj->vflip = 0;
    obj->accel = accel;
    obj->ignore_invalid_data = 0;
    obj->debug = 0;
    obj->cbuf = trp_gc_malloc_atomic( TRP_AV_BUFSIZE * sizeof( trp_avcodec_buf_t ) );
    obj->video_stream_idx = video_stream_idx;
    obj->last_error_fun = NULL;
    obj->last_error = 0;
    obj->original_dar = NULL;
    obj->dar = NULL;
    obj->gamma = NULL;
    obj->contrast = NULL;
    obj->hue = NULL;
    obj->temperature = NULL;
    obj->video_time_base = trp_av_rational( &( fmt_ctx->streams[ video_stream_idx ]->time_base ) );
    for ( cnt = 0 ; cnt < TRP_AV_BUFSIZE ; cnt++ ) {
        if ( ( obj->cbuf[ cnt ].frame = av_frame_alloc() ) == NULL ) {
            obj->buf_max = cnt;
            obj->last_error_fun = "av_frame_alloc";
            obj->last_error = AVERROR(ENOMEM);
            goto error2;
        }
        obj->cbuf[ cnt ].status = 0;
        obj->cbuf[ cnt ].fmin = obj->cbuf[ cnt ].fmax = TRP_AV_FRAMENO_UNDEF;
        obj->cbuf[ cnt ].skip = 0;
    }
    obj->buf_max = TRP_AV_BUFSIZE;
    /* trp_av_read_frame_no_seek può aver bisogno di video_duration */
    trp_av_set_frame_rate( obj, trp_av_get_frame_rate( obj ) );
    if ( trp_av_read_frame_no_seek( obj, 0, 1, 0, &( obj->first_ts ) ) )
        goto error2;
    if ( trp_av_read_frame_no_seek( obj, 1, 1, 1, &( obj->second_ts ) ) )
        goto error2;
    obj->cbuf[ 0 ].fmin = obj->cbuf[ 0 ].fmax = 0;
    obj->cbuf[ 1 ].fmin = obj->cbuf[ 1 ].fmax = 1;
    obj->buf_cur = 1;
    obj->original_video_frame_rate = obj->video_frame_rate = trp_av_get_frame_rate( obj );
    obj->video_duration = trp_av_ratio( UNO, obj->video_frame_rate );
//    PRINT_BUF( obj );
    return (trp_obj_t *)obj;
error1:
    avformat_close_input( &fmt_ctx );
    if ( avctx )
        avcodec_free_context( &avctx );
    if ( hw_device_ref )
        av_buffer_unref( &hw_device_ref );
    if ( hw_frame )
        av_frame_free( &hw_frame );
    return failure_cause;
error2:
    if ( flags & 1 )
        failure_cause = trp_av_last_error( (trp_obj_t *)obj );
    trp_av_close( obj );
    trp_gc_free( obj );
    return failure_cause;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns8b *pixmap = trp_pix_get_mapp( pix );
    AVCodecContext *avctx;
    AVFrame *frame, *frameo = NULL, *filt_rows = NULL, *filt0_frame = NULL, *filt1_frame = NULL;
    struct SwsContext *sws_ctx;
    uns32b fframeno, w, h, wo, ho;

    if ( ( fmt_ctx == NULL ) || ( pixmap == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return 1;
    if ( trp_av_read_frame_frameno( (trp_avcodec_t *)fmtctx, fframeno, &frame ) )
        return 1;

    avctx = ((trp_avcodec_t *)fmtctx)->avctx;
    w = avctx->width;
    h = avctx->height;

    if ( ((trp_avcodec_t *)fmtctx)->filter_rows ) {
        trp_pix_color_t *p, *q;
        uns32b w2, w4, hh;

        if ( ( sws_ctx = sws_getContext( w, h, avctx->pix_fmt,
                                         w, h, AV_PIX_FMT_RGBA,
                                         SWS_BICUBIC, NULL, NULL, NULL ) ) == NULL )
            return 1;
        if ( ( filt_rows = av_frame_alloc() ) == NULL ) {
            sws_freeContext( sws_ctx );
            return 1;
        }
        if ( av_image_alloc( filt_rows->data, filt_rows->linesize, w, h, AV_PIX_FMT_RGBA, 32 ) < 0 ) {
            av_frame_free( &filt_rows );
            sws_freeContext( sws_ctx );
            return 1;
        }
        sws_scale( sws_ctx, (const uint8_t * const *)( frame->data ), frame->linesize, 0, h, filt_rows->data, filt_rows->linesize );
        sws_freeContext( sws_ctx );
        p = (trp_pix_color_t *)( filt_rows->data[ 0 ] );
        q = p + w;
        if ( ((trp_avcodec_t *)fmtctx)->filter_rows == 2 ) {
            trp_pix_color_t *r = p;
            p = q;
            q = r;
        }
        hh = h >> 1;
        w2 = w << 1;
        w4 = w << 2;
        for ( ; hh ; hh--, p += w2, q += w2 )
            memcpy( q, p, w4 );
        frame = filt_rows;
    }

#ifdef TRP_AV_FILTERS_ENABLED
    if ( ((trp_avcodec_t *)fmtctx)->filter ) {
        if ( ( filt0_frame = av_frame_alloc() ) &&
             ( filt1_frame = av_frame_alloc() ) ) {
            if ( av_buffersrc_add_frame_flags( ((trp_avcodec_t *)fmtctx)->filter->buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0 ) {
                int which, ret, done;

                for ( which = 0, done = 0 ; ; which = 1 - which ) {
                    av_frame_unref( which ? filt1_frame : filt0_frame );
                    ret = av_buffersink_get_frame( ((trp_avcodec_t *)fmtctx)->filter->buffersink_ctx, which ? filt1_frame : filt0_frame );
                    if ( ( ret == AVERROR(EAGAIN) ) || ( ret == AVERROR_EOF ) )
                        break;
                    if ( ret < 0 )
                        break;
                    done = 1;
                }
                if ( done )
                    frame = ( which ? filt0_frame : filt1_frame );
            }
        }
    }
#endif

    wo = ((trp_pix_t *)pix)->w;
    ho = ((trp_pix_t *)pix)->h;
    sws_ctx = ((trp_avcodec_t *)fmtctx)->sws_ctx =
        sws_getCachedContext( ((trp_avcodec_t *)fmtctx)->sws_ctx,
                              w, h, filt_rows ? AV_PIX_FMT_RGBA : avctx->pix_fmt,
                              wo, ho, AV_PIX_FMT_RGBA,
                              SWS_BICUBIC, NULL, NULL, NULL );
    if ( ( frameo = av_frame_alloc() ) == NULL ) {
        trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
        return 1;
    }
    if ( av_image_alloc( frameo->data, frameo->linesize, wo, ho, AV_PIX_FMT_RGBA, 32 ) < 0 ) {
        trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
        return 1;
    }
    sws_scale( sws_ctx, (const uint8_t * const *)( frame->data ), frame->linesize, 0, h, frameo->data, frameo->linesize );
    av_image_copy_to_buffer( ((trp_pix_t *)pix)->map.p, ( wo * ho ) << 2,
                             (const uint8_t * const *)( frameo->data ), frameo->linesize, AV_PIX_FMT_RGBA, wo, ho, 1 );
    av_freep( &( frameo->data[ 0 ] ) );
    trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
    if ( ((trp_avcodec_t *)fmtctx)->gamma )
        trp_pix_gamma( pix, ((trp_avcodec_t *)fmtctx)->gamma );
    if ( ((trp_avcodec_t *)fmtctx)->contrast )
        trp_pix_contrast( pix, ((trp_avcodec_t *)fmtctx)->contrast );
    if ( ((trp_avcodec_t *)fmtctx)->hue )
        trp_pix_color_hue_test( pix,
                                ((trp_cons_t *)( ((trp_avcodec_t *)fmtctx)->hue ))->car,
                                ((trp_cons_t *)( ((trp_avcodec_t *)fmtctx)->hue ))->cdr );
    if ( ((trp_avcodec_t *)fmtctx)->temperature )
        trp_pix_color_temperature_adm_test( pix,
                                            ((trp_cons_t *)( ((trp_avcodec_t *)fmtctx)->temperature ))->car,
                                            ((trp_cons_t *)( ((trp_avcodec_t *)fmtctx)->temperature ))->cdr );
    if ( ((trp_avcodec_t *)fmtctx)->hflip )
        trp_pix_hflip( pix );
    if ( ((trp_avcodec_t *)fmtctx)->vflip )
        trp_pix_vflip( pix );
    return 0;
}

static void trp_av_read_frame_free( AVFrame *frameo, AVFrame *filt_rows, AVFrame *filt0_frame, AVFrame *filt1_frame )
{
    if ( frameo ) {
        av_frame_free( &frameo );
    }
    if ( filt_rows ) {
        av_freep( &( filt_rows->data[ 0 ] ) );
        av_frame_free( &filt_rows );
    }
    if ( filt0_frame ) {
        av_frame_unref( filt0_frame );
        av_frame_free( &filt0_frame );
    }
    if ( filt1_frame ) {
        av_frame_unref( filt1_frame );
        av_frame_free( &filt1_frame );
    }
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_path( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ( (trp_avcodec_t *)fmtctx )->path;
}

trp_obj_t *trp_av_last_error( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_obj_t *fun;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->last_error_fun == NULL )
        fun = EMPTYCORD;
    else
        fun = trp_cord( ((trp_avcodec_t *)fmtctx)->last_error_fun );
    return trp_cons( fun, trp_av_error( ((trp_avcodec_t *)fmtctx)->last_error ) );
}

trp_obj_t *trp_av_nb_streams( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( fmt_ctx->nb_streams );
}

trp_obj_t *trp_av_video_stream_idx( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( ( (trp_avcodec_t *)fmtctx )->video_stream_idx );
}

trp_obj_t *trp_av_nb_frames( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_sig64( fmt_ctx->streams[ n ]->nb_frames );
}

trp_obj_t *trp_av_sample_aspect_ratio( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_av_rational( &( fmt_ctx->streams[ n ]->sample_aspect_ratio ) );
}

trp_obj_t *trp_av_avg_frame_rate( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_av_rational( &( fmt_ctx->streams[ n ]->avg_frame_rate ) );
}

trp_obj_t *trp_av_r_frame_rate( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_av_rational( &( fmt_ctx->streams[ n ]->r_frame_rate ) );
}

trp_obj_t *trp_av_video_frame_rate( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ((trp_avcodec_t *)fmtctx)->video_frame_rate;
}

trp_obj_t *trp_av_original_video_frame_rate( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ((trp_avcodec_t *)fmtctx)->original_video_frame_rate;
}

trp_obj_t *trp_av_dar( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->dar == NULL ) {
        ((trp_avcodec_t *)fmtctx)->dar = trp_av_original_dar( fmtctx );
    }
    return ((trp_avcodec_t *)fmtctx)->dar;
}

trp_obj_t *trp_av_original_dar( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->original_dar == NULL ) {
        AVStream *stream = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->video_stream_idx ];
        trp_obj_t *par = trp_av_rational( &( stream->sample_aspect_ratio ) );

        ((trp_avcodec_t *)fmtctx)->original_dar = trp_av_times( trp_av_equal( par, ZERO ) ? UNO : par,
                                                                trp_av_ratio( trp_sig64( stream->codecpar->width ),
                                                                              trp_sig64( stream->codecpar->height ) ) );
    }
    return ((trp_avcodec_t *)fmtctx)->original_dar;
}

trp_obj_t *trp_av_codec_type( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_sig64( fmt_ctx->streams[ n ]->codecpar->codec_type );
}

trp_obj_t *trp_av_codec_id( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_cord( avcodec_get_name( fmt_ctx->streams[ n ]->codecpar->codec_id ) );
}

trp_obj_t *trp_av_codec_name( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_cord( ((trp_avcodec_t *)fmtctx)->avctx->codec->name );
}

trp_obj_t *trp_av_start_time( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_obj_t *start_time;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        if ( fmt_ctx->streams[ n ]->start_time == AV_NOPTS_VALUE )
            return UNDEF;
        start_time = trp_av_times( trp_sig64( fmt_ctx->streams[ n ]->start_time ),
                                   trp_av_rational( &( fmt_ctx->streams[ n ]->time_base ) ) );
    } else {
        if ( fmt_ctx->start_time == AV_NOPTS_VALUE )
            return UNDEF;
        start_time = trp_av_ratio( trp_sig64( fmt_ctx->start_time ),
                                   trp_sig64( AV_TIME_BASE ) );
    }
    return start_time;
}

trp_obj_t *trp_av_duration( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_obj_t *duration;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        if ( fmt_ctx->streams[ n ]->duration == AV_NOPTS_VALUE )
            return UNDEF;
        duration = trp_av_times( trp_sig64( fmt_ctx->streams[ n ]->duration ),
                                 trp_av_rational( &( fmt_ctx->streams[ n ]->time_base ) ) );
    } else {
        if ( fmt_ctx->duration == AV_NOPTS_VALUE )
            return UNDEF;
        duration = trp_av_ratio( trp_sig64( fmt_ctx->duration ),
                                 trp_sig64( AV_TIME_BASE ) );
    }
    return duration;
}

static trp_obj_t *trp_av_metadata_low( AVDictionary *metadata )
{
    trp_obj_t *res = NIL;
    AVDictionaryEntry *elems;
    int count;

    if ( metadata )
        for ( count = metadata->count ; count ; ) {
            elems = &( metadata->elems[ --count ] );
            res = trp_cons( trp_cons( trp_cord( elems->key ), trp_cord( elems->value ) ), res );
        }
    return res;
}

trp_obj_t *trp_av_metadata( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVDictionary *metadata;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        metadata = fmt_ctx->streams[ n ]->metadata;
    } else
        metadata = fmt_ctx->metadata;
    return trp_av_metadata_low( metadata );
}

trp_obj_t *trp_av_ts( trp_obj_t *fmtctx, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVFrame *frame;
    uns32b fframeno;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return UNDEF;
    if ( trp_av_read_frame_frameno( (trp_avcodec_t *)fmtctx, fframeno, &frame ) )
        return UNDEF;
    return trp_av_pts( (trp_avcodec_t *)fmtctx, frame );
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_nearest_keyframe( trp_obj_t *fmtctx, trp_obj_t *frameno, trp_obj_t *backward )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b fframeno;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return UNDEF;
    if ( backward ) {
        if ( ( backward != TRP_TRUE ) && ( backward != TRP_FALSE ) )
            return UNDEF;
    } else
        backward = TRP_FALSE;
    if ( trp_av_seek_and_read_frame( (trp_avcodec_t *)fmtctx,
                                     fframeno,
                                     ( backward == TRP_FALSE ) ? 0 : AVSEEK_FLAG_BACKWARD ) )
        return UNDEF;
    fframeno = trp_av_last_frameno( (trp_avcodec_t *)fmtctx );
    return ( fframeno == TRP_AV_FRAMENO_UNDEF ) ? UNDEF : trp_sig64( fframeno );
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_get_buf_size( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( ((trp_avcodec_t *)fmtctx)->buf_max );
}

trp_obj_t *trp_av_get_buf_content( trp_obj_t *fmtctx )
{
    trp_obj_t *l;
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_avcodec_t *ffmtctx = (trp_avcodec_t *)fmtctx;
    trp_avcodec_buf_t *cbuf = ffmtctx->cbuf;
    uns32b idx = ffmtctx->buf_cur;
    uns8b sep;

    if ( fmt_ctx == NULL )
        return UNDEF;
    for ( l = NIL, sep = 0 ; ; ) {
        if ( cbuf[ idx ].status ) {
            for ( ; sep ; sep-- )
                l = trp_cons( UNDEF, l);
            l = trp_cons( trp_list( trp_sig64( ffmtctx->cbuf[ idx ].fmin ),
                                    trp_sig64( ffmtctx->cbuf[ idx ].fmax ),
                                    trp_av_pts( ffmtctx, ffmtctx->cbuf[ idx ].frame ),
                                    trp_sig64( ffmtctx->cbuf[ idx ].skip ),
                                    NULL ), l );
        } else {
            sep++;
        }
        idx = trp_av_prev_buf_idx( ffmtctx, idx );
        if ( idx == ffmtctx->buf_cur )
            break;
    }
    return l;
}

trp_obj_t *trp_av_get_accel( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( ((trp_avcodec_t *)fmtctx)->accel );
}

trp_obj_t *trp_av_get_pix_fmt( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_cord( av_get_pix_fmt_name( ((trp_avcodec_t *)fmtctx)->avctx->pix_fmt ) );
}

trp_obj_t *trp_av_get_hw_pix_fmt( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->accel == TRP_AV_ACCEL_NONE )
        return UNDEF;
    return trp_cord( av_get_pix_fmt_name( ((trp_avcodec_t *)fmtctx)->hw_pix_fmt ) );
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

uns8b trp_av_set_video_frame_rate( trp_obj_t *fmtctx, trp_obj_t *framerate )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return 1;
    if ( framerate ) {
        flt64b fr;

        if ( trp_cast_flt64b( framerate, &fr ) )
            return 1;
        if ( fr <= 0.0 )
            return 1;
    } else {
        framerate = ( (trp_avcodec_t *)fmtctx )->original_video_frame_rate;
    }
    if ( trp_av_notequal( framerate, ( (trp_avcodec_t *)fmtctx )->video_frame_rate ) )
        trp_av_set_frame_rate( (trp_avcodec_t *)fmtctx, framerate );
    return 0;
}

uns8b trp_av_set_dar( trp_obj_t *fmtctx, trp_obj_t *dar )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return 1;
    if ( dar ) {
        flt64b d;

        if ( trp_cast_flt64b( dar, &d ) )
            return 1;
        if ( d <= 0.0 )
            return 1;
    } else {
        dar = trp_av_original_dar( fmtctx );
    }
    ((trp_avcodec_t *)fmtctx)->dar = dar;
    return 0;
}

uns8b trp_av_set_buf_size( trp_obj_t *fmtctx, trp_obj_t *bufsize )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_avcodec_buf_t *oldcbuf, *cbuf;
    uns32b size, oldsize, cnt, idx;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b_range( bufsize, &size, 2, 1000 ) )
        return 1;
    oldsize = ((trp_avcodec_t *)fmtctx)->buf_max;
    if ( size == oldsize )
        return 0;
    oldcbuf = ((trp_avcodec_t *)fmtctx)->cbuf;
    cbuf = trp_gc_malloc_atomic( size * sizeof( trp_avcodec_buf_t ) );
    if ( size > oldsize ) {
        idx = ( ((trp_avcodec_t *)fmtctx)->buf_cur + 1 ) % oldsize;
        for ( cnt = 0 ; cnt < oldsize ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            memcpy( cbuf + cnt, oldcbuf + idx, sizeof( trp_avcodec_buf_t ) );
        }
        for ( ; cnt < size ; cnt++ ) {
            if ( ( cbuf[ cnt ].frame = av_frame_alloc() ) == NULL ) {
                while ( cnt > oldsize ) {
                    cnt--;
                    av_frame_free( &( cbuf[ cnt ].frame ) );
                }
                trp_gc_free( cbuf );
                return 1;
            }
            cbuf[ cnt ].status = 0;
            cbuf[ cnt ].fmin = cbuf[ cnt ].fmax = TRP_AV_FRAMENO_UNDEF;
            cbuf[ cnt ].skip = 0;
        }
        ((trp_avcodec_t *)fmtctx)->buf_cur = oldsize - 1;
    } else {
        if ( ((trp_avcodec_t *)fmtctx)->buf_cur >= size - 1 )
            idx = ((trp_avcodec_t *)fmtctx)->buf_cur - ( size - 1 );
        else
            idx = oldsize - ( ( size - 1 ) - ((trp_avcodec_t *)fmtctx)->buf_cur );
        for ( cnt = 0 ; cnt < size ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            memcpy( cbuf + cnt, oldcbuf + idx, sizeof( trp_avcodec_buf_t ) );
        }
        for ( ; cnt < oldsize ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            trp_av_frame_unref( oldcbuf, idx );
            av_frame_free( &( oldcbuf[ idx ].frame ) );
        }
        ((trp_avcodec_t *)fmtctx)->buf_cur = size - 1;
    }
    trp_gc_free( oldcbuf );
    ((trp_avcodec_t *)fmtctx)->cbuf = cbuf;
    ((trp_avcodec_t *)fmtctx)->buf_max = size;
    return 0;
}

uns8b trp_av_set_ignore_invalid_data( trp_obj_t *fmtctx, trp_obj_t *val )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( ( fmt_ctx == NULL ) || ( ( val != TRP_TRUE ) && ( val != TRP_FALSE ) ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->ignore_invalid_data = ( val == TRP_TRUE ) ? 1 : 0;
    return 0;
}

uns8b trp_av_set_debug( trp_obj_t *fmtctx, trp_obj_t *val )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( ( fmt_ctx == NULL ) || ( ( val != TRP_TRUE ) && ( val != TRP_FALSE ) ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->debug = ( val == TRP_TRUE ) ? 1 : 0;
    return 0;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_get_filter_rows( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( ((trp_avcodec_t *)fmtctx)->filter_rows );
}

trp_obj_t *trp_av_get_filter( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->filter == NULL )
        return UNDEF;
    return ((trp_avcodec_t *)fmtctx)->filter->descr;
}

trp_obj_t *trp_av_get_gamma( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->gamma == NULL )
        return UNO;
    return ((trp_avcodec_t *)fmtctx)->gamma;
}

trp_obj_t *trp_av_get_contrast( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->contrast == NULL )
        return UNO;
    return ((trp_avcodec_t *)fmtctx)->contrast;
}

trp_obj_t *trp_av_get_hue( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->hue == NULL )
        return trp_cons( ZERO, ZERO );
    return ((trp_avcodec_t *)fmtctx)->hue;
}

trp_obj_t *trp_av_get_temperature( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( ((trp_avcodec_t *)fmtctx)->temperature == NULL )
        return trp_cons( ZERO, ZERO );
    return ((trp_avcodec_t *)fmtctx)->temperature;
}

trp_obj_t *trp_av_get_hflip( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ((trp_avcodec_t *)fmtctx)->hflip ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_av_get_vflip( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ((trp_avcodec_t *)fmtctx)->vflip ? TRP_TRUE : TRP_FALSE;
}

uns8b trp_av_set_filter_rows( trp_obj_t *fmtctx, trp_obj_t *mode )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b filter_rows;

    if ( fmt_ctx == NULL )
        return 1;
    if ( mode ) {
        if ( trp_cast_uns32b_range( mode, &filter_rows, 0, 2 ) )
            return 1;
    } else
        filter_rows = 0;
    ((trp_avcodec_t *)fmtctx)->filter_rows = (uns8b)filter_rows;
    return 0;
}

uns8b trp_av_set_filter( trp_obj_t *fmtctx, trp_obj_t *descr )
{
#ifdef TRP_AV_FILTERS_ENABLED
    char args[ 512 ];
    const AVFilter *buffersrc  = avfilter_get_by_name( "buffer" );
    const AVFilter *buffersink = avfilter_get_by_name( "buffersink" );
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *dec_ctx;
    trp_avcodec_filter_t *filter;
    AVFilterInOut *outputs = NULL;
    AVFilterInOut *inputs = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    uns8b *c = NULL;
    AVRational time_base;

    if ( fmt_ctx == NULL )
        return 1;
    if ( descr == NULL ) {
        trp_av_close_filter( (trp_avcodec_t *)fmtctx );
        return 0;
    }
    if ( ( ( filter_graph = avfilter_graph_alloc() ) == NULL ) ||
         ( ( inputs = avfilter_inout_alloc() ) == NULL ) ||
         ( ( outputs = avfilter_inout_alloc() ) == NULL ) )
        goto fail;

    dec_ctx = ((trp_avcodec_t *)fmtctx)->avctx;
    time_base = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->video_stream_idx ]->time_base;

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
            time_base.num, time_base.den,
            dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    if ( avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                      args, NULL, filter_graph) < 0 )
        goto fail;

    /* buffer video sink: to terminate the filter chain. */
    if ( ( buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, "out") ) == NULL )
        goto fail;

    if ( av_opt_set(buffersink_ctx, "pixel_formats", av_get_pix_fmt_name( dec_ctx->pix_fmt ), AV_OPT_SEARCH_CHILDREN) < 0 )
        goto fail;

    if ( avfilter_init_dict(buffersink_ctx, NULL) < 0 )
        goto fail;

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup( "in" );
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup( "out" );
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    c = trp_csprint( descr );

    if ( ( avfilter_graph_parse_ptr( filter_graph, c, &inputs, &outputs, NULL ) ) < 0 )
        goto fail;

    if ( ( avfilter_graph_config( filter_graph, NULL ) ) < 0 )
        goto fail;

    trp_csprint_free( c );
    avfilter_inout_free( &inputs );
    avfilter_inout_free( &outputs );

    filter = trp_gc_malloc( sizeof( trp_avcodec_filter_t ) );
    filter->buffersink_ctx = buffersink_ctx;
    filter->buffersrc_ctx = buffersrc_ctx;
    filter->filter_graph = filter_graph;
    filter->descr = descr;
    trp_av_close_filter( (trp_avcodec_t *)fmtctx );
    ((trp_avcodec_t *)fmtctx)->filter = filter;
    return 0;
fail:
    if ( c )
        trp_csprint_free( c );
    if ( filter_graph )
        avfilter_graph_free( &filter_graph );
    if ( inputs )
        avfilter_inout_free( &inputs );
    if ( outputs )
        avfilter_inout_free( &outputs );
    return 1;
#else
    return 1;
#endif
}

uns8b trp_av_set_gamma( trp_obj_t *fmtctx, trp_obj_t *gamma )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    flt64b v;

    if ( fmt_ctx == NULL )
        return 1;
    if ( gamma ) {
        if ( trp_cast_flt64b( gamma, &v ) )
            return 1;
        if ( v <= 0.0 )
            return 1;
    } else
        v = 1.0;
    ((trp_avcodec_t *)fmtctx)->gamma = ( ( v == 1.0 ) ? NULL : gamma );
    return 0;
}

uns8b trp_av_set_contrast( trp_obj_t *fmtctx, trp_obj_t *contrast )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    flt64b v;

    if ( fmt_ctx == NULL )
        return 1;
    if ( contrast ) {
        if ( trp_cast_flt64b( contrast, &v ) )
            return 1;
        if ( v < 0.0 )
            return 1;
    } else
        v = 1.0;
    ((trp_avcodec_t *)fmtctx)->contrast = ( ( v == 1.0 ) ? NULL : contrast );
    return 0;
}

uns8b trp_av_set_hue( trp_obj_t *fmtctx, trp_obj_t *hue )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return 1;
    if ( hue ) {
        flt64b h, s;

        if ( hue->tipo != TRP_CONS )
            return 1;
        if ( trp_cast_flt64b_range( ( (trp_cons_t *)hue )->car, &h, -90.0, 90.0 ) ||
             trp_cast_flt64b_range( ( (trp_cons_t *)hue )->cdr, &s, -10.0, 10.0 ) )
            return 1;
        ((trp_avcodec_t *)fmtctx)->hue = ( ( ( h == 0.0 ) && ( s == 0.0 ) ) ? NULL : hue );
    } else {
        ((trp_avcodec_t *)fmtctx)->hue = NULL;
    }
    return 0;
}

uns8b trp_av_set_temperature( trp_obj_t *fmtctx, trp_obj_t *temperature )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return 1;
    if ( temperature ) {
        flt64b t, a;

        if ( temperature->tipo != TRP_CONS )
            return 1;
        if ( trp_cast_flt64b_range( ( (trp_cons_t *)temperature )->car, &t, -1.0, 1.0 ) ||
             trp_cast_flt64b_range( ( (trp_cons_t *)temperature )->cdr, &a, 0.0, 180.0 ) )
            return 1;
        ((trp_avcodec_t *)fmtctx)->temperature = ( ( ( t == 0.0 ) && ( a == 0.0 ) ) ? NULL : temperature );
    } else {
        ((trp_avcodec_t *)fmtctx)->temperature = NULL;
    }
    return 0;
}

uns8b trp_av_set_hflip( trp_obj_t *fmtctx, trp_obj_t *hflip )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( ( fmt_ctx == NULL ) || ( ( hflip != TRP_TRUE ) && ( hflip != TRP_FALSE ) ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->hflip = ( hflip == TRP_TRUE ) ? 1 : 0;
    return 0;
}

uns8b trp_av_set_vflip( trp_obj_t *fmtctx, trp_obj_t *vflip )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( ( fmt_ctx == NULL ) || ( ( vflip != TRP_TRUE ) && ( vflip != TRP_FALSE ) ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->vflip = ( vflip == TRP_TRUE ) ? 1 : 0;
    return 0;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

trp_obj_t *trp_av_sws_context( trp_obj_t *wi, trp_obj_t *hi, trp_obj_t *wo, trp_obj_t *ho, trp_obj_t *alg )
{
    trp_avcodec_sws_t *obj;
    struct SwsContext *sws_ctx;
    uns32b wwi, hhi, wwo, hho, aalg;

    if ( trp_cast_uns32b_rint( wi, &wwi ) ||
         trp_cast_uns32b_rint( hi, &hhi ) ||
         trp_cast_uns32b_rint( wo, &wwo ) ||
         trp_cast_uns32b_rint( ho, &hho ) )
        return UNDEF;
    if ( alg ) {
        if ( trp_cast_uns32b( alg, &aalg ) )
            return UNDEF;
    } else
        aalg = (uns32b)SWS_BICUBIC;
    if ( ( sws_ctx = sws_getContext( wwi, hhi, AV_PIX_FMT_RGBA, wwo, hho, AV_PIX_FMT_RGBA, aalg, NULL, NULL, NULL ) ) == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avcodec_sws_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 0;
    obj->sws_ctx = sws_ctx;
    obj->wi = wwi;
    obj->hi = hhi;
    obj->wo = wwo;
    obj->ho = hho;
    return (trp_obj_t *)obj;
}

uns8b trp_av_sws_scale( trp_obj_t *swsctx, trp_obj_t *pi, trp_obj_t *po )
{
    struct SwsContext *sws_ctx = trp_av_extract_sws_context( (trp_avcodec_sws_t *)swsctx );
    int wi, hi, wo, ho, stridei[ 4 ], strideo[ 4 ];
    uns8b *mapi[ 4 ], *mapo[ 4 ];

    if ( ( sws_ctx == NULL ) || ( pi->tipo != TRP_PIX ) || ( po->tipo != TRP_PIX ) )
        return 1;
    if ( ( ((trp_pix_t *)pi)->map.p == NULL ) ||
         ( ((trp_pix_t *)po)->map.p == NULL ) )
        return 1;
    wi = ((trp_pix_t *)pi)->w;
    hi = ((trp_pix_t *)pi)->h;
    if ( ( wi != ((trp_avcodec_sws_t *)swsctx)->wi ) ||
         ( hi != ((trp_avcodec_sws_t *)swsctx)->hi ) )
        return 1;
    wo = ((trp_pix_t *)po)->w;
    ho = ((trp_pix_t *)po)->h;
    if ( ( wo != ((trp_avcodec_sws_t *)swsctx)->wo ) ||
         ( ho != ((trp_avcodec_sws_t *)swsctx)->ho ) )
        return 1;
    mapi[ 0 ] = ((trp_pix_t *)pi)->map.p;
    mapi[ 1 ] = mapi[ 2 ] = mapi[ 3 ] = NULL;
    mapo[ 0 ] = ((trp_pix_t *)po)->map.p;
    mapo[ 1 ] = mapo[ 2 ] = mapo[ 3 ] = NULL;
    stridei[ 0 ] = wi << 2;
    strideo[ 0 ] = wo << 2;
    stridei[ 1 ] = stridei[ 2 ] = stridei[ 3 ] = 0;
    strideo[ 1 ] = strideo[ 2 ] = strideo[ 3 ] = 0;
    sws_scale( sws_ctx, (const uint8_t * const *)mapi, stridei, 0, hi, mapo, strideo );
    return 0;
}

/*************************************************************************************
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *                                                                                   *
 *************************************************************************************
 */

static void trp_av_print_begin( uns8b *msg, uns32b frameno )
{
    printf( "inizio funzione %s (frameno = %u)\n", msg, frameno );
}

static void trp_av_print_end( uns8b *msg )
{
    printf( "fine funzione %s\n", msg );
}

static void trp_av_print_buf( trp_avcodec_t *fmtctx )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    uns32b idx = trp_av_next_buf_cur( fmtctx );
    int cnt = -1;

    printf( "\n\nContenuto buffer:\n\n" );
    for ( ; ; idx = trp_av_next_buf_idx( fmtctx, idx ) ) {
        if ( cbuf[ idx ].status ) {
            if ( cnt == -1 )
                cnt = 0;
            printf( "%04d: [%u-%u] ts = ", cnt++, cbuf[ idx ].fmin, cbuf[ idx ].fmax );
            trp_print( trp_av_pts( fmtctx, cbuf[ idx ].frame ), NL, NULL );
        } else {
            if ( cnt != -1 ) {
                printf( "%04d: blank\n", cnt++ );
            }
        }
        if ( idx == fmtctx->buf_cur )
            break;
    }
    printf( "\n" );
}

