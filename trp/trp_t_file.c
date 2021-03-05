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

#include "trp.h"
#ifndef MINGW
#include <netdb.h>
#endif

#ifdef MINGW
#define trp_off_t sig64b
#define fseeko fseeko64
#define ftello ftello64
#else
#define trp_off_t off_t
#endif

static void trp_file_finalize( void *obj, void *data );
static trp_obj_t *trp_file_internal( FILE *fp, uns8b flags );
static uns8b trp_file_read_char( FILE *fp, uns8b *c );

uns8b trp_file_print( trp_print_t *p, trp_file_t *obj )
{
    /*
     FIXME
     */
    if ( trp_print_char_star( p, "#file" ) )
        return 1;
    if ( obj->fp == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

trp_obj_t *trp_file_equal( trp_file_t *o1, trp_file_t *o2 )
{
    /*
     FIXME
     */
    return TRP_FALSE;
}

trp_obj_t *trp_file_length( trp_file_t *obj )
{
    FILE *fp;
    sig64b i, j;

    if ( ( fp = trp_file_readable_fp( (trp_obj_t *)obj ) ) == NULL )
        return UNDEF;
    i = (sig64b)ftello( fp );
    if ( i < 0 )
        return UNDEF;
    if ( fseeko( fp, 0, SEEK_END ) )
        return UNDEF;
    j = (sig64b)ftello( fp );
    fseeko( fp, (trp_off_t)i, SEEK_SET );
    if ( j < 0 )
        return UNDEF;
    return trp_sig64( j );
}

uns8b trp_file_close( trp_file_t *obj )
{
    uns8b res = 0;

    if ( ( (trp_obj_t *)obj == TRP_STDIN ) ||
         ( (trp_obj_t *)obj == TRP_STDOUT ) ||
         ( (trp_obj_t *)obj == TRP_STDERR ) )
        return 1;
    if ( obj->fp ) {
        (void)pthread_mutex_destroy( &( obj->mutex ) );
        if ( obj->flags & 8 ) {
            if ( pclose( obj->fp ) < 0 )
                res = 1;
        } else
            fclose( obj->fp );
        obj->fp = NULL;
        trp_gc_remove_finalizer( (trp_obj_t *)obj );
    }
    return res;
}

static void trp_file_finalize( void *obj, void *data )
{
    trp_file_close( (trp_file_t *)obj );
}

static trp_obj_t *trp_file_internal( FILE *fp, uns8b flags )
{
    trp_file_t *obj;

    if ( fp == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_file_t ), trp_file_finalize );
    obj->tipo = TRP_FILE;
    obj->flags = flags;
    (void)pthread_mutex_init( &( obj->mutex ), NULL );
    obj->fp = fp;
    obj->line = 0;
    obj->last = 0;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_stdin()
{
    static trp_obj_t *c = NULL;

    if ( c == NULL )
        c = trp_file_internal( stdin, 1 );
    return c;
}

trp_obj_t *trp_stdout()
{
    static trp_obj_t *c = NULL;

    if ( c == NULL )
        c = trp_file_internal( stdout, 2 );
    return c;
}

trp_obj_t *trp_stderr()
{
    static trp_obj_t *c = NULL;

    if ( c == NULL )
        c = trp_file_internal( stderr, 2 );
    return c;
}

FILE *trp_file_readable_fp( trp_obj_t *stream )
{
    if ( stream->tipo != TRP_FILE )
        return NULL;
    if ( ~(((trp_file_t *)stream)->flags) & 1 )
        return NULL;
    return ((trp_file_t *)stream)->fp;
}

FILE *trp_file_writable_fp( trp_obj_t *stream )
{
    if ( stream->tipo != TRP_FILE )
        return NULL;
    if ( ~(((trp_file_t *)stream)->flags) & 2 )
        return NULL;
    return ((trp_file_t *)stream)->fp;
}

trp_obj_t *trp_file_openro( trp_obj_t *path )
{
    trp_obj_t *res;
    uns8b *cpath = trp_csprint( path );

    res = trp_file_internal( trp_fopen( cpath, "rb" ), 1 );
    trp_csprint_free( cpath );
    return res;
}

trp_obj_t *trp_file_openrw( trp_obj_t *path )
{
    trp_obj_t *res;
    FILE *fp;
    uns8b *cpath = trp_csprint( path );

    if ( ( fp = trp_fopen( cpath, "r+b" ) ) == NULL )
        fp = trp_fopen( cpath, "w+b" );
    res = trp_file_internal( fp, 3 );
    trp_csprint_free( cpath );
    return res;
}

trp_obj_t *trp_file_create( trp_obj_t *path )
{
    trp_obj_t *res;
    uns8b *cpath = trp_csprint( path );

    res = trp_file_internal( trp_fopen( cpath, "w+b" ), 3 );
    trp_csprint_free( cpath );
    return res;
}

trp_obj_t *trp_file_open_client( trp_obj_t *server, trp_obj_t *port )
{
#ifdef MINGW
    return UNDEF;
#else
    int fildes;
    uns32b pp;
    uns8b *p, *q;
    struct hostent *hp;
    struct sockaddr_in address;

    if ( trp_cast_uns32b( port, &pp ) )
        return UNDEF;
    p = trp_csprint( server );
    hp = gethostbyname( p );
    trp_csprint_free( p );
    if ( hp == NULL )
        return UNDEF;
    if ( ( fildes = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
        return UNDEF;
    address.sin_addr.s_addr = 0;
    address.sin_family = AF_INET;
    address.sin_port = htons( pp );
    for ( pp = (uns32b)hp->h_length, p = (uns8b *)&address.sin_addr, q = (uns8b *)hp->h_addr ;
          pp ;
          pp--, *p++ = *q++ );
    if ( connect( fildes, (struct sockaddr *)&address, sizeof( address ) ) <0 ) {
        close( fildes );
        return UNDEF;
    }
    return trp_file_internal( fdopen( fildes, "r+b" ), 7 );
#endif
}

trp_obj_t *trp_file_popenr( trp_obj_t *cmd, ... )
{
    FILE *fp;
    uns8b *c;
    va_list args;

    va_start( args, cmd );
    c = trp_csprint_multi( cmd, args );
    va_end( args );
    fp = popen( c, "r" );
    trp_csprint_free( c );
    return trp_file_internal( fp, 9 );
}

trp_obj_t *trp_file_popenw( trp_obj_t *cmd, ... )
{
    FILE *fp;
    uns8b *c;
    va_list args;

    va_start( args, cmd );
    c = trp_csprint_multi( cmd, args );
    va_end( args );
    fflush( NULL );
    fp = popen( c, "w" );
    trp_csprint_free( c );
    return trp_file_internal( fp, 10 );
}

uns8b trp_file_flush( trp_obj_t *stream )
{
    FILE *fp = trp_file_writable_fp( stream );

    if ( fp == NULL )
        return 1;
    return fflush( fp ) ? 1 : 0;
}

uns8b trp_file_set_pos( trp_obj_t *pos, trp_obj_t *obj )
{
    FILE *fp;

    if ( pos->tipo != TRP_SIG64 )
        return 1;
    if ( ((trp_sig64_t *)pos)->val < 0 )
        return 1;
    if ( ( fp = trp_file_readable_fp( (trp_obj_t *)obj ) ) == NULL )
        return 1;
    if ( fseeko( fp, (trp_off_t)( ((trp_sig64_t *)pos)->val ), SEEK_SET ) )
        return 1;
    ((trp_file_t *)obj)->line = ((trp_sig64_t *)pos)->val ? 0xffffffff : 0;
    return 0;
}

trp_obj_t *trp_file_pos( trp_obj_t *obj )
{
    FILE *fp;
    sig64b i;

    if ( ( fp = trp_file_readable_fp( (trp_obj_t *)obj ) ) == NULL )
        return UNDEF;
    i = (sig64b)ftello( fp );
    if ( i < 0 )
        return UNDEF;
    return trp_sig64( i );
}

trp_obj_t *trp_file_pos_line( trp_obj_t *obj )
{
    FILE *fp;

    if ( ( fp = trp_file_readable_fp( (trp_obj_t *)obj ) ) == NULL )
        return UNDEF;
    if ( ((trp_file_t *)obj)->line == 0xffffffff )
        return UNDEF;
    return trp_sig64( ((trp_file_t *)obj)->line + 1 );
}

uns32b trp_file_read_chars( FILE *fp, uns8b *buf, uns32b n )
{
    uns32b i, off = 0;

    while ( n ) {
        i = fread( buf + off, 1, n, fp );
        if ( i == 0 )
            if ( feof( fp ) )
                return off;
        n -= i;
        off += i;
    }
    return off;
}

uns32b trp_file_write_chars( FILE *fp, uns8b *buf, uns32b n )
{
    /*
     FIXME
     andrà riscritta meglio...
     */
    return fwrite( buf, 1, n, fp );
}

static uns8b trp_file_read_char( FILE *fp, uns8b *c )
{
    return ( trp_file_read_chars( fp, c, 1 ) == 1 ) ? 0 : 1;
}

static uns8b trp_file_read_char2( FILE *fp, uns8b *c, uns8b issocket )
{
    int i;

    for ( ; ; ) {
        if ( issocket ) {
            i = read( fileno( fp ), c, 1 );
        } else
            i = fread( c, 1, 1, fp );
        if ( i == 1 )
            break;
        if ( i < 0 )
            return 1;
        if ( issocket ) {
#ifdef MINGW
            return 1;
#else
            struct pollfd ufds[ 1 ];

            ufds[ 0 ].fd = fileno( fp );
            ufds[ 0 ].events = POLLHUP;
            if ( poll( ufds, 1, -1 ) == 1 )
                if ( ufds[ 0 ].revents & POLLHUP )
                    return 1;
#endif
        } else if ( feof( fp ) )
            return 1;
    }
    return 0;
}

trp_obj_t *trp_read_char( trp_obj_t *stream )
{
    FILE *fp;
    uns8b c, issocket;

    if ( ( fp = trp_file_readable_fp( stream ) ) == NULL )
        return UNDEF;
   issocket = ( (((trp_file_t* )stream)->flags) & 4 ) ? 1 : 0;
    if ( trp_file_read_char2( fp, &c, issocket ) )
        return UNDEF;
    if ( ( ( c == '\n' ) && ( ((trp_file_t *)stream)->last != '\r' ) ) ||
         ( ( c == '\r' ) && ( ((trp_file_t *)stream)->last != '\n' ) ) ) {
        if ( ((trp_file_t *)stream)->line < 0xffffffff )
            (((trp_file_t *)stream)->line)++;
        ((trp_file_t *)stream)->last = c;
    } else {
        ((trp_file_t *)stream)->last = 0;
    }
    return trp_char( c );
}

trp_obj_t *trp_read_line( trp_obj_t *stream )
{
    uns32b cnt = 0;
    FILE *fp;
    CORD_ec x;
    uns8b c, issocket;

    if ( ( fp = trp_file_readable_fp( stream ) ) == NULL )
        return UNDEF;
    issocket = ( (((trp_file_t* )stream)->flags) & 4 ) ? 1 : 0;
    if ( trp_file_read_char2( fp, &c, issocket ) )
        return UNDEF;
    if ( ( ( c == '\n' ) && ( ((trp_file_t *)stream)->last == '\r' ) ) ||
         ( ( c == '\r' ) && ( ((trp_file_t *)stream)->last == '\n' ) ) )
        if ( trp_file_read_char2( fp, &c, issocket ) )
            return UNDEF;
    CORD_ec_init( x );
    while( c && ( c != '\n' ) && ( c != '\r' ) ) {
        cnt++;
        CORD_ec_append( x, c );
        if ( trp_file_read_char2( fp, &c, issocket ) )
            c = 0;
    }
    ((trp_file_t *)stream)->last = c;
    (((trp_file_t *)stream)->line)++;
    if ( cnt == 0 )
        return EMPTYCORD;
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), cnt );
}

trp_obj_t *trp_read_str( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n, i;
    int c;
    CORD_ec x;

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    CORD_ec_init( x );
    for ( i = n ; i ; ) {
        i--;
        c = getc( fp );
        if ( c == 0 ) {
            /* Append the right number of NULs */
            /* Note that any string of NULs is represented in 4 words, */
            /* independent of its length. */
            register size_t count = 1;

            CORD_ec_flush_buf( x );
            while ( i ) {
                i--;
                if ( c = getc( fp ) )
                    break;
                count++;
            }
            x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( count ) );
            if ( c == 0 )
                break;
        }
        if ( c == EOF )
            return UNDEF;
        CORD_ec_append( x, c );
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), n );
}

trp_obj_t *trp_read_uint_le( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n;
    uns64b c64;

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    if ( ( n == 0 ) || ( n > 64 ) || ( n & 7 ) )
        return UNDEF;
    n >>= 3;
    c64 = 0;
    if ( trp_file_read_chars( fp, (uns8b *)(&c64), n ) != n )
        return UNDEF;
#ifdef TRP_BIG_ENDIAN
    c64 = trp_swap_endian64( c64 );
#endif
    if ( c64 & 0x8000000000000000LL )
        return trp_cat( trp_sig64( (sig64b)( c64 & 0x7fffffffffffffffLL ) ), TRP_MAXINT, UNO, NULL );
    return trp_sig64( (sig64b)c64 );
}

trp_obj_t *trp_read_uint_be( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n;
    uns64b c64;

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    if ( ( n == 0 ) || ( n > 64 ) || ( n & 7 ) )
        return UNDEF;
    n >>= 3;
    c64 = 0;
    if ( trp_file_read_chars( fp, ( (uns8b *)(&c64) ) + ( 8 - n ), n ) != n )
        return UNDEF;
#ifdef TRP_LITTLE_ENDIAN
    c64 = trp_swap_endian64( c64 );
#endif
    if ( c64 & 0x8000000000000000LL )
        return trp_cat( trp_sig64( (sig64b)( c64 & 0x7fffffffffffffffLL ) ), TRP_MAXINT, UNO, NULL );
    return trp_sig64( (sig64b)c64 );
}

trp_obj_t *trp_read_sint_le( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n, i;
    uns64b v, m;
    sig64b c64;
    uns8b c[ 8 ];

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    if ( ( n == 0 ) || ( n > 64 ) || ( n & 7 ) )
        return UNDEF;
    n >>= 3;
    if ( trp_file_read_chars( fp, c, n ) != n )
        return UNDEF;
    for ( i = 0, v = 0, m = 0 ; i < n ; i++ ) {
#ifdef TRP_LITTLE_ENDIAN
        v = ( v << 8 ) | c[ n - 1 - i ];
#else
        v = ( v << 8 ) | c[ i ];
#endif
        if ( m )
            m <<= 8;
        else
            m = 128;
    }
    if ( v < m ) {
        c64 = v;
    } else {
        c64 = (sig64b)( v - m );
        c64 -= m;
    }
    return trp_sig64( c64 );
}

trp_obj_t *trp_read_sint_be( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n, i;
    uns64b v, m;
    sig64b c64;
    uns8b c[ 8 ];

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    if ( ( n == 0 ) || ( n > 64 ) || ( n & 7 ) )
        return UNDEF;
    n >>= 3;
    if ( trp_file_read_chars( fp, c, n ) != n )
        return UNDEF;
    for ( i = 0, v = 0, m = 0 ; i < n ; i++ ) {
#ifdef TRP_LITTLE_ENDIAN
        v = ( v << 8 ) | c[ i ];
#else
        v = ( v << 8 ) | c[ n - 1 - i ];
#endif
        if ( m )
            m <<= 8;
        else
            m = 128;
    }
    if ( v < m ) {
        c64 = v;
    } else {
        c64 = (sig64b)( v - m );
        c64 -= m;
    }
    return trp_sig64( c64 );
}

trp_obj_t *trp_read_float_le( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n;
    uns64b c64;
    uns32b c32;
    flt64b d64;
    flt32b d32;

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    switch ( n ) {
    case 32:
#ifdef TRP_BIG_ENDIAN
        if ( trp_file_read_chars( fp, (uns8b *)(&c32), 4 ) != 4 )
            return UNDEF;
        c32 = trp_swap_endian32( c32 );
        d32 = *((flt32b *)(&c32));
#else
        if ( trp_file_read_chars( fp, (uns8b *)(&d32), 4 ) != 4 )
            return UNDEF;
#endif
        d64 = (flt64b)d32;
        break;
    case 64:
#ifdef TRP_BIG_ENDIAN
        if ( trp_file_read_chars( fp, (uns8b *)(&c64), 8 ) != 8 )
            return UNDEF;
        c64 = trp_swap_endian64( c64 );
        d64 = *((flt64b *)(&c64));
#else
        if ( trp_file_read_chars( fp, (uns8b *)(&d64), 8 ) != 8 )
            return UNDEF;
#endif
        break;
    default:
        return UNDEF;
    }
    return trp_double( (double)d64 );
}

trp_obj_t *trp_read_float_be( trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b n;
    uns64b c64;
    uns32b c32;
    flt64b d64;
    flt32b d32;

    if ( ( ( fp = trp_file_readable_fp( stream ) ) == NULL ) ||
         trp_cast_uns32b( cnt, &n ) )
        return UNDEF;
    switch ( n ) {
    case 32:
#ifdef TRP_LITTLE_ENDIAN
        if ( trp_file_read_chars( fp, (uns8b *)(&c32), 4 ) != 4 )
            return UNDEF;
        c32 = trp_swap_endian32( c32 );
        d32 = *((flt32b *)(&c32));
#else
        if ( trp_file_read_chars( fp, (uns8b *)(&d32), 4 ) != 4 )
            return UNDEF;
#endif
        d64 = (flt64b)d32;
        break;
    case 64:
#ifdef TRP_LITTLE_ENDIAN
        if ( trp_file_read_chars( fp, (uns8b *)(&c64), 8 ) != 8 )
            return UNDEF;
        c64 = trp_swap_endian64( c64 );
        d64 = *((flt64b *)(&c64));
#else
        if ( trp_file_read_chars( fp, (uns8b *)(&d64), 8 ) != 8 )
            return UNDEF;
#endif
        break;
    default:
        return UNDEF;
    }
    return trp_double( (double)d64 );
}

