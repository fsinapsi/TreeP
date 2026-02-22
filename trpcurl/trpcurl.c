/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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
#include "./trpcurl.h"

#define TRP_CURL_STRINGS_FIELDNO 10

typedef union {
    uns8b *str[ TRP_CURL_STRINGS_FIELDNO ];
    struct {
        uns8b *errorbuffer;
        uns8b *url;
        uns8b *postfields;
        uns8b *proxy;
        uns8b *pre_proxy;
        uns8b *sslcert;
        uns8b *ssl_cipher_list;
        uns8b *cainfo;
        uns8b *ftpport;
        uns8b *nterface;
    } name;
} trp_curl_strings;

typedef struct {
    uns8b tipo;
    CURL *curl;
    trp_curl_strings s;
    struct curl_slist *quote;
    struct curl_slist *postquote;
    struct curl_slist *prequote;
    struct curl_slist *httpheader;
    trp_obj_t *transfer_cback_net;
    trp_obj_t *transfer_cback_progress_net;
    trp_obj_t *transfer_raw;
    trp_obj_t *transfer_data;
    trp_obj_t *transfer_progress_data;
    curl_off_t transfer_rem;
} trp_curl_t;

#define trp_curl_strings(c) (((trp_curl_t *)(c))->s.name)

static uns8b trp_curl_print( trp_print_t *p, trp_curl_t *obj );
static uns8b trp_curl_close( trp_curl_t *obj );
static uns8b trp_curl_close_basic( uns8b flags, trp_curl_t *obj );
static void trp_curl_finalize( void *obj, void *data );
static CURL *trp_curl_get( trp_obj_t *curl );
static uns8b trp_curl_easy_setopt_copied_string_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt );
static uns8b trp_curl_easy_setopt_strings_internal( CURL *c, trp_obj_t *val, CURLoption opt, uns8b **p );
static uns8b trp_curl_easy_setopt_list_internal( CURL *c, va_list args, CURLoption opt, struct curl_slist **l );
static uns8b trp_curl_easy_setopt_boolean_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt );
static uns8b trp_curl_easy_setopt_curloff_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt );
static uns8b trp_curl_easy_setopt_long_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt );
static size_t trp_curl_cback_receive( void *p, size_t size, size_t nmemb, void *curl );
static size_t trp_curl_cback_send( void *p, size_t size, size_t nmemb, void *curl );
static int trp_curl_cback_progress( void *curl, sig64b dltotal, sig64b dlnow, sig64b ultotal, sig64b ulnow );

uns8b trp_curl_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];

    if ( curl_global_init( CURL_GLOBAL_ALL ) ) {
        fprintf( stderr, "Initialization of libcurl failed\n" );
        return 1;
    }
    _trp_print_fun[ TRP_CURL ] = trp_curl_print;
    _trp_close_fun[ TRP_CURL ] = trp_curl_close;
    return 0;
}

void trp_curl_quit()
{
    curl_global_cleanup();
}

static uns8b trp_curl_print( trp_print_t *p, trp_curl_t *obj )
{
    if ( trp_print_char_star( p, "#curl" ) )
        return 1;
    if ( obj->curl == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_curl_close( trp_curl_t *obj )
{
    return trp_curl_close_basic( 1, obj );
}

static uns8b trp_curl_close_basic( uns8b flags, trp_curl_t *obj )
{
    if ( obj->curl ) {
        int i;

        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        curl_easy_cleanup( obj->curl );
        obj->curl = NULL;
        for ( i = 0 ; i < TRP_CURL_STRINGS_FIELDNO ; i++ )
            if ( obj->s.str[ i ] ) {
                trp_csprint_free( obj->s.str[ i ] );
                obj->s.str[ i ] = NULL;
            }
        if ( obj->quote )
            curl_slist_free_all( obj->quote );
        if ( obj->postquote )
            curl_slist_free_all( obj->postquote );
        if ( obj->prequote )
            curl_slist_free_all( obj->prequote );
        if ( obj->httpheader )
            curl_slist_free_all( obj->httpheader );
        obj->transfer_cback_net = NULL;
        obj->transfer_cback_progress_net = NULL;
        obj->transfer_raw = NULL;
        obj->transfer_data = NULL;
        obj->transfer_progress_data = NULL;
        obj->transfer_rem = 0;
    }
    return 0;
}

static void trp_curl_finalize( void *obj, void *data )
{
    trp_curl_close_basic( 0, (trp_curl_t *)obj );
}

trp_obj_t *trp_curl_version()
{
    return trp_cord( curl_version() );
}

trp_obj_t *trp_curl_easy_init()
{
    CURL *c = curl_easy_init();
    trp_curl_t *obj;

    if ( c == NULL )
        return UNDEF;
    obj = trp_gc_malloc_finalize( sizeof( trp_curl_t ), trp_curl_finalize );
    memset( obj, 0, sizeof( trp_curl_t ) );
    obj->tipo = TRP_CURL;
    obj->curl = c;
    return (trp_obj_t *)obj;
}

static CURL *trp_curl_get( trp_obj_t *curl )
{
    if ( curl->tipo != TRP_CURL )
        return NULL;
    return ((trp_curl_t *)curl)->curl;
}

static uns8b trp_curl_easy_setopt_copied_string_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt )
{
    CURL *c = trp_curl_get( curl );
    uns8b *p, res;

    if ( c == NULL )
        return 1;
    if ( val == NULL )
        return curl_easy_setopt( c, opt, NULL ) ? 1 : 0;
    p = trp_csprint( val );
    res = curl_easy_setopt( c, opt, p ) ? 1 : 0;
    trp_csprint_free( p );
    return res;
}

static uns8b trp_curl_easy_setopt_strings_internal( CURL *c, trp_obj_t *val, CURLoption opt, uns8b **p )
{
    if ( val == NULL ) {
        if ( curl_easy_setopt( c, opt, NULL ) )
            return 1;
        if ( *p ) {
            trp_csprint_free( *p );
            *p = NULL;
        }
        return 0;
    }
    if ( *p )
        trp_csprint_free( *p );
    *p = trp_csprint( val );
    if ( curl_easy_setopt( c, opt, *p ) ) {
        curl_easy_setopt( c, opt, NULL );
        trp_csprint_free( *p );
        *p = NULL;
        return 1;
    }
    return 0;
}

static uns8b trp_curl_easy_setopt_list_internal( CURL *c, va_list args, CURLoption opt, struct curl_slist **l )
{
    trp_obj_t *obj = va_arg( args, trp_obj_t * );
    uns8b *p;

    if ( obj == NULL ) {
        if ( curl_easy_setopt( c, opt, NULL ) )
            return 1;
        if ( *l ) {
            curl_slist_free_all( *l );
            *l = NULL;
        }
        return 0;
    }
    if ( *l ) {
        curl_slist_free_all( *l );
        *l = NULL;
    }
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) ) {
        p = trp_csprint( obj );
        *l = curl_slist_append( *l, p );
        trp_csprint_free( p );
    }
    if ( curl_easy_setopt( c, opt, *l ) ) {
        curl_easy_setopt( c, opt, NULL );
        curl_slist_free_all( *l );
        *l = NULL;
        return 1;
    }
    return 0;
}

static uns8b trp_curl_easy_setopt_boolean_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt )
{
    CURL *c = trp_curl_get( curl );

    if ( ( c == NULL ) ||
         ( !TRP_BOOLP( val ) ) )
        return 1;
    return curl_easy_setopt( c, opt, (long)( ( val == TRP_TRUE ) ? 1 : 0 ) ) ? 1 : 0;
}

static uns8b trp_curl_easy_setopt_curloff_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt )
{
    CURL *c = trp_curl_get( curl );

    if ( ( c == NULL ) ||
         ( val->tipo != TRP_SIG64 ) )
        return 1;
    return curl_easy_setopt( c, opt, (curl_off_t)( ((trp_sig64_t *)val)->val ) ) ? 1 : 0;
}

static uns8b trp_curl_easy_setopt_long_internal( trp_obj_t *curl, trp_obj_t *val, CURLoption opt )
{
    CURL *c = trp_curl_get( curl );

    if ( ( c == NULL ) ||
         ( val->tipo != TRP_SIG64 ) )
        return 1;
    return curl_easy_setopt( c, opt, (long)( ((trp_sig64_t *)val)->val ) ) ? 1 : 0;
}

trp_obj_t *trp_curl_easy_escape( trp_obj_t *curl, trp_obj_t *url )
{
    CURL *c = trp_curl_get( curl );
    uns8b *p, *q;

    if ( ( c == NULL ) ||
         ( url->tipo != TRP_CORD ) )
        return UNDEF;
    p = trp_csprint( url );
    q = curl_easy_escape( c, p, ((trp_cord_t *)url)->len );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    url = trp_cord( q );
    curl_free( q );
    return url;
}

trp_obj_t *trp_curl_easy_unescape( trp_obj_t *curl, trp_obj_t *url )
{
    CURL *c = trp_curl_get( curl );
    int len, i;
    uns8b *p, *q;
    CORD_ec x;

    if ( ( c == NULL ) ||
         ( url->tipo != TRP_CORD ) )
        return UNDEF;
    p = trp_csprint( url );
    q = curl_easy_unescape( c, p, ((trp_cord_t *)url)->len, &len );
    trp_csprint_free( p );
    if ( q == NULL )
        return UNDEF;
    CORD_ec_init( x );
    for ( i = 0 ; i < len ; i++ )
        if ( q[ i ] ) {
            CORD_ec_append( x, q[ i ] );
        } else {
            CORD_ec_flush_buf( x );
            x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( 1 ) );
        }
    curl_free( q );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), (uns32b)len );
}

uns8b trp_curl_easy_reset( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    curl_easy_reset( c );
    return 0;
}

uns8b trp_curl_easy_perform( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return curl_easy_perform( c ) ? 1 : 0;
}

trp_obj_t *trp_curl_easy_getinfo_errorbuffer( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    int i;

    if ( c == NULL )
        return UNDEF;
    if ( trp_curl_strings(curl).errorbuffer == NULL )
        return UNDEF;
    for ( i = CURL_ERROR_SIZE - 1 ; i ; ) {
        i--;
        if ( ( trp_curl_strings(curl).errorbuffer[ i ] != ' ' ) &&
             ( trp_curl_strings(curl).errorbuffer[ i ] != '\r' ) &&
             ( trp_curl_strings(curl).errorbuffer[ i ] != '\n' ) &&
             ( trp_curl_strings(curl).errorbuffer[ i ] != '\t' ) ) {
            i++;
            break;
        }
    }
    if ( i ) {
        trp_curl_strings(curl).errorbuffer[ i ] = 0;
        return trp_cord( trp_curl_strings(curl).errorbuffer );
    }
    return EMPTYCORD;
}

trp_obj_t *trp_curl_easy_getinfo_effective_url( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    trp_obj_t *res = UNDEF;
    uns8b *p;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_EFFECTIVE_URL, (char **)( &p ) ) != CURLE_OK )
        return UNDEF;
    if ( p )
        if ( p[ 0 ] )
            res = trp_cord( p );
    return res;
}

trp_obj_t *trp_curl_easy_getinfo_response_code( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    long res;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_RESPONSE_CODE, &res ) != CURLE_OK )
        return UNDEF;
    return trp_sig64( res );
}

trp_obj_t *trp_curl_easy_getinfo_http_connectcode( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    long res;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_HTTP_CONNECTCODE, &res ) != CURLE_OK )
        return UNDEF;
    return trp_sig64( res );
}

trp_obj_t *trp_curl_easy_getinfo_filetime( trp_obj_t *curl )
{
    extern trp_obj_t *trp_date_19700101();
    CURL *c = trp_curl_get( curl );
    long res;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_FILETIME, &res ) != CURLE_OK )
        return UNDEF;
    if ( res == -1 )
        return UNDEF;
    return trp_date_change_timezone( trp_cat( trp_date_19700101(),
                                              trp_sig64( res ),
                                              NULL ),
                                     trp_date_timezone( NULL ) );
}

trp_obj_t *trp_curl_easy_getinfo_size_download( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    sig64b res;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_SIZE_DOWNLOAD_T, &res ) != CURLE_OK )
        return UNDEF;
    return trp_sig64( res );
}

trp_obj_t *trp_curl_easy_getinfo_size_upload( trp_obj_t *curl )
{
    CURL *c = trp_curl_get( curl );
    sig64b res;

    if ( c == NULL )
        return UNDEF;
    if ( curl_easy_getinfo( c, CURLINFO_SIZE_UPLOAD_T, &res ) != CURLE_OK )
        return UNDEF;
    return trp_sig64( res );
}

uns8b trp_curl_easy_setopt_errorbuffer( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    if ( set_on_off == TRP_TRUE ) {
        if ( trp_curl_strings(curl).errorbuffer == NULL ) {
            trp_curl_strings(curl).errorbuffer = trp_gc_malloc_atomic( CURL_ERROR_SIZE );
            memset( trp_curl_strings(curl).errorbuffer, 0, CURL_ERROR_SIZE );
        }
    } else if ( set_on_off == TRP_FALSE ) {
        trp_gc_free( trp_curl_strings(curl).errorbuffer );
        trp_curl_strings(curl).errorbuffer = NULL;
    } else
        return 1;
    return curl_easy_setopt( c, CURLOPT_ERRORBUFFER,
                             (char *)( trp_curl_strings(curl).errorbuffer ) ) ? 1 : 0;
}

uns8b trp_curl_easy_setopt_url( trp_obj_t *curl, trp_obj_t *url )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, url, CURLOPT_URL,
                                                  &( trp_curl_strings(curl).url ) );
}

uns8b trp_curl_easy_setopt_postfields( trp_obj_t *curl, trp_obj_t *s )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, s, CURLOPT_POSTFIELDS,
                                                  &( trp_curl_strings(curl).postfields ) );
}

uns8b trp_curl_easy_setopt_postfieldsize_large( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_POSTFIELDSIZE_LARGE );
}

static size_t trp_curl_cback_receive( void *p, size_t size, size_t nmemb, void *curl )
{
    uns8bfun_t f = ((trp_netptr_t *)( ((trp_curl_t *)curl)->transfer_cback_net ))->f;
    trp_raw_t *r = (trp_raw_t *)( ((trp_curl_t *)curl)->transfer_raw );
    size_t len = size * nmemb;
    uns8b res;

    r->len = len;
    r->data = (uns8b *)p;
    if ( ((trp_curl_t *)curl)->transfer_data )
        res = (f)( r, ((trp_curl_t *)curl)->transfer_data );
    else
        res = (f)( r );
    return res ? -1 : len;
}

uns8b trp_curl_easy_setopt_writefunction( trp_obj_t *curl, trp_obj_t *net, trp_obj_t *data )
{
    CURL *c = trp_curl_get( curl );

    if ( ( c == NULL ) ||
         ( net->tipo != TRP_NETPTR ) )
        return 1;
    if ( ( ( data == NULL ) && ( ((trp_netptr_t *)net)->nargs != 1 ) ) ||
         ( ( data != NULL ) && ( ((trp_netptr_t *)net)->nargs != 2 ) ) )
        return 1;
    ((trp_curl_t *)curl)->transfer_cback_net = net;
    if ( ((trp_curl_t *)curl)->transfer_raw == NULL )
        ((trp_curl_t *)curl)->transfer_raw = trp_raw( ZERO );
    ((trp_curl_t *)curl)->transfer_data = data;
    ((trp_curl_t *)curl)->transfer_rem = -1;
    return ( /* curl_easy_setopt( c, CURLOPT_UPLOAD, (long)0 ) || può dare fastidio! */
             curl_easy_setopt( c, CURLOPT_WRITEDATA, curl ) ||
             curl_easy_setopt( c, CURLOPT_WRITEFUNCTION, trp_curl_cback_receive ) ) ? 1 : 0;
}

static size_t trp_curl_cback_send( void *p, size_t size, size_t nmemb, void *curl )
{
    uns8bfun_t f = ((trp_netptr_t *)( ((trp_curl_t *)curl)->transfer_cback_net ))->f;
    trp_raw_t *r = (trp_raw_t *)( ((trp_curl_t *)curl)->transfer_raw );
    curl_off_t rem = ((trp_curl_t *)curl)->transfer_rem;
    size_t len = size * nmemb;
    uns8b res;

    if ( len > rem ) {
        len = rem;
        if ( len == 0 )
            return 0;
    }
    r->len = len;
    r->data = (uns8b *)p;
    if ( ((trp_curl_t *)curl)->transfer_data )
        res = (f)( r, ((trp_curl_t *)curl)->transfer_data );
    else
        res = (f)( r );
    ((trp_curl_t *)curl)->transfer_rem = rem - len;
    return res ? CURL_READFUNC_ABORT : len;
}

uns8b trp_curl_easy_setopt_readfunction( trp_obj_t *curl, trp_obj_t *len, trp_obj_t *net, trp_obj_t *data )
{
    CURL *c = trp_curl_get( curl );
    curl_off_t infile_size;

    if ( ( c == NULL ) ||
         ( len->tipo != TRP_SIG64 ) ||
         ( net->tipo != TRP_NETPTR ) )
        return 1;
    infile_size = (curl_off_t)( ((trp_sig64_t *)len)->val );
    if ( ( infile_size < 0 ) ||
         ( ( data == NULL ) && ( ((trp_netptr_t *)net)->nargs != 1 ) ) ||
         ( ( data != NULL ) && ( ((trp_netptr_t *)net)->nargs != 2 ) ) )
        return 1;
    ((trp_curl_t *)curl)->transfer_cback_net = net;
    if ( ((trp_curl_t *)curl)->transfer_raw == NULL )
        ((trp_curl_t *)curl)->transfer_raw = trp_raw( ZERO );
    ((trp_curl_t *)curl)->transfer_data = data;
    ((trp_curl_t *)curl)->transfer_rem = infile_size;
    return ( curl_easy_setopt( c, CURLOPT_UPLOAD, (long)1 ) ||
             curl_easy_setopt( c, CURLOPT_INFILESIZE_LARGE, infile_size ) ||
             curl_easy_setopt( c, CURLOPT_READDATA, curl ) ||
             curl_easy_setopt( c, CURLOPT_READFUNCTION, trp_curl_cback_send ) ) ? 1 : 0;
}

static int trp_curl_cback_progress( void *curl, sig64b dltotal, sig64b dlnow, sig64b ultotal, sig64b ulnow )
{
    uns8bfun_t f = ((trp_netptr_t *)( ((trp_curl_t *)curl)->transfer_cback_progress_net ))->f;
    sig64b total, now;
    uns8b res;

    if ( ((trp_curl_t *)curl)->transfer_rem == -1 ) {
        total = dltotal;
        now = dlnow;
    } else {
        total = ultotal;
        now = ulnow;
    }
    if ( ( total < now ) ||
         ( total == 0 ) ||
         ( now < 0 ) )
        return 0;
    if ( ((trp_curl_t *)curl)->transfer_progress_data )
        res = (f)( trp_sig64( total), trp_sig64( now ),
                   ((trp_curl_t *)curl)->transfer_progress_data );
    else
        res = (f)( trp_sig64( total), trp_sig64( now ) );
    return res;
}

uns8b trp_curl_easy_setopt_progressfunction( trp_obj_t *curl, trp_obj_t *net, trp_obj_t *data )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    if ( net == UNDEF )
        return curl_easy_setopt( c, CURLOPT_NOPROGRESS, (long)1 ) ? 1 : 0;
    if ( net->tipo != TRP_NETPTR )
        return 1;
    if ( ( ( data == NULL ) && ( ((trp_netptr_t *)net)->nargs != 2 ) ) ||
         ( ( data != NULL ) && ( ((trp_netptr_t *)net)->nargs != 3 ) ) )
        return 1;
    ((trp_curl_t *)curl)->transfer_cback_progress_net = net;
    ((trp_curl_t *)curl)->transfer_progress_data = data;
    return ( curl_easy_setopt( c, CURLOPT_NOPROGRESS, (long)0 ) ||
             curl_easy_setopt( c, CURLOPT_PROGRESSDATA, curl ) ||
             curl_easy_setopt( c, CURLOPT_XFERINFOFUNCTION, trp_curl_cback_progress ) ) ? 1 : 0;
}

uns8b trp_curl_easy_setopt_filetime( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_FILETIME );
}

uns8b trp_curl_easy_setopt_proxy( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_PROXY,
                                                  &( trp_curl_strings(curl).proxy ) );
}

uns8b trp_curl_easy_setopt_pre_proxy( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_PRE_PROXY,
                                                  &( trp_curl_strings(curl).pre_proxy ) );
}

uns8b trp_curl_easy_setopt_proxyport( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_PROXYPORT );
}

uns8b trp_curl_easy_setopt_noproxy( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_NOPROXY );
}

uns8b trp_curl_easy_setopt_httpproxytunnel( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_HTTPPROXYTUNNEL );
}

uns8b trp_curl_easy_setopt_username( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_USERNAME );
}

uns8b trp_curl_easy_setopt_password( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_PASSWORD );
}

uns8b trp_curl_easy_setopt_proxyusername( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_PROXYUSERNAME );
}

uns8b trp_curl_easy_setopt_proxypassword( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_PROXYPASSWORD );
}

uns8b trp_curl_easy_setopt_autoreferer( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_AUTOREFERER );
}

uns8b trp_curl_easy_setopt_referer( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_REFERER );
}

uns8b trp_curl_easy_setopt_accept_encoding( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_ACCEPT_ENCODING );
}

uns8b trp_curl_easy_setopt_useragent( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_USERAGENT );
}

uns8b trp_curl_easy_setopt_resume_from( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_curloff_internal( curl, val, CURLOPT_RESUME_FROM_LARGE );
}

uns8b trp_curl_easy_setopt_httpget( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_HTTPGET );
}

uns8b trp_curl_easy_setopt_post( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_POST );
}

uns8b trp_curl_easy_setopt_followlocation( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_FOLLOWLOCATION );
}

uns8b trp_curl_easy_setopt_maxredirs( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_MAXREDIRS );
}

uns8b trp_curl_easy_setopt_crlf( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_CRLF );
}

uns8b trp_curl_easy_setopt_cookie( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_COOKIE );
}

uns8b trp_curl_easy_setopt_cookiefile( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_COOKIEFILE );
}

uns8b trp_curl_easy_setopt_cookiejar( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_COOKIEJAR );
}

uns8b trp_curl_easy_setopt_cookiesession( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_boolean_internal( curl, val, CURLOPT_COOKIESESSION );
}

uns8b trp_curl_easy_setopt_sslcert( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_SSLCERT,
                                                  &( trp_curl_strings(curl).sslcert ) );
}

uns8b trp_curl_easy_setopt_ssl_cipher_list( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_SSL_CIPHER_LIST,
                                                  &( trp_curl_strings(curl).ssl_cipher_list ) );
}

uns8b trp_curl_easy_setopt_keypasswd( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_KEYPASSWD );
}

uns8b trp_curl_easy_setopt_capath( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_CAPATH );
}

uns8b trp_curl_easy_setopt_ssl_verifypeer( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_SSL_VERIFYPEER );
}

uns8b trp_curl_easy_setopt_cainfo( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_CAINFO,
                                                  &( trp_curl_strings(curl).cainfo ) );
}

uns8b trp_curl_easy_setopt_ftpport( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_FTPPORT,
                                                  &( trp_curl_strings(curl).ftpport ) );
}

uns8b trp_curl_easy_setopt_interface( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );

    if ( c == NULL )
        return 1;
    return trp_curl_easy_setopt_strings_internal( c, val, CURLOPT_INTERFACE,
                                                  &( trp_curl_strings(curl).nterface ) );
}

uns8b trp_curl_easy_setopt_customrequest( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_CUSTOMREQUEST );
}

/*

uns8b trp_curl_easy_setopt_krblevel( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_KRBLEVEL );
}

*/

uns8b trp_curl_easy_setopt_nobody( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_NOBODY );
}

uns8b trp_curl_easy_setopt_quote( trp_obj_t *curl, ... )
{
    CURL *c = trp_curl_get( curl );
    va_list args;
    uns8b res;

    if ( c == NULL )
        return 1;
    va_start( args, curl );
    res = trp_curl_easy_setopt_list_internal( c, args, CURLOPT_QUOTE,
                                              &( ((trp_curl_t *)curl)->quote ) );
    va_end( args );
    return res;
}

uns8b trp_curl_easy_setopt_postquote( trp_obj_t *curl, ... )
{
    CURL *c = trp_curl_get( curl );
    va_list args;
    uns8b res;

    if ( c == NULL )
        return 1;
    va_start( args, curl );
    res = trp_curl_easy_setopt_list_internal( c, args, CURLOPT_POSTQUOTE,
                                              &( ((trp_curl_t *)curl)->postquote ) );
    va_end( args );
    return res;
}

uns8b trp_curl_easy_setopt_prequote( trp_obj_t *curl, ... )
{
    CURL *c = trp_curl_get( curl );
    va_list args;
    uns8b res;

    if ( c == NULL )
        return 1;
    va_start( args, curl );
    res = trp_curl_easy_setopt_list_internal( c, args, CURLOPT_PREQUOTE,
                                              &( ((trp_curl_t *)curl)->prequote ) );
    va_end( args );
    return res;
}

uns8b trp_curl_easy_setopt_httpheader( trp_obj_t *curl, ... )
{
    CURL *c = trp_curl_get( curl );
    va_list args;
    uns8b res;

    if ( c == NULL )
        return 1;
    va_start( args, curl );
    res = trp_curl_easy_setopt_list_internal( c, args, CURLOPT_HTTPHEADER,
                                              &( ((trp_curl_t *)curl)->httpheader ) );
    va_end( args );
    return res;
}

uns8b trp_curl_easy_setopt_header( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_HEADER );
}

uns8b trp_curl_easy_setopt_stderr( trp_obj_t *curl, trp_obj_t *val )
{
    CURL *c = trp_curl_get( curl );
    FILE *fp;

    if ( c == NULL )
        return 1;
    if ( val ) {
        fp = trp_file_writable_fp( val );
        if ( fp == NULL )
            return 1;
    } else
        fp = NULL;
    return curl_easy_setopt( c, CURLOPT_STDERR, fp ) ? 1 : 0;
}

uns8b trp_curl_easy_setopt_verbose( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_VERBOSE );
}

uns8b trp_curl_easy_setopt_failonerror( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_FAILONERROR );
}

uns8b trp_curl_easy_setopt_ignore_content_length( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_IGNORE_CONTENT_LENGTH );
}

uns8b trp_curl_easy_setopt_ssh_public_keyfile( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_SSH_PUBLIC_KEYFILE );
}

uns8b trp_curl_easy_setopt_ssh_private_keyfile( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_SSH_PRIVATE_KEYFILE );
}

uns8b trp_curl_easy_setopt_dirlistonly( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_DIRLISTONLY );
}

uns8b trp_curl_easy_setopt_append( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_APPEND );
}

uns8b trp_curl_easy_setopt_ftp_account( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_copied_string_internal( curl, val, CURLOPT_FTP_ACCOUNT );
}

uns8b trp_curl_easy_setopt_maxconnects( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_MAXCONNECTS );
}

uns8b trp_curl_easy_setopt_timeout_ms( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_TIMEOUT_MS );
}

uns8b trp_curl_easy_setopt_fresh_connect( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_FRESH_CONNECT );
}

uns8b trp_curl_easy_setopt_forbid_reuse( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_FORBID_REUSE );
}

uns8b trp_curl_easy_setopt_tcp_keepalive( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_TCP_KEEPALIVE );
}

uns8b trp_curl_easy_setopt_tcp_nodelay( trp_obj_t *curl, trp_obj_t *set_on_off )
{
    return trp_curl_easy_setopt_boolean_internal( curl, set_on_off, CURLOPT_TCP_NODELAY );
}

uns8b trp_curl_easy_setopt_http_version( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_HTTP_VERSION );
}

uns8b trp_curl_easy_setopt_expect_100_timeout_ms( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_EXPECT_100_TIMEOUT_MS  );
}

uns8b trp_curl_easy_setopt_use_ssl( trp_obj_t *curl, trp_obj_t *val )
{
    return trp_curl_easy_setopt_long_internal( curl, val, CURLOPT_USE_SSL );
}

uns8b trp_curl_easy_setopt_low_speed( trp_obj_t *curl, trp_obj_t *speed_limit, trp_obj_t *speed_time )
{
    CURL *c = trp_curl_get( curl );
    uns32b limit, time;

    if ( ( c == NULL ) ||
         trp_cast_uns32b( speed_limit, &limit ) ||
         trp_cast_uns32b( speed_time, &time ) )
        return 1;
    if ( curl_easy_setopt( c, CURLOPT_LOW_SPEED_LIMIT, (long)limit ) )
        return 1;
    return curl_easy_setopt( c, CURLOPT_LOW_SPEED_TIME, (long)time ) ? 1 : 0;
}

