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

#include "./trppix_internal.h"
#include <png.h>
#include <zlib.h>

#ifndef PNG_BYTES_TO_CHECK
#define PNG_BYTES_TO_CHECK 8
#endif
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

extern uns16b trp_pix_colors_type( trp_pix_t *pix );
extern uns8b trp_pix_has_alpha( trp_pix_t *pix );
static void trp_pix_load_png_memory_cb( png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead );
static uns8b trp_pix_load_png_low( uns8b *cpath, trp_raw_t *raw, uns32b *w, uns32b *h, uns8b **data );

typedef struct {
    trp_raw_t *raw;
    uns32b read;
} trp_pix_png_raw_t;

uns8b trp_pix_info_png( uns8b *cpath, uns32b *w, uns32b *h )
{
    FILE *fp = trp_fopen( cpath, "rb" );
    png_structp png_ptr;
    png_infop info_ptr;
    uns8b buf[ PNG_BYTES_TO_CHECK ];

    if ( fp == NULL )
        return 1;
    if ( fread( buf, 1, PNG_BYTES_TO_CHECK, fp ) != PNG_BYTES_TO_CHECK ) {
        fclose( fp );
        return 1;
    }
    if ( png_sig_cmp( buf, (png_size_t)0, PNG_BYTES_TO_CHECK ) ) {
        fclose( fp );
        return 1;
    }
    if ( ( png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
                                             NULL, NULL, NULL ) ) == NULL ) {
        fclose( fp );
        return 1;
    }
    if ( ( info_ptr = png_create_info_struct( png_ptr ) ) == NULL ) {
        png_destroy_read_struct( &png_ptr, NULL, NULL );
        fclose( fp );
        return 1;
    }
    if ( setjmp( png_jmpbuf( png_ptr ) ) ) {
        png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
        fclose( fp );
        return 1;
    }
    png_init_io( png_ptr, fp );
    png_set_sig_bytes( png_ptr, PNG_BYTES_TO_CHECK );
    png_read_info( png_ptr, info_ptr );
    *w = png_get_image_width( png_ptr, info_ptr );
    *h = png_get_image_height( png_ptr, info_ptr );
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    fclose( fp );
    return 0;
}

static void trp_pix_load_png_memory_cb( png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead )
{
    trp_pix_png_raw_t *r = (trp_pix_png_raw_t *)png_get_io_ptr( png_ptr );

    if ( r->read + byteCountToRead > r->raw->len )
        byteCountToRead = r->raw->len - r->read;
    if ( byteCountToRead ) {
        memcpy( outBytes, r->raw->data + r->read, byteCountToRead );
        r->read += byteCountToRead;
    }
}

static uns8b trp_pix_load_png_low( uns8b *cpath, trp_raw_t *raw, uns32b *w, uns32b *h, uns8b **data )
{
    FILE *fp = NULL;
    uns32b color_type, bpl, i;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *rows = NULL;
    trp_pix_png_raw_t r;
    uns8b res = 1;

    if ( cpath ) {
        uns8b buf[ PNG_BYTES_TO_CHECK ];

        if ( ( fp = trp_fopen( cpath, "rb" ) ) == NULL )
            return 1;
        if ( fread( buf, 1, PNG_BYTES_TO_CHECK, fp ) != PNG_BYTES_TO_CHECK ) {
            fclose( fp );
            return 1;
        }
        if ( png_sig_cmp( buf, (png_size_t)0, PNG_BYTES_TO_CHECK ) ) {
            fclose( fp );
            return 1;
        }
    } else if ( raw ) {
        if ( raw->tipo != TRP_RAW )
            return 1;
        if ( raw->len < PNG_BYTES_TO_CHECK )
            return 1;
        if ( png_sig_cmp( raw->data, (png_size_t)0, PNG_BYTES_TO_CHECK ) )
            return 1;
        r.raw = raw;
        r.read = PNG_BYTES_TO_CHECK;
    } else
        return 1;
    if ( ( png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
                                             NULL, NULL, NULL ) ) == NULL ) {
        if ( fp )
            fclose( fp );
        return 1;
    }
    if ( ( info_ptr = png_create_info_struct( png_ptr ) ) == NULL ) {
        png_destroy_read_struct( &png_ptr, NULL, NULL );
        if ( fp )
            fclose( fp );
        return 1;
    }
    *data = NULL;
    if ( setjmp( png_jmpbuf( png_ptr ) ) ) {
        free( (void *)rows );
        free( (void *)( *data ) );
        png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
        if ( fp )
            fclose( fp );
        return 1;
    }
    if ( fp )
        png_init_io( png_ptr, fp );
    else
        png_set_read_fn( png_ptr, &r, trp_pix_load_png_memory_cb );
    png_set_sig_bytes( png_ptr, PNG_BYTES_TO_CHECK );
    png_read_info( png_ptr, info_ptr );
    if ( png_get_bit_depth( png_ptr, info_ptr ) == 16 )
        png_set_strip_16( png_ptr );
    color_type = png_get_color_type( png_ptr, info_ptr );
    switch ( color_type & ~PNG_COLOR_MASK_ALPHA ) {
    case PNG_COLOR_TYPE_RGB:
        break;
    case PNG_COLOR_TYPE_PALETTE:
        png_set_expand( png_ptr );
        break;
    case PNG_COLOR_TYPE_GRAY:
        png_set_expand( png_ptr );
        png_set_gray_to_rgb( png_ptr );
        break;
    default:
        /* printf( "color_type = %u\n", color_type ); */
        png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
        if ( fp )
            fclose( fp );
        return 1;
    }
    if ( ~color_type & PNG_COLOR_MASK_ALPHA )
        png_set_filler( png_ptr, 0xff, PNG_FILLER_AFTER );
    png_read_update_info( png_ptr, info_ptr );
    *w = png_get_image_width( png_ptr, info_ptr );
    *h = png_get_image_height( png_ptr, info_ptr );
    bpl = *w << 2;
    if ( png_get_rowbytes( png_ptr, info_ptr ) == bpl )
        if ( *data = malloc( *h * bpl ) )
            if ( rows = malloc( sizeof( png_bytep ) * *h ) ) {
                for ( i = 0 ; i < *h ; i++ )
                    rows[ i ] = *data + i * bpl;
                png_read_image( png_ptr, rows );
                png_read_end( png_ptr, NULL );
                free( (void *)rows );
                res = 0;
            } else
                free( (void *)( *data ) );
    png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
    if ( fp )
        fclose( fp );
    return res;
}

uns8b trp_pix_load_png( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_png_low( cpath, NULL, w, h, data );
}

uns8b trp_pix_load_png_memory( trp_raw_t *raw, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_png_low( NULL, raw, w, h, data );
}

uns8b trp_pix_save_png( trp_obj_t *pix, trp_obj_t *path )
{
    FILE *fp;
    uns32b w, h, bpl, i;
    uns8b *cpath, *map = NULL;
    trp_pix_color_t *c;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *rows = NULL;

    if ( pix->tipo != TRP_PIX )
        return 1;
    c = ((trp_pix_t *)pix)->map.c;
    if ( c == NULL )
        return 1;
    cpath = trp_csprint( path );
    fp = trp_fopen( cpath, "wb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return 1;
    if ( ( png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL ) ) == NULL ) {
        fclose( fp );
        return 1;
    }
    if ( ( info_ptr = png_create_info_struct( png_ptr ) ) == NULL ) {
        png_destroy_write_struct( &png_ptr, NULL );
        fclose( fp );
        return 1;
    }
    if ( setjmp( png_jmpbuf( png_ptr ) ) ) {
        free( (void *)map );
        free( (void *)rows );
        png_destroy_write_struct( &png_ptr, &info_ptr );
        fclose( fp );
        return 1;
    }
    png_init_io( png_ptr, fp );
    png_set_compression_level( png_ptr, Z_BEST_COMPRESSION );
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( rows = malloc( sizeof( png_bytep ) * h ) ) == NULL ) {
        png_destroy_write_struct( &png_ptr, &info_ptr );
        fclose( fp );
        return 1;
    }
    switch ( trp_pix_colors_type( (trp_pix_t *)pix ) ) {
    case 0xfffe:
        bpl = ( w >> 3 ) + ( ( w & 7 ) ? 1 : 0 );
        if ( ( map = malloc( bpl * h ) ) == NULL ) {
            free( (void *)rows );
            png_destroy_write_struct( &png_ptr, &info_ptr );
            fclose( fp );
            return 1;
        }
        {
            uns32b j;
            uns8b *p, cnt;

            for ( i = 0 ; i < h ; i++ ) {
                rows[ i ] = p = map + i * bpl;
                for ( j = 0 ; j < w ; j++, c++ ) {
                    cnt = (uns8b)( j & 7 );
                    if ( cnt == 0 )
                        *p = 0;
                    if ( c->red )
                        *p |= ( 0x80 >> cnt );
                    if ( cnt == 7 )
                        p++;
                }
            }
        }
        png_set_IHDR( png_ptr, info_ptr, w, h, 1,
                      PNG_COLOR_TYPE_GRAY,
                      PNG_INTERLACE_NONE,
                      PNG_COMPRESSION_TYPE_DEFAULT,
                      PNG_FILTER_TYPE_DEFAULT );
        break;
    case 0xffff:
        bpl = w;
        if ( ( map = malloc( bpl * h ) ) == NULL ) {
            free( (void *)rows );
            png_destroy_write_struct( &png_ptr, &info_ptr );
            fclose( fp );
            return 1;
        }
        {
            uns32b j;
            uns8b *p = map;

            for ( i = 0 ; i < h ; i++ ) {
                rows[ i ] = p;
                for ( j = 0 ; j < w ; j++, c++ )
                    *p++ = c->red;
            }
        }
        png_set_IHDR( png_ptr, info_ptr, w, h, 8,
                      PNG_COLOR_TYPE_GRAY,
                      PNG_INTERLACE_NONE,
                      PNG_COMPRESSION_TYPE_DEFAULT,
                      PNG_FILTER_TYPE_DEFAULT );
        break;
    default:
        if ( trp_pix_has_alpha( (trp_pix_t *)pix ) ) {
            bpl = w << 2;
            for ( i = 0 ; i < h ; i++ )
                rows[ i ] = ((trp_pix_t *)pix)->map.p + i * bpl;
            png_set_IHDR( png_ptr, info_ptr, w, h, 8,
                          PNG_COLOR_TYPE_RGB_ALPHA,
                          PNG_INTERLACE_NONE,
                          PNG_COMPRESSION_TYPE_DEFAULT,
                          PNG_FILTER_TYPE_DEFAULT );
        } else {
            bpl = w * 3;
            if ( ( map = malloc( bpl * h ) ) == NULL ) {
                free( (void *)rows );
                png_destroy_write_struct( &png_ptr, &info_ptr );
                fclose( fp );
                return 1;
            }
            {
                uns32b j;
                uns8b *p = map;

                for ( i = 0 ; i < h ; i++ ) {
                    rows[ i ] = p;
                    for ( j = 0 ; j < w ; j++, c++ ) {
                        *p++ = c->red;
                        *p++ = c->green;
                        *p++ = c->blue;
                    }
                }
            }
            png_set_IHDR( png_ptr, info_ptr, w, h, 8,
                          PNG_COLOR_TYPE_RGB,
                          PNG_INTERLACE_NONE,
                          PNG_COMPRESSION_TYPE_DEFAULT,
                          PNG_FILTER_TYPE_DEFAULT );
        }
        break;
    }
    png_set_rows( png_ptr, info_ptr, rows );
    png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );
    free( (void *)map );
    free( (void *)rows );
    png_destroy_write_struct( &png_ptr, &info_ptr );
    fclose( fp );
    return 0;
}

