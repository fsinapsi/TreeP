/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

#include "./trppix_internal.h"
#include <setjmp.h>
#include <jerror.h>
#include <jpeglib.h>

extern uns16b trp_pix_colors_type( trp_pix_t *pix );

struct my_error_mgr {
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

METHODDEF(void)
my_error_exit( j_common_ptr cinfo )
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    /* Return control to the setjmp point */
    longjmp( myerr->setjmp_buffer, 1 );
}

uns8b trp_pix_info_jpg( uns8b *cpath, uns32b *w, uns32b *h )
{
    Epeg_Image *ep = epeg_file_open( cpath );
    const void *p;
    int ws, hs;

    if ( ep == NULL )
        return 1;
    epeg_size_get( ep, &ws, &hs );
    epeg_close( ep );
    *w = (uns32b)ws;
    *h = (uns32b)hs;
    return 0;
}

uns8b trp_pix_load_jpg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    Epeg_Image *ep = epeg_file_open( cpath );
    const void *p;
    int ws, hs;

    if ( ep == NULL )
        return 1;
    epeg_size_get( ep, &ws, &hs );
    epeg_decode_size_set( ep, ws, hs );
    epeg_decode_colorspace_set( ep, EPEG_RGBA8 );
    p = epeg_pixels_get( ep, 0, 0, ws, hs );
    epeg_close( ep );
    if ( p == NULL )
        return 1;
    *w = (uns32b)ws;
    *h = (uns32b)hs;
    *data = (uns8b *)p;
    return 0;
}

uns8b trp_pix_save_jpg( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *quality )
{
    FILE *fp;
    uns32b ql, w, h, i;
    trp_pix_color_t *c;
    uns8b *cpath, *row, *p;
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW row_pointer[ 1 ];
    uns8b color;

    if ( pix->tipo != TRP_PIX )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    if ( quality ) {
        if (  trp_cast_uns32b_rint_range( quality, &ql, 0, 100 ) )
            return 1;
    } else
        ql = 75;
    cpath = trp_csprint( path );
    fp = trp_fopen( cpath, "wb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return 1;
    color = ( trp_pix_colors_type( (trp_pix_t *)pix ) & 0xfffe == 0xfffe ) ? 0 : 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( row = malloc( w * ( color ? 3 : 1 ) ) ) == NULL ) {
        fclose( fp );
        return 1;
    }
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = my_error_exit;
    if ( setjmp( jerr.setjmp_buffer ) ) {
        jpeg_destroy_compress( &cinfo );
        free( (void *)row );
        fclose( fp );
        return 1;
    }
    jpeg_create_compress( &cinfo );
    jpeg_stdio_dest( &cinfo, fp );
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = color ? 3 : 1;
    cinfo.in_color_space = color ? JCS_RGB : JCS_GRAYSCALE;
    jpeg_set_defaults( &cinfo );
    jpeg_set_quality( &cinfo, ql, TRUE /* limit to baseline-JPEG values */ );
    cinfo.optimize_coding = TRUE;
    jpeg_start_compress( &cinfo, TRUE );
    while ( cinfo.next_scanline < cinfo.image_height ) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[ 0 ] = row;
        for ( i = 0, p = row ; i < w ; i++, c++ ) {
            *p++ = c->red;
            if ( color ) {
                *p++ = c->green;
                *p++ = c->blue;
            }
        }
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    free( (void *)row );
    fclose( fp );
    return 0;
}

trp_obj_t *trp_pix_save_jpg_memory( trp_obj_t *pix, trp_obj_t *quality )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    uns32b ql, w, h, i;
    trp_pix_color_t *c;
    uns8b *outbuffer = NULL, *row, *p;
    unsigned long outsize = 0;
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW row_pointer[ 1 ];
    uns8b color;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    if ( quality ) {
        if (  trp_cast_uns32b_rint_range( quality, &ql, 0, 100 ) )
            return UNDEF;
    } else
        ql = 75;
    color = ( trp_pix_colors_type( (trp_pix_t *)pix ) & 0xfffe == 0xfffe ) ? 0 : 1;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( row = malloc( w * ( color ? 3 : 1 ) ) ) == NULL )
        return UNDEF;
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = my_error_exit;
    if ( setjmp( jerr.setjmp_buffer ) ) {
        jpeg_destroy_compress( &cinfo );
        free( (void *)row );
        if ( outbuffer )
            free( (void *)outbuffer );
        return UNDEF;
    }
    jpeg_create_compress( &cinfo );
    jpeg_mem_dest( &cinfo, &outbuffer, &outsize );
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = color ? 3 : 1;
    cinfo.in_color_space = color ? JCS_RGB : JCS_GRAYSCALE;
    jpeg_set_defaults( &cinfo );
    jpeg_set_quality( &cinfo, ql, TRUE /* limit to baseline-JPEG values */ );
    cinfo.optimize_coding = TRUE;
    jpeg_start_compress( &cinfo, TRUE );
    while ( cinfo.next_scanline < cinfo.image_height ) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[ 0 ] = row;
        for ( i = 0, p = row ; i < w ; i++, c++ ) {
            *p++ = c->red;
            if ( color ) {
                *p++ = c->green;
                *p++ = c->blue;
            }
        }
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    free( (void *)row );
    pix = trp_raw_internal( outsize, 0 );
    memcpy( (void *)( ((trp_raw_t *)pix)->data ), (void *)outbuffer, outsize );
    free( (void *)outbuffer );
    return pix;
}

