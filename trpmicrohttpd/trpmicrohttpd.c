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
#include "./trpmicrohttpd.h"
#include <fcntl.h>
#include <sys/stat.h>
#ifdef MINGW
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    union {
        struct MHD_Daemon *daemon;
        struct MHD_Connection *connection;
    };
} trp_mhd_t;

static uns8b trp_mhd_close( trp_mhd_t *obj );
static uns8b trp_mhd_close_basic( uns8b flags, trp_mhd_t *obj );
static void trp_mhd_finalize( void *obj, void *data );

uns8b trp_mhd_init()
{
    extern uns8bfun_t _trp_close_fun[];

    _trp_close_fun[ TRP_MHD ] = trp_mhd_close;
    return 0;
}

static uns8b trp_mhd_close( trp_mhd_t *obj )
{
    if ( obj->sottotipo )
        return 0;
    return trp_mhd_close_basic( 1, obj );
}

static uns8b trp_mhd_close_basic( uns8b flags, trp_mhd_t *obj )
{
    if ( obj->daemon ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        MHD_stop_daemon( obj->daemon );
        obj->daemon = NULL;
    }
    return 0;
}

static void trp_mhd_finalize( void *obj, void *data )
{
    trp_mhd_close_basic( 0, (trp_mhd_t *)obj );
}

trp_obj_t *trp_mhd_version()
{
    return trp_cord( MHD_get_version() );
}

trp_obj_t *trp_mhd_is_feature_supported( trp_obj_t *feat )
{
    uns32b ffeat;

    if ( trp_cast_uns32b( feat, &ffeat ) )
            return TRP_FALSE;
    return ( MHD_is_feature_supported( (enum MHD_FEATURE)ffeat ) == MHD_YES ) ? TRP_TRUE : TRP_FALSE;
}

static enum MHD_Result trp_mhd_cback( void *cls,
                                      struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **ptr )
{
    static int dummy;
    trp_mhd_t *conn;
    trp_obj_t *page;
    struct MHD_Response *response = NULL;
    enum MHD_Result ret;

    if ( strcmp( method, "GET" ) )
        return MHD_NO; /* unexpected method */
    if ( *ptr != &dummy ) {
        /*
         * The first time only the headers are valid,
         * do not respond in the first round...
         */
        *ptr = &dummy;
        return MHD_YES;
    }
    if ( *upload_data_size )
        return MHD_NO; /* upload data in a GET!? */
    *ptr = NULL; /* clear context pointer */

    conn = trp_gc_malloc_atomic( sizeof( trp_mhd_t ) );
    conn->tipo = TRP_MHD;
    conn->sottotipo = 1;
    conn->connection = connection;

    page = ( (objfun_t)cls )( (trp_obj_t *)conn, trp_cord( url ), trp_cord( method ), trp_cord( version ) );
    switch ( page->tipo ) {
        case TRP_CORD:
            response = MHD_create_response_from_buffer( ( (trp_cord_t *)page )->len,
                                                        (void *)( CORD_to_char_star( ( (trp_cord_t *)page )->c ) ),
                                                        MHD_RESPMEM_MUST_COPY );
            break;
        case TRP_RAW:
            response = MHD_create_response_from_buffer( ( (trp_raw_t *)page )->len,
                                                        ( (trp_raw_t *)page )->data,
                                                        MHD_RESPMEM_MUST_COPY );
            break;
        case TRP_CONS:
            page = trp_car( page );
            if ( page->tipo == TRP_CORD ) {
                uns8b *path = CORD_to_char_star( ( (trp_cord_t *)page )->c );
                int fd;
#ifdef MINGW
                wchar_t *wpath;

                wpath = trp_utf8_to_wc( path );
                fd = _wopen( wpath, _O_RDONLY );
                trp_gc_free( wpath );
                if ( fd >= 0 ) {
                    struct _stati64 st;

                    if ( _fstati64( fd, &st ) == 0 )
                        if ( ( st.st_mode & S_IFMT ) == S_IFREG )
                            response = MHD_create_response_from_fd64( st.st_size, fd );
                }
#else
                fd = open( path, O_RDONLY );
                if ( fd >= 0 ) {
                    struct stat st;

                    if ( fstat( fd, &st ) == 0 )
                        if ( ( st.st_mode & S_IFMT ) == S_IFREG )
                            response = MHD_create_response_from_fd64( st.st_size, fd );
                }
#endif
            }
            break;
        default:
            break;
    }
    if ( response == NULL )
        return MHD_NO;
    ret = MHD_queue_response( connection, MHD_HTTP_OK, response );
    MHD_destroy_response( response );
    return ret;
}

trp_obj_t *trp_mhd_start_daemon( trp_obj_t *cbfun, trp_obj_t *port )
{
    trp_mhd_t *obj;
    struct MHD_Daemon *daemon;
    uns32b pport;

    if ( cbfun->tipo != TRP_FUNPTR )
        return UNDEF;
    if ( ( (trp_funptr_t *)cbfun )->nargs != 4 )
        return UNDEF;
    if ( port ) {
        if ( trp_cast_uns32b( port, &pport ) )
            return UNDEF;
    } else
        pport = 8888;
    daemon = MHD_start_daemon( 0, /* MHD_USE_THREAD_PER_CONNECTION */
                               (uint16_t)pport,
                               NULL,
                               NULL,
                               &trp_mhd_cback,
                               (void *)( ( (trp_funptr_t *)cbfun )->f ),
                               MHD_OPTION_END );
    if ( daemon == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mhd_t ), trp_mhd_finalize );
    obj->tipo = TRP_MHD;
    obj->sottotipo = 0;
    obj->daemon = daemon;
    return (trp_obj_t *)obj;
}

uns8b trp_mhd_run_wait( trp_obj_t *obj, trp_obj_t *timeout )
{
    struct MHD_Daemon *daemon;
    sig32b ms;

    if ( obj->tipo != TRP_MHD )
        return 1;
    if ( ((trp_mhd_t *)obj)->sottotipo != 0 )
        return 1;
    daemon = ((trp_mhd_t *)obj)->daemon;
    if ( daemon == NULL )
        return 1;
    if ( timeout ) {
        if ( trp_cast_sig32b( trp_math_rint( trp_math_times( timeout, trp_sig64( 1000 ), NULL ) ), &ms ) )
            return 1;
        if ( ms < 0 )
            return 1;
    } else
        ms = -1;
    return ( MHD_run_wait( daemon, ms ) == MHD_YES ) ? 0 : 1;
}

uns8b trp_mhd_stop_daemon( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_MHD )
        return 1;
    return trp_mhd_close( (trp_mhd_t *)obj );
}

trp_obj_t *trp_mhd_get_connection_info_client_address( trp_obj_t *conn )
{
    struct sockaddr *client_addr;
    uns8b *bytes;
    uns8b buf[ 18 ];

    if ( conn->tipo != TRP_MHD )
        return UNDEF;
    if ( ((trp_mhd_t *)conn)->sottotipo != 1 )
        return UNDEF;
    if ( ((trp_mhd_t *)conn)->daemon == NULL )
        return UNDEF;
    client_addr = MHD_get_connection_info( ( (trp_mhd_t *)conn )->connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS )->client_addr;
    if ( client_addr == NULL )
        return UNDEF;
    bytes = (uns8b *)&( ( ( struct sockaddr_in *)client_addr )->sin_addr );
    snprintf( buf, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3] );
    return trp_cord( buf );
}

