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

#include "./trppix_internal.h"

typedef struct {
    FILE *fp;
    uns8b *idata;
    uns32b isize;
    uns32b read;
} trp_pix_xpm_t;

static int trppix_xpm_getc( trp_pix_xpm_t *is )
{
    if ( is->fp )
        return fgetc( is->fp );
    if ( is->read == is->isize )
        return EOF;
    return is->idata[ is->read++ ];
}

static uns8b trppix_xpm_match_string( trp_pix_xpm_t *is, char *target )
{
    while ( *target )
        if ( trppix_xpm_getc( is ) != *target++ )
            return 1;
    return 0;
}

static uns8b trppix_xpm_next_match_string( trp_pix_xpm_t *is, char *target )
{
    char *p;
    int i, j, c;

    for ( i = 0 ; target[ i ] ; ) {
        if ( ( c = trppix_xpm_getc( is ) ) == EOF )
            return 1;
        if ( c != target[ i++ ] ) {
            for ( j = i, p = target + 1 ; j ; j--, p++ )
                if ( target[ j - 1 ] == c ) {
                    if ( j == 1 )
                        break;
                    if ( strncmp( target, p, j - 1 ) == 0 )
                        break;
                }
            i = j;
        }
    }
    return 0;
}

static int trppix_xpm_next_valid_char( trp_pix_xpm_t *is )
{
    int c;

    for ( ; ; ) {
        c = trppix_xpm_getc( is );
        if ( ( c == ' ' ) || ( c == '\t' ) || ( c == '\n' ) || ( c == '\r' ) )
            continue;
        if ( c != '/' )
            break;
        c = trppix_xpm_getc( is );
        if ( c == '/' ) {
            for ( ; ; ) {
                c = trppix_xpm_getc( is );
                if ( ( c == '\n' ) || ( c == '\r' ) )
                    break;
                if ( c == EOF )
                    return EOF;
            }
        } else {
            if ( c != '*' )
                return EOF;
            if ( trppix_xpm_next_match_string( is, "*/" ) )
                return EOF;
        }
    }
    return c;
}

static uns8b trppix_xpm_read_next_valid_char( trp_pix_xpm_t *is, int target )
{
    int c;

    for ( ; ; ) {
        c = trppix_xpm_next_valid_char( is );
        if ( c == target )
            break;
        if ( c == EOF )
            return 1;
    }
    return 0;
}

static uns8b trppix_xpm_read_int( trp_pix_xpm_t *is, uns32b *val )
{
    int c;

    for ( *val = 0, c = trppix_xpm_next_valid_char( is ) ; ; c = trppix_xpm_getc( is ) ) {
        if ( ( c < '0' ) || ( c > '9' ) )
            break;
        *val = ( *val * 10 ) + (uns8b)( c - '0' );
    }
    return ( c == EOF ) ? 1 : 0;
}

static uns8b trppix_xpm_read_code( trp_pix_xpm_t *is, uns32b dimpix, uns32b *code )
{
    int c;

    *code = 0;
    do {
        if ( ( c = trppix_xpm_getc( is ) ) == EOF )
            return 1;
        *code = ( *code << 8 ) | (uns8b)c;
    } while ( --dimpix );
    return 0;
}

static uns32b trppix_xpm_search_code( uns32b code, uns32b *pal_cor, uns32b dimpal )
{
    uns32b i;

    for ( i = 0 ; i < dimpal ; i++ )
        if ( code == pal_cor[ i ] )
            break;
    return i;
}

static uns8b trppix_xpm_read_color_nibble( trp_pix_xpm_t *is )
{
    int c;

    c = trppix_xpm_getc( is );
    if ( ( c >= 'a' ) && ( c <= 'f' ) )
        c = c - 'a' + 10;
    else if ( ( c >= 'A' ) && ( c <= 'F' ) )
        c = c - 'A' + 10;
    else if ( ( c >= '0' ) && ( c <= '9' ) )
        c -= '0';
    else c = 0;
    return c;
}

static uns8b trppix_xpm_read_color_byte( trp_pix_xpm_t *is )
{
    uns8b c;

    c = trppix_xpm_read_color_nibble( is );
    c = ( c << 4 ) | trppix_xpm_read_color_nibble( is );
    return c;
}

static uns8b trppix_xpm_read_color( trp_pix_xpm_t *is, trp_pix_color_t *color )
{
    int c;

    if ( ( c = trppix_xpm_next_valid_char( is ) ) == EOF )
        return 1;
    if ( ( c == 'N' ) || ( c == 'n' ) ) {
        if ( trppix_xpm_match_string( is, "one" ) )
            return 1;
        color->red = 0;
        color->green = 0;
        color->blue = 0;
        color->alpha = 0;
    } else if ( ( c == 'W' ) || ( c == 'w' ) ) {
        if ( trppix_xpm_match_string( is, "hite" ) )
            return 1;
        color->red = 0xff;
        color->green = 0xff;
        color->blue = 0xff;
        color->alpha = 0xff;
    } else if ( ( c == 'B' ) || ( c == 'b' ) ) {
        if ( trppix_xpm_match_string( is, "lack" ) )
            return 1;
        color->red = 0;
        color->green = 0;
        color->blue = 0;
        color->alpha = 0xff;
    } else if ( ( c == 'G' ) || ( c == 'g' ) ) {
        uns32b val;

        if ( trppix_xpm_match_string( is, "ray" ) )
            return 1;
        if ( trppix_xpm_read_int( is, &val ) )
            return 1;
        if ( val > 100 )
            return 1;
        val = ( val * 255 + 50 ) / 100;
        color->red = val;
        color->green = val;
        color->blue = val;
        color->alpha = 0xff;
    } else {
        if ( c != '#' )
            return 1;
        color->red = trppix_xpm_read_color_byte( is );
        color->green = trppix_xpm_read_color_byte( is );
        color->blue = trppix_xpm_read_color_byte( is );
        color->alpha = 0xff;
    }
    return 0;
}

static uns8b trp_pix_load_xpm_low( trp_pix_xpm_t *is, uns32b *w, uns32b *h, uns8b **data )
{
    trp_pix_color_t *pal = NULL;
    uns32b *pal_cor = NULL;
    uns8b *p;
    uns32b dimpal, dimpix, i, j, code;
    uns8b r;

    *data = NULL;
    if ( trppix_xpm_match_string( is, "/* XPM */" ) )
        goto error;
    if ( trppix_xpm_read_next_valid_char( is, '{' ) )
        goto error;
    if ( trppix_xpm_next_valid_char( is ) != '\"' )
        goto error;
    r  = trppix_xpm_read_int( is, w );
    r |= trppix_xpm_read_int( is, h );
    r |= trppix_xpm_read_int( is, &dimpal );
    r |= trppix_xpm_read_int( is, &dimpix );
    if ( r || ( *w == 0 ) || ( *h == 0 ) || ( dimpal == 0 ) || ( dimpix == 0 ) )
        goto error;
    if ( ( pal = malloc( dimpal * sizeof( trp_pix_color_t ) ) ) == NULL )
        goto error;
    if ( ( pal_cor = malloc( dimpal * sizeof( uns32b ) ) ) == NULL )
        goto error;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL )
        goto error;
    for ( i = 0 ; i < dimpal ; i++ ) {
        if ( trppix_xpm_read_next_valid_char( is, ',' ) )
            goto error;
        if ( trppix_xpm_next_valid_char( is ) != '\"' )
            goto error;
        if ( trppix_xpm_read_code( is, dimpix, &( pal_cor[ i ] ) ) )
            goto error;
        if ( trppix_xpm_next_valid_char( is ) != 'c' )
            goto error;
        if ( trppix_xpm_read_color( is, &( pal[ i ] ) ) )
            goto error;
    }
    for ( i = 0, p = *data ; i < *h ; i++ ) {
        if ( trppix_xpm_read_next_valid_char( is, ',' ) )
            goto error;
        if ( trppix_xpm_next_valid_char( is ) != '\"' )
            goto error;
        for ( j = 0 ; j < *w ; j++ ) {
            if ( trppix_xpm_read_code( is, dimpix, &code ) )
                goto error;
            if ( ( code = trppix_xpm_search_code( code, pal_cor, dimpal ) ) == dimpal )
                goto error;
            *p++ = pal[ code ].red;
            *p++ = pal[ code ].green;
            *p++ = pal[ code ].blue;
            *p++ = pal[ code ].alpha;
        }
    }
    free( pal );
    free( pal_cor );
    return 0;
error:
    free( *data );
    free( pal );
    free( pal_cor );
    return 1;
}

uns8b trp_pix_load_xpm( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    FILE *fp;
    trp_pix_xpm_t is;
    uns8b res;

    if ( ( fp = trp_fopen( cpath, "rb" ) ) == NULL )
        return 1;
    is.fp = fp;
    res = trp_pix_load_xpm_low( &is, w, h, data );
    fclose( fp );
    return res;
}

uns8b trp_pix_load_xpm_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    trp_pix_xpm_t is;

    is.fp = NULL;
    is.idata = idata;
    is.isize = isize;
    is.read = 0;
    return trp_pix_load_xpm_low( &is, w, h, data );
}

uns8b trp_pix_save_xpm( trp_obj_t *pix, trp_obj_t *path )
{
    /*
     FIXME
     */
    return 1;
}

