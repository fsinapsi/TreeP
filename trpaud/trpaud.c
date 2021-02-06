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
#include "./trpaud.h"
#include "./dca.h"
#include "./dca_internal.h"

#ifdef MINGW
#define trp_off_t sig64b
#define fseeko fseeko64
#define ftello ftello64
#else
#define trp_off_t off_t
#endif

typedef struct {
    uns32b       version;
    uns32b       layer;
    uns32b       bitrate;
    uns32b       crc;
    uns32b       freq;
    uns32b       padding;
    uns32b       extension;
    uns32b       mode;
    uns32b       mode_extension;
    uns32b       copyright;
    uns32b       original;
    uns32b       emphasis;
} aud_header_t;

typedef struct {
    uns8b        tipo;
    uns8b        codec;
    uns8b        vbr;
    uns8b        splitted;
    FILE        *fp;
    FILE        *fpout;
    sig64b       tot_read;
    sig64b       tot_bytes;
    sig64b       tot_samples;
    sig64b       initial_skip;
    sig64b       internal_skip;
    uns8b       *buf;
    uns8b       *encoder;
    dca_state_t *dca_state;
    uns32b       buf_len;
    uns32b       buf_act;
    uns32b       frames;
    aud_header_t h_first;
    aud_header_t h_last;
    uns8b        fpout_skip_garbage;
} trp_aud_t;

static uns8b trp_aud_print( trp_print_t *p, trp_aud_t *obj );
static uns8b trp_aud_close( trp_aud_t *obj );
static uns8b trp_aud_close_basic( uns8b flags, trp_aud_t *obj );
static void trp_aud_finalize( void *obj, void *data );
static void trp_aud_set_mp3_encoder_internal( trp_aud_t *aud, uns8b *buf, uns32b fl );
static void trp_aud_set_mp3_encoder( trp_aud_t *aud, uns8b *buf, uns32b fl );
static uns32b trp_mp3_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy );
static uns32b trp_ac3_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy );
static uns32b trp_dts_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy );
static uns32b trp_aac_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy );
static uns8b trp_aud_parse_internal( uns8b flags, trp_aud_t *aud, uns32b l, trp_raw_t *stripped );
static uns8b trp_aud_parse_step_internal( trp_aud_t *aud );

#define MAX_BUF_SIZE 1048576

#define MP3_MIN_FRAME_SIZE 21
#define MP3_MAX_FRAME_SIZE 8192
#define AC3_MIN_FRAME_SIZE 7
#define AC3_MAX_FRAME_SIZE 8192
#define DTS_MIN_FRAME_SIZE 20
#define DTS_MAX_FRAME_SIZE 8192
#define AAC_MIN_FRAME_SIZE 1
#define AAC_MAX_FRAME_SIZE 65536

#define A52_CHANNEL 0
#define A52_MONO 1
#define A52_STEREO 2
#define A52_3F 3
#define A52_2F1R 4
#define A52_3F1R 5
#define A52_2F2R 6
#define A52_3F2R 7
#define A52_CHANNEL1 8
#define A52_CHANNEL2 9
#define A52_DOLBY 10
#define A52_CHANNEL_MASK 15
#define A52_LFE 16
#define A52_ADJUST_LEVEL 32

static uns32b _mp3_frame_size_index[] = { 24000, 72000, 72000 };

static uns32b _mp3_bitrate[ 2 ][ 3 ][ 15 ] = {
    { /* MPEG 2.0, MPEG 2.5 */
        {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},  /* layer 1 */
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},       /* layer 2 */
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}        /* layer 3 */

    },
    { /* MPEG 1.0 */
        {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448}, /* layer 1 */
        {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},    /* layer 2 */
        {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}      /* layer 3 */
    }
};

static uns32b _mp3_freq[ 3 ][ 3 ] = {
    {22050,24000,16000}, /* MPEG 2.0 */
    {44100,48000,32000}, /* MPEG 1.0 */
    {11025,12000,8000}   /* MPEG 2.5 */
};

static double _mp3_vers[] = { 2.0, 1.0, 2.5 };

static char *_mp3_emphasis[] = {
    "none", "50/15 ms", "reserved", "CCITT J.17"
};

static char *_mp3_mode[] = {
    "stereo", "joint stereo", "dual channel", "mono"
};

static int _ac3_bitrate[] = {
    32,  40,  48,  56,  64,  80,  96, 112,
    128, 160, 192, 224, 256, 320, 384, 448,
    512, 576, 640
};

static uns8b _ac3_halfrate[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };

static uns8b _ac3_lfeon[8] = { 0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01 };

static char *_ac3_mode[16] = {
    "channel", "mono", "stereo", "3 front",
    "2 front, 1 rear", "3 front, 1 rear", "2 front, 2 rear", "3 front, 2 rear",
    "channel1", "channel2", "dolby", "unknown",
    "unknown", "unknown", "unknown", "unknown" };

extern int dca_sample_rates[];
extern int dca_bit_rates[];
extern uns8b dca_channels[];
extern uns8b dca_bits_per_sample[];

static char *_dca_mode[16] = {
    "A",
    "A + B (dual mono)",
    "L + R (stereo)",
    "(L+R) + (L-R) (sum-difference)",
    "LT +RT (left and right total)",
    "C+L+R",
    "L + R+ S",
    "C + L + R+ S",
    "L + R+ SL+SR",
    "C + L + R+ SL+SR",
    "CL + CR + L + R + SL + SR",
    "C + L + R+ LR + RR + OV",
    "CF+ CR+LF+ RF+LR + RR",
    "CL + C + CR + L + R + SL + SR",
    "CL + CR + L + R + SL1 + SL2+ SR1 + SR2",
    "CL + C+ CR + L + R + SL + S+ S"
};

static uns32b _aac_freq[ 15 ] = {
    96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0
};

static char *_aac_type[ 46 ] = {
    "Null",
    "Main",
    "LC (Low Complexity)",
    "SSR (Scalable Sample Rate)",
    "LTP (Long Term Prediction)",
    "SBR (Spectral Band Replication)",
    "Scalable",
    "TwinVQ",
    "CELP (Code Excited Linear Prediction)",
    "HXVC (Harmonic Vector eXcitation Coding)",
    "Reserved",
    "Reserved",
    "TTSI (Text-To-Speech Interface)",
    "Main Synthesis",
    "Wavetable Synthesis",
    "General MIDI",
    "Algorithmic Synthesis and Audio Effects",
    "ER (Error Resilient) AAC LC",
    "Reserved",
    "ER AAC LTP",
    "ER AAC Scalable",
    "ER TwinVQ",
    "ER BSAC (Bit-Sliced Arithmetic Coding)",
    "ER AAC LD (Low Delay)",
    "ER CELP",
    "ER HVXC",
    "ER HILN (Harmonic and Individual Lines plus Noise)",
    "ER Parametric",
    "SSC (SinuSoidal Coding)",
    "PS (Parametric Stereo)",
    "MPEG Surround",
    "(Escape value)",
    "Layer-1",
    "Layer-2",
    "Layer-3",
    "DST (Direct Stream Transfer)",
    "ALS (Audio Lossless)",
    "SLS (Scalable LosslesS)",
    "SLS non-core",
    "ER AAC ELD (Enhanced Low Delay)",
    "SMR (Symbolic Music Representation) Simple",
    "SMR Main",
    "USAC (Unified Speech and Audio Coding) (no SBR)",
    "SAOC (Spatial Audio Object Coding)",
    "LD MPEG Surround",
    "USAC"
};

static char *_aac_mode[ 16 ] = {
    "unknown", /* FIXME */
    "1: front-center",
    "2: front-left, front-right",
    "3: front-center, front-left, front-right",
    "4: front-center, front-left, front-right, back-center",
    "5: front-center, front-left, front-right, back-left, back-right",
    "5.1: front-center, front-left, front-right, back-left, back-right, LFE-channel",
    "7.1: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel"
    "reserved value",
    "reserved value",
    "reserved value",
    "6.1",
    "7.1",
    "reserved value",
    "7.1 top",
    "reserved value"
};

uns8b trp_aud_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];

    _trp_print_fun[ TRP_AUD ] = trp_aud_print;
    _trp_close_fun[ TRP_AUD ] = trp_aud_close;
    return 0;
}

static uns8b trp_aud_print( trp_print_t *p, trp_aud_t *obj )
{
    if ( trp_print_char_star( p, "#aud" ) )
        return 1;
    if ( obj->fp == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_aud_close( trp_aud_t *obj )
{
    return trp_aud_close_basic( 1, obj );
}

static uns8b trp_aud_close_basic( uns8b flags, trp_aud_t *obj )
{
    if ( obj->fp ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->buf );
        free( obj->encoder );
        free( obj->dca_state );
        memset( obj, 0, sizeof( trp_aud_t ) );
        obj->tipo = TRP_AUD;
        obj->codec = 0xff;
    }
    return 0;
}

static void trp_aud_finalize( void *obj, void *data )
{
    trp_aud_close_basic( 0, (trp_aud_t *)obj );
}

trp_obj_t *trp_aud_create( trp_obj_t *f, trp_obj_t *forced_codec )
{
    trp_obj_t *aud;
    FILE *fp;

    if ( ( fp = trp_file_readable_fp( f ) ) == NULL )
        return UNDEF;
    aud = trp_gc_malloc_atomic_finalize( sizeof( trp_aud_t ), trp_aud_finalize );
    memset( aud, 0, sizeof( trp_aud_t ) );
    aud->tipo = TRP_AUD;
    ((trp_aud_t *)aud)->fp = fp;
    if ( forced_codec )
        if ( forced_codec->tipo == TRP_SIG64 )
            ((trp_aud_t *)aud)->codec = ((trp_sig64_t *)forced_codec)->val;
    return aud;
}

#define flags_field(flags,pos,len) (((flags)>>(pos))&(0xffffffffffffffffLL>>(64-(len))))

uns8b trp_aud_parse_aac_header( trp_obj_t *aud, trp_obj_t *len )
{
    uns64b flags;
    uns32b l, s, i;

    if ( ( aud->tipo != TRP_AUD ) ||
         trp_cast_uns32b( len, &l ) )
        return 1;
    if ( ( ((trp_aud_t *)aud)->fp == NULL ) ||
         ( l < 2 ) || ( l > 8 ) )
        return 1;
    flags = 0;
    if ( trp_file_read_chars( ((trp_aud_t *)aud)->fp, (uns8b *)(&flags), l ) != l )
        return 1;
#ifdef TRP_LITTLE_ENDIAN
    flags = trp_swap_endian64( flags );
#endif
    s = 64 - 5;
    i = flags_field( flags, s, 5);
    if ( i == 31 ) {
        s -= 6;
        i = flags_field( flags, s, 6) + 32;
    }
    ((trp_aud_t *)aud)->h_last.version = i;
    s -= 4;
    ((trp_aud_t *)aud)->h_last.extension = i = flags_field( flags, s, 4);
    if ( i == 15 ) {
        s -= 24;
        i = flags_field( flags, s, 24);
    } else {
        i = _aac_freq[ i ];
    }
    ((trp_aud_t *)aud)->h_last.freq = i;
    s -= 4;
    ((trp_aud_t *)aud)->h_last.mode = flags_field( flags, s, 4);
    s -= 1;
    ((trp_aud_t *)aud)->h_last.emphasis = flags_field( flags, s, 1) ? 960 : 1024;
    ((trp_aud_t *)aud)->h_last.padding = ( ( ((trp_aud_t *)aud)->h_last.version == 5 ) ||
                                           ( ( ((trp_aud_t *)aud)->h_last.version == 2 ) &&
                                             ( ((trp_aud_t *)aud)->h_last.freq <= 24000 ) ) ) ? 1 : 0;
    ((trp_aud_t *)aud)->codec = 7;
    ((trp_aud_t *)aud)->vbr = 1;
    return 0;
}

static void trp_aud_set_mp3_encoder_internal( trp_aud_t *aud, uns8b *buf, uns32b fl )
/*
 FIXME
 andrebbe riscritto secondo specifiche...
 */
{
    uns32b shift, made = 0, i;

    if (        ( strncasecmp( buf + 21, "info", 4 ) == 0 ) ||
                ( strncasecmp( buf + 21, "xing", 4 ) == 0 ) ) {
        shift = 0;
    } else if ( ( strncasecmp( buf + 36, "info", 4 ) == 0 ) ||
                ( strncasecmp( buf + 36, "xing", 4 ) == 0 ) ) {
        shift = 15;
    } else
        return;
    if ( fl < 143 + shift )
        return;
    for ( i = 142 + shift ; i < fl ; i++ ) {
        if ( ( i == 150 + shift ) ||
             ( buf[ i ] == ' ' ) || ( buf[ i ] == 0 ) ) {
            if ( fl > 245 )
                if ( strncasecmp( buf + 240, "made ", 5 ) == 0 ) {
                    for ( made = 245 ; made < fl ; made++ )
                        if ( buf[ made ] == 0 )
                            break;
                    if ( made == fl ) {
                        made = 0;
                    } else {
                        made = made - 237;
                    }
                }
            aud->encoder = trp_malloc( i + made - 140 - shift );
            memcpy( aud->encoder, buf + 141 + shift, i - 141 - shift );
            if ( made ) {
                aud->encoder[ i - 141 - shift ] = ' ';
                aud->encoder[ i - 140 - shift ] = '(';
                memcpy( aud->encoder + i - 139 - shift, buf + 240, made - 3 );
                aud->encoder[ i + made - 142 - shift ] = ')';
            }
            aud->encoder[ i + made - 141 - shift ] = 0;
            break;
        }
        if ( ( buf[ i ] == '\r' ) ||
             ( buf[ i ] == '\n' ) ||
             ( buf[ i ] == '\t' ) )
            break;
    }
}

static void trp_aud_set_mp3_encoder( trp_aud_t *aud, uns8b *buf, uns32b fl )
{
    trp_aud_set_mp3_encoder_internal( aud, buf, fl );
    if ( aud->encoder == NULL ) {
        uns32b i;

        for ( i = 0 ; i + 5 <= fl ; i++ )
            if ( strncmp( buf + i, "Aud-X", 5 ) == 0 ) {
                aud->encoder = trp_malloc( 14 );
                strcpy( aud->encoder, "Aud-X MP3 5.1" );
                break;
            }
    }
}

static uns32b trp_mp3_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy )
/*
 decodifica un header (scrivendolo su h);
 buf deve avere almeno 4 bytes;
 se tutto ok, rende la lunghezza in bytes del frame;
 altrimenti 0
 */
{
    uns32b fl;

    if ( ( ( (int)buf[0] << 4 ) | ( (int)( buf[1] & 0xE0 ) >> 4 ) ) != 0xFFE )
        return 0;
    h->version = ( buf[1] & 0x10 ) ? ( buf[1] >> 3 ) & 1 : 2;
    h->layer = ( buf[1] >> 1 ) & 3;
    h->bitrate = ( buf[2] >> 4 ) & 0xF;
    h->crc = buf[1] & 1;
    h->freq = ( buf[2] >> 2 ) & 0x3;
    h->padding = ( buf[2] >> 1 ) & 0x1;
    h->extension = buf[2] & 0x1;
    h->mode = ( buf[3] >> 6 ) & 0x3;
    h->mode_extension = ( buf[3] >> 4 ) & 0x3;
    h->copyright = ( buf[3] >> 3 ) & 0x1;
    h->original = ( buf[3] >> 2 ) & 0x1;
    h->emphasis = buf[3] & 0x3;
    if ( ( h->bitrate == 0 ) || ( h->bitrate == 0xF ) ||
         ( h->freq == 0x3 ) || ( h->layer == 0 ) )
        return 0;
    h->layer = 3 - h->layer;
    if ( hp )
        if ( ( h->version    != hp->version ) ||
             ( h->layer      != hp->layer ) ||
             ( h->crc        != hp->crc ) ||
             ( h->freq       != hp->freq ) ||
            /*
             ( h->copyright  != hp->copyright ) ||
             ( h->original   != hp->original ) ||
             */
             ( h->emphasis   != hp->emphasis ) ||
             ( h->mode       != hp->mode ) )
            return 0;
    if ( h->padding && ( h->layer == 0 ) )
        h->padding = 4;
    fl = ( _mp3_frame_size_index[ h->layer ] * ( ( h->version & 1 ) + 1 ) *
           _mp3_bitrate[ h->version & 1 ][ h->layer ][ h->bitrate ] ) /
        _mp3_freq[ h->version ][ h->freq ] + h->padding;
    return ( ( fl >= MP3_MIN_FRAME_SIZE ) &&
             ( fl <= MP3_MAX_FRAME_SIZE ) ) ? fl : 0;
}

static uns32b trp_ac3_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy )
{
    uns32b fl = 0;
    uns32b bitrate;
    uns32b frmsizecod;
    uns32b half;
    uns32b acmod;

    if ( ( buf[0] != 0x0b ) || ( buf[1] != 0x77 ) ) /* syncword */
        return 0;

    if ( buf[5] >= 0x60 ) /* bsid >= 12 */
        return 0;

    memset( h, 0, sizeof( aud_header_t ) );

    half = _ac3_halfrate[ buf[5] >> 3 ];

    /* acmod, dsurmod and lfeon */
    acmod = buf[6] >> 5;
    h->mode = ( ( ( ( buf[6] & 0xf8 ) == 0x50 ) ? A52_DOLBY : acmod ) |
                ( ( buf[6] & _ac3_lfeon[ acmod ] ) ? A52_LFE : 0 ) );

    frmsizecod = buf[4] & 63;
    if ( frmsizecod >= 38 )
        return 0;
    bitrate = _ac3_bitrate[ frmsizecod >> 1 ];
    h->bitrate = bitrate >> half;

    switch ( buf[4] & 0xc0 ) {
    case 0:
        h->freq = 48000 >> half;
        fl = 4 * bitrate;
        break;
    case 0x40:
        h->freq = 44100 >> half;
        fl = 2 * ( 320 * bitrate / 147 + ( frmsizecod & 1 ) );
        break;
    case 0x80:
        h->freq = 32000 >> half;
        fl = 6 * bitrate;
        break;
    }
    if ( hp )
        if ( ( h->freq       != hp->freq ) ||
             ( ( h->mode != hp->mode ) &&
               ( ( ( h->mode != 2 ) && ( h->mode != 10 ) ) ||
                 ( ( hp->mode != 2 ) && ( hp->mode != 10 ) ) ) ) ||
             ( h->bitrate    != hp->bitrate ) )
            return 0;
    return ( ( fl >= AC3_MIN_FRAME_SIZE ) &&
             ( fl <= AC3_MAX_FRAME_SIZE ) ) ? fl : 0;
}

static uns32b trp_dts_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy )
{
    int flags, sample_rate, bit_rate, frame_length;
    uns32b fl;

    if ( aud->dca_state == NULL )
        if ( ( aud->dca_state = dca_init ( 0 ) ) == NULL )
            return 0;
    fl = (uns32b)dca_syncinfo( aud->dca_state, buf, &flags, &sample_rate, &bit_rate, &frame_length );
    if ( fl == 0 )
        return 0;
    dca_frame( aud->dca_state, buf );
    if ( aud->dca_state->bit_rate > 28 )
        return 0;
    memset( h, 0, sizeof( aud_header_t ) );
    h->freq = dca_sample_rates[ aud->dca_state->sample_rate ];
    h->mode = aud->dca_state->amode;
    h->bitrate = aud->dca_state->bit_rate;
    if ( hp )
        if ( ( h->freq       != hp->freq ) ||
             ( h->mode       != hp->mode ) ||
             ( h->bitrate    != hp->bitrate ) )
            return 0;
    return fl;
}

static uns32b trp_aac_get_header( uns8b *buf, aud_header_t *h, aud_header_t *hp, trp_aud_t *aud, uns32b dummy )
{
    return ( aud->codec == 7 ) ? dummy : 0;
}

#define TRP_LIMITE_BOH 262144

static uns8b trp_aud_parse_internal( uns8b flags, trp_aud_t *aud, uns32b l, trp_raw_t *stripped )
{
    uns32b ll, read, skip, fl, splitsgn, min_frame_size, max_frame_size, fakes = 0, i;
    uns8b *p;
    uns8b res = 0, codec = aud->codec & 0xfe;
    uns32bfun_t get_header;

    if ( stripped ) {
        ll = stripped->len;
    } else {
        ll = 0;
    }
    switch ( codec ) {
    case 0:
        get_header = trp_mp3_get_header;
        min_frame_size = MP3_MIN_FRAME_SIZE;
        max_frame_size = MP3_MAX_FRAME_SIZE;
        break;
    case 2:
        get_header = trp_ac3_get_header;
        min_frame_size = AC3_MIN_FRAME_SIZE;
        max_frame_size = AC3_MAX_FRAME_SIZE;
        break;
    case 4:
        get_header = trp_dts_get_header;
        min_frame_size = DTS_MIN_FRAME_SIZE;
        max_frame_size = DTS_MAX_FRAME_SIZE;
        break;
    case 6:
        get_header = trp_aac_get_header;
        min_frame_size = AAC_MIN_FRAME_SIZE;
        max_frame_size = AAC_MAX_FRAME_SIZE;
        break;
    }
    splitsgn = aud->buf_act;
    if ( ll || l ) {
        i = aud->buf_act + ll + l;

        if ( aud->buf_len < i ) {
            if ( ( i > MAX_BUF_SIZE ) ||
                 ( aud->internal_skip > aud->tot_bytes ) ) {
                aud->codec = codec;
                return 1;
            }
            if ( aud->buf_len >= TRP_LIMITE_BOH )
                if ( ( aud->frames < ( aud->buf_len - (TRP_LIMITE_BOH-max_frame_size) ) / max_frame_size ) ) {
                    aud->codec = codec;
                    return 1;
                }
            aud->buf = trp_realloc( aud->buf, i );
            aud->buf_len = i;
        }
        if ( ll ) {
            memcpy( aud->buf + aud->buf_act, stripped->data, ll );
            aud->buf_act += ll;
            aud->tot_read += ll;
        }
        if ( l ) {
            i = fread( aud->buf + aud->buf_act, 1, l, aud->fp );
            if ( i == 0 )
                res = 1;
            aud->buf_act += i;
            aud->tot_read += i;
        }
    }
    for ( l = 0 ; ; splitsgn = 0 ) {
        i = aud->buf_act - l;
        p = aud->buf + l;
        for ( skip = 0 ; ; i--, p++, skip++ ) {
            if ( i < min_frame_size )
                break;
            fl = (get_header)( p, &( aud->h_last ),
                               ( aud->frames > 1 ) ? &( aud->h_first ) : NULL,
                               aud, i );
            if ( fl )
                break;
        }
        if ( ( i < min_frame_size ) || ( i < fl ) )
            break;
        read = fl + skip;
        /*
         printf( "#%d header di lunghezza: %d\n", codec, fl );
         */
        if ( skip < splitsgn )
            if ( read > splitsgn )
                aud->splitted = 1;
        if ( aud->frames == 1 )
            if ( ( ( codec == 0 ) &&
                   ( ( aud->h_last.version    != aud->h_first.version ) ||
                     ( aud->h_last.layer      != aud->h_first.layer ) ||
                     ( aud->h_last.crc        != aud->h_first.crc ) ||
                     ( aud->h_last.freq       != aud->h_first.freq ) ||
                     /*
                     ( aud->h_last.copyright  != aud->h_first.copyright ) ||
                     ( aud->h_last.original   != aud->h_first.original ) ||
                     */
                     ( aud->h_last.emphasis   != aud->h_first.emphasis ) ||
                     ( aud->h_last.mode       != aud->h_first.mode ) ) ) ||
                 ( ( ( codec == 2 ) || ( codec == 4 ) ) &&
                   ( ( aud->h_last.freq       != aud->h_first.freq ) ||
                     ( aud->h_last.mode       != aud->h_first.mode ) ||
                     ( aud->h_last.bitrate    != aud->h_first.bitrate ) ) ) ) {
                if ( aud->fpout )
                    fseeko( aud->fpout, 0, SEEK_SET );
                if ( ( aud->buf_act == aud->tot_read ) &&
                     ( fakes < 2 ) ) {
                    /*
                     potrei aver letto un frame fittizio...
                     ignoro il precedente...
                     */
                    aud->initial_skip = l;
                    aud->tot_bytes = 0;
                    aud->tot_samples = 0;
                    aud->frames = 0;
                    aud->splitted = 0;
                    free( aud->encoder );
                    free( aud->dca_state );
                    aud->encoder = NULL;
                    aud->dca_state = NULL;
                    fakes++;
                } else {
                    aud->codec = codec;
                    l = 0;
                    res = 1;
                    break;
                }
            }
        if ( aud->frames ) {
            if ( aud->h_last.padding )
                aud->h_first.padding = aud->h_last.padding;
            if ( aud->h_last.bitrate !=  aud->h_first.bitrate )
                aud->vbr = 1;
            aud->internal_skip += skip;
        } else {
            memcpy( &( aud->h_first ), &( aud->h_last ), sizeof( aud_header_t ) );
            aud->initial_skip += skip;
            if ( codec == 0 )
                trp_aud_set_mp3_encoder( aud, aud->buf + l + skip, fl );
        }
        aud->tot_bytes += fl;
        switch ( codec ) {
        case 0:
            aud->tot_samples += 32 * ( ( aud->h_first.layer == 0 ) ?
                                       12 : ( ( ( aud->h_first.layer == 2 ) &&
                                                ( aud->h_first.version != 1 ) ) ? 18 : 36 ) );
            break;
        case 4:
            /*
             FIXME
             tenere conto di aud->dca_state->samples_deficit
             */
            aud->tot_samples += ( aud->dca_state->sample_blocks << 5 );
            break;
        case 6:
            aud->tot_samples += aud->h_first.emphasis;
        default:
            break;
        }
        if ( aud->fpout ) {
            if ( codec == 6 ) {
                char adts[ 7 ];

                i = read + 7;
                adts[0]  = 0xff;
                adts[1]  = 0xf1;
                adts[2]  = ( aud->h_first.version - 1 ) << 6;
                adts[2] |= aud->h_first.extension << 2;
                adts[2] |= ( aud->h_first.mode & 4 ) >> 2;
                adts[3]  = ( aud->h_first.mode & 3 ) << 6;
                adts[3] |= i >> 11;
                adts[4]  = ( i >> 3 ) & 0xff;
                adts[5]  = ( ( i & 7 ) << 5 ) | 0x1f;
                adts[6]  = 0xfc;
                (void)trp_file_write_chars( aud->fpout, adts, 7 );
            }
            if ( aud->fpout_skip_garbage )
                (void)trp_file_write_chars( aud->fpout, aud->buf + l + skip, fl );
            else
                (void)trp_file_write_chars( aud->fpout, aud->buf + l, read );
        }
        aud->frames++;
        l += read;
        if ( flags & 1 )
            if ( aud->frames > 1 )
                break;
    }
    if ( l ) {
        if ( ( aud->frames == 1 ) && ( codec != 6 ) ) {
            /*
             potrei aver letto un frame fittizio...
             faccio finta di non averlo visto...
             */
            aud->initial_skip = 0;
            aud->tot_bytes = 0;
            aud->tot_samples = 0;
            aud->frames = 0;
            aud->splitted = 0;
            free( aud->encoder );
            free( aud->dca_state );
            aud->encoder = NULL;
            aud->dca_state = NULL;
            if ( aud->fpout )
                fseeko( aud->fpout, 0, SEEK_SET );
        } else {
            aud->codec = codec | 1;
            aud->buf_act -= l;
            if ( aud->buf_act )
                memmove( aud->buf, aud->buf + l, aud->buf_act );
            res = 0;
        }
    }
    return res;
}

uns8b trp_aud_parse( trp_obj_t *aud, trp_obj_t *len, trp_obj_t *stripped )
{
    FILE *fp, *fpout;
    uns8b *buf;
    uns32b buf_len;
    uns32b buf_act;
    uns32b l;
    uns8b codec, skipg;

    if ( ( aud->tipo != TRP_AUD ) ||
         trp_cast_uns32b( len, &l ) )
        return 1;
    if ( ( ((trp_aud_t *)aud)->fp == NULL ) ||
         ( l > MAX_BUF_SIZE ) )
        return 1;
    if ( stripped )
        if ( stripped->tipo != TRP_RAW )
            return 1;
    while ( trp_aud_parse_internal( 0, (trp_aud_t *)aud, l, (trp_raw_t *)stripped ) ) {
        codec = ((trp_aud_t *)aud)->codec;
        if ( codec & 1 )
            return 1;
        if ( ( codec == 6 ) || ( ((trp_aud_t *)aud)->buf_act != ((trp_aud_t *)aud)->tot_read ) ) {
            trp_aud_close( (trp_aud_t *)aud );
            return 1;
        }
        fp = ((trp_aud_t *)aud)->fp;
        fpout = ((trp_aud_t *)aud)->fpout;
        buf = ((trp_aud_t *)aud)->buf;
        buf_len = ((trp_aud_t *)aud)->buf_len;
        buf_act = ((trp_aud_t *)aud)->buf_act;
        skipg = ((trp_aud_t *)aud)->fpout_skip_garbage;
        free( ((trp_aud_t *)aud)->encoder );
        free( ((trp_aud_t *)aud)->dca_state );
        memset( ((trp_aud_t *)aud), 0, sizeof( trp_aud_t ) );
        ((trp_aud_t *)aud)->tipo = TRP_AUD;
        ((trp_aud_t *)aud)->codec = codec + 2;
        ((trp_aud_t *)aud)->fp = fp;
        ((trp_aud_t *)aud)->fpout = fpout;
        ((trp_aud_t *)aud)->fpout_skip_garbage = skipg;
        ((trp_aud_t *)aud)->buf = buf;
        ((trp_aud_t *)aud)->buf_len = buf_len;
        ((trp_aud_t *)aud)->buf_act = buf_act;
        ((trp_aud_t *)aud)->tot_read = buf_act;
        if ( fpout )
            fseeko( fpout, 0, SEEK_SET );
        l = 0;
    }
    return 0;
}

#define TRP_AUD_PARSE_STEP_L 32768

static uns8b trp_aud_parse_step_internal( trp_aud_t *aud )
{
    uns32b i;

    if ( ( aud->codec == 0 ) ||
         ( aud->codec == 1 ) ) {
        if ( aud->frames == 0 ) {
            for ( ; ; ) {
                if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) )
                    break;
                if ( aud->frames )
                    break;
            }
            if ( ( aud->codec == 1 ) && aud->frames )
                return 0;
        }
        i = aud->frames;
        if ( i && ( aud->codec == 1 ) )
            if ( trp_aud_parse_internal( 1, aud, 0, (trp_raw_t *)NULL ) == 0 )
                if ( aud->codec == 1 ) {
                    if ( aud->frames > i )
                        return 0;
                    if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) == 0 )
                        if ( aud->codec == 1 )
                            if ( aud->frames > i )
                                return 0;
                }
        if ( aud->buf_act == aud->tot_read ) {
            FILE *fp = aud->fp, *fpout = aud->fpout;
            uns8b *buf = aud->buf;
            uns32b buf_len = aud->buf_len;
            uns32b buf_act = aud->buf_act;
            uns8b skipg = aud->fpout_skip_garbage;
            free( aud->encoder );
            free( aud->dca_state );
            memset( aud, 0, sizeof( trp_aud_t ) );
            aud->tipo = TRP_AUD;
            aud->codec = 2;
            aud->fp = fp;
            aud->fpout = fpout;
            aud->fpout_skip_garbage = skipg;
            aud->buf = buf;
            aud->buf_len = buf_len;
            aud->buf_act = buf_act;
            aud->tot_read = buf_act;
            if ( fpout )
                fseeko( fpout, 0, SEEK_SET );
        } else {
            trp_aud_close( aud );
            return 1;
        }
    }
    if ( ( aud->codec == 2 ) ||
         ( aud->codec == 3 ) ) {
        if ( aud->frames == 0 ) {
            for ( ; ; ) {
                if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) )
                    break;
                if ( aud->frames )
                    break;
            }
            if ( ( aud->codec == 3 ) && aud->frames )
                return 0;
        }
        i = aud->frames;
        if ( i && ( aud->codec == 3 ) )
            if ( trp_aud_parse_internal( 1, aud, 0, (trp_raw_t *)NULL ) == 0 )
                if ( aud->codec == 3 ) {
                    if ( aud->frames > i )
                        return 0;
                    if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) == 0 )
                        if ( aud->codec == 3 )
                            if ( aud->frames > i )
                                return 0;
                }
        if ( aud->buf_act == aud->tot_read ) {
            FILE *fp = aud->fp, *fpout = aud->fpout;
            uns8b *buf = aud->buf;
            uns32b buf_len = aud->buf_len;
            uns32b buf_act = aud->buf_act;
            uns8b skipg = aud->fpout_skip_garbage;
            free( aud->dca_state );
            memset( ((trp_aud_t *)aud), 0, sizeof( trp_aud_t ) );
            aud->tipo = TRP_AUD;
            aud->codec = 4;
            aud->fp = fp;
            aud->fpout = fpout;
            aud->fpout_skip_garbage = skipg;
            aud->buf = buf;
            aud->buf_len = buf_len;
            aud->buf_act = buf_act;
            aud->tot_read = buf_act;
            if ( fpout )
                fseeko( fpout, 0, SEEK_SET );
        } else {
            trp_aud_close( aud );
            return 1;
        }
    }
    if ( ( aud->codec == 4 ) ||
         ( aud->codec == 5 ) ) {
        if ( aud->frames == 0 ) {
            for ( ; ; ) {
                if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) )
                    break;
                if ( aud->frames )
                    break;
            }
            if ( ( aud->codec == 5 ) && aud->frames )
                return 0;
        }
        i = aud->frames;
        if ( i && ( aud->codec == 5 ) )
            if ( trp_aud_parse_internal( 1, aud, 0, (trp_raw_t *)NULL ) == 0 )
                if ( aud->codec == 5 ) {
                    if ( aud->frames > i )
                        return 0;
                    if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) == 0 )
                        if ( aud->codec == 5 )
                            if ( aud->frames > i )
                                return 0;
                }
        if ( aud->buf_act == aud->tot_read ) {
            FILE *fp = aud->fp, *fpout = aud->fpout;
            uns8b *buf = aud->buf;
            uns32b buf_len = aud->buf_len;
            uns32b buf_act = aud->buf_act;
            uns8b skipg = aud->fpout_skip_garbage;
            free( aud->dca_state );
            memset( ((trp_aud_t *)aud), 0, sizeof( trp_aud_t ) );
            aud->tipo = TRP_AUD;
            aud->codec = 6;
            aud->fp = fp;
            aud->fpout = fpout;
            aud->fpout_skip_garbage = skipg;
            aud->buf = buf;
            aud->buf_len = buf_len;
            aud->buf_act = buf_act;
            aud->tot_read = buf_act;
            if ( fpout )
                fseeko( fpout, 0, SEEK_SET );
        } else {
            trp_aud_close( aud );
            return 1;
        }
    }
    if ( ( aud->codec == 6 ) ||
         ( aud->codec == 7 ) ) {
        if ( aud->frames == 0 ) {
            for ( ; ; ) {
                if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) )
                    break;
                if ( aud->frames )
                    break;
            }
            if ( ( aud->codec == 7 ) && aud->frames )
                return 0;
        }
        i = aud->frames;
        if ( i && ( aud->codec == 7 ) )
            if ( trp_aud_parse_internal( 1, aud, 0, (trp_raw_t *)NULL ) == 0 )
                if ( aud->codec == 7 ) {
                    if ( aud->frames > i )
                        return 0;
                    if ( trp_aud_parse_internal( 1, aud, TRP_AUD_PARSE_STEP_L, (trp_raw_t *)NULL ) == 0 )
                        if ( aud->codec == 7 )
                            if ( aud->frames > i )
                                return 0;
                }
        if ( aud->buf_act == aud->tot_read ) {
            FILE *fp = aud->fp, *fpout = aud->fpout;
            uns8b *buf = aud->buf;
            uns32b buf_len = aud->buf_len;
            uns32b buf_act = aud->buf_act;
            uns8b skipg = aud->fpout_skip_garbage;
            free( aud->dca_state );
            memset( ((trp_aud_t *)aud), 0, sizeof( trp_aud_t ) );
            aud->tipo = TRP_AUD;
            aud->codec = 8;
            aud->fp = fp;
            aud->fpout = fpout;
            aud->fpout_skip_garbage = skipg;
            aud->buf = buf;
            aud->buf_len = buf_len;
            aud->buf_act = buf_act;
            aud->tot_read = buf_act;
            if ( fpout )
                fseeko( fpout, 0, SEEK_SET );
        } else {
            trp_aud_close( aud );
            return 1;
        }
    }
    trp_aud_close( aud );
    return 1;
}

uns8b trp_aud_parse_step( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return 1;
    if ( ((trp_aud_t *)aud)->fp == NULL )
        return 1;
    return trp_aud_parse_step_internal( (trp_aud_t *)aud );
}

uns8b trp_aud_fpout_begin( trp_obj_t *aud, trp_obj_t *f, trp_obj_t *skip_garbage )
{
    FILE *fpout = trp_file_writable_fp( f );
    uns8b skipg;

    if ( ( aud->tipo != TRP_AUD ) || ( fpout == NULL ) )
        return 1;
    if ( ((trp_aud_t *)aud)->fpout )
        return 1;
    if ( skip_garbage ) {
        if ( !TRP_BOOLP( skip_garbage ) )
            return 1;
        skipg = ( skip_garbage == TRP_TRUE ) ? 1 : 0;
    } else
        skipg = 0;
    ((trp_aud_t *)aud)->fpout = fpout;
    ((trp_aud_t *)aud)->fpout_skip_garbage = skipg;
    return 0;
}

uns8b trp_aud_fpout_end( trp_obj_t *aud )
{
    uns32b i;

    if ( aud->tipo != TRP_AUD )
        return 1;
    if ( ((trp_aud_t *)aud)->fpout == NULL )
        return 1;
    if ( ((trp_aud_t *)aud)->fpout_skip_garbage == 0 )
        if ( i = ((trp_aud_t *)aud)->buf_act )
            if ( trp_file_write_chars( ((trp_aud_t *)aud)->fpout, ((trp_aud_t *)aud)->buf, i ) != i )
                return 1;
    ((trp_aud_t *)aud)->fpout = NULL;
    return 0;
}

trp_obj_t *trp_aud_codec( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_cord( "MP3" );
        break;
    case 3:
        res = trp_cord( "AC3" );
        break;
    case 5:
        res = trp_cord( "DTS" );
        break;
    case 7:
        res = trp_cord( "AAC" );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_version( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_double( _mp3_vers[ ((trp_aud_t *)aud)->h_first.version ] );
        break;
    case 7:
        res = trp_sig64( ((trp_aud_t *)aud)->h_first.version );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_layer( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_sig64( ((trp_aud_t *)aud)->h_first.layer + 1 );
        break;
    case 7:
        if ( ((trp_aud_t *)aud)->h_first.version > 45 )
            res = UNDEF;
        else if ( ( ((trp_aud_t *)aud)->h_first.version == 2 ) &&
                  ((trp_aud_t *)aud)->h_first.padding )
            res = trp_cord( "LC-SBR (Spectral Band Replication)" );
        else
            res = trp_cord( _aac_type[ ((trp_aud_t *)aud)->h_first.version ] );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_frames( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return trp_sig64( ((trp_aud_t *)aud)->frames );
}

trp_obj_t *trp_aud_duration( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_samples ),
                              trp_sig64( _mp3_freq[ ((trp_aud_t *)aud)->h_first.version ][ ((trp_aud_t *)aud)->h_first.freq ] ),
                              NULL );
        break;
    case 3:
        res = trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_bytes ),
                              trp_sig64( ((trp_aud_t *)aud)->h_first.bitrate ),
                              trp_sig64( 125 ),
                              NULL );
        break;
    case 5:
    case 7:
        res = trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_samples ),
                              trp_sig64( ((trp_aud_t *)aud)->h_first.freq ),
                              NULL );
        break;
    default:
        res = ZERO;
        break;
    }
    return ( res == UNDEF ) ? ZERO : res;
}

trp_obj_t *trp_aud_vbr( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return ((trp_aud_t *)aud)->vbr ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_aud_padding( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return ((trp_aud_t *)aud)->h_first.padding ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_aud_bitrate( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        if ( ((trp_aud_t *)aud)->vbr == 0 )
            res = trp_sig64( _mp3_bitrate[ ((trp_aud_t *)aud)->h_first.version & 1 ]
                             [ ((trp_aud_t *)aud)->h_first.layer ]
                             [ ((trp_aud_t *)aud)->h_first.bitrate ] );
        else
            res = trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_bytes ),
                                  trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_samples ),
                                                  trp_sig64( _mp3_freq[ ((trp_aud_t *)aud)->h_first.version ][ ((trp_aud_t *)aud)->h_first.freq ] ),
                                                  NULL ),
                                  trp_sig64( 125 ),
                                  NULL );
        break;
    case 3:
        res = trp_sig64( ((trp_aud_t *)aud)->h_first.bitrate );
        break;
    case 5:
        res = trp_math_ratio( trp_sig64( dca_bit_rates[ ((trp_aud_t *)aud)->h_first.bitrate ] ),
                              trp_sig64( 1000 ),
                              NULL );
        break;
    case 7:
        res = trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_bytes ),
                              trp_math_ratio( trp_sig64( ((trp_aud_t *)aud)->tot_samples ),
                                              trp_sig64( ((trp_aud_t *)aud)->h_first.freq ),
                                              NULL ),
                              trp_sig64( 125 ),
                              NULL );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_frequency( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_sig64( _mp3_freq[ ((trp_aud_t *)aud)->h_first.version ]
                         [ ((trp_aud_t *)aud)->h_first.freq ] );
        break;
    case 3:
    case 5:
    case 7:
        res = trp_sig64( ((trp_aud_t *)aud)->h_first.freq );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_emphasis( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_cord( _mp3_emphasis[ ((trp_aud_t *)aud)->h_first.emphasis ] );
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_mode( trp_obj_t *aud )
{
    trp_obj_t *res;

    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    switch ( ((trp_aud_t *)aud)->codec ) {
    case 1:
        res = trp_cord( _mp3_mode[ ((trp_aud_t *)aud)->h_first.mode ] );
        break;
    case 3:
        {
            char buf[ 23 ];
            sprintf( buf, "%s%s",
                     _ac3_mode[ ((trp_aud_t *)aud)->h_first.mode & A52_CHANNEL_MASK ],
                     ( ((trp_aud_t *)aud)->h_first.mode & A52_LFE ) ? ", 1 LFE" : "" );
            res = trp_cord( buf );
        }
        break;
    case 5:
        if ( ((trp_aud_t *)aud)->h_first.mode < 16 )
            res = trp_cord( _dca_mode[ ((trp_aud_t *)aud)->h_first.mode ] );
        else
            res = UNDEF;
        break;
    case 7:
        if ( ((trp_aud_t *)aud)->h_first.mode < 16 )
            res = trp_cord( _aac_mode[ ((trp_aud_t *)aud)->h_first.mode ] );
        else
            res = UNDEF;
        break;
    default:
        res = UNDEF;
        break;
    }
    return res;
}

trp_obj_t *trp_aud_initial_skip( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return trp_sig64( ((trp_aud_t *)aud)->initial_skip );
}

trp_obj_t *trp_aud_internal_skip( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return trp_sig64( ((trp_aud_t *)aud)->internal_skip );
}

trp_obj_t *trp_aud_buf_act( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return trp_sig64( ((trp_aud_t *)aud)->buf_act );
}

trp_obj_t *trp_aud_tot_read( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return trp_sig64( ((trp_aud_t *)aud)->tot_read );
}

trp_obj_t *trp_aud_encoder( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    if ( ((trp_aud_t *)aud)->encoder == NULL )
        return UNDEF;
    return trp_cord( ((trp_aud_t *)aud)->encoder );
}

trp_obj_t *trp_aud_splitted( trp_obj_t *aud )
{
    if ( aud->tipo != TRP_AUD )
        return UNDEF;
    return ((trp_aud_t *)aud)->splitted ? TRP_TRUE : TRP_FALSE;
}

