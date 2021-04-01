/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    union {
        struct SwsContext     *sws_ctx;
        AVFormatContext       *fmt_ctx;
    };
    union {
        struct {
            uns32b wi;
            uns32b hi;
            uns32b wo;
            uns32b ho;
        } sws;
        struct {
            AVCodecContext    *avctx;
            AVFrame           *frame;
            AVPacket          *packet;
            struct SwsContext *sws_ctx;
            uns32b             video_stream_idx;
            uns32b             frameno;
            sig64b             first_ts;
            sig64b             second_ts;
        } fmt;
    };
} trp_avcodec_t;

struct AVDictionary {
    int count;
    AVDictionaryEntry *elems;
};

#define TRP_AV_FRAMENO_UNDEF 0xffffffff
#define trp_av_stream2framerate(stream) ((stream)->avg_frame_rate.den ? &( (stream)->avg_frame_rate ) : &( (stream)->r_frame_rate ))

static void trp_av_log_default_callback( void *ptr, int level, const char *fmt, va_list vl );
static uns8b trp_av_print( trp_print_t *p, trp_avcodec_t *obj );
static uns8b trp_av_close( trp_avcodec_t *obj );
static uns8b trp_av_close_basic( uns8b flags, trp_avcodec_t *obj );
static void trp_av_finalize( void *obj, void *data );
static trp_obj_t *trp_av_width( trp_avcodec_t *obj );
static trp_obj_t *trp_av_height( trp_avcodec_t *obj );
static trp_obj_t *trp_av_length( trp_avcodec_t *obj );
static trp_obj_t *trp_av_rational( struct AVRational *r );
static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_t *swsctx );
static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_avformat_open_input_low( uns8b flags, trp_obj_t *path, trp_obj_t *par );
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
    if ( obj->sws_ctx == NULL )
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
    uns8b res = 0;

    if ( obj->sws_ctx ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        switch ( obj->sottotipo ) {
        case 0:
            sws_freeContext( obj->sws_ctx );
            break;
        case 1:
            avformat_close_input( &obj->fmt_ctx );
            avcodec_free_context( &obj->fmt.avctx );
            av_frame_free( &obj->fmt.frame );
            av_packet_free( &obj->fmt.packet );
            sws_freeContext( obj->fmt.sws_ctx ); /* If swsContext is NULL, then does nothing. */
            break;
        }
        obj->sws_ctx = NULL;
    }
    return res;
}

static void trp_av_finalize( void *obj, void *data )
{
    trp_av_close_basic( 0, (trp_avcodec_t *)obj );
}

static trp_obj_t *trp_av_width( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->sws_ctx )
        switch ( obj->sottotipo ) {
        case 1:
            res = trp_sig64( obj->fmt_ctx->streams[ obj->fmt.video_stream_idx ]->codecpar->width );
            break;
        }
    return res;
}

static trp_obj_t *trp_av_height( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->sws_ctx )
        switch ( obj->sottotipo ) {
        case 1:
            res = trp_sig64( obj->fmt_ctx->streams[ obj->fmt.video_stream_idx ]->codecpar->height );
            break;
        }
    return res;
}

static trp_obj_t *trp_av_length( trp_avcodec_t *obj )
{
    trp_obj_t *res = UNDEF;

    if ( obj->sws_ctx )
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

static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_t *swsctx )
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
    trp_avcodec_t *obj;
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
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 0;
    obj->sws_ctx = sws_ctx;
    obj->sws.wi = wwi;
    obj->sws.hi = hhi;
    obj->sws.wo = wwo;
    obj->sws.ho = hho;
    return (trp_obj_t *)obj;
}

uns8b trp_av_sws_scale( trp_obj_t *swsctx, trp_obj_t *pi, trp_obj_t *po )
{
    struct SwsContext *sws_ctx = trp_av_extract_sws_context( (trp_avcodec_t *)swsctx );
    int wi, hi, wo, ho, stridei[ 4 ], strideo[ 4 ];
    uns8b *mapi[ 4 ], *mapo[ 4 ];

    if ( ( sws_ctx == NULL ) || ( pi->tipo != TRP_PIX ) || ( po->tipo != TRP_PIX ) )
        return 1;
    if ( ( ((trp_pix_t *)pi)->map.p == NULL ) ||
         ( ((trp_pix_t *)po)->map.p == NULL ) )
        return 1;
    wi = ((trp_pix_t *)pi)->w;
    hi = ((trp_pix_t *)pi)->h;
    if ( ( wi != ((trp_avcodec_t *)swsctx)->sws.wi ) ||
         ( hi != ((trp_avcodec_t *)swsctx)->sws.hi ) )
        return 1;
    wo = ((trp_pix_t *)po)->w;
    ho = ((trp_pix_t *)po)->h;
    if ( ( wo != ((trp_avcodec_t *)swsctx)->sws.wo ) ||
         ( ho != ((trp_avcodec_t *)swsctx)->sws.ho ) )
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
    AVCodec *codec;
    AVCodecContext *avctx;
    AVFrame *frame;
    AVPacket *packet;
    sig64b first_ts;
    uns32b debug_mv, video_stream_idx;
    int res, cnt;

    if ( par ) {
        if ( trp_cast_uns32b_range( par, &debug_mv, 0,
                                    FF_DEBUG_VIS_MV_P_FOR |
                                    FF_DEBUG_VIS_MV_B_FOR |
                                    FF_DEBUG_VIS_MV_B_BACK ) )
            return UNDEF;
    } else
        debug_mv = 0;
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
    avctx->debug_mv = (int)debug_mv;
    if ( avcodec_open2( avctx, codec, NULL ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( ( frame = av_frame_alloc() ) == NULL ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        return UNDEF;
    }
    if ( ( packet = av_packet_alloc() ) == NULL ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        av_frame_free( &frame );
        return UNDEF;
    }

    /*
     leggo due frame video perché voglio memorizzare first_ts e second_ts
     */
    for ( cnt = 0 ; ; ) {
        if ( av_read_frame( fmt_ctx, packet ) < 0 ) {
            avformat_close_input( &fmt_ctx );
            avcodec_free_context( &avctx );
            av_frame_free( &frame );
            av_packet_free( &packet );
            return UNDEF;
        }
        if ( packet->stream_index != video_stream_idx ) {
            av_packet_unref( packet );
            continue;
        }
        if ( avcodec_send_packet( avctx, packet ) < 0 ) {
            av_packet_unref( packet );
            avformat_close_input( &fmt_ctx );
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
        frame->pts = packet->dts;
        av_packet_unref( packet );
        if ( ( res < 0 ) || ( frame->pts == AV_NOPTS_VALUE ) ) {
            avformat_close_input( &fmt_ctx );
            avcodec_free_context( &avctx );
            av_frame_free( &frame );
            av_packet_free( &packet );
            return UNDEF;
        }
        if ( cnt == 1 )
            break;
        cnt = 1;
        first_ts = frame->pts;
    }
    avcodec_flush_buffers( avctx );
    if ( av_seek_frame( fmt_ctx, video_stream_idx, 0, 0 ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        avcodec_free_context( &avctx );
        av_frame_free( &frame );
        av_packet_free( &packet );
        return UNDEF;
    }

    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 1;
    obj->fmt_ctx = fmt_ctx;
    obj->fmt.avctx = avctx;
    obj->fmt.frame = frame;
    obj->fmt.packet = packet;
    obj->fmt.sws_ctx = NULL;
    obj->fmt.video_stream_idx = video_stream_idx;
    obj->fmt.frameno = TRP_AV_FRAMENO_UNDEF;
    obj->fmt.first_ts = first_ts;
    obj->fmt.second_ts = frame->pts;
    frame->pts = AV_NOPTS_VALUE;
    return (trp_obj_t *)obj;
}

#define TRP_AV_READ_FRAME \
    for ( ; ; ) { \
        if ( av_read_frame( fmt_ctx, packet ) < 0 ) { \
            ((trp_avcodec_t *)fmtctx)->fmt.frameno = TRP_AV_FRAMENO_UNDEF; \
            frame->pts = AV_NOPTS_VALUE; \
            return 1; \
        } \
        if ( packet->stream_index != video_stream_idx ) { \
            av_packet_unref( packet ); \
            continue; \
        } \
        if ( avcodec_send_packet( avctx, packet ) < 0 ) { \
            av_packet_unref( packet ); \
            ((trp_avcodec_t *)fmtctx)->fmt.frameno = TRP_AV_FRAMENO_UNDEF; \
            frame->pts = AV_NOPTS_VALUE; \
            return 1; \
        } \
        res = avcodec_receive_frame( avctx, frame ); \
        if ( ( res == AVERROR(EAGAIN) ) || ( res == AVERROR_EOF ) ) { \
            av_packet_unref( packet ); \
            continue; \
        } \
        if ( res < 0 ) { \
            av_packet_unref( packet ); \
            ((trp_avcodec_t *)fmtctx)->fmt.frameno = TRP_AV_FRAMENO_UNDEF; \
            frame->pts = AV_NOPTS_VALUE; \
            return 1; \
        } \
        frame->pts = packet->dts; \
        av_packet_unref( packet ); \
        break; \
    }

uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame, *frameo;
    AVPacket *packet;
    struct SwsContext *sws_ctx;
    uns32b video_stream_idx, fframeno, wo, ho;
    int res;

    if ( ( fmt_ctx == NULL ) || ( pix->tipo != TRP_PIX ) )
        return 1;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return 1;
    video_stream_idx = ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx;
    avctx = ((trp_avcodec_t *)fmtctx)->fmt.avctx;
    frame = ((trp_avcodec_t *)fmtctx)->fmt.frame;
    packet = ((trp_avcodec_t *)fmtctx)->fmt.packet;
    if ( frameno ) {
        AVStream *stream = fmt_ctx->streams[ video_stream_idx ];
        trp_obj_t *fr, *tb, *dur, *durm, *target_ts, *target_ts_min, *target_ts_max;
        sig64b ts;

        if ( trp_cast_uns32b( frameno, &fframeno ) )
            return 1;
        fr = trp_av_rational( trp_av_stream2framerate( stream ) );
        tb = trp_av_rational( &( stream->time_base ) );

        dur = trp_math_ratio( UNO, fr, NULL );
        durm = trp_math_ratio( dur, trp_sig64( 2 ), NULL );

        if ( fframeno == 0 )
            target_ts = trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.first_ts ), tb, NULL );
        else
            target_ts = trp_cat( trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.second_ts ), tb, NULL ),
                                 trp_math_ratio( trp_sig64( fframeno - 1 ), fr, NULL ),
                                 NULL );
        target_ts_min = trp_math_minus( target_ts,
                                        durm,
                                        NULL );
        target_ts_max = trp_cat( target_ts,
                                 durm,
                                 NULL );

        /*
         ottimizzazione: se siamo prima di target_ts_max e non siamo
         troppo prima (2.5 secondi) di target_ts_min, evitiamo il seek...
         */
        if ( ( frame->pts == AV_NOPTS_VALUE ) ||
             ( trp_less( trp_math_times( trp_sig64( frame->pts ), tb, NULL ), target_ts_max ) != TRP_TRUE ) ||
             ( trp_less( trp_cat( trp_math_times( trp_sig64( frame->pts ), tb, NULL ), trp_double( 2.5 ), NULL ),
                         target_ts_min ) == TRP_TRUE ) ) {
            for ( ; ; ) {
                ts = ((trp_sig64_t *)trp_math_ceil( trp_math_ratio( target_ts, tb, NULL ) ) )->val;
                avcodec_flush_buffers( avctx );
                if ( av_seek_frame( fmt_ctx, video_stream_idx, ts, AVSEEK_FLAG_BACKWARD ) < 0 )
                    return 1;
                TRP_AV_READ_FRAME
                if ( trp_less( trp_math_times( trp_sig64( frame->pts ), tb, NULL ), target_ts_max ) == TRP_TRUE )
                    break;
                if ( trp_less( target_ts, dur ) == TRP_TRUE )
                    return 1;
                target_ts = trp_math_minus( target_ts, dur, NULL );
            }
        }
        for ( ; ; ) {
            if ( trp_less( target_ts_min, trp_math_times( trp_sig64( frame->pts ), tb, NULL ) ) == TRP_TRUE )
                break;
            TRP_AV_READ_FRAME
        }
    } else { /* frameno == NULL */
        fframeno = ((trp_avcodec_t *)fmtctx)->fmt.frameno;
        TRP_AV_READ_FRAME
        if ( fframeno == TRP_AV_FRAMENO_UNDEF )
            fframeno = 0;
        else
            fframeno++;
    }
    ((trp_avcodec_t *)fmtctx)->fmt.frameno = fframeno;

//    printf( "### ...: %d\n", stream->codecpar->frame_offset );

    wo = ((trp_pix_t *)pix)->w;
    ho = ((trp_pix_t *)pix)->h;
    sws_ctx = ((trp_avcodec_t *)fmtctx)->fmt.sws_ctx =
        sws_getCachedContext( ((trp_avcodec_t *)fmtctx)->fmt.sws_ctx,
                              avctx->width, avctx->height, avctx->pix_fmt,
                              wo, ho, AV_PIX_FMT_BGR32,
                              SWS_BICUBIC, NULL, NULL, NULL );
    if ( ( frameo = av_frame_alloc() ) == NULL )
        return 1;
    if ( av_image_alloc( frameo->data, frameo->linesize, wo, ho, AV_PIX_FMT_BGR32, 32 ) < 0 ) {
        av_frame_free( &frameo );
        return 1;
    }
    sws_scale( sws_ctx, (const uint8_t * const *)( frame->data ), frame->linesize, 0, frame->height, frameo->data, frameo->linesize );
    av_image_copy_to_buffer( ((trp_pix_t *)pix)->map.p, ( wo * ho ) << 2,
                             (const uint8_t * const *)( frameo->data ), frameo->linesize, AV_PIX_FMT_BGR32, wo, ho, 1 );
    av_freep( &( frameo->data[ 0 ] ) );
    av_frame_free( &frameo );
    return 0;
}

uns8b trp_av_skip_frame( trp_obj_t *fmtctx, trp_obj_t *n )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame;
    AVPacket *packet;
    uns32b nn, video_stream_idx, fframeno;
    int res;

    if ( fmt_ctx == NULL )
        return 1;
    if ( n ) {
        if ( trp_cast_uns32b( n, &nn ) )
            return 1;
    } else
        nn = 1;
    video_stream_idx = ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx;
    avctx = ((trp_avcodec_t *)fmtctx)->fmt.avctx;
    frame = ((trp_avcodec_t *)fmtctx)->fmt.frame;
    packet = ((trp_avcodec_t *)fmtctx)->fmt.packet;
    fframeno = ((trp_avcodec_t *)fmtctx)->fmt.frameno + nn;
    for ( ; nn ; nn-- ) {
        TRP_AV_READ_FRAME
    }
    ((trp_avcodec_t *)fmtctx)->fmt.frameno = fframeno;
    return 0;
}

uns8b trp_av_seek_frame( trp_obj_t *fmtctx, trp_obj_t *ts )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    sig64b tts;

    if ( ( fmt_ctx == NULL ) || trp_cast_sig64b( ts, &tts ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->fmt.frameno = TRP_AV_FRAMENO_UNDEF;
    ((trp_avcodec_t *)fmtctx)->fmt.frame->pts = AV_NOPTS_VALUE;
    avcodec_flush_buffers( ((trp_avcodec_t *)fmtctx)->fmt.avctx );
    return ( av_seek_frame( fmt_ctx,
                            ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx,
                            tts,
                            ( tts > ((trp_avcodec_t *)fmtctx)->fmt.first_ts ) ? AVSEEK_FLAG_BACKWARD : 0 ) < 0 ) ? 1 : 0;
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
    return trp_sig64( ( (trp_avcodec_t *)fmtctx )->fmt.video_stream_idx );
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
    return trp_cord( ((trp_avcodec_t *)fmtctx)->fmt.avctx->codec->name );
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

trp_obj_t *trp_av_nb_index_entries( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    return trp_sig64( fmt_ctx->streams[ n ]->nb_index_entries );
}

trp_obj_t *trp_av_nb_index_iskeyframe( trp_obj_t *fmtctx, trp_obj_t *streamno, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVStream *stream;
    uns32b n, m;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( streamno, &n ) || trp_cast_uns32b( frameno, &m ) )
        return UNDEF;
    if ( n >= fmt_ctx->nb_streams )
        return UNDEF;
    stream = fmt_ctx->streams[ n ];
    if ( m >= stream->nb_index_entries )
        return TRP_FALSE;
    return ( stream->index_entries[ m ].flags & AVINDEX_KEYFRAME ) ? TRP_TRUE : TRP_FALSE;
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
    AVStream *stream;
    trp_obj_t *fr, *tb;
    uns32b fframeno;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &fframeno ) )
        return UNDEF;
    stream = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx ];
    fr = trp_av_rational( trp_av_stream2framerate( stream ) );
    tb = trp_av_rational( &( stream->time_base ) );
    if ( fframeno == 0 )
        return trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.first_ts ), tb, NULL );
    return trp_cat( trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.second_ts ), tb, NULL ),
                    trp_math_ratio( trp_sig64( fframeno - 1 ), fr, NULL ),
                    NULL );
}

trp_obj_t *trp_av_frameno( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b fframeno;

    if ( fmt_ctx == NULL )
        return UNDEF;
    fframeno = ((trp_avcodec_t *)fmtctx)->fmt.frameno;
    return ( fframeno == TRP_AV_FRAMENO_UNDEF ) ? UNDEF : trp_sig64( fframeno );
}

trp_obj_t *trp_av_ts( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    sig64b ts;

    if ( fmt_ctx == NULL )
        return UNDEF;
    ts = ((trp_avcodec_t *)fmtctx)->fmt.frame->pts;
    return ( ts == AV_NOPTS_VALUE ) ? UNDEF : trp_sig64( ts );
}

trp_obj_t *trp_av_first_ts( trp_obj_t *fmtctx, trp_obj_t *streamno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodec *codec;
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
        stream_idx = ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx;
    if ( stream_idx == ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx )
        return trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.first_ts );

    /* non so se questo seek sia necessario */
    avcodec_flush_buffers( ((trp_avcodec_t *)fmtctx)->fmt.avctx );
    if ( av_seek_frame( fmt_ctx, ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx, 0, 0 ) < 0 )
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

/*
 rende true sse l'ultimo frame letto (da trp_av_read_frame) è rileggibile
 correttamente (da trp_av_read_frame) specificando il numero di frame
 */

trp_obj_t *trp_av_is_frame_recoverable( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVStream *stream;
    trp_obj_t *fr, *tb, *durm, *target_ts, *target_ts_min, *target_ts_max;
    sig64b ts;
    uns32b frameno;

    if ( fmt_ctx == NULL )
        return TRP_FALSE;
    frameno = ((trp_avcodec_t *)fmtctx)->fmt.frameno;
    if ( frameno == TRP_AV_FRAMENO_UNDEF )
        return TRP_FALSE;
    ts = ((trp_avcodec_t *)fmtctx)->fmt.frame->pts;
    if ( ts == AV_NOPTS_VALUE )
        return TRP_FALSE;

    stream = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->fmt.video_stream_idx ];
    fr = trp_av_rational( trp_av_stream2framerate( stream ) );
    tb = trp_av_rational( &( stream->time_base ) );

    durm = trp_math_ratio( UNO, fr, trp_sig64( 2 ), NULL );
    if ( frameno == 0 )
        target_ts = trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.first_ts ), tb, NULL );
    else
        target_ts = trp_cat( trp_math_times( trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.second_ts ), tb, NULL ),
                             trp_math_ratio( trp_sig64( frameno - 1 ), fr, NULL ),
                             NULL );
    target_ts_min = trp_math_minus( target_ts,
                                    durm,
                                    NULL );
    target_ts_max = trp_cat( target_ts,
                             durm,
                             NULL );

    target_ts = trp_math_times( trp_sig64( ts ), tb, NULL );

    return ( ( trp_less( target_ts_min, target_ts ) == TRP_TRUE ) &&
             ( trp_less( target_ts, target_ts_max ) == TRP_TRUE ) ) ? TRP_TRUE : TRP_FALSE;
}

