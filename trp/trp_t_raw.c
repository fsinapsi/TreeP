/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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
#include <zlib.h>

#ifdef MINGW
#define fseeko fseeko64
#define ftello ftello64
#endif

uns32b trp_size_internal( trp_obj_t *obj );
void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
trp_obj_t *trp_decode_internal( uns8b **buf );
trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );

static void trp_raw_realloc_internal( trp_raw_t *obj, uns32b sz );

uns8b trp_raw_print( trp_print_t *p, trp_raw_t *obj )
{
    uns8b buf[ 11 ];

    if ( trp_print_char_star( p, "#raw (length=" ) )
        return 1;
    sprintf( buf, "%u", obj->len );
    if ( trp_print_char_star( p, buf ) )
        return 1;
    if ( obj->mode ) {
        extern uns8b *_trp_tipo_descr[];
        uns8b *t;

        if ( trp_print_char_star( p, ", uncompressed=" ) )
            return 1;
        sprintf( buf, "%u", obj->unc_len );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( trp_print_char_star( p, ", level=" ) )
            return 1;
        sprintf( buf, "%d", (int)( obj->compression_level ) );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( trp_print_char_star( p, ", " ) )
            return 1;
        t = _trp_tipo_descr[ obj->unc_tipo ];
        if ( trp_print_char_star( p, t ) )
            return 1;
    }
    return trp_print_char_star( p, ")#" );
}

uns32b trp_raw_size( trp_raw_t *obj )
{
    return 1 + 1 + 1 + 1 + 4 + 4 + obj->len;
}

void trp_raw_encode( trp_raw_t *obj, uns8b **buf )
{
    uns32b *p;

    **buf = TRP_RAW;
    ++(*buf);
    **buf = obj->mode;
    ++(*buf);
    **buf = obj->unc_tipo;
    ++(*buf);
    **buf = obj->compression_level;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    p = (uns32b *)(*buf);
    *p = norm32( obj->unc_len );
    (*buf) += 4;
    memcpy( *buf, obj->data, obj->len );
    (*buf) += obj->len;
}

trp_obj_t *trp_raw_decode( uns8b **buf )
{
    trp_obj_t *res;
    uns32b len, unc_len;
    uns8b mode, unc_tipo, compression_level;

    mode = **buf;
    ++(*buf);
    unc_tipo = **buf;
    ++(*buf);
    compression_level = **buf;
    ++(*buf);
    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    unc_len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    res = trp_raw_internal( len, 0 );
    ((trp_raw_t *)res)->mode = mode;
    ((trp_raw_t *)res)->unc_tipo = unc_tipo;
    ((trp_raw_t *)res)->compression_level = compression_level;
    ((trp_raw_t *)res)->unc_len = unc_len;
    memcpy( ((trp_raw_t *)res)->data, *buf, len );
    (*buf) += len;
    return res;
}

trp_obj_t *trp_raw_equal( trp_raw_t *o1, trp_raw_t *o2 )
{
    if ( o1->len != o2->len )
        return TRP_FALSE;
    return memcmp( o1->data, o2->data, o1->len ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_raw_length( trp_raw_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_raw_nth( uns32b n, trp_raw_t *obj )
{
    return ( n < obj->len ) ? trp_char( obj->data[ n ] ) : UNDEF;
}

trp_obj_t *trp_raw_cat( trp_raw_t *obj, va_list args )
{
    trp_obj_t *q = trp_queue();
    trp_raw_t *raw;
    uns32b len = 0;

    for ( ; obj ; obj = va_arg( args, trp_raw_t * ) ) {
        if ( obj->tipo != TRP_RAW ) {
            while ( ((trp_queue_t *)q)->len )
                trp_queue_get( q );
            trp_gc_free( q );
            return UNDEF;
        }
        trp_queue_put( q, (trp_obj_t *)obj );
        len += obj->len;
    }
    raw = (trp_raw_t *)trp_raw_internal( len, 0 );
    for ( len = 0 ; ((trp_queue_t *)q)->len ; ) {
        obj = (trp_raw_t *)trp_queue_get( q );
        memcpy( raw->data + len, obj->data, obj->len );
        len += obj->len;
    }
    trp_gc_free( q );
    return (trp_obj_t *)raw;
}

uns8b trp_raw_close( trp_raw_t *obj )
{
    trp_raw_realloc_internal( obj, 0 );
    return 0;
}

trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc )
{
    trp_raw_t *obj;

    obj = trp_gc_malloc( sizeof( trp_raw_t ) );
    obj->tipo = TRP_RAW;
    obj->mode = 0;
    obj->unc_tipo = 0;
    obj->compression_level = 0;
    obj->len = sz;
    obj->unc_len = 0;
    if ( sz )
        if ( use_malloc ) {
            obj->data = trp_malloc( sz );
        } else
            obj->data = trp_gc_malloc_atomic( sz );
    else
        obj->data = NULL;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_raw( trp_obj_t *n )
{
    uns32b sz;

    if ( trp_cast_uns32b( n, &sz ) )
        return UNDEF;
    return trp_raw_internal( sz, 0 );
}

static void trp_raw_realloc_internal( trp_raw_t *obj, uns32b sz )
{
    obj->mode = 0;
    obj->unc_tipo = 0;
    obj->compression_level = 0;
    obj->len = sz;
    obj->unc_len = 0;
    if ( sz ) {
        if ( obj->data )
            obj->data = trp_gc_realloc( obj->data, sz );
        else
            obj->data = trp_gc_malloc_atomic( sz );
    } else {
        trp_gc_free( obj->data );
        obj->data = NULL;
    }
}

uns8b trp_raw_realloc( trp_obj_t *obj, trp_obj_t *n )
{
    uns32b sz;

    if ( ( obj->tipo != TRP_RAW ) || trp_cast_uns32b( n, &sz ) )
        return 1;
    trp_raw_realloc_internal( (trp_raw_t *)obj, sz );
    return 0;
}

trp_obj_t *trp_raw_mode( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_RAW )
        return UNDEF;
    return trp_sig64( ((trp_raw_t *)obj)->mode );
}

trp_obj_t *trp_raw_compression_level( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_RAW )
        return UNDEF;
    return trp_sig64( ((trp_raw_t *)obj)->compression_level );
}

trp_obj_t *trp_raw_uncompressed_len( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_RAW )
        return UNDEF;
    return trp_sig64( ((trp_raw_t *)obj)->unc_len );
}

trp_obj_t *trp_raw_uncompressed_type( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_RAW )
        return UNDEF;
    return trp_sig64( ((trp_raw_t *)obj)->unc_tipo );
}

uns32b trp_size_internal( trp_obj_t *obj )
{
    extern uns32bfun_t _trp_size_fun[];

    return (_trp_size_fun[ obj->tipo ])( obj );
}

void trp_encode_internal( trp_obj_t *obj, uns8b **buf )
{
    extern voidfun_t _trp_encode_fun[];

    (_trp_encode_fun[ obj->tipo ])( obj, buf );
}

trp_obj_t *trp_decode_internal( uns8b **buf )
{
    extern objfun_t _trp_decode_fun[];
    uns8b tipo;

    tipo = **buf;
    ++(*buf);
    return (_trp_decode_fun[ tipo ])( buf );
}

trp_obj_t *trp_compress( trp_obj_t *obj, trp_obj_t *level )
{
    int lv;
    unsigned long slen, dlen;
    trp_raw_t *raw, *res;

    if ( level ) {
        if ( level->tipo != TRP_SIG64 )
            return UNDEF;
        if ( ( ((trp_sig64_t *)level)->val < 0 ) ||
             ( ((trp_sig64_t *)level)->val > 10 ) )
            return UNDEF;
        lv = ((trp_sig64_t *)level)->val;
    } else {
        lv = 6;
    }
    if ( obj->tipo == TRP_RAW ) {
        /*
         per il momento, un raw compresso non si puo' ricomprimere...
         */
        if ( ((trp_raw_t *)obj)->mode )
            return UNDEF;
        if ( lv ) {
            raw = (trp_raw_t *)obj;
        } else {
            raw = (trp_raw_t *)trp_raw_internal( ((trp_raw_t *)obj)->len, 0 );
            memcpy( raw->data, ((trp_raw_t *)obj)->data, ((trp_raw_t *)obj)->len );
        }
    } else {
        uns8b *buf;

        raw = (trp_raw_t *)trp_raw_internal( trp_size_internal( obj ), 0 );
        buf = raw->data;
        trp_encode_internal( obj, &buf );
    }
    if ( lv == 0 ) {
        raw->mode = 1;
        raw->unc_tipo = obj->tipo;
        raw->compression_level = 0;
        raw->unc_len = raw->len;
        return (trp_obj_t *)raw;
    }
    slen = raw->len;
    dlen = compressBound( slen );
    res = (trp_raw_t *)trp_raw_internal( dlen, 0 );
    if ( compress2( res->data, &dlen, raw->data, slen, ( lv < 10 ) ? lv : 9 ) != Z_OK ) {
        trp_gc_free( res->data );
        trp_gc_free( res );
        if ( obj->tipo != TRP_RAW ) {
            trp_gc_free( raw->data );
            trp_gc_free( raw );
        }
        return UNDEF;
    }
    if ( ( lv == 10 ) && ( dlen >= slen ) ) {
        trp_gc_free( res->data );
        trp_gc_free( res );
        raw->mode = 1;
        raw->unc_tipo = obj->tipo;
        raw->compression_level = 0;
        raw->unc_len = raw->len;
        return (trp_obj_t *)raw;
    }
    if ( obj->tipo != TRP_RAW ) {
        trp_gc_free( raw->data );
        trp_gc_free( raw );
    }
    trp_raw_realloc_internal( res, dlen );
    res->mode = 2;
    res->unc_tipo = obj->tipo;
    res->compression_level = (uns8b)( ( lv < 10 ) ? lv : 9 );
    res->unc_len = slen;
    return (trp_obj_t *)res;
}

trp_obj_t *trp_uncompress( trp_obj_t *obj )
{
    unsigned long slen, dlen;
    trp_raw_t *raw;
    trp_obj_t *res;
    uns8b *buf, unc_tipo_is_raw;

    if ( obj->tipo != TRP_RAW )
        return UNDEF;
    if ( ((trp_raw_t *)obj)->mode == 0 )
        return UNDEF;

    unc_tipo_is_raw = ( ((trp_raw_t *)obj)->unc_tipo == TRP_RAW ) ? 1 : 0;

    if ( ((trp_raw_t *)obj)->compression_level == 0 ) {
        if ( unc_tipo_is_raw ) {
            raw = (trp_raw_t *)trp_raw_internal( ((trp_raw_t *)obj)->len, 0 );
            memcpy( raw->data, ((trp_raw_t *)obj)->data, ((trp_raw_t *)obj)->len );
            return (trp_obj_t *)raw;
        }
        buf = ((trp_raw_t *)obj)->data;
        return trp_decode_internal( &buf );
    }
    slen = ((trp_raw_t *)obj)->len;
    dlen = ((trp_raw_t *)obj)->unc_len;
    raw = (trp_raw_t *)trp_raw_internal( dlen, unc_tipo_is_raw ? 0 : 1 );
    if ( uncompress( raw->data, &dlen, ((trp_raw_t *)obj)->data, slen ) != Z_OK ) {
        if ( unc_tipo_is_raw )
            trp_gc_free( raw->data );
        else
            free( raw->data );
        trp_gc_free( raw );
        return UNDEF;
    }

    if ( unc_tipo_is_raw )
        return (trp_obj_t *)raw;

    buf = raw->data;
    res = trp_decode_internal( &buf );
    free( raw->data );
    trp_gc_free( raw );
    return res;
}

trp_obj_t *trp_raw_read( trp_obj_t *raw, trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b c;

    if ( ( fp = trp_file_readable_fp( stream ) ) == NULL )
        return UNDEF;
    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( cnt ) {
        uns64b cc;

        if (  trp_cast_sig64b_range( cnt, &cc, 0, 0x7fffffffffffffff ) )
            return UNDEF;
        if ( cc > ((trp_raw_t *)raw)->len )
            c = ((trp_raw_t *)raw)->len;
        else
            c = (uns32b)cc;
    } else {
        c = ((trp_raw_t *)raw)->len;
    }
    if ( c == 0 )
        return ZERO;
    ((trp_file_t *)stream)->last = 0;
    ((trp_file_t *)stream)->line = 0xffffffff;
    ((trp_raw_t *)raw)->mode = 0;
    ((trp_raw_t *)raw)->unc_tipo = 0;
    ((trp_raw_t *)raw)->compression_level = 0;
    ((trp_raw_t *)raw)->unc_len = 0;
    return trp_sig64( trp_file_read_chars( fp, ((trp_raw_t *)raw)->data, c ) );
}

trp_obj_t *trp_raw_write( trp_obj_t *raw, trp_obj_t *stream, trp_obj_t *cnt )
{
    FILE *fp;
    uns32b c;

    if ( ( fp = trp_file_writable_fp( stream ) ) == NULL )
        return UNDEF;
    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( cnt ) {
        uns64b cc;

        if (  trp_cast_sig64b_range( cnt, &cc, 0, 0x7fffffffffffffff ) )
            return UNDEF;
        if ( cc > ((trp_raw_t *)raw)->len )
            c = ((trp_raw_t *)raw)->len;
        else
            c = (uns32b)cc;
    } else {
        c = ((trp_raw_t *)raw)->len;
    }
    if ( c == 0 )
        return ZERO;
    return trp_sig64( trp_file_write_chars( fp, ((trp_raw_t *)raw)->data, c ) );
}

trp_obj_t *trp_raw2str( trp_obj_t *raw, trp_obj_t *cnt )
{
    register int c;
    uns32b i, len;
    uns8b *p;
    CORD_ec x;

    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( cnt ) {
        if ( trp_cast_uns32b( cnt, &len ) )
            return UNDEF;
        if ( len > ((trp_raw_t *)raw)->len )
            len = ((trp_raw_t *)raw)->len;
    } else
        len = ((trp_raw_t *)raw)->len;
    p = ((trp_raw_t *)raw)->data;
    CORD_ec_init( x );
    for( i = 0 ; i < len ; i++ ) {
        c = *p++;
        if ( c == 0 ) {
            register size_t count = 1;

            CORD_ec_flush_buf( x );
            for ( i++ ; i < len ; i++ ) {
                c = *p++;
                if ( c )
                    break;
                count++;
            }
            x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( count ) );
            if ( i == len )
                break;
        }
        CORD_ec_append( x, c );
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_raw_load( trp_obj_t *path )
{
    trp_obj_t *res;
    uns8b *cpath = trp_csprint( path );
    FILE *fp;
    sig64b i, len;

    fp = trp_fopen( cpath, "rb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return UNDEF;
    if ( fseeko( fp, 0, SEEK_END ) ) {
        (void)fclose( fp );
        return UNDEF;
    }
    len = (sig64b)ftello( fp );
    fseeko( fp, 0, SEEK_SET );
    if ( ( len < 0 ) || ( len > 0xffffffff ) ) {
        (void)fclose( fp );
        return UNDEF;
    }
    res = trp_raw_internal( (uns32b)len, 0 );
    if ( trp_file_read_chars( fp, ((trp_raw_t *)res)->data, (uns32b)len ) != (uns32b)len ) {
        trp_gc_free( ((trp_raw_t *)res)->data );
        trp_gc_free( res );
        res = UNDEF;
    }
    (void)fclose( fp );
    return res;
}

trp_obj_t *trp_raw_cmp( trp_obj_t *raw1, trp_obj_t *raw2, trp_obj_t *cnt )
{
    uns32b l, m;
    sig64b res;
    uns64b cx, *p1x, *p2x;
    uns8b c, *p1, *p2;

    if ( ( raw1->tipo != TRP_RAW ) || ( raw2->tipo != TRP_RAW ) )
        return UNDEF;
    if ( cnt ) {
        if ( trp_cast_uns32b( cnt, &l ) )
            return UNDEF;
        if ( ( l > ((trp_raw_t *)raw1)->len ) || ( l > ((trp_raw_t *)raw2)->len ) )
            return UNDEF;
    } else
        l = TRP_MIN( ((trp_raw_t *)raw1)->len, ((trp_raw_t *)raw2)->len );
    res = 0;
    p1x = (uns64b *)(((trp_raw_t *)raw1)->data);
    p2x = (uns64b *)(((trp_raw_t *)raw2)->data);
    for ( m = l >> 3 ; m ; m-- )
        for ( cx = (*p1x++) ^ (*p2x++) ; cx ; cx &= ( cx - 1 ) )
            res++;
    p1 = (uns8b *)p1x;
    p2 = (uns8b *)p2x;
    for ( m = l & 7 ; m ; m-- )
        for ( c = (*p1++) ^ (*p2++) ; c ; c &= ( c - 1 ) )
            res++;
    return trp_sig64( res );
}

uns8b trp_raw_swap( trp_obj_t *raw )
{
    uns32b l;
    uns8b *p, c;

    if ( raw->tipo != TRP_RAW )
        return 1;
    l = ((trp_raw_t *)raw)->len;
    if ( l & 1 )
        return 1;
    p = (uns8b *)(((trp_raw_t *)raw)->data);
    while ( l ) {
        c = p[ 0 ];
        p[ 0 ] = p[ 1 ];
        p[ 1 ] = c;
        p += 2;
        l -= 2;
    }
    return 0;
}

uns8b trp_raw_set( trp_obj_t *raw, trp_obj_t *c )
{
    if ( ( raw->tipo != TRP_RAW ) || ( c->tipo != TRP_CHAR ) )
        return 1;
    memset( (void *)(((trp_raw_t *)raw)->data), (int)(((trp_char_t *)c)->c), (size_t)(((trp_raw_t *)raw)->len) );
    return 0;
}

