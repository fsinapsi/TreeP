/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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

#define TRP_AV_FILTERS_ENABLED

typedef struct {
    AVFrame *frame;
    uns32b   fmin;
    uns32b   fmax;
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
    trp_avcodec_filter_t *filter;
    trp_avcodec_buf_t    *cbuf;
    trp_obj_t            *path;
    uns32b                buf_max;
    uns32b                buf_cur;
    uns32b                video_stream_idx;
    uns32b                act_frameno;
    sig64b                act_ts;
    sig64b                cur_ts;
    sig64b                first_ts;
    sig64b                second_ts;
    uns8b                 filter_rows;
} trp_avcodec_t;

struct AVDictionary {
    int count;
    AVDictionaryEntry *elems;
};

#define TRP_AV_BUFSIZE 2 // almeno 2
#define TRP_AV_FRAMENO_UNDEF 0xffffffff
#define trp_av_stream2framerate(stream) ((stream)->avg_frame_rate.den ? &( (stream)->avg_frame_rate ) : &( (stream)->r_frame_rate ))

static void trp_av_log_default_callback( void *ptr, int level, const char *fmt, va_list vl );
static uns8b trp_av_print( trp_print_t *p, trp_avcodec_t *obj );
static uns8b trp_av_close( trp_avcodec_t *obj );
static uns8b trp_av_close_basic( uns8b flags, trp_avcodec_t *obj );
static void trp_av_finalize( void *obj, void *data );
static void trp_av_close_filter( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_width( trp_avcodec_t *obj );
static trp_obj_t *trp_av_height( trp_avcodec_t *obj );
static trp_obj_t *trp_av_length( trp_avcodec_t *obj );
static trp_obj_t *trp_av_rational( struct AVRational *r );
static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_sws_t *swsctx );
static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_avformat_open_input_low( uns8b flags, trp_obj_t *path, trp_obj_t *par );
static void trp_av_frameno2ts_range_low( trp_avcodec_t *fmtctx, uns32b frameno,
                                         trp_obj_t **ts, trp_obj_t **ts_min, trp_obj_t **ts_max, trp_obj_t **tb, trp_obj_t **dur );
static void trp_av_frameno2ts_range( trp_avcodec_t *fmtctx, uns32b frameno, trp_obj_t **ts, trp_obj_t **ts_min, trp_obj_t **ts_max );
static uns8b trp_av_ts_is_in_range( trp_avcodec_t *fmtctx, sig64b ts, trp_obj_t *ts_min, trp_obj_t *ts_max );
static uns8b trp_av_read_frame_low_low( trp_avcodec_t *fmtctx, AVFormatContext *fmt_ctx, uns32b frameno );
static uns8b trp_av_read_frame_low( trp_avcodec_t *fmtctx, AVFormatContext *fmt_ctx, uns32b frameno, AVFrame **frame );
static trp_obj_t *trp_av_metadata_low( AVDictionary *metadata );

uns8b trp_av_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];
    extern objfun_t _trp_length_fun[];

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
    uns8b res = 0;

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
            sws_freeContext( obj->sws_ctx ); /* If swsContext is NULL, then does nothing. */
            trp_av_close_filter( obj );
            for ( cnt = 0 ; cnt < obj->buf_max ; cnt++ ) {
                av_frame_unref( obj->cbuf[ cnt ].frame );
                av_frame_free( &( obj->cbuf[ cnt ].frame ) );
            }
            trp_gc_free( obj->cbuf );
            break;
        }
        obj->fmt_ctx = NULL;
    }
    return res;
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

static trp_obj_t *trp_av_rational( struct AVRational *r )
{
    return trp_math_ratio( trp_sig64( r->num ), trp_sig64( r->den ), NULL );
}

static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_sws_t *swsctx )
{
    if ( swsctx->tipo == TRP_AVCODEC )
        if ( swsctx->sottotipo == 0 )
            return swsctx->sws_ctx;
    return NULL;
}

static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx )
{
    if ( fmtctx->tipo == TRP_AVCODEC )
        if ( fmtctx->sottotipo == 1 )
            return fmtctx->fmt_ctx;
    return NULL;
}

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
    if ( ( sws_ctx = sws_getContext( wwi, hhi, AV_PIX_FMT_BGR32, wwo, hho, AV_PIX_FMT_BGR32, aalg, NULL, NULL, NULL ) ) == NULL )
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

trp_obj_t *trp_av_avformat_open_input( trp_obj_t *path, trp_obj_t *par )
{
    return trp_av_avformat_open_input_low( 0, path, par );
}

trp_obj_t *trp_av_avformat_open_input_cuvid( trp_obj_t *path, trp_obj_t *par )
{
    return trp_av_avformat_open_input_low( 1, path, par );
}

static trp_obj_t *trp_av_avformat_open_input_low( uns8b flags, trp_obj_t *path, trp_obj_t *par )
{
    trp_avcodec_t *obj;
    AVFormatContext *fmt_ctx = NULL;
    uns8b *cpath;
    const AVCodec *codec;
    AVCodecContext *avctx;
    AVPacket *packet;
    uns32b video_stream_idx, cnt;
    int res;

    cpath = trp_csprint( path );
    res = avformat_open_input( &fmt_ctx, cpath, NULL, NULL );
    trp_csprint_free( cpath );
    if ( res )
        return UNDEF;
    if ( avformat_find_stream_info( fmt_ctx, NULL ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
    for ( video_stream_idx = 0 ; ; video_stream_idx++ ) {
        if ( video_stream_idx == fmt_ctx->nb_streams ) {
            avformat_close_input( &fmt_ctx );
            return UNDEF;
        }
        if ( fmt_ctx->streams[ video_stream_idx ]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
            break;
    }
    if ( flags & 1 ) {
        switch ( fmt_ctx->streams[ video_stream_idx ]->codecpar->codec_id ) {
            case AV_CODEC_ID_H264:
                codec = avcodec_find_decoder_by_name( "h264_cuvid" );
                break;
            case AV_CODEC_ID_HEVC:
                codec = avcodec_find_decoder_by_name( "hevc_cuvid" );
                break;
            default:
                codec = avcodec_find_decoder( fmt_ctx->streams[ video_stream_idx ]->codecpar->codec_id );
                break;
        }
    } else
        codec = avcodec_find_decoder( fmt_ctx->streams[ video_stream_idx ]->codecpar->codec_id );
    if ( codec == NULL ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
    if ( ( avctx = avcodec_alloc_context3( codec ) ) == NULL ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
    if ( avcodec_parameters_to_context( avctx, fmt_ctx->streams[ video_stream_idx ]->codecpar ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( avcodec_open2( avctx, codec, NULL ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( ( packet = av_packet_alloc() ) == NULL ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    obj = trp_gc_malloc_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 1;
    obj->fmt_ctx = fmt_ctx;
    obj->path = path;
    obj->avctx = avctx;
    obj->packet = packet;
    obj->sws_ctx = NULL;
    obj->filter = NULL;
    obj->filter_rows = 0;
    obj->cbuf = trp_gc_malloc_atomic( TRP_AV_BUFSIZE * sizeof( trp_avcodec_buf_t ) );
    obj->video_stream_idx = video_stream_idx;
    for ( cnt = 0 ; cnt < TRP_AV_BUFSIZE ; cnt++ ) {
        if ( ( obj->cbuf[ cnt ].frame = av_frame_alloc() ) == NULL ) {
            obj->buf_max = cnt;
            trp_av_close( obj );
            trp_gc_free( obj );
            return UNDEF;
        }
        obj->cbuf[ cnt ].status = 0;
    }
    obj->buf_max = TRP_AV_BUFSIZE;

    /*
     leggo due frame video perché voglio memorizzare first_ts e second_ts
     */
    for ( cnt = 0 ; ; ) {
        if ( av_read_frame( fmt_ctx, packet ) < 0 ) {
            trp_av_close( obj );
            trp_gc_free( obj );
            return UNDEF;
        }
        if ( packet->stream_index != video_stream_idx ) {
            av_packet_unref( packet );
            continue;
        }
        if ( avcodec_send_packet( avctx, packet ) < 0 ) {
            av_packet_unref( packet );
            trp_av_close( obj );
            trp_gc_free( obj );
            return UNDEF;
        }
        res = avcodec_receive_frame( avctx, obj->cbuf[ cnt ].frame );
        if ( ( res == AVERROR(EAGAIN) ) || ( res == AVERROR_EOF ) ) {
            av_packet_unref( packet );
            continue;
        }
        obj->cbuf[ cnt ].frame->pts = packet->dts;
        obj->cbuf[ cnt ].fmin = obj->cbuf[ cnt ].fmax = cnt;
        obj->cbuf[ cnt ].status = 1;
        av_packet_unref( packet );
        if ( ( res < 0 ) || ( obj->cbuf[ cnt ].frame->pts == AV_NOPTS_VALUE ) ) {
            trp_av_close( obj );
            trp_gc_free( obj );
            return UNDEF;
        }
        if ( cnt == 0 ) {
            obj->first_ts = obj->cbuf[ cnt ].frame->pts;
        } else {
            obj->cur_ts = obj->second_ts = obj->cbuf[ cnt ].frame->pts;
            break;
        }
        cnt = 1;
    }
    obj->buf_cur = cnt;
    obj->act_frameno = TRP_AV_FRAMENO_UNDEF;
    obj->act_ts = AV_NOPTS_VALUE;
    return (trp_obj_t *)obj;
}

static void trp_av_frameno2ts_range_low( trp_avcodec_t *fmtctx, uns32b frameno,
                                         trp_obj_t **ts, trp_obj_t **ts_min, trp_obj_t **ts_max, trp_obj_t **tb, trp_obj_t **dur )
{
    AVStream *stream = fmtctx->fmt_ctx->streams[ fmtctx->video_stream_idx ];
    trp_obj_t *durm;

    *tb = trp_av_rational( &( stream->time_base ) );
    *dur = trp_math_ratio( UNO, trp_av_rational( trp_av_stream2framerate( stream ) ), NULL );
    durm = trp_math_ratio( *dur, trp_sig64( 2 ), NULL );
    if ( frameno == 0 )
        *ts = trp_math_times( trp_sig64( fmtctx->first_ts ), *tb, NULL );
    else
        *ts = trp_cat( trp_math_times( trp_sig64( fmtctx->second_ts ), *tb, NULL ),
                       trp_math_times( trp_sig64( frameno - 1 ), *dur, NULL ),
                       NULL );
    *ts_min = trp_math_minus( *ts, durm, NULL );
    *ts_max = trp_cat( *ts, durm, NULL );
}

static void trp_av_frameno2ts_range( trp_avcodec_t *fmtctx, uns32b frameno, trp_obj_t **ts, trp_obj_t **ts_min, trp_obj_t **ts_max )
{
    trp_obj_t *tb, *dur;

    trp_av_frameno2ts_range_low( fmtctx, frameno, ts, ts_min, ts_max, &tb, &dur );
}

static uns8b trp_av_ts_is_in_range( trp_avcodec_t *fmtctx, sig64b ts, trp_obj_t *ts_min, trp_obj_t *ts_max )
{
    trp_obj_t *trp_ts;

    if ( ts == AV_NOPTS_VALUE )
        return 3;
    trp_ts = trp_math_times( trp_sig64( ts ), trp_av_rational( &( fmtctx->fmt_ctx->streams[ fmtctx->video_stream_idx ]->time_base ) ), NULL );
    if ( trp_less( trp_ts, ts_min ) == TRP_TRUE )
        return 2;
    if ( trp_less( trp_ts, ts_max ) != TRP_TRUE )
        return 1;
    return 0;
}

#define TRP_AV_READ_FRAME \
    for ( ; ; ) { \
        if ( av_read_frame( fmt_ctx, packet ) < 0 ) \
            return 1; \
        if ( packet->stream_index != video_stream_idx ) { \
            av_packet_unref( packet ); \
            continue; \
        } \
        if ( avcodec_send_packet( avctx, packet ) < 0 ) { \
            av_packet_unref( packet ); \
            return 1; \
        } \
        cbuf[ buf_cur ].status = 2; \
        res = avcodec_receive_frame( avctx, cbuf[ buf_cur ].frame ); \
        if ( ( res == AVERROR(EAGAIN) ) || ( res == AVERROR_EOF ) ) { \
            av_packet_unref( packet ); \
            continue; \
        } \
        if ( res < 0 ) { \
            av_packet_unref( packet ); \
            return 1; \
        } \
        fmtctx->cur_ts = cbuf[ buf_cur ].frame->pts = packet->dts; \
        cur_ts = trp_math_times( trp_sig64( fmtctx->cur_ts ), tb, NULL ); \
        cbuf[ buf_cur ].fmin = cbuf[ buf_cur ].fmax = TRP_AV_FRAMENO_UNDEF; \
        av_packet_unref( packet ); \
        break; \
    }

static uns8b trp_av_read_frame_low_low( trp_avcodec_t *fmtctx, AVFormatContext *fmt_ctx, uns32b frameno )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    AVCodecContext *avctx = fmtctx->avctx;
    AVStream *stream;
    AVPacket *packet = fmtctx->packet;
    trp_obj_t *tb, *dur, *target_ts, *target_ts_min, *target_ts_max, *cur_ts;
    sig64b ts;
    uns32b video_stream_idx = fmtctx->video_stream_idx;
    uns32b buf_cur;
    int almeno_una_lettura = 0, noseek = 1, res;

    buf_cur = ( fmtctx->buf_cur + 1 ) % fmtctx->buf_max;
    stream = fmt_ctx->streams[ video_stream_idx ];

    trp_av_frameno2ts_range_low( fmtctx, frameno, &target_ts, &target_ts_min, &target_ts_max, &tb, &dur );

    cur_ts = trp_math_times( trp_sig64( fmtctx->cur_ts ), tb, NULL );

    /*
     * ottimizzazione: se siamo prima di target_ts_max e non siamo
     * troppo prima (2.5 secondi) di target_ts_min, evitiamo il seek...
     */
    if ( ( fmtctx->cur_ts == AV_NOPTS_VALUE ) ||
         ( trp_less( cur_ts, target_ts_max ) != TRP_TRUE ) ||
         ( trp_less( cur_ts, trp_math_minus( target_ts_min, trp_double( 2.5 ), NULL ) ) == TRP_TRUE ) ) {
        noseek = 0;
        for ( ; ; ) {
            ts = ((trp_sig64_t *)trp_math_ceil( trp_math_ratio( target_ts, tb, NULL ) ) )->val;
            avcodec_flush_buffers( avctx );
            if ( av_seek_frame( fmt_ctx, video_stream_idx, ts, ( ts <= fmtctx->first_ts ) ? 0 : AVSEEK_FLAG_BACKWARD ) < 0 )
                return 1;
            TRP_AV_READ_FRAME
            almeno_una_lettura = 1;
            if ( trp_less( cur_ts, target_ts_max ) == TRP_TRUE )
                break;
            if ( trp_less( target_ts, dur ) == TRP_TRUE )
                return 1;
            target_ts = trp_math_minus( target_ts, dur, NULL );
        }
    }
    for ( ; ; ) {
        if ( trp_less( cur_ts, target_ts_min ) != TRP_TRUE )
            break;
        if ( almeno_una_lettura )
            buf_cur = ( buf_cur + 1 ) % fmtctx->buf_max;
        if ( noseek )
            cbuf[ fmtctx->buf_cur ].status = 2;
        TRP_AV_READ_FRAME
        almeno_una_lettura = 1;
    }
    if ( almeno_una_lettura )
        fmtctx->buf_cur = buf_cur;
    return 0;
}

static uns8b trp_av_read_frame_low( trp_avcodec_t *fmtctx, AVFormatContext *fmt_ctx, uns32b frameno, AVFrame **frame )
{
    trp_avcodec_buf_t *cbuf = fmtctx->cbuf;
    trp_obj_t *ts, *ts_min, *ts_max;
    uns32b mframeno, i, choice;

    choice = fmtctx->buf_max;

    for ( i = 0 ; i < fmtctx->buf_max ; i++ ) {
        if ( cbuf[ i ].status == 1 )
            if ( ( frameno >= cbuf[ i ].fmin ) && ( frameno <= cbuf[ i ].fmax ) ) {
                choice = i;
                break;
            }
    }
    if ( choice == fmtctx->buf_max ) {
        if ( trp_av_read_frame_low_low( fmtctx, fmt_ctx, frameno ) ) {
            for ( i = fmtctx->buf_cur ; ; ) {
                i = ( i + 1 ) % fmtctx->buf_max;
                if ( cbuf[ i ].status != 2 )
                    break;
                cbuf[ i ].status = 0;
            }
            fmtctx->act_frameno = TRP_AV_FRAMENO_UNDEF;
            fmtctx->act_ts = fmtctx->cur_ts = AV_NOPTS_VALUE;
            return 1;
        }
        mframeno = frameno;
        i = fmtctx->buf_cur;
        if ( cbuf[ i ].status == 2 ) {
            for ( ; ; mframeno++ ) {
                trp_av_frameno2ts_range( fmtctx, mframeno, &ts, &ts_min, &ts_max );
                if ( trp_av_ts_is_in_range( fmtctx, cbuf[ i ].frame->pts, ts_min, ts_max ) != 1 )
                    break;
            }
            cbuf[ i ].fmin = cbuf[ i ].fmax = mframeno;
            cbuf[ i ].status = 1;
        }
        if ( frameno == mframeno )
            choice = i;
        /*
         * setto fmin e fmax di tutti i frame precedenti che sono stati
         * letti dall'ultima chiamata a trp_av_read_frame_low_low
         */
        for ( ; ; ) {
            i = ( i ? i : fmtctx->buf_max ) - 1;
            if ( cbuf[ i ].status != 2 )
                break;
            for ( ; ; ) {
                /*
                 * FIXME
                 * capire perché è necessario questo controllo...
                 */
                if ( mframeno == 0 ) {
                    cbuf[ i ].status = 0;
                    break;
                }
                mframeno--;
                trp_av_frameno2ts_range( fmtctx, mframeno, &ts, &ts_min, &ts_max );
                if ( trp_av_ts_is_in_range( fmtctx, cbuf[ i ].frame->pts, ts_min, ts_max ) == 0 ) {
                    cbuf[ i ].fmin = mframeno;
                    cbuf[ i ].fmax = cbuf[ ( i + 1 ) % fmtctx->buf_max ].fmin - 1;
                    cbuf[ i ].status = 1;
                    if ( ( frameno >= cbuf[ i ].fmin ) && ( frameno <= cbuf[ i ].fmax ) )
                        choice = i;
                    break;
                }
            }
        }
    }
    if ( choice == fmtctx->buf_max ) {
        if ( cbuf[ fmtctx->buf_cur ].status != 1 ) {
            fmtctx->act_frameno = TRP_AV_FRAMENO_UNDEF;
            fmtctx->act_ts = fmtctx->cur_ts = AV_NOPTS_VALUE;
            return 1;
        }
        choice = fmtctx->buf_cur;
    }
    *frame = cbuf[ choice ].frame;
    fmtctx->act_frameno = frameno;
    fmtctx->act_ts = (*frame)->pts;
//    printf( "### scelto per %u : ts = %ld\n", frameno, fmtctx->act_ts );
    if ( frameno > cbuf[ choice ].fmin ) {
        AVStream *stream = fmt_ctx->streams[ fmtctx->video_stream_idx ];

        fmtctx->act_ts += ( (trp_sig64_t *)trp_math_rint( trp_math_ratio( trp_sig64( frameno - cbuf[ choice ].fmin ),
                                                                          trp_av_rational( trp_av_stream2framerate( stream ) ),
                                                                          trp_av_rational( &( stream->time_base ) ),
                                                                          NULL ) ) )->val;
    }
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

uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame, *frameo = NULL, *filt_rows = NULL, *filt0_frame = NULL, *filt1_frame = NULL;
    struct SwsContext *sws_ctx;
    uns32b fframeno, w, h, wo, ho;

    if ( ( fmt_ctx == NULL ) || ( pix->tipo != TRP_PIX ) )
        return 1;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return 1;
    if ( frameno ) {
        if ( trp_cast_uns32b( frameno, &fframeno ) )
            return 1;
    } else {
        fframeno = ((trp_avcodec_t *)fmtctx)->act_frameno;
        if ( fframeno == TRP_AV_FRAMENO_UNDEF )
            fframeno = 0;
        else
            fframeno++;
    }
    if ( trp_av_read_frame_low( (trp_avcodec_t *)fmtctx, fmt_ctx, fframeno, &frame ) )
        return 1;

    avctx = ((trp_avcodec_t *)fmtctx)->avctx;
    w = avctx->width;
    h = avctx->height;

    if ( ((trp_avcodec_t *)fmtctx)->filter_rows ) {
        trp_pix_color_t *p, *q;
        uns32b w2, w4, hh;

        if ( ( sws_ctx = sws_getContext( w, h, avctx->pix_fmt,
                                         w, h, AV_PIX_FMT_BGR32,
                                         SWS_BICUBIC, NULL, NULL, NULL ) ) == NULL )
            return 1;
        if ( ( filt_rows = av_frame_alloc() ) == NULL ) {
            sws_freeContext( sws_ctx );
            return 1;
        }
        if ( av_image_alloc( filt_rows->data, filt_rows->linesize, w, h, AV_PIX_FMT_BGR32, 32 ) < 0 ) {
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
                              w, h, filt_rows ? AV_PIX_FMT_BGR32 : avctx->pix_fmt,
                              wo, ho, AV_PIX_FMT_BGR32,
                              SWS_BICUBIC, NULL, NULL, NULL );
    if ( ( frameo = av_frame_alloc() ) == NULL ) {
        trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
        return 1;
    }
    if ( av_image_alloc( frameo->data, frameo->linesize, wo, ho, AV_PIX_FMT_BGR32, 32 ) < 0 ) {
        trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
        return 1;
    }
    sws_scale( sws_ctx, (const uint8_t * const *)( frame->data ), frame->linesize, 0, h, frameo->data, frameo->linesize );
    av_image_copy_to_buffer( ((trp_pix_t *)pix)->map.p, ( wo * ho ) << 2,
                             (const uint8_t * const *)( frameo->data ), frameo->linesize, AV_PIX_FMT_BGR32, wo, ho, 1 );
    av_freep( &( frameo->data[ 0 ] ) );
    trp_av_read_frame_free( frameo, filt_rows, filt0_frame, filt1_frame );
    return 0;
}

uns8b trp_av_skip_frame( trp_obj_t *fmtctx, trp_obj_t *n )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVFrame *frame;
    uns32b nn, fframeno;

    if ( fmt_ctx == NULL )
        return 1;
    if ( n ) {
        if ( trp_cast_uns32b( n, &nn ) )
            return 1;
    } else
        nn = 1;
    fframeno = ((trp_avcodec_t *)fmtctx)->act_frameno;
    if ( fframeno == TRP_AV_FRAMENO_UNDEF )
        fframeno = 0;
    else
        fframeno++;
    for ( ; nn ; nn--, fframeno++ )
        if ( trp_av_read_frame_low( (trp_avcodec_t *)fmtctx, fmt_ctx, fframeno, &frame ) )
            return 1;
    return 0;
}

uns8b trp_av_read_scd_histogram_set( trp_obj_t *fmtctx, trp_obj_t *raw )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame, *frameo;
    struct SwsContext *sws_ctx;
    uns32b *h;
    uns32b fframeno, ww, hh, n;
    uns8b *p;

    if ( ( fmt_ctx == NULL ) || ( raw->tipo != TRP_RAW ) )
        return 1;
    if ( ((trp_raw_t *)raw)->len != 256 )
        return 1;
    fframeno = ((trp_avcodec_t *)fmtctx)->act_frameno;
    if ( fframeno == TRP_AV_FRAMENO_UNDEF )
        fframeno = 0;
    else
        fframeno++;
    avctx = ((trp_avcodec_t *)fmtctx)->avctx;
    ww = avctx->width;
    hh = avctx->height;
    if ( ( frameo = av_frame_alloc() ) == NULL )
        return 1;
    if ( av_image_alloc( frameo->data, frameo->linesize, ww, hh, AV_PIX_FMT_GRAY8, 8 ) < 0 ) {
        av_frame_free( &frameo );
        return 1;
    }
    if ( ( sws_ctx = sws_getContext( ww, hh, avctx->pix_fmt, ww, hh, AV_PIX_FMT_GRAY8, SWS_BICUBIC, NULL, NULL, NULL ) ) == NULL ) {
        av_freep( &( frameo->data[ 0 ] ) );
        av_frame_free( &frameo );
        return 1;
    }
    if ( trp_av_read_frame_low( (trp_avcodec_t *)fmtctx, fmt_ctx, fframeno, &frame ) ) {
        sws_freeContext( sws_ctx );
        av_freep( &( frameo->data[ 0 ] ) );
        av_frame_free( &frameo );
        return 1;
    }
    sws_scale( sws_ctx, (const uint8_t * const *)( frame->data ), frame->linesize, 0, frame->height, frameo->data, frameo->linesize );
    sws_freeContext( sws_ctx );
    h = (uns32b *)( ((trp_raw_t *)raw)->data );
    memset( h, 0, 256 );
    for ( n = ww * hh, p = frameo->data[ 0 ] ; n ; n--, p++ )
        h[ (*p) >> 2 ]++;
    av_freep( &( frameo->data[ 0 ] ) );
    av_frame_free( &frameo );
    return 0;
}

uns8b trp_av_rewind( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return 1;
    ((trp_avcodec_t *)fmtctx)->act_frameno = TRP_AV_FRAMENO_UNDEF;
    ((trp_avcodec_t *)fmtctx)->act_ts = AV_NOPTS_VALUE;
    return 0;
}

/*
 rende true sse l'ultimo frame letto (da trp_av_read_frame) è rileggibile
 correttamente (da trp_av_read_frame) specificando il numero di frame
 */

trp_obj_t *trp_av_is_frame_recoverable( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_obj_t *ts, *ts_min, *ts_max;
    uns32b frameno;

    if ( fmt_ctx == NULL )
        return TRP_FALSE;
    frameno = ((trp_avcodec_t *)fmtctx)->act_frameno;
    if ( frameno == TRP_AV_FRAMENO_UNDEF )
        return TRP_FALSE;
    trp_av_frameno2ts_range( (trp_avcodec_t *)fmtctx, frameno, &ts, &ts_min, &ts_max );
    return trp_av_ts_is_in_range( (trp_avcodec_t *)fmtctx, ((trp_avcodec_t *)fmtctx)->act_ts, ts_min, ts_max ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_av_path( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ( (trp_avcodec_t *)fmtctx )->path;
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
    return trp_av_rational( trp_av_stream2framerate( fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->video_stream_idx ] ) );
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

trp_obj_t *trp_av_time_base( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        return trp_av_rational(  &( fmt_ctx->streams[ n ]->time_base ) );
    }
    return trp_math_ratio( UNO, trp_sig64( AV_TIME_BASE ), NULL );
}

trp_obj_t *trp_av_start_time( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    sig64b start_time;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        start_time = fmt_ctx->streams[ n ]->start_time;
    } else
        start_time = fmt_ctx->start_time;
    return ( start_time == AV_NOPTS_VALUE ) ? UNDEF : trp_sig64( start_time );
}

trp_obj_t *trp_av_duration( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    sig64b duration;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        uns32b n;

        if ( trp_cast_uns32b( streamno, &n ) )
            return UNDEF;
        if ( n >= fmt_ctx->nb_streams )
            return UNDEF;
        duration = fmt_ctx->streams[ n ]->duration;
    } else
        duration = fmt_ctx->duration;
    return ( duration == AV_NOPTS_VALUE ) ? UNDEF : trp_sig64( duration );
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

trp_obj_t *trp_av_frameno2ts( trp_obj_t *fmtctx, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_obj_t *ts, *ts_min, *ts_max;
    uns32b fframeno;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return UNDEF;
    trp_av_frameno2ts_range( (trp_avcodec_t *)fmtctx, fframeno, &ts, &ts_min, &ts_max );
    return ts;
}

trp_obj_t *trp_av_frameno( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b fframeno;

    if ( fmt_ctx == NULL )
        return UNDEF;
    fframeno = ((trp_avcodec_t *)fmtctx)->act_frameno;
    return ( fframeno == TRP_AV_FRAMENO_UNDEF ) ? UNDEF : trp_sig64( fframeno );
}

trp_obj_t *trp_av_ts( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    sig64b ts;

    if ( fmt_ctx == NULL )
        return UNDEF;
    ts = ((trp_avcodec_t *)fmtctx)->act_ts;
    return ( ts == AV_NOPTS_VALUE ) ? UNDEF : trp_sig64( ts );
}

trp_obj_t *trp_av_nearest_keyframe( trp_obj_t *fmtctx, trp_obj_t *frameno, trp_obj_t *max_diff )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVStream *stream;
    AVPacket *packet;
    trp_avcodec_buf_t *cbuf;
    trp_obj_t *tb, *dur, *target_ts, *target_ts_min, *target_ts_max;
    sig64b ts;
    uns32b video_stream_idx;
    uns32b buf_cur;
    uns32b fframeno, mmax_diff, i, j, k, l, m;
    int res;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return UNDEF;
    if ( max_diff ) {
        if ( trp_cast_uns32b( max_diff, &mmax_diff ) )
            return UNDEF;
    } else
        mmax_diff = 100;

    cbuf = ((trp_avcodec_t *)fmtctx)->cbuf;
    j = fframeno + mmax_diff;
    k = ( fframeno < mmax_diff ) ? 0 : fframeno - mmax_diff;
    l = mmax_diff + 1;

    for ( i = 0 ; i < ((trp_avcodec_t *)fmtctx)->buf_max ; i++ )
        if ( cbuf[ i ].status == 1 )
            if ( ( j >= cbuf[ i ].fmin ) && ( k <= cbuf[ i ].fmax ) ) {
                uns32b t, d;

                if ( ( fframeno >= cbuf[ i ].fmin ) && ( fframeno <= cbuf[ i ].fmax ) )
                    return frameno;
                if ( fframeno < cbuf[ i ].fmin ) {
                    t = cbuf[ i ].fmin;
                    d = t - fframeno;
                } else {
                    t = cbuf[ i ].fmax;
                    d = fframeno - t;
                }
                if ( d < l ) {
                    m = t;
                    l = d;
                }
            }
    if ( l < mmax_diff + 1 )
        return trp_sig64( m );

    avctx = ((trp_avcodec_t *)fmtctx)->avctx;
    packet = ((trp_avcodec_t *)fmtctx)->packet;
    video_stream_idx = ((trp_avcodec_t *)fmtctx)->video_stream_idx;
    stream = fmt_ctx->streams[ video_stream_idx ];
    trp_av_frameno2ts_range_low( ((trp_avcodec_t *)fmtctx), fframeno, &target_ts, &target_ts_min, &target_ts_max, &tb, &dur );
    ts = ((trp_sig64_t *)trp_math_ceil( trp_math_ratio( target_ts, tb, NULL ) ) )->val;

    ((trp_avcodec_t *)fmtctx)->act_frameno = TRP_AV_FRAMENO_UNDEF;
    ((trp_avcodec_t *)fmtctx)->act_ts = ((trp_avcodec_t *)fmtctx)->cur_ts = AV_NOPTS_VALUE;

    avcodec_flush_buffers( avctx );
    if ( av_seek_frame( fmt_ctx, video_stream_idx, ts, ( ts <= ((trp_avcodec_t *)fmtctx)->first_ts ) ? 0 : AVSEEK_FLAG_BACKWARD ) < 0 )
        return UNDEF;

    buf_cur = ( ((trp_avcodec_t *)fmtctx)->buf_cur + 1 ) % ((trp_avcodec_t *)fmtctx)->buf_max;

    for ( ; ; ) {
        if ( av_read_frame( fmt_ctx, packet ) < 0 )
            return UNDEF;
        if ( packet->stream_index != video_stream_idx ) {
            av_packet_unref( packet );
            continue;
        }
        if ( avcodec_send_packet( avctx, packet ) < 0 ) {
            av_packet_unref( packet );
            return UNDEF;
        }
        cbuf[ buf_cur ].status = 0;
        res = avcodec_receive_frame( avctx, cbuf[ buf_cur ].frame );
        if ( ( res == AVERROR(EAGAIN) ) || ( res == AVERROR_EOF ) ) {
            av_packet_unref( packet );
            continue;
        }
        if ( res < 0 ) {
            av_packet_unref( packet );
            return UNDEF;
        }
        ts = packet->dts;
        av_packet_unref( packet );
        break;
    }
    target_ts = trp_math_times( trp_sig64( ts ), tb, NULL );
    while ( fframeno && ( trp_less( target_ts, target_ts_min ) == TRP_TRUE ) )
        trp_av_frameno2ts_range( ((trp_avcodec_t *)fmtctx), --fframeno, &dur, &target_ts_min, &target_ts_max );
    while ( trp_less( target_ts, target_ts_max ) != TRP_TRUE )
        trp_av_frameno2ts_range( ((trp_avcodec_t *)fmtctx), ++fframeno, &dur, &target_ts_min, &target_ts_max );
    ((trp_avcodec_t *)fmtctx)->act_ts = ((trp_avcodec_t *)fmtctx)->cur_ts = cbuf[ buf_cur ].frame->pts = ts;
    ((trp_avcodec_t *)fmtctx)->act_frameno = cbuf[ buf_cur ].fmin = cbuf[ buf_cur ].fmax = fframeno;
    ((trp_avcodec_t *)fmtctx)->buf_cur = buf_cur;
    cbuf[ buf_cur ].status = 1;
    return trp_sig64( fframeno );
}

trp_obj_t *trp_av_get_buf_size( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( ((trp_avcodec_t *)fmtctx)->buf_max );
}

trp_obj_t *trp_av_get_buf_content( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_avcodec_t *ffmtctx = (trp_avcodec_t *)fmtctx;
    trp_obj_t *l;
    uns32b cnt, i;

    if ( fmt_ctx == NULL )
        return UNDEF;
    for ( cnt = 0, l = NIL, i = ffmtctx->buf_cur ; cnt < ffmtctx->buf_max ; cnt++ ) {
        if ( ffmtctx->cbuf[ i ].status == 1 )
            l = trp_cons( trp_list( trp_sig64( ffmtctx->cbuf[ i ].fmin ),
                                    trp_sig64( ffmtctx->cbuf[ i ].fmax ),
                                    trp_math_times( trp_sig64( ffmtctx->cbuf[ i ].frame->pts ),
                                                    trp_av_rational( &( ffmtctx->fmt_ctx->streams[ ffmtctx->video_stream_idx ]->time_base ) ),
                                                    NULL ),
                                    NULL ), l );
        i = ( i ? i : ffmtctx->buf_max ) - 1;
    }
    return l;
}

uns8b trp_av_set_buf_size( trp_obj_t *fmtctx, trp_obj_t *bufsize )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    trp_avcodec_buf_t *cbuf;
    uns32b size, oldsize, cnt, idx;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b_range( bufsize, &size, 2, 1000 ) )
        return 1;
    oldsize = ((trp_avcodec_t *)fmtctx)->buf_max;
    if ( size == oldsize )
        return 0;
    cbuf = trp_gc_malloc_atomic( size * sizeof( trp_avcodec_buf_t ) );
    if ( size > oldsize ) {
        idx = ( ((trp_avcodec_t *)fmtctx)->buf_cur + 1 ) % oldsize;
        for ( cnt = 0 ; cnt < oldsize ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            cbuf[ cnt ].frame = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].frame;
            cbuf[ cnt ].fmin = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].fmin;
            cbuf[ cnt ].fmax = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].fmax;
            cbuf[ cnt ].status = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].status;
        }
        for ( ; cnt < size ; cnt++ ) {
            if ( ( cbuf[ cnt ].frame = av_frame_alloc() ) == NULL ) {
                while ( cnt > oldsize ) {
                    cnt--;
                    av_frame_unref( cbuf[ cnt ].frame );
                    av_frame_free( &( cbuf[ cnt ].frame ) );
                }
                trp_gc_free( cbuf );
                return 1;
            }
            cbuf[ cnt ].status = 0;
        }
        ((trp_avcodec_t *)fmtctx)->buf_cur = oldsize - 1;
    } else {
        if ( ((trp_avcodec_t *)fmtctx)->buf_cur >= size - 1 )
            idx = ((trp_avcodec_t *)fmtctx)->buf_cur - ( size - 1 );
        else
            idx = oldsize - ( ( size - 1 ) - ((trp_avcodec_t *)fmtctx)->buf_cur );
        for ( cnt = 0 ; cnt < size ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            cbuf[ cnt ].frame = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].frame;
            cbuf[ cnt ].fmin = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].fmin;
            cbuf[ cnt ].fmax = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].fmax;
            cbuf[ cnt ].status = ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].status;
        }
        for ( ; cnt < oldsize ; cnt++, idx = ( idx + 1 ) % oldsize ) {
            av_frame_unref( ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].frame );
            av_frame_free( &( ((trp_avcodec_t *)fmtctx)->cbuf[ idx ].frame ) );
        }
        ((trp_avcodec_t *)fmtctx)->buf_cur = size - 1;
    }
    trp_gc_free( ((trp_avcodec_t *)fmtctx)->cbuf );
    ((trp_avcodec_t *)fmtctx)->cbuf = cbuf;
    ((trp_avcodec_t *)fmtctx)->buf_max = size;
    return 0;
}

trp_obj_t *trp_av_first_ts( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    const AVCodec *codec;
    AVCodecContext *avctx;
    AVFrame *frame;
    AVPacket *packet;
    sig64b first_ts;
    uns32b stream_idx;
    int res;

    if ( fmt_ctx == NULL )
        return UNDEF;
    if ( streamno ) {
        if ( trp_cast_uns32b( streamno, &stream_idx ) )
            return UNDEF;
        if ( stream_idx >= fmt_ctx->nb_streams )
            return UNDEF;
    } else
        stream_idx = ((trp_avcodec_t *)fmtctx)->video_stream_idx;
    if ( stream_idx == ((trp_avcodec_t *)fmtctx)->video_stream_idx )
        return trp_sig64( ((trp_avcodec_t *)fmtctx)->first_ts );

    /* non so se questo seek sia necessario */
    avcodec_flush_buffers( ((trp_avcodec_t *)fmtctx)->avctx );
    if ( av_seek_frame( fmt_ctx, ((trp_avcodec_t *)fmtctx)->video_stream_idx, 0, 0 ) < 0 )
        return UNDEF;

    codec = avcodec_find_decoder( fmt_ctx->streams[ stream_idx ]->codecpar->codec_id );
    if ( codec == NULL )
        return UNDEF;
    if ( ( avctx = avcodec_alloc_context3( codec ) ) == NULL )
        return UNDEF;
    if ( avcodec_parameters_to_context( avctx, fmt_ctx->streams[ stream_idx ]->codecpar ) < 0 ) {
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( avcodec_open2( avctx, codec, NULL ) < 0 ) {
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( ( frame = av_frame_alloc() ) == NULL ) {
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( ( packet = av_packet_alloc() ) == NULL ) {
        avcodec_free_context( &avctx );
        av_frame_free( &frame );
        return UNDEF;
    }
    for ( ; ; ) {
        if ( av_read_frame( fmt_ctx, packet ) < 0 ) {
            avcodec_free_context( &avctx );
            av_frame_free( &frame );
            av_packet_free( &packet );
            return UNDEF;
        }
        if ( packet->stream_index != stream_idx ) {
            av_packet_unref( packet );
            continue;
        }
        if ( avcodec_send_packet( avctx, packet ) < 0 ) {
            av_packet_unref( packet );
            avcodec_free_context( &avctx );
            av_frame_free( &frame );
            av_packet_free( &packet );
            return UNDEF;
        }
        res = avcodec_receive_frame( avctx, frame );
        if ( ( res == AVERROR(EAGAIN) ) || ( res == AVERROR_EOF ) ) {
            av_packet_unref( packet );
            continue;
        }
        first_ts = packet->dts;
        av_packet_unref( packet );
        avcodec_free_context( &avctx );
        av_frame_free( &frame );
        av_packet_free( &packet );
        if ( ( res < 0 ) || ( first_ts == AV_NOPTS_VALUE ) )
            return UNDEF;
        break;
    }
    return trp_sig64( first_ts );
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

uns8b trp_av_set_filter( trp_obj_t *fmtctx, trp_obj_t *descr )
{
#ifdef TRP_AV_FILTERS_ENABLED
    char args[ 512 ];
    const AVFilter *buffersrc  = avfilter_get_by_name( "buffer" );
    const AVFilter *buffersink = avfilter_get_by_name( "buffersink" );
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *dec_ctx;
    trp_avcodec_filter_t *filter;
    AVFilterInOut *outputs, *inputs;
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    uns8b *c;
    AVRational time_base;
    enum AVPixelFormat pix_fmts[ 2 ];

    if ( fmt_ctx == NULL )
        return 1;
    if ( descr == NULL ) {
        trp_av_close_filter( (trp_avcodec_t *)fmtctx );
        return 0;
    }
    if ( ( outputs = avfilter_inout_alloc() ) == NULL )
        return 1;
    if ( ( inputs = avfilter_inout_alloc() ) == NULL ) {
        avfilter_inout_free( &outputs );
        return 1;
    }
    if ( ( filter_graph = avfilter_graph_alloc() ) == NULL ) {
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }
    dec_ctx = ((trp_avcodec_t *)fmtctx)->avctx;
    time_base = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->video_stream_idx ]->time_base;

    snprintf( args, sizeof( args ),
              "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
              dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
              time_base.num, time_base.den,
              dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den );

    if ( avfilter_graph_create_filter( &buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph ) < 0 ) {
        avfilter_graph_free( &filter_graph );
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }

    /* buffer video sink: to terminate the filter chain. */
    if ( avfilter_graph_create_filter( &buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph ) < 0 ) {
        avfilter_graph_free( &filter_graph );
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }

    pix_fmts[ 0 ] = dec_ctx->pix_fmt;
    pix_fmts[ 1 ] = AV_PIX_FMT_NONE;

    if ( av_opt_set_int_list( buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN ) < 0 ) {
        avfilter_graph_free( &filter_graph );
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }

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

    if ( avfilter_graph_parse_ptr( filter_graph, c,
                                   &inputs, &outputs, NULL ) < 0 ) {
        trp_csprint_free( c );
        avfilter_graph_free( &filter_graph );
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }

    trp_csprint_free( c );

    if ( avfilter_graph_config( filter_graph, NULL ) < 0 ) {
        avfilter_graph_free( &filter_graph );
        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );
        return 1;
    }

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
#else
    return 1;
#endif
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

