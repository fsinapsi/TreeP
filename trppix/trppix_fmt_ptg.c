/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

typedef struct {
    FILE  *fp;
    uns8b *idata;
    uns32b isize;
    uns32b read;
} trp_pix_ptg_t;

static int trppix_ptg_getc( trp_pix_ptg_t *is )
{
    if ( is->fp )
        return fgetc( is->fp );
    if ( is->read == is->isize )
        return EOF;
    return is->idata[ is->read++ ];
}

static uns8b trppix_ptg_match_string( trp_pix_ptg_t *is, char *target )
{
    while ( *target )
        if ( trppix_ptg_getc( is ) != *target++ )
            return 1;
    return 0;
}

static uns8b trppix_ptg_read_uns32b( trp_pix_ptg_t *is, uns32b *n )
{
    int c0, c1, c2, c3;

    if ( ( c0 = trppix_ptg_getc( is ) ) == EOF )
        return 1;
    if ( ( c1 = trppix_ptg_getc( is ) ) == EOF )
        return 1;
    if ( ( c2 = trppix_ptg_getc( is ) ) == EOF )
        return 1;
    if ( ( c3 = trppix_ptg_getc( is ) ) == EOF )
        return 1;
    *n = ( (uns32b)c0 ) | ( ( (uns32b)c1 ) << 8 ) |
         ( ( (uns32b)c2 ) << 16 ) | ( ( (uns32b)c3 ) << 24 ) ;
    return 0;
}

static uns8b trppix_ptg_read_uns64b( trp_pix_ptg_t *is, uns64b *n )
{
    uns32b i;

    if ( trppix_ptg_read_uns32b( is, &i ) )
        return 1;
    *n = i;
    if ( trppix_ptg_read_uns32b( is, &i ) )
        return 1;
    *n |= ( ( (uns64b)i ) << 32 );
    return 0;
}

static uns8b trppix_ptg_read_flt64b( trp_pix_ptg_t *is, flt64b *d )
{
    return trppix_ptg_read_uns64b( is, (uns64b *)d );
}

static trp_obj_t *trppix_ptg_read_pix( trp_pix_ptg_t *is )
{
    trp_obj_t *pix;
    uns64b size;

    if ( trppix_ptg_read_uns64b( is, &size ) )
        return UNDEF;
    if ( is->fp ) {
        if ( size > is->isize ) {
            uns8b *buftmp;

            if ( ( buftmp = realloc( is->idata, size ) ) == NULL )
                return UNDEF;
            is->idata = buftmp;
            is->isize = size;
        }
        if ( fread( is->idata, size, 1, is->fp ) != 1 )
            return UNDEF;
        pix = trp_pix_load_memory_low( is->idata, size );
    } else {
        if ( is->isize - is->read < size )
            return UNDEF;
        pix = trp_pix_load_memory_low( is->idata + is->read, size );
        is->read += size;
    }
    return pix;
}

static uns8b trp_pix_load_ptg_puzzle( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_pix_color_t *pdst, *psrc;
    trp_obj_t *pix;
    uns64b size;
    uns32b n, x, y;
    uns32b wsrco, wsrc, hsrc;

    if ( trppix_ptg_read_uns32b( is, w ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, h ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, &n ) )
        return 1;
    if ( ( *w == 0 ) || ( *h == 0 ) || ( n == 0 ) )
        return 1;
    size = ( ( *w * *h ) << 2 );
    if ( ( *data = malloc( size ) ) == NULL )
        return 1;
    memset( *data, 0, size );
    while ( n-- ) {
        if ( trppix_ptg_read_uns32b( is, &x ) )
            goto error;
        if ( trppix_ptg_read_uns32b( is, &y ) )
            goto error;
        if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
            goto error;
        if ( ( x < *w ) && ( y < *h ) ) {
            pdst = (trp_pix_color_t *)( *data );
            psrc = ((trp_pix_t *)pix)->map.c;
            wsrco = wsrc = ((trp_pix_t *)pix)->w;
            hsrc = ((trp_pix_t *)pix)->h;
            if ( wsrc > *w - x )
                wsrc = *w - x;
            if ( hsrc > *h - y )
                hsrc = *h - y;
            pdst += ( x + y * *w );
            wsrc <<= 2;
            for ( ; hsrc ; hsrc--, pdst += *w, psrc += wsrco )
                memcpy( pdst, psrc, wsrc );
        }
        (void)trp_pix_close( (trp_pix_t *)pix );
    }
    return 0;
error:
    free( *data );
    return 1;
}

static uns8b trp_pix_load_ptg_crop( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_obj_t *pix;
    trp_pix_color_t *p, *q;
    uns32b x, y, pw, ph, w4, i;

    if ( trppix_ptg_read_uns32b( is, &x ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, &y ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, w ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, h ) )
        return 1;
    if ( ( *w < 1) || ( *h < 1 ) )
        return 1;
    if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
        return 1;
    pw = ((trp_pix_t *)pix)->w;
    ph = ((trp_pix_t *)pix)->h;
    if ( ( x >= pw ) || ( y >= ph ) )
        return 1;
    if ( *w > pw - x )
        *w = pw - x;
    if ( *h > ph - y )
        *h = ph - y;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL )
        return 1;
    q = (trp_pix_color_t *)( *data );
    p = ((trp_pix_t *)pix)->map.c + ( x + pw * y );
    w4 = *w << 2;
    for ( i = *h ; i ; i--, q += *w, p += pw )
        memcpy( q, p, w4 );
    (void)trp_pix_close( (trp_pix_t *)pix );
    return 0;
}

static uns8b trp_pix_load_ptg_scale( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_obj_t *pix;
    uns8b res;

    if ( trppix_ptg_read_uns32b( is, w ) )
        return 1;
    if ( trppix_ptg_read_uns32b( is, h ) )
        return 1;
    if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
        return 1;
    *data = NULL;
    res = trp_pix_scale_low( ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h, ((trp_pix_t *)pix)->map.p,
                             *w, *h, data );
    (void)trp_pix_close( (trp_pix_t *)pix );
    return res;
}

static uns8b trp_pix_load_ptg_rotate( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_obj_t *pix;
    flt64b a;
    uns8b res;

    if ( trppix_ptg_read_flt64b( is, &a ) )
        return 1;
    if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
        return 1;
    res = trp_pix_rotate_low( pix, a, w, h, data );
    (void)trp_pix_close( (trp_pix_t *)pix );
    return res;
}

static uns8b trp_pix_load_ptg_negative( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_obj_t *pix;

    if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
        return 1;
    if ( trp_pix_negative( pix ) ) {
        (void)trp_pix_close( (trp_pix_t *)pix );
        return 1;
    }
    trp_gc_remove_finalizer( pix );
    *w = ((trp_pix_t *)pix)->w;
    *h = ((trp_pix_t *)pix)->h;
    *data = ((trp_pix_t *)pix)->map.p;
    trp_gc_free( pix );
    return 0;
}

static uns8b trp_pix_load_ptg_alpha( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_obj_t *pix, *pix_alpha;

    if ( ( pix = trppix_ptg_read_pix( is ) ) == UNDEF )
        return 1;
    if ( ( pix_alpha = trppix_ptg_read_pix( is ) ) == UNDEF ) {
        (void)trp_pix_close( (trp_pix_t *)pix );
        return 1;
    }
    if ( trp_pix_setalpha( pix, pix_alpha ) ) {
        (void)trp_pix_close( (trp_pix_t *)pix );
        (void)trp_pix_close( (trp_pix_t *)pix_alpha );
        return 1;
    }
    (void)trp_pix_close( (trp_pix_t *)pix_alpha );
    trp_gc_remove_finalizer( pix );
    *w = ((trp_pix_t *)pix)->w;
    *h = ((trp_pix_t *)pix)->h;
    *data = ((trp_pix_t *)pix)->map.p;
    trp_gc_free( pix );
    return 0;
}

static uns8b trp_pix_load_ptg_low( trp_pix_ptg_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    int c;

    if ( trppix_ptg_match_string( is, "hOro1%lB@fmAnGkoQbaWgaSx35ZuXBC" ) )
        return 1;
    c = trppix_ptg_getc( is );
    if ( c == 'h' )
        return trp_pix_load_ptg_puzzle( is, w, h, data );
    if ( c == 'c' )
        return trp_pix_load_ptg_crop( is, w, h, data );
    if ( c == 's' )
        return trp_pix_load_ptg_scale( is, w, h, data );
    if ( c == 'r' )
        return trp_pix_load_ptg_rotate( is, w, h, data );
    if ( c == 'n' )
        return trp_pix_load_ptg_negative( is, w, h, data );
    if ( c == 'a' )
        return trp_pix_load_ptg_alpha( is, w, h, data );
    return 1;
}

uns8b trp_pix_load_ptg( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    trp_pix_ptg_t is;
    uns8b res;

    if ( ( is.fp = trp_fopen( cpath, "rb" ) ) == NULL )
        return 1;
    is.idata = NULL;
    is.isize = 0;
    res = trp_pix_load_ptg_low( &is, w, h, data );
    free( is.idata );
    fclose( is.fp );
    return res;
}

uns8b trp_pix_load_ptg_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    trp_pix_ptg_t is;

    is.fp = NULL;
    is.idata = idata;
    is.isize = isize;
    is.read = 0;
    return trp_pix_load_ptg_low( &is, w, h, data );
}

