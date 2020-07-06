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
#include "../trppix/trppix_internal.h"
#include "./trpavcodec.h"

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
            uns32b             vstream;
            AVFrame           *frame;
            struct SwsContext *sws_ctx;
            uns64b             pts;
            uns8b              pts_shift;
        } fmt;
    };
} trp_avcodec_t;

static void trp_av_log_default_callback( void *ptr, int level, const char *fmt, va_list vl );
static uns8b trp_av_print( trp_print_t *p, trp_avcodec_t *obj );
static uns8b trp_av_close( trp_avcodec_t *obj );
static uns8b trp_av_close_basic( uns8b flags, trp_avcodec_t *obj );
static void trp_av_finalize( void *obj, void *data );
static trp_obj_t *trp_av_width( trp_avcodec_t *obj );
static trp_obj_t *trp_av_height( trp_avcodec_t *obj );
static trp_obj_t *trp_av_rational( struct AVRational *r );
static struct SwsContext *trp_av_extract_sws_context( trp_avcodec_t *swsctx );
static AVFormatContext *trp_av_extract_fmt_context( trp_avcodec_t *fmtctx );
static trp_obj_t *trp_av_open_input_file_basic( trp_obj_t *path, int debug_mv );

uns8b trp_av_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_AVCODEC ] = trp_av_print;
    _trp_close_fun[ TRP_AVCODEC ] = trp_av_close;
    _trp_width_fun[ TRP_AVCODEC ] = trp_av_width;
    _trp_height_fun[ TRP_AVCODEC ] = trp_av_height;
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
            av_free( obj->fmt.frame );
            avcodec_close( obj->fmt_ctx->streams[ obj->fmt.vstream ]->codec );
            avformat_close_input( &obj->fmt_ctx );
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
            res = trp_sig64( obj->fmt_ctx->streams[ obj->fmt.vstream ]->codec->width );
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
            res = trp_sig64( obj->fmt_ctx->streams[ obj->fmt.vstream ]->codec->height );
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

trp_obj_t *trp_av_sws_context( trp_obj_t *wi, trp_obj_t *hi, trp_obj_t *wo, trp_obj_t *ho, trp_obj_t *alg )
{
    trp_avcodec_t *obj = (trp_avcodec_t *)UNDEF;
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
    } else {
        aalg = (uns32b)SWS_BICUBIC;
    }
#if (LIBAVFORMAT_VERSION_MAJOR > 56)
    if ( sws_ctx = sws_getContext( wwi, hhi, AV_PIX_FMT_BGR32, wwo, hho, AV_PIX_FMT_BGR32, aalg, NULL, NULL, NULL ) ) {
#else
    if ( sws_ctx = sws_getContext( wwi, hhi, PIX_FMT_RGBA, wwo, hho, PIX_FMT_RGBA, aalg, NULL, NULL, NULL ) ) {
#endif
        obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
        obj->tipo = TRP_AVCODEC;
        obj->sottotipo = 0;
        obj->sws_ctx = sws_ctx;
        obj->sws.wi = wwi;
        obj->sws.hi = hhi;
        obj->sws.wo = wwo;
        obj->sws.ho = hho;
    }
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
    stridei[ 1 ] = stridei[ 2 ] = stridei[ 3 ] = 0;
    strideo[ 1 ] = strideo[ 2 ] = strideo[ 3 ] = 0;
    stridei[ 0 ] = wi << 2;
    strideo[ 0 ] = wo << 2;
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
    sws_scale( sws_ctx, (const uint8_t* const *)mapi, stridei, 0, hi, mapo, strideo );
#else
    sws_scale( sws_ctx, mapi, stridei, 0, hi, mapo, strideo );
#endif
    return 0;
}

static trp_obj_t *trp_av_open_input_file_basic( trp_obj_t *path, int debug_mv )
{
    trp_avcodec_t *obj;
    uns8b *cpath = trp_csprint( path );
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *avctx;
    AVCodec *codec;
    uns32b vstream;

#if (LIBAVFORMAT_VERSION_MAJOR > 52)
    if ( avformat_open_input( &fmt_ctx, cpath, NULL, NULL ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
#else
    if ( av_open_input_file( &fmt_ctx, cpath, NULL, 0, NULL ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
#endif
    trp_csprint_free( cpath );
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
    if ( avformat_find_stream_info( fmt_ctx, NULL ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
#else
    if ( av_find_stream_info( fmt_ctx ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
#endif
    for ( vstream = 0 ; vstream < fmt_ctx->nb_streams ; vstream++ ) {
        if ( vstream == fmt_ctx->nb_streams ) {
            avformat_close_input( &fmt_ctx );
            return UNDEF;
        }
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
        if ( fmt_ctx->streams[ vstream ]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
            break;
#else
        if ( fmt_ctx->streams[ vstream ]->codec->codec_type == CODEC_TYPE_VIDEO )
            break;
#endif
    }
    avctx = fmt_ctx->streams[ vstream ]->codec;
    avctx->debug_mv = debug_mv;
    if ( ( codec = avcodec_find_decoder( avctx->codec_id ) ) == NULL ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
    if ( avcodec_open2( avctx, codec, NULL ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
#else
    if ( avcodec_open( avctx, codec ) < 0 ) {
        avformat_close_input( &fmt_ctx );
        return UNDEF;
    }
#endif
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_avcodec_t ), trp_av_finalize );
    obj->tipo = TRP_AVCODEC;
    obj->sottotipo = 1;
    obj->fmt_ctx = fmt_ctx;
    obj->fmt.vstream = vstream;
#if (LIBAVFORMAT_VERSION_MAJOR > 55)
    obj->fmt.frame = av_frame_alloc();
#else
    obj->fmt.frame = avcodec_alloc_frame();
#endif
    obj->fmt.sws_ctx = NULL;
    obj->fmt.pts = AV_NOPTS_VALUE;
    obj->fmt.pts_shift = 1;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_av_open_input_file( trp_obj_t *path, trp_obj_t *par )
{
    uns32b v = 0;

    if ( par )
        if ( trp_cast_uns32b_range( par, &v, 0,
                                    FF_DEBUG_VIS_MV_P_FOR |
                                    FF_DEBUG_VIS_MV_B_FOR |
                                    FF_DEBUG_VIS_MV_B_BACK ) )
            return UNDEF;
    return trp_av_open_input_file_basic( path, v );
}

uns8b trp_av_read_frame( trp_obj_t *fmtctx, trp_obj_t *pix )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame;
    struct SwsContext *sws_ctx;
    uns8b *mapo[ 4 ];
    uns32b vstream;
    int finished, strideo[ 4 ];
    AVPacket packet;

    if ( ( fmt_ctx == NULL ) || ( pix->tipo != TRP_PIX ) )
        return 1;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return 1;
    vstream = ((trp_avcodec_t *)fmtctx)->fmt.vstream;
    avctx = fmt_ctx->streams[ vstream ]->codec;
    frame = ((trp_avcodec_t *)fmtctx)->fmt.frame;
    for ( ; ; ) {
        if ( av_read_frame( fmt_ctx, &packet ) < 0 )
            return 1;
        if ( packet.stream_index == vstream ) {
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
            avcodec_decode_video2( avctx, frame, &finished, &packet );
#else
            avcodec_decode_video( avctx, frame, &finished, packet.data, packet.size );
#endif
            if ( finished ) {
                ((trp_avcodec_t *)fmtctx)->fmt.pts = packet.dts;
                if ( packet.dts == 0 )
                    ((trp_avcodec_t *)fmtctx)->fmt.pts_shift = 0;
#if (LIBAVFORMAT_VERSION_MAJOR > 56)
                av_packet_unref( &packet );
#else
                av_free_packet( &packet );
#endif
                break;
            }
        }
#if (LIBAVFORMAT_VERSION_MAJOR > 56)
        av_packet_unref( &packet );
#else
        av_free_packet( &packet );
#endif
    }
    sws_ctx = ((trp_avcodec_t *)fmtctx)->fmt.sws_ctx;
    ((trp_avcodec_t *)fmtctx)->fmt.sws_ctx = sws_ctx =
        sws_getCachedContext( ((trp_avcodec_t *)fmtctx)->fmt.sws_ctx,
                              avctx->width, avctx->height, avctx->pix_fmt,
#if (LIBAVFORMAT_VERSION_MAJOR > 56)
                              ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h, AV_PIX_FMT_BGR32,
#else
                              ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h, PIX_FMT_RGBA,
#endif
                              SWS_BICUBIC, NULL, NULL, NULL );
    mapo[ 0 ] = ((trp_pix_t *)pix)->map.p;
    mapo[ 1 ] = mapo[ 2 ] = mapo[ 3 ] = NULL;
    strideo[ 1 ] = strideo[ 2 ] = strideo[ 3 ] = 0;
    strideo[ 0 ] = ((trp_pix_t *)pix)->w << 2;
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
    sws_scale( sws_ctx, (const uint8_t* const *)( frame->data ), frame->linesize, 0, avctx->height, mapo, strideo );
#else
    sws_scale( sws_ctx, frame->data, frame->linesize, 0, avctx->height, mapo, strideo );
#endif
    return 0;
}

uns8b trp_av_skip_frame( trp_obj_t *fmtctx, trp_obj_t *n )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVCodecContext *avctx;
    AVFrame *frame;
    uns32b nn, vstream;
    int finished;
    AVPacket packet;

    if ( fmt_ctx == NULL )
        return 1;
    if ( n == NULL )
        nn = 1;
    else if ( trp_cast_uns32b( n, &nn ) )
        return 1;
    vstream = ((trp_avcodec_t *)fmtctx)->fmt.vstream;
    avctx = fmt_ctx->streams[ vstream ]->codec;
    frame = ((trp_avcodec_t *)fmtctx)->fmt.frame;
    while ( nn ) {
        if ( av_read_frame( fmt_ctx, &packet ) < 0 )
            return 1;
        if ( packet.stream_index == vstream ) {
#if (LIBAVFORMAT_VERSION_MAJOR > 52)
            avcodec_decode_video2( avctx, frame, &finished, &packet );
#else
            avcodec_decode_video( avctx, frame, &finished, packet.data, packet.size );
#endif
            if ( finished ) {
                ((trp_avcodec_t *)fmtctx)->fmt.pts = packet.dts;
                if ( packet.dts == 0 )
                    ((trp_avcodec_t *)fmtctx)->fmt.pts_shift = 0;
                nn--;
            }
        }
#if (LIBAVFORMAT_VERSION_MAJOR > 56)
        av_packet_unref( &packet );
#else
        av_free_packet( &packet );
#endif
    }
    return 0;
}

uns8b trp_av_seek_frame( trp_obj_t *fmtctx, trp_obj_t *ts )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    uns32b vstream;

    if ( ( fmt_ctx == NULL ) || ( ts->tipo != TRP_SIG64 ) )
        return 1;
    ((trp_avcodec_t *)fmtctx)->fmt.pts = AV_NOPTS_VALUE;
    vstream = ((trp_avcodec_t *)fmtctx)->fmt.vstream;
    avcodec_flush_buffers( fmt_ctx->streams[ vstream ]->codec );
    return ( av_seek_frame( fmt_ctx, vstream, ((trp_sig64_t *)ts)->val, 0 ) < 0 ) ? 1 : 0;
}

trp_obj_t *trp_av_time_base( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_av_rational( &( fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->fmt.vstream ]->time_base ) );
}

trp_obj_t *trp_av_pts( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return ( ((trp_avcodec_t *)fmtctx)->fmt.pts == AV_NOPTS_VALUE )
        ? UNDEF
        : trp_sig64( ((trp_avcodec_t *)fmtctx)->fmt.pts - ((trp_avcodec_t *)fmtctx)->fmt.pts_shift );
}

trp_obj_t *trp_av_index_entries( trp_obj_t *fmtctx )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );

    if ( fmt_ctx == NULL )
        return UNDEF;
    return trp_sig64( fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->fmt.vstream ]->nb_index_entries );
}

trp_obj_t *trp_av_index_keyframe( trp_obj_t *fmtctx, trp_obj_t *frameno )
{
    AVFormatContext *fmt_ctx = trp_av_extract_fmt_context( (trp_avcodec_t *)fmtctx );
    AVStream *stream;
    uns32b n;

    if ( ( fmt_ctx == NULL ) || trp_cast_uns32b( frameno, &n ) )
        return UNDEF;
    stream = fmt_ctx->streams[ ((trp_avcodec_t *)fmtctx)->fmt.vstream ];
    if ( n >= stream->nb_index_entries )
        return TRP_FALSE;
    return ( stream->index_entries[ n ].flags & AVINDEX_KEYFRAME ) ? TRP_TRUE : TRP_FALSE;
}



