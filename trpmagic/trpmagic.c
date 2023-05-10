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

#include "../trp/trp.h"
#include "./trpmagic.h"
#include <magic.h>

#define trp_magic_lock() (void)pthread_mutex_lock( &_trp_magic_mutex )
#define trp_magic_unlock() (void)pthread_mutex_unlock( &_trp_magic_mutex )
static pthread_mutex_t _trp_magic_mutex = PTHREAD_MUTEX_INITIALIZER;
static magic_t _trp_magic;

static trp_obj_t *trp_magic_internal( const uns8b *m );
static trp_obj_t *trp_magic_hack_pre( const uns8b *m, uns32b sl );
static trp_obj_t *trp_magic_hack_post( const uns8b *m, uns32b sl );

#define TRP_MAGIC_FLAGS ( MAGIC_COMPRESS_TRANSP | MAGIC_NO_CHECK_COMPRESS )

uns8b trp_magic_init()
{
    if ( _trp_magic = magic_open( TRP_MAGIC_FLAGS ) )
        if ( magic_load( _trp_magic, NULL ) )
            if ( magic_load( _trp_magic, "magic.mgc" ) ) {
                magic_close( _trp_magic );
                _trp_magic = NULL;
            }
    return 0;
}

void trp_magic_quit()
{
    trp_magic_lock();
    if ( _trp_magic ) {
        magic_close( _trp_magic );
        _trp_magic = NULL;
    }
    trp_magic_unlock();
}

uns8b trp_magic_reinit( trp_obj_t *path )
{
    trp_magic_lock();
    if ( _trp_magic == NULL ) {
        uns8b *s = trp_csprint( path );

        if ( _trp_magic = magic_open( TRP_MAGIC_FLAGS ) )
            if ( magic_load( _trp_magic, s ) ) {
                magic_close( _trp_magic );
                _trp_magic = NULL;
            }
        trp_csprint_free( s );
    }
    trp_magic_unlock();
    return _trp_magic ? 0 : 1;
}

trp_obj_t *trp_magic_available()
{
    trp_obj_t *res;

    trp_magic_lock();
    res = _trp_magic ? TRP_TRUE : TRP_FALSE;
    trp_magic_unlock();
    return res;
}

static trp_obj_t *trp_magic_internal( const uns8b *m )
{
    trp_obj_t *res = UNDEF;

    if ( m ) {
        uns8b *r;
        uns32b l;

        l = strlen( m );
        r = trp_gc_malloc( l + 1 );
        memcpy( r, m, l );
        while ( l ) {
            l--;
            if ( ( r[ l ] != ' ' ) &&
                ( r[ l ] != '\t' ) &&
                ( r[ l ] != '\r' ) &&
                ( r[ l ] != '\n' ) ) {
                r[ l + 1 ] = '\0';
                if ( strcmp( r, "data" ) )
                    res = trp_cord( r );
                break;
            }
        }
        trp_gc_free( r );
    }
    return res;
}

static trp_obj_t *trp_magic_hack_pre( const uns8b *m, uns32b sl )
{
    trp_obj_t *res = UNDEF;

    if ( sl >= 31 )
        if ( memcmp( m, "hOro1%lB@fmAnGkoQbaWgaSx35ZuXBC", 31 ) == 0 )
            res = trp_cord( "TreeP image data" );
    if ( sl >= 8 )
        if ( memcmp( m, "\227\112\102\062\015\012\032\012", 8 ) == 0 )
            res = trp_cord( "JBIG2 image data" );
    if ( sl >= 13 )
        if ( memcmp( m,
                     "\000\000\000\000\060\000\001\000\000\000\023\000\000\015",
                     13 ) == 0 )
            res = trp_cord( "JB2E image data" );
    if ( sl >= 7 )
        if ( memcmp( m, "%PDF-1.", 7 ) == 0 )
            res = trp_cord( "PDF document" );
    return res;
}

static trp_obj_t *trp_magic_hack_post( const uns8b *m, uns32b sl )
{
    trp_obj_t *res = UNDEF;

    if ( sl >= 74 )
        if ( memcmp( m + 66, "%PDF-1.4", 8 ) == 0 )
            res = trp_cord( "PDF document, version 1.4" );
    if ( sl >= 10 )
        if ( m[ 0 ] == 0xff &&
             m[ 1 ] == 0xd8 &&
             m[ 2 ] == 0xff &&
             ( ( m[ 3 ] == 0xe1 &&
                 m[ 4 ] == 0x82 &&
                 m[ 5 ] == 0x2e &&
                 m[ 6 ] == 0x45 &&
                 m[ 7 ] == 0x78 &&
                 m[ 8 ] == 0x69 &&
                 m[ 9 ] == 0x66 ) ||
               ( m[ 3 ] == 0xe0 &&
                 m[ 4 ] == 0x00 &&
                 m[ 5 ] == 0x10 &&
                 m[ 6 ] == 0x4a &&
                 m[ 7 ] == 0x46 &&
                 m[ 8 ] == 0x49 &&
                 m[ 9 ] == 0x46 ) ) )
            res = trp_cord( "JPEG image data" );
    return res;
}

trp_obj_t *trp_magic_file( trp_obj_t *path )
{
    trp_obj_t *res = UNDEF;
    FILE *fp;
    uns8b *s;
    uns32b si, sl;
    uns8b m[ 80 ];

    s = trp_csprint( path );
    si = strlen( s );
    if ( si >= 4 ) {
        if ( ( strcmp( s + si - 4, ".trp" ) == 0 ) ||
             ( strcmp( s + si - 4, ".tin" ) == 0 ) )
            res = trp_cord( "TreeP source code text" );
        if ( res != UNDEF ) {
            trp_csprint_free( s );
            return res;
        }
    }
    if ( ( fp = trp_fopen( s, "rb" ) ) == NULL ) {
        trp_csprint_free( s );
        return UNDEF;
    }
    sl = fread( m, 1, 80, fp );
    if ( ( res = trp_magic_hack_pre( m, sl ) ) != UNDEF ) {
        fclose( fp );
        trp_csprint_free( s );
        return res;
    }
    fseek( fp, 0, SEEK_SET );
    trp_magic_lock();
    if ( _trp_magic )
        res = trp_magic_internal( magic_descriptor( _trp_magic, fileno( fp ) ) );
    trp_magic_unlock();
    fclose( fp );
    if ( res == UNDEF ) {
        res = trp_magic_hack_post( m, sl );
        if ( res == UNDEF ) {
            if ( si >= 3 )
                if ( strcmp( s + si - 3, ".br" ) == 0 )
                    res = trp_cord( "brotli compressed data" );
            if ( res == UNDEF )
                res = trp_cord( "data" );
        }
    }
    trp_csprint_free( s );
    return res;
}

trp_obj_t *trp_magic_buffer( trp_obj_t *raw, trp_obj_t *cnt )
{
    trp_obj_t *res = UNDEF;
    uns8b *m;
    uns32b sl;

    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( cnt ) {
        if (  trp_cast_uns32b( cnt, &sl ) )
            return UNDEF;
        if ( sl > ((trp_raw_t *)raw)->len )
            sl = ((trp_raw_t *)raw)->len;
    } else
        sl = ((trp_raw_t *)raw)->len;
    if ( sl == 0 )
        return UNDEF;
    m = ((trp_raw_t *)raw)->data;
    if ( ( res = trp_magic_hack_pre( m, sl ) ) != UNDEF )
        return res;
    trp_magic_lock();
    if ( _trp_magic )
        res = trp_magic_internal( magic_buffer( _trp_magic, m, sl ) );
    trp_magic_unlock();
    if ( res == UNDEF ) {
        res = trp_magic_hack_post( m, sl );
        if ( res == UNDEF )
            res = trp_cord( "data" );
    }
    return res;
}

