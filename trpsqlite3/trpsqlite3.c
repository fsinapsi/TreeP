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
#include "./trpsqlite3.h"
#include <sqlite3.h>

typedef struct {
    uns8b tipo;
    sqlite3 *s;
    uns32b level;
} trp_sqlite3_t;

typedef struct {
    uns8b mode;
    uns8b unc_tipo;
    uns8b compression_level;
    uns32b len;
    uns32b unc_len;
} trp_sqlite3_extra_t;

typedef struct {
    uns8b tipo;
    trp_sqlite3_extra_t extra;
    union {
        trp_raw_t *raw;
        uns8b *p;
        trp_char_t *c;
        trp_cord_t *s;
    } val;
    uns32b len;
} trp_sqlite3_arg_t;

static uns8b trp_sqlite3_print( trp_print_t *p, trp_sqlite3_t *obj );
static uns8b trp_sqlite3_close( trp_sqlite3_t *obj );
static uns8b trp_sqlite3_close_basic( uns8b flags, trp_sqlite3_t *obj );
static void trp_sqlite3_finalize( void *obj, void *data );
static uns8b trp_sqlite3_check( trp_obj_t *obj, sqlite3 **s, uns32b *level );
static int trp_sqlite3_encode_binary( const uns8b *in, int n, uns8b *out );
static int trp_sqlite3_decode_binary( const uns8b *in, uns8b *out );
static uns8b *trp_sqlite3_create_query( trp_obj_t *query, va_list args, uns32b n );
static trp_obj_t *trp_sqlite3_get_value( uns8b *v );
static int trp_sqlite3_callback( trp_obj_t **res, int argc, char **argv, char **col_names );
static int trp_sqlite3_test_callback( trp_obj_t **cdata, int argc, char **argv, char **col_names );

uns8b trp_sqlite3_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];

    _trp_print_fun[ TRP_SQLITE3 ] = trp_sqlite3_print;
    _trp_close_fun[ TRP_SQLITE3 ] = trp_sqlite3_close;
    return 0;
}

void trp_sqlite3_quit()
{
    /*
     FIXME
     */
}

static uns8b trp_sqlite3_print( trp_print_t *p, trp_sqlite3_t *obj )
{
    if ( trp_print_char_star( p, "#sqlite3 db" ) )
        return 1;
    if ( obj->s == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_sqlite3_close( trp_sqlite3_t *obj )
{
    return trp_sqlite3_close_basic( 1, obj );
}

static uns8b trp_sqlite3_close_basic( uns8b flags, trp_sqlite3_t *obj )
{
    uns8b res = 0;

    if ( obj->s ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        if ( obj->level ) {
            (void)sqlite3_exec( obj->s, "ROLLBACK", NULL, NULL, NULL );
            obj->level = 0;
        }
        res = sqlite3_close( obj->s ) ? 1 : 0;
        obj->s = NULL;
    }
    return res;
}

static void trp_sqlite3_finalize( void *obj, void *data )
{
    trp_sqlite3_close_basic( 0, (trp_sqlite3_t *)obj );
}

static uns8b trp_sqlite3_check( trp_obj_t *obj, sqlite3 **s, uns32b *level )
{
    if ( obj->tipo != TRP_SQLITE3 )
        return 1;
    *s = ((trp_sqlite3_t *)obj)->s;
    if ( *s == NULL )
        return 1;
    if ( level )
        *level = ((trp_sqlite3_t *)obj)->level;
    return 0;
}

trp_obj_t *trp_sqlite3_open( trp_obj_t *path )
{
    trp_sqlite3_t *obj = (trp_sqlite3_t *)UNDEF;
    sqlite3 *s;
    uns8b *cpath = trp_csprint( path );

    if ( sqlite3_open( cpath, &s ) == SQLITE_OK ) {
        obj = trp_gc_malloc_atomic_finalize( sizeof( trp_sqlite3_t ), trp_sqlite3_finalize );
        obj->tipo = TRP_SQLITE3;
        obj->s = s;
        obj->level = 0;
    }
    trp_csprint_free( cpath );
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_sqlite3_escape_strings( trp_obj_t *s, ... )
{
    uns32b len = 0;
    va_list args;
    CORD_pos i;
    CORD_ec x;
    uns8b c;

    CORD_ec_init( x );
    va_start( args, s );
    for ( ; s ; s = va_arg( args, trp_obj_t * ) ) {
        if ( s->tipo != TRP_CORD ) {
            uns8b *p = trp_csprint( s ), *q;
            for ( q = p ; *q ; q++ ) {
                if ( *q == '\'' ) {
                    CORD_ec_append( x, *q );
                    ++len;
                }
                CORD_ec_append( x, *q );
                ++len;
            }
            trp_csprint_free( p );
        } else {
            CORD_FOR( i, ((trp_cord_t *)s)->c ) {
                c = CORD_pos_fetch( i );
                if ( c == '\'' ) {
                    CORD_ec_append( x, c );
                    ++len;
                }
                if ( c ) {
                    CORD_ec_append( x, c );
                } else {
                    CORD_ec_flush_buf( x );
                    x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( 1 ) );
                }
                ++len;
            }
        }
    }
    va_end( args );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

/*
 ** Encode a binary buffer "in" of size n bytes so that it contains
 ** no instances of characters '\'' or '\000'.  The output is
 ** null-terminated and can be used as a string value in an INSERT
 ** or UPDATE statement.  Use sqlite_decode_binary() to convert the
 ** string back into its original binary.
 **
 ** The result is written into a preallocated output buffer "out".
 ** "out" must be able to hold at least 2 +(257*n)/254 bytes.
 ** In other words, the output will be expanded by as much as 3
 ** bytes for every 254 bytes of input plus 2 bytes of fixed overhead.
 ** (This is approximately 2 + 1.0118*n or about a 1.2% size increase.)
 **
 ** The return value is the number of characters in the encoded
 ** string, excluding the "\000" terminator.
 **
 ** If out==NULL then no output is generated but the routine still returns
 ** the number of characters that would have been generated if out had
 ** not been NULL.
 */
static int trp_sqlite3_encode_binary( const uns8b *in, int n, uns8b *out )
{
    int i, j, e, m;
    uns8b x;
    int cnt[256];

    if( n<=0 ){
        if( out ){
            out[0] = 'x';
            out[1] = 0;
        }
        return 1;
    }
    memset(cnt, 0, sizeof(cnt));
    for(i=n-1; i>=0; i--){ cnt[in[i]]++; }
    m = n;
    for(i=1; i<256; i++){
        int sum;
        if( i=='\'' ) continue;
        sum = cnt[i] + cnt[(i+1)&0xff] + cnt[(i+'\'')&0xff];
        if( sum<m ){
            m = sum;
            e = i;
            if( m==0 ) break;
        }
    }
    if( out==0 ){
        return n+m+1;
    }
    out[0] = e;
    j = 1;
    for(i=0; i<n; i++){
        x = in[i] - e;
        if( x==0 || x==1 || x=='\''){
            out[j++] = 1;
            x++;
        }
        out[j++] = x;
    }
    out[j] = 0;
    return j;
}

/*
 ** Decode the string "in" into binary data and write it into "out".
 ** This routine reverses the encoding created by sqlite_encode_binary().
 ** The output will always be a few bytes less than the input.  The number
 ** of bytes of output is returned.  If the input is not a well-formed
 ** encoding, -1 is returned.
 **
 ** The "in" and "out" parameters may point to the same buffer in order
 ** to decode a string in place.
 */
static int trp_sqlite3_decode_binary( const uns8b *in, uns8b *out )
{
    int i, e;
    uns8b c;

    e = *(in++);
    i = 0;
    while( (c = *(in++))!=0 ){
        if( c==1 ){
            c = *(in++) - 1;
        }
        out[i++] = c + e;
    }
    return i;
}

static uns8b *trp_sqlite3_create_query( trp_obj_t *query, va_list args, uns32b n )
{
    trp_sqlite3_arg_t *qargs = trp_gc_malloc( sizeof( trp_sqlite3_arg_t ) * n );
    uns8b *q;
    uns32b i, len = 0;
    uns8b error = 0;

    for ( i = 0 ; i < n ; i++, query = va_arg( args, trp_obj_t * ) ) {
        switch ( query->tipo ) {
        case TRP_SIG64:
        case TRP_RATIO:
            qargs[ i ].tipo = 2;
            qargs[ i ].val.p = trp_csprint( query );
            qargs[ i ].len = strlen( qargs[ i ].val.p );
            break;
        case TRP_CHAR:
            qargs[ i ].tipo = 3;
            qargs[ i ].val.c = (trp_char_t *)query;
            qargs[ i ].len = 1;
            break;
        case TRP_CORD:
            qargs[ i ].tipo = 4;
            qargs[ i ].val.s = (trp_cord_t *)query;
            qargs[ i ].len = ((trp_cord_t *)query)->len;
            break;
        default:
            if ( query->tipo != TRP_RAW ) {
                qargs[ i ].tipo = 0;
                qargs[ i ].val.raw = (trp_raw_t *)trp_compress( query, DIECI );
                if ( (trp_obj_t *)(qargs[ i ].val.raw) == UNDEF ) {
                    error = 1;
                    break;
                }
            } else {
                qargs[ i ].tipo = 1;
                qargs[ i ].val.raw = (trp_raw_t *)query;
            }
            qargs[ i ].extra.mode = qargs[ i ].val.raw->mode;
            qargs[ i ].extra.unc_tipo = qargs[ i ].val.raw->unc_tipo;
            qargs[ i ].extra.compression_level = qargs[ i ].val.raw->compression_level;
            qargs[ i ].extra.len = norm32( qargs[ i ].val.raw->len );
            qargs[ i ].extra.unc_len = norm32( qargs[ i ].val.raw->unc_len );
            qargs[ i ].len = 4 +
                trp_sqlite3_encode_binary( (uns8b *)(&(qargs[ i ].extra)), sizeof( trp_sqlite3_extra_t ), NULL ) +
                trp_sqlite3_encode_binary( qargs[ i ].val.raw->data, qargs[ i ].val.raw->len, NULL );
            break;
        }
        if ( error )
            break;
        len += qargs[ i ].len;
    }
    if ( error ) {
        for ( i = 0 ; i < n ; i++ )
            switch ( qargs[ i ].tipo ) {
            case 0:
                trp_raw_close( qargs[ i ].val.raw );
                trp_gc_free( qargs[ i ].val.raw );
                break;
            case 2:
                trp_csprint_free( qargs[ i ].val.p );
                break;
            }
        trp_gc_free( qargs );
        return NULL;
    }
    q = trp_malloc( len + 1 );
    for ( i = 0, len = 0 ; i < n ; i++ ) {
        switch ( qargs[ i ].tipo ) {
        case 0:
        case 1:
            q[ len     ] = 155;
            q[ len + 1 ] = 155;
            q[ len + 2 ] = 152 + qargs[ i ].tipo;
            q[ len + 3 ] = trp_sqlite3_encode_binary( (uns8b *)(&(qargs[ i ].extra)), sizeof( trp_sqlite3_extra_t ), q + len + 4 );
            trp_sqlite3_encode_binary( qargs[ i ].val.raw->data, qargs[ i ].val.raw->len, q + len + 4 + q[ len + 3 ] );
            if ( qargs[ i ].tipo == 0 ) {
                trp_raw_close( qargs[ i ].val.raw );
                trp_gc_free( qargs[ i ].val.raw );
            }
            break;
        case 2:
            memcpy( q + len, qargs[ i ].val.p, qargs[ i ].len );
            trp_csprint_free( qargs[ i ].val.p );
            break;
        case 3:
            q[ len ] = qargs[ i ].val.c->c;
            break;
        case 4:
            memcpy( q + len, CORD_to_const_char_star( qargs[ i ].val.s->c ), qargs[ i ].len );
            break;
        }
        len += qargs[ i ].len;
    }
    trp_gc_free( qargs );
    q[ len ] = 0;
    return q;
}

static trp_obj_t *trp_sqlite3_get_value( uns8b *v )
{
    if ( v == NULL )
        return UNDEF;
    if ( v[ 0 ] == 155 )
        if ( v[ 1 ] == 155 )
            if ( ( v[ 2 ] & 0xfe ) == 152 ) {
                trp_obj_t *res;
                trp_raw_t *raw;
                uns8b tipo;
                trp_sqlite3_extra_t extra;

                tipo = v[ 4 + v[ 3 ] ];
                v[ 4 + v[ 3 ] ] = 0;
                if ( trp_sqlite3_decode_binary( v + 4, (uns8b *)(&extra) ) != sizeof( trp_sqlite3_extra_t ) )
                    return UNDEF;
                v[ 4 + v[ 3 ] ] = tipo;
                tipo = v[ 2 ] & 1;
                raw = trp_gc_malloc( sizeof( trp_raw_t ) );
                raw->tipo = TRP_RAW;
                raw->mode = extra.mode;
                raw->unc_tipo = extra.unc_tipo;
                raw->compression_level = extra.compression_level;
                raw->len = norm32( extra.len );
                raw->unc_len = norm32( extra.unc_len );
                if ( tipo )
                    raw->data = trp_gc_malloc_atomic( raw->len );
                else
                    raw->data = trp_malloc( raw->len );
                if ( trp_sqlite3_decode_binary( v + 4 + v[ 3 ], raw->data ) != raw->len ) {
                    if ( tipo )
                        trp_gc_free( raw->data );
                    else
                        free( raw->data );
                    trp_gc_free( raw );
                    return UNDEF;
                }
                if ( tipo )
                    res = (trp_obj_t *)raw;
                else {
                    res = trp_uncompress( (trp_obj_t *)raw );
                    free( raw->data );
                    trp_gc_free( raw );
                }
                return res;
            }
    return trp_cord( v );
}

static int trp_sqlite3_callback( trp_obj_t **res, int argc, char **argv, char **col_names )
{
    trp_obj_t *l = NIL;

    while ( argc )
        l = trp_cons( trp_sqlite3_get_value( argv[ --argc ] ), l );
    l = trp_cons( l, NIL );
    if ( *res == UNDEF ) {
        *res = trp_gc_malloc( sizeof( trp_cons_t ) );
        ((trp_cons_t *)*res)->car = l;
    } else {
        ((trp_cons_t *)(((trp_cons_t *)*res)->cdr))->cdr = l;
    }
    ((trp_cons_t *)*res)->cdr = l;
    return 0;
}

trp_obj_t *trp_sqlite3_exec( trp_obj_t *obj, trp_obj_t *query, ... )
{
    trp_obj_t *res = UNDEF;
    uns32b n;
    sqlite3 *s;
    uns8b *q;
    va_list args;

    if ( trp_sqlite3_check( obj, &s, NULL ) )
        return UNDEF;
    va_start( args, query );
    n = trp_nargs( args );
    va_end( args );
    va_start( args, query );
    q = trp_sqlite3_create_query( query, args, n );
    va_end( args );
    if ( q == NULL )
        return UNDEF;
    switch ( sqlite3_exec( s, q, (void *)trp_sqlite3_callback, (void *)(&res), NULL ) ) {
    case SQLITE_OK:
        if ( res == UNDEF ) {
            res = NIL;
        } else {
            res = ((trp_cons_t *)res)->car;
        }
        break;
    default:
        res = UNDEF;
        break;
    }
    free( q );
    return res;
}

static int trp_sqlite3_test_callback( trp_obj_t **cdata, int argc, char **argv, char **col_names )
{
    int i, j = 0;
    uns8bfun_t f = ((trp_netptr_t *)( cdata[ 0 ] ))->f;
    trp_obj_t *p[ 21 ];

    if ( argc > 20 )
        return 1;
    if ( cdata[ 1 ] )
        p[ j++ ] = cdata[ 1 ];
    for ( i = 0 ; i < argc ; )
        p[ j++ ] = trp_sqlite3_get_value( argv[ i++ ] );
    while ( j < 21 )
        p[ j++ ] = UNDEF;
    switch ( ((trp_netptr_t *)( cdata[ 0 ] ))->nargs ) {
    case 0:
        i = (int)( (f)() );
        break;
    case 1:
        i = (int)( (f)( p[ 0 ] ) );
        break;
    case 2:
        i = (int)( (f)( p[ 0 ], p[ 1 ] ) );
        break;
    case 3:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ] ) );
        break;
    case 4:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ]) );
        break;
    case 5:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ] ) );
        break;
    case 6:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ] ) );
        break;
    case 7:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ] ) );
        break;
    case 8:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ] ) );
        break;
    case 9:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ] ) );
        break;
    case 10:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ] ) );
        break;
    case 11:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ] ) );
        break;
    case 12:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ] ) );
        break;
    case 13:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ] ) );
        break;
    case 14:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ] ) );
        break;
    case 15:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ] ) );
        break;
    case 16:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ] ) );
        break;
    case 17:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ], p[ 16 ] ) );
        break;
    case 18:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ], p[ 16 ], p[ 17 ] ) );
        break;
    case 19:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ] ) );
        break;
    case 20:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ] ) );
        break;
    case 21:
        i = (int)( (f)( p[ 0 ], p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ],
                        p[ 5 ], p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ],
                        p[ 10 ], p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ],
                        p[ 15 ], p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ],
                        p[ 20 ] ) );
        break;
    }
    return i;
}

uns8b trp_sqlite3_exec_data( trp_obj_t *obj, trp_obj_t *net, trp_obj_t *data, trp_obj_t *query, ... )
{
    trp_obj_t *cdata[ 2 ];
    uns32b n;
    sqlite3 *s;
    uns8b *q, res = 1;
    va_list args;

    if ( trp_sqlite3_check( obj, &s, NULL ) )
        return 1;
    if ( net->tipo == TRP_NETPTR ) {
        if ( ((trp_netptr_t *)net)->nargs > 20 )
            return 1;
    } else if ( net != UNDEF )
        return 1;
    cdata[ 0 ] = net;
    cdata[ 1 ] = data;
    va_start( args, query );
    n = trp_nargs( args );
    va_end( args );
    va_start( args, query );
    q = trp_sqlite3_create_query( query, args, n );
    va_end( args );
    if ( q == NULL )
        return 1;
    if ( sqlite3_exec( s, q,
                       ( net == UNDEF ) ? NULL : (void *)trp_sqlite3_test_callback,
                       ( net == UNDEF ) ? NULL : (void *)cdata, NULL ) == SQLITE_OK )
        res = 0;
    free( q );
    return res;
}

uns8b trp_sqlite3_begin( trp_obj_t *obj )
{
    sqlite3 *s;
    uns32b level;

    if ( trp_sqlite3_check( obj, &s, &level ) )
        return 1;
    ((trp_sqlite3_t *)obj)->level = level + 1;
    if ( level )
        return 0;
    return ( sqlite3_exec( s, "BEGIN", NULL, NULL, NULL ) == SQLITE_OK ) ? 0 : 1;
}

uns8b trp_sqlite3_begin_exclusive( trp_obj_t *obj )
{
    sqlite3 *s;
    uns32b level;

    if ( trp_sqlite3_check( obj, &s, &level ) )
        return 1;
    ((trp_sqlite3_t *)obj)->level = level + 1;
    if ( level )
        return 0;
    return ( sqlite3_exec( s, "BEGIN EXCLUSIVE", NULL, NULL, NULL ) == SQLITE_OK ) ? 0 : 1;
}

uns8b trp_sqlite3_end( trp_obj_t *obj )
{
    sqlite3 *s;
    uns32b level;

    if ( trp_sqlite3_check( obj, &s, &level ) )
        return 1;
    if ( level == 0 )
        return 1;
    ((trp_sqlite3_t *)obj)->level = level - 1;
    if ( level > 1 )
        return 0;
    return ( sqlite3_exec( s, "COMMIT", NULL, NULL, NULL ) == SQLITE_OK ) ? 0 : 1;
}

uns8b trp_sqlite3_rollback( trp_obj_t *obj )
{
    sqlite3 *s;
    uns32b level;

    if ( trp_sqlite3_check( obj, &s, &level ) )
        return 1;
    if ( level == 0 )
        return 1;
    ((trp_sqlite3_t *)obj)->level = level - 1;
    if ( level > 1 )
        return 0;
    return ( sqlite3_exec( s, "ROLLBACK", NULL, NULL, NULL ) == SQLITE_OK ) ? 0 : 1;
}

trp_obj_t *trp_sqlite3_changes( trp_obj_t *obj )
{
    sqlite3 *s;

    if ( trp_sqlite3_check( obj, &s, NULL ) )
        return UNDEF;
    return trp_sig64( sqlite3_changes( s ) );
}

trp_obj_t *trp_sqlite3_total_changes( trp_obj_t *obj )
{
    sqlite3 *s;

    if ( trp_sqlite3_check( obj, &s, NULL ) )
        return UNDEF;
    return trp_sig64( sqlite3_total_changes( s ) );
}

