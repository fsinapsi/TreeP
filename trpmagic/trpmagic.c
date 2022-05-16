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

#include "../trp/trp.h"
#include "./trpmagic.h"
#include <magic.h>

#define trp_magic_lock() (void)pthread_mutex_lock( &_trp_magic_mutex )
#define trp_magic_unlock() (void)pthread_mutex_unlock( &_trp_magic_mutex )
static pthread_mutex_t _trp_magic_mutex = PTHREAD_MUTEX_INITIALIZER;
static magic_t _trp_magic;

static trp_obj_t *trp_magic_internal_unlock( const uns8b *m );
static trp_obj_t *trp_magic_hack1( const uns8b *m, uns32b sl );
static trp_obj_t *trp_magic_hack2( const uns8b *m, uns32b sl );

uns8b trp_magic_init()
{
    if ( _trp_magic = magic_open( MAGIC_CONTINUE ) )
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
    if ( _trp_magic == NULL ) {
        uns8b *s = trp_csprint( path );

        if ( _trp_magic = magic_open( MAGIC_CONTINUE ) )
            if ( magic_load( _trp_magic, s ) ) {
                magic_close( _trp_magic );
                _trp_magic = NULL;
            }
        trp_csprint_free( s );
    }
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

static trp_obj_t *trp_magic_internal_unlock( const uns8b *m )
{
    trp_obj_t *res;
    uns8b *r;
    uns32b l;

    if ( ( m == NULL ) || ( strcmp( m, "data" ) == 0 ) ) {
        trp_magic_unlock();
        return NULL;
    }
    l = strlen( m );
    r = trp_gc_malloc( l + 1 );
    memcpy( r, m, l );
    trp_magic_unlock();
    for ( ; ; ) {
        if ( l == 0 ) {
            res = NULL;
            break;
        }
        l--;
        if ( ( r[ l ] != ' ' ) &&
             ( r[ l ] != '\t' ) &&
             ( r[ l ] != '\r' ) &&
             ( r[ l ] != '\n' ) ) {
            r[ l + 1 ] = '\0';
            res = trp_cord( r );
            break;
        }
    }
    trp_gc_free( r );
    return res;
}

static trp_obj_t *trp_magic_hack1( const uns8b *m, uns32b sl )
{
    if ( sl < 10 )
        return UNDEF;
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
        return trp_cord( "JPEG image data" );
    return NULL;
}

static trp_obj_t *trp_magic_hack2( const uns8b *m, uns32b sl )
{
    if ( sl < 80 )
        return UNDEF;
    if ( m[ 66 ] == '%' &&
         m[ 67 ] == 'P' &&
         m[ 68 ] == 'D' &&
         m[ 69 ] == 'F' &&
         m[ 70 ] == '-' &&
         m[ 71 ] == '1' &&
         m[ 72 ] == '.' &&
         m[ 73 ] == '4' )
        return trp_cord( "PDF document, version 1.4" );
    return NULL;
}

trp_obj_t *trp_magic_file( trp_obj_t *path )
{
    uns8b *s;
    trp_obj_t *res = UNDEF;
    uns32b si;

    trp_magic_lock();
    if ( _trp_magic == NULL ) {
        trp_magic_unlock();
        return UNDEF;
    }
    s = trp_csprint( path );
    si = strlen( s );
    if ( si >= 4 ) {
        if ( ( strcmp( s + si - 4, ".trp" ) == 0 ) ||
             ( strcmp( s + si - 4, ".tin" ) == 0 ) )
            res = trp_cord( "TreeP source code text" );
        if ( strcmp( s + si - 4, ".lyx" ) == 0 )
            res = trp_cord( "LyX source code text" );
    }
    if ( res == UNDEF ) {
        FILE *fp = trp_fopen( s, "rb" );

        if ( fp ) {
            res = trp_magic_internal_unlock( magic_descriptor( _trp_magic, fileno( fp ) ) );
            if ( res == NULL ) {
                uns32b sl;
                uns8b m[ 80 ];

                fseek( fp, 0, SEEK_SET );
                sl = fread( m, 1, 80, fp );
                if ( ( res = trp_magic_hack1( m, sl ) ) == NULL )
                    if ( ( res = trp_magic_hack2( m, sl ) ) == NULL ) {
                        if ( si >= 3 )
                            if ( strcmp( s + si - 3, ".br" ) == 0 )
                                res = trp_cord( "brotli compressed data" );
                        if ( res == NULL )
                            res = trp_cord( "data" );
                    }
            }
            fclose( fp );
        } else
            trp_magic_unlock();
    } else
        trp_magic_unlock();
    trp_csprint_free( s );
    return res;
}

trp_obj_t *trp_magic_buffer( trp_obj_t *raw, trp_obj_t *cnt )
{
    trp_obj_t *res;
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
    trp_magic_lock();
    if ( _trp_magic == NULL ) {
        trp_magic_unlock();
        return UNDEF;
    }
    res = trp_magic_internal_unlock( magic_buffer( _trp_magic,
                                                   ((trp_raw_t *)raw)->data,
                                                   sl ) );
    if ( res == NULL )
        if ( ( res = trp_magic_hack1( ((trp_raw_t *)raw)->data, sl ) ) == NULL )
            if ( (res = trp_magic_hack2( ((trp_raw_t *)raw)->data, sl ) ) == NULL )
                res = trp_cord( "data" );
    return res;
}

