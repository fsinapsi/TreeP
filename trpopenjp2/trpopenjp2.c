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
#include "./trpopenjp2.h"
#include <openjpeg.h>
#include <lcms2.h>
#include "color.h"
#include "../trppix/trppix_internal.h"
#include <sys/stat.h>

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

typedef struct {
    FILE  *fp;
    uns8b *data;
    uns32b size;
    uns32b pos;
} trp_openjp2_data_t;

#ifdef MINGW
#define fseeko fseeko64
#endif

static uns8b trp_pix_load_openjp2( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_openjp2_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

uns8b trp_openjp2_init()
{
    extern uns8bfun_t _trp_pix_load_openjp2;
    extern uns8bfun_t _trp_pix_load_openjp2_memory;

    _trp_pix_load_openjp2 = trp_pix_load_openjp2;
    _trp_pix_load_openjp2_memory = trp_pix_load_openjp2_memory;
    return 0;
}

trp_obj_t *trp_openjp2_version()
{
    return trp_cord( opj_version() );
}

trp_obj_t *trp_openjp2_has_thread_support()
{
    return opj_has_thread_support() ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_openjp2_get_num_cpus()
{
    return trp_sig64( opj_get_num_cpus() );
}

static opj_image_t *trpopenjp2_gray2rgb( opj_image_t *original )
{
    OPJ_UINT32 compno;
    opj_image_t *l_new_image = NULL;
    opj_image_cmptparm_t *l_new_components = NULL;

    l_new_components = (opj_image_cmptparm_t*)malloc( ( original->numcomps + 2U ) * sizeof( opj_image_cmptparm_t ) );
    if ( l_new_components == NULL ) {
        opj_image_destroy( original );
        return NULL;
    }

    l_new_components[0].dx   = l_new_components[1].dx   = l_new_components[2].dx   =
                                   original->comps[0].dx;
    l_new_components[0].dy   = l_new_components[1].dy   = l_new_components[2].dy   =
                                   original->comps[0].dy;
    l_new_components[0].h    = l_new_components[1].h    = l_new_components[2].h    =
                                   original->comps[0].h;
    l_new_components[0].w    = l_new_components[1].w    = l_new_components[2].w    =
                                   original->comps[0].w;
    l_new_components[0].prec = l_new_components[1].prec = l_new_components[2].prec =
                                   original->comps[0].prec;
    l_new_components[0].sgnd = l_new_components[1].sgnd = l_new_components[2].sgnd =
                                   original->comps[0].sgnd;
    l_new_components[0].x0   = l_new_components[1].x0   = l_new_components[2].x0   =
                                   original->comps[0].x0;
    l_new_components[0].y0   = l_new_components[1].y0   = l_new_components[2].y0   =
                                   original->comps[0].y0;

    for (compno = 1U; compno < original->numcomps; ++compno) {
        l_new_components[compno + 2U].dx   = original->comps[compno].dx;
        l_new_components[compno + 2U].dy   = original->comps[compno].dy;
        l_new_components[compno + 2U].h    = original->comps[compno].h;
        l_new_components[compno + 2U].w    = original->comps[compno].w;
        l_new_components[compno + 2U].prec = original->comps[compno].prec;
        l_new_components[compno + 2U].sgnd = original->comps[compno].sgnd;
        l_new_components[compno + 2U].x0   = original->comps[compno].x0;
        l_new_components[compno + 2U].y0   = original->comps[compno].y0;
    }

    l_new_image = opj_image_create( original->numcomps + 2U, l_new_components,
                                    OPJ_CLRSPC_SRGB );
    free( l_new_components );
    if ( l_new_image == NULL ) {
        opj_image_destroy( original );
        return NULL;
    }

    l_new_image->x0 = original->x0;
    l_new_image->x1 = original->x1;
    l_new_image->y0 = original->y0;
    l_new_image->y1 = original->y1;

    l_new_image->comps[0].factor        = l_new_image->comps[1].factor        =
            l_new_image->comps[2].factor        = original->comps[0].factor;
    l_new_image->comps[0].alpha         = l_new_image->comps[1].alpha         =
            l_new_image->comps[2].alpha         = original->comps[0].alpha;
    l_new_image->comps[0].resno_decoded = l_new_image->comps[1].resno_decoded =
            l_new_image->comps[2].resno_decoded = original->comps[0].resno_decoded;

    memcpy(l_new_image->comps[0].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);
    memcpy(l_new_image->comps[1].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);
    memcpy(l_new_image->comps[2].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);

    for (compno = 1U; compno < original->numcomps; ++compno) {
        l_new_image->comps[compno + 2U].factor        = original->comps[compno].factor;
        l_new_image->comps[compno + 2U].alpha         = original->comps[compno].alpha;
        l_new_image->comps[compno + 2U].resno_decoded =
            original->comps[compno].resno_decoded;
        memcpy(l_new_image->comps[compno + 2U].data, original->comps[compno].data,
               sizeof(OPJ_INT32) * original->comps[compno].w * original->comps[compno].h);
    }
    opj_image_destroy( original );
    return l_new_image;
}

static int trpopenjp2_are_comps_similar( opj_image_t *image )
{
    unsigned int i;

    for ( i = 1 ; i < image->numcomps ; i++ )
        if ( ( image->comps[0].dx != image->comps[i].dx ) ||
             ( image->comps[0].dy != image->comps[i].dy ) ||
             ( (i <= 2 ) &&
               ( ( image->comps[0].prec != image->comps[i].prec ) ||
                 ( image->comps[0].sgnd != image->comps[i].sgnd ) ) ) )
            return 0;
    return 1;
}

static uns8b trpopenjp2_image2trp( opj_image_t *image, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *p;
    int *red, *green, *blue, *alpha;
    unsigned int compno, ncomp;
    int adjustR, adjustG, adjustB, adjustA;
    int two, has_alpha;
    int prec, v;
    uns32b i;

    if ( ( prec = (int)image->comps[0].prec ) > 16 )
        return 1;
    ncomp = image->numcomps;
    if ( ( ncomp < 3 ) || ( trpopenjp2_are_comps_similar( image ) == 0 ) )
        return 1;
    red = image->comps[0].data;
    green = image->comps[1].data;
    blue = image->comps[2].data;
    if ( ( red == NULL ) || ( green == NULL ) || ( blue == NULL ) )
        return 1;

    *w = (int)image->comps[0].w;
    *h = (int)image->comps[0].h;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL )
        return 1;

    two = ( prec > 8 );
    has_alpha = ( ( ncomp == 4 ) || ( ncomp == 2 ) );

    if ( has_alpha ) {
        alpha = image->comps[ ncomp - 1 ].data;
        adjustA = ( image->comps[ ncomp - 1 ].sgnd ? 1 << ( image->comps[ ncomp - 1 ].prec - 1 ) : 0 );
    } else {
        alpha = NULL;
        adjustA = 0;
    }
    adjustR = ( image->comps[0].sgnd ? 1 << ( image->comps[0].prec - 1 ) : 0 );
    adjustG = ( image->comps[1].sgnd ? 1 << ( image->comps[1].prec - 1 ) : 0 );
    adjustB = ( image->comps[2].sgnd ? 1 << ( image->comps[2].prec - 1 ) : 0 );

    for ( i = 0, p = *data ; i < *w * *h ; i++ ) {
        if ( two ) {
            v = *red + adjustR;
            ++red;
            if (v > 65535) {
                v = 65535;
            } else if (v < 0) {
                v = 0;
            }
            *p++ = ( v >> 8 );

            v = *green + adjustG;
            ++green;
            if (v > 65535) {
                v = 65535;
            } else if (v < 0) {
                v = 0;
            }
            *p++ = ( v >> 8 );

            v =  *blue + adjustB;
            ++blue;
            if (v > 65535) {
                v = 65535;
            } else if (v < 0) {
                v = 0;
            }
            *p++ = ( v >> 8 );

            if ( has_alpha ) {
                v = *alpha + adjustA;
                ++alpha;
                if (v > 65535) {
                    v = 65535;
                } else if (v < 0) {
                    v = 0;
                }
            } else
                v = 0xffff;
            *p++ = ( v >> 8 );
            continue;
        }

        /* prec <= 8: */
        v = *red++;
        if (v > 255) {
            v = 255;
        } else if (v < 0) {
            v = 0;
        }
        *p++ = v;

        v = *green++;
        if (v > 255) {
            v = 255;
        } else if (v < 0) {
            v = 0;
        }
        *p++ = v;

        v = *blue++;
        if (v > 255) {
            v = 255;
        } else if (v < 0) {
            v = 0;
        }
        *p++ = v;

        if ( has_alpha ) {
            v = *alpha++;
            if (v > 255) {
                v = 255;
            } else if (v < 0) {
                v = 0;
            }
        } else
            v = 0xff;
        *p++ = v;
    }
    return 0;
}

static opj_image_t *trpopenjp2_trp2image( uns32b w, uns32b h, uns8b *data, uns32b numcomps, opj_cparameters_t *parameters )
{
    opj_image_t *image;
    int subsampling_dx = parameters->subsampling_dx;
    int subsampling_dy = parameters->subsampling_dy;
    opj_image_cmptparm_t cmptparm[ 4 ]; /* RGBA: max. 4 components */
    uns32b i;

    memset( &cmptparm[ 0 ], 0, (size_t)numcomps * sizeof( opj_image_cmptparm_t ) );

    for ( i = 0 ; i < numcomps ; i++ ) {
        cmptparm[ i ].prec = 8;
        cmptparm[ i ].sgnd = 0;
        cmptparm[ i ].dx = (OPJ_UINT32)subsampling_dx;
        cmptparm[ i ].dy = (OPJ_UINT32)subsampling_dy;
        cmptparm[ i ].w = w;
        cmptparm[ i ].h = h;
    }

    if ( ( image = opj_image_create( (OPJ_UINT32)numcomps, &cmptparm[ 0 ],
                                     ( numcomps < 3 ) ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB ) ) == NULL )
        return NULL;

    /* set image offset and reference grid */
    image->x0 = (OPJ_UINT32)parameters->image_offset_x0;
    image->y0 = (OPJ_UINT32)parameters->image_offset_y0;
    image->x1 = (OPJ_UINT32)(parameters->image_offset_x0 + ( w - 1 ) * subsampling_dx
                             + 1 );
    image->y1 = (OPJ_UINT32)(parameters->image_offset_y0 + ( h - 1 ) * subsampling_dy
                             + 1 );

    switch ( numcomps ) {
        case 1: for ( i = 0 ; i < w * h ; i++ ) {
                    image->comps[ 0 ].data[ i ] = *data++;
                    data += 3;
                }
                break;
        case 3: for ( i = 0 ; i < w * h ; i++ ) {
                    image->comps[ 0 ].data[ i ] = *data++;
                    image->comps[ 1 ].data[ i ] = *data++;
                    image->comps[ 2 ].data[ i ] = *data++;
                    data++;
                }
                break;
        case 4: for ( i = 0 ; i < w * h ; i++ ) {
                    image->comps[ 0 ].data[ i ] = *data++;
                    image->comps[ 1 ].data[ i ] = *data++;
                    image->comps[ 2 ].data[ i ] = *data++;
                    image->comps[ 3 ].data[ i ] = *data++;
                }
                break;
    }
    return image;
}

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

static int trpopenjp2_idata_format( trp_openjp2_data_t *idata )
{
    uns8b *p;
    uns8b buf[ 12 ];

    if ( idata->size < 12 )
        return -1;
    if ( idata->fp ) {
        if ( fread( buf, 12, 1, idata->fp ) != 1 )
            return -1;
        if ( fseeko( idata->fp, 0, SEEK_SET ) )
            return -1;
        p = buf;
    } else
        p = idata->data;
    if ( ( memcmp( p, JP2_RFC3745_MAGIC, 12 ) == 0 ) ||
         ( memcmp( p, JP2_MAGIC, 4 ) == 0 ) )
        return JP2_CFMT;
    if ( memcmp( p, J2K_CODESTREAM_MAGIC, 4 ) == 0 )
        return J2K_CFMT;
    return JPT_CFMT;
}

static void trpopenjp2_quiet_cback( const char *msg, void *client_data )
{
    (void)msg;
    (void)client_data;
}

static void trpopenjp2_void_cback( void *p_user_data )
{
    (void)p_user_data;
}

static OPJ_SIZE_T trpopenjp2_read_cback( void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data )
{
    trp_openjp2_data_t *idata = (trp_openjp2_data_t *)p_user_data;

    if ( idata->fp ) {
        p_nb_bytes = fread( p_buffer, 1, p_nb_bytes, idata->fp );
    } else {
        if ( p_nb_bytes > idata->size - idata->pos )
            p_nb_bytes = idata->size - idata->pos;
        if ( p_nb_bytes ) {
            memcpy( p_buffer, idata->data + idata->pos, p_nb_bytes );
            idata->pos += p_nb_bytes;
        }
    }
    return p_nb_bytes ? p_nb_bytes : (OPJ_SIZE_T) - 1;
}

static OPJ_SIZE_T trpopenjp2_write_cback( void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data )
{
    trp_openjp2_data_t *odata = (trp_openjp2_data_t *)p_user_data;

    if ( odata->fp )
        p_nb_bytes = fwrite( p_buffer, 1, p_nb_bytes, odata->fp );
    else {
        if ( odata->pos + p_nb_bytes > odata->size ) {
            uns8b *new_data;
            uns32b new_size;

            new_size = ( odata->size << 1 );
            if ( odata->pos + p_nb_bytes > new_size )
                new_size = odata->pos + p_nb_bytes;
            new_data = realloc( odata->data, new_size );
            if ( new_data ) {
                odata->data = new_data;
                odata->size = new_size;
            } else
                p_nb_bytes = 0;
        }
        if ( p_nb_bytes ) {
            memcpy( odata->data + odata->pos, p_buffer, p_nb_bytes );
            odata->pos += p_nb_bytes;
        }
    }
    return p_nb_bytes;
}

static OPJ_OFF_T trpopenjp2_skip_cback( OPJ_OFF_T p_nb_bytes, void *p_user_data )
{
    trp_openjp2_data_t *idata = (trp_openjp2_data_t *)p_user_data;

    if ( idata->fp ) {
        if ( fseeko( idata->fp, p_nb_bytes, SEEK_CUR ) )
            return -1;
    } else {
        if ( p_nb_bytes > idata->size - idata->pos )
            p_nb_bytes = idata->size - idata->pos;
        idata->pos += p_nb_bytes;
    }
    return p_nb_bytes;
}

static OPJ_BOOL trpopenjp2_seek_cback( OPJ_OFF_T p_nb_bytes, void *p_user_data )
{
    trp_openjp2_data_t *idata = (trp_openjp2_data_t *)p_user_data;

    if ( idata->fp ) {
        if ( fseeko( idata->fp, p_nb_bytes, SEEK_SET ) )
            return OPJ_FALSE;
    } else {
        if ( p_nb_bytes > idata->size )
            return OPJ_FALSE;
        idata->pos = p_nb_bytes;
    }
    return OPJ_TRUE;
}

static uns8b trp_pix_load_openjp2_low( trp_openjp2_data_t *idata, uns32b *w, uns32b *h, uns8b **data )
{
    opj_stream_t *l_stream = NULL;
    opj_codec_t *l_codec = NULL;
    opj_image_t *image = NULL;
    opj_dparameters_t core;
    int format;
    uns8b res = 1;

    if ( ( format = trpopenjp2_idata_format( idata ) ) < 0 )
        goto error;

    if ( ( l_stream = opj_stream_default_create( 1 ) ) == NULL )
        goto error;
    opj_stream_set_user_data( l_stream, idata, trpopenjp2_void_cback );
    opj_stream_set_user_data_length( l_stream, idata->size );
    opj_stream_set_read_function( l_stream, trpopenjp2_read_cback );
    /*
     * questa dovrebbe non essere necessaria...
     * opj_stream_set_write_function( l_stream, trpopenjp2_write_cback );
     */
    opj_stream_set_skip_function( l_stream, trpopenjp2_skip_cback );
    opj_stream_set_seek_function( l_stream, trpopenjp2_seek_cback );

    switch ( format ) {
        case J2K_CFMT: /* JPEG-2000 codestream */
            /* Get a decoder handle */
            l_codec = opj_create_decompress( OPJ_CODEC_J2K );
            break;
        case JP2_CFMT: /* JPEG 2000 compressed image data */
            /* Get a decoder handle */
            l_codec = opj_create_decompress( OPJ_CODEC_JP2 );
            break;
        case JPT_CFMT: /* JPEG 2000, JPIP */
            /* Get a decoder handle */
            l_codec = opj_create_decompress( OPJ_CODEC_JPT );
            break;
    }
    if ( l_codec == NULL )
        goto error;

    opj_set_info_handler( l_codec, trpopenjp2_quiet_cback, 0 );
    opj_set_warning_handler( l_codec, trpopenjp2_quiet_cback, 0 );
    opj_set_error_handler( l_codec, trpopenjp2_quiet_cback, 0 );

    opj_set_default_decoder_parameters( &core );

    /* Setup the decoder decoding parameters using user parameters */
    if ( !opj_setup_decoder( l_codec, &core ) )
        goto error;

    if ( opj_has_thread_support() ) {
        int num_cpus = opj_get_num_cpus();

        if ( num_cpus > 1 )
            if ( !opj_codec_set_threads( l_codec, num_cpus ) )
                goto error;
    }

    /* Read the main header of the codestream and if necessary the JP2 boxes */
    if ( !opj_read_header( l_stream, l_codec, &image ) )
        goto error;

    if ( !opj_set_decode_area( l_codec, image, 0, 0, 0, 0 ) )
        goto error;

    /* Get the decoded image */
    if ( !opj_decode( l_codec, l_stream, image ) )
        goto error;

    if ( !opj_end_decompress( l_codec, l_stream ) )
        goto error;

    opj_stream_destroy( l_stream );
    l_stream = NULL;
    opj_destroy_codec( l_codec );
    l_codec = NULL;

    if ( ( image->color_space != OPJ_CLRSPC_SYCC ) &&
         ( image->numcomps == 3 ) && ( image->comps[ 0 ].dx == image->comps[ 0 ].dy ) &&
         ( image->comps[ 1 ].dx != 1 ) ) {
        image->color_space = OPJ_CLRSPC_SYCC;
    } else if ( image->numcomps <= 2 ) {
        image->color_space = OPJ_CLRSPC_GRAY;
    }

    if ( image->color_space == OPJ_CLRSPC_SYCC ) {
        color_sycc_to_rgb( image );
    } else if ( image->color_space == OPJ_CLRSPC_CMYK ) {
        color_cmyk_to_rgb( image );
    } else if ( image->color_space == OPJ_CLRSPC_EYCC ) {
        color_esycc_to_rgb( image );
    }

    if ( image->icc_profile_buf ) {
        if ( image->icc_profile_len ) {
            color_apply_icc_profile( image );
        } else {
            color_cielab_to_rgb( image );
        }
        free( image->icc_profile_buf );
        image->icc_profile_buf = NULL;
        image->icc_profile_len = 0;
    }

    if ( image->color_space == OPJ_CLRSPC_GRAY )
        if ( ( image = trpopenjp2_gray2rgb( image ) ) == NULL )
            goto error;

    if ( ( image->color_space != OPJ_CLRSPC_SRGB ) &&
         ( image->color_space != OPJ_CLRSPC_UNSPECIFIED ) )
        goto error;

    if ( trpopenjp2_image2trp( image, w, h, data ) )
        goto error;

    res = 0;
error:
    if ( l_stream )
        opj_stream_destroy( l_stream );
    if ( l_codec )
        opj_destroy_codec( l_codec );
    if ( image )
        opj_image_destroy( image );
    return res;
}

static uns8b trp_pix_load_openjp2( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    trp_openjp2_data_t idata_cback;
    uns8b res;

#ifdef MINGW
    wchar_t *wpath;
    struct _stati64 st;

    wpath = trp_utf8_to_wc( cpath );
    if ( wpath == NULL )
        return 1;
    if ( _wstati64( wpath, &st ) ) {
        trp_gc_free( wpath );
        return 1;
    }
    trp_gc_free( wpath );
#else
    struct stat st;

    if ( lstat( cpath, &st ) )
        return 1;
#endif
    idata_cback.fp = trp_fopen( cpath, "rb" );
    if ( idata_cback.fp == NULL )
        return 1;
    idata_cback.size = (uns32b)( st.st_size );
    res = trp_pix_load_openjp2_low( &idata_cback, w, h, data );
    fclose( idata_cback.fp );
    return res;
}

static uns8b trp_pix_load_openjp2_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{

    trp_openjp2_data_t idata_cback;

    idata_cback.fp = NULL;
    idata_cback.data = idata;
    idata_cback.size = isize;
    idata_cback.pos = 0;
    return trp_pix_load_openjp2_low( &idata_cback, w, h, data );
}

static uns8b trp_openjp2_save_low( trp_obj_t *pix, trp_openjp2_data_t *odata, trp_obj_t *quality )
{
    opj_stream_t *l_stream = NULL;
    opj_codec_t *l_codec = NULL;
    opj_image_t *image = NULL;
    opj_cparameters_t parameters;
    uns32b numcomps;
    uns8b res = 1;

    if ( pix->tipo != TRP_PIX )
        goto error;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        goto error;

    if ( trp_pix_has_alpha_low( (trp_pix_t *)pix ) )
        numcomps = 4;
    else if ( ( trp_pix_colors_type( (trp_pix_t *)pix, 0 ) & 0xfffe ) == 0xfffe )
        numcomps = 1;
    else
        numcomps = 3;

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters( &parameters );

    if ( quality ) {
        double rate;

        parameters.tcp_numlayers = 0;
        if ( trp_cast_flt64b_range( quality, &rate, 0.0, 99.0 ) ) {
            for ( ; ; ) {
                if ( quality->tipo != TRP_CONS )
                    goto error;
                if ( trp_cast_flt64b_range( ((trp_cons_t *)quality)->car, &rate, 0.0, 99.0 ) )
                    goto error;
                parameters.tcp_rates[ parameters.tcp_numlayers++ ] = rate;
                quality = ((trp_cons_t *)quality)->cdr;
                if ( quality == NIL )
                    break;
                if ( parameters.tcp_numlayers == 10 )
                    goto error;
            }
        } else
            parameters.tcp_rates[ parameters.tcp_numlayers++ ] = rate;
        parameters.cp_disto_alloc = 1;
    }

    if ( ( image = trpopenjp2_trp2image( ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h,
                                         ((trp_pix_t *)pix)->map.p,
                                         numcomps, &parameters ) ) == NULL )
        goto error;

    parameters.tcp_mct = ( image->numcomps >= 3 ) ? 1 : 0;
    parameters.cod_format = JP2_CFMT;

    if ( ( l_codec = opj_create_compress( OPJ_CODEC_JP2 ) ) == NULL )
        goto error;

    if ( opj_has_thread_support() ) {
        int num_cpus = opj_get_num_cpus();

        if ( num_cpus > 1 )
            if ( !opj_codec_set_threads( l_codec, num_cpus ) )
                goto error;
    }

    opj_set_info_handler( l_codec, trpopenjp2_quiet_cback, 0 );
    opj_set_warning_handler( l_codec, trpopenjp2_quiet_cback, 0 );
    opj_set_error_handler( l_codec, trpopenjp2_quiet_cback, 0 );

    if ( !opj_setup_encoder( l_codec, &parameters, image ) )
        goto error;

    if ( ( l_stream = opj_stream_default_create( 0 ) ) == NULL )
        goto error;
    opj_stream_set_user_data( l_stream, odata, trpopenjp2_void_cback );
    opj_stream_set_user_data_length( l_stream, 0 );
    opj_stream_set_read_function( l_stream, trpopenjp2_read_cback );
    opj_stream_set_write_function( l_stream, trpopenjp2_write_cback );
    opj_stream_set_skip_function( l_stream, trpopenjp2_skip_cback );
    opj_stream_set_seek_function( l_stream, trpopenjp2_seek_cback );

    if ( !opj_start_compress( l_codec, image, l_stream ) )
        goto error;

    if ( !opj_encode( l_codec, l_stream ) )
        goto error;

    if ( !opj_end_compress( l_codec, l_stream ) )
        goto error;

    res = 0;

error:
    if ( l_stream )
        opj_stream_destroy( l_stream );
    if ( l_codec )
        opj_destroy_codec( l_codec );
    if ( image )
        opj_image_destroy( image );
    return res;
}

uns8b trp_openjp2_save( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *quality )
{
    uns8b *cpath = trp_csprint( path );
    trp_openjp2_data_t odata;
    uns8b res;

    odata.fp = trp_fopen( cpath, "w+b" );
    trp_csprint_free( cpath );
    if ( odata.fp == NULL )
        return 1;
    res = trp_openjp2_save_low( pix, &odata, quality );
    fclose( odata.fp );
    if ( res )
        trp_remove( path );
    return res;
}

trp_obj_t *trp_openjp2_save_memory( trp_obj_t *pix, trp_obj_t *quality )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    trp_obj_t *raw = UNDEF;
    trp_openjp2_data_t odata;

    odata.fp = NULL;
    odata.data = NULL;
    odata.size = 0;
    odata.pos = 0;
    if ( trp_openjp2_save_low( pix, &odata, quality ) == 0 )
        if ( odata.pos ) {
            raw = trp_raw_internal( odata.pos, 0 );
            memcpy( ((trp_raw_t *)raw)->data, odata.data, odata.pos );
        }
    free( odata.data );
    return raw;
}

