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

#include "trp.h"
#ifdef MINGW
#include <io.h>
#include <windows.h>
#include <direct.h>
#include <fcntl.h>
#else
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <utime.h>
#include <locale.h>

#ifndef RUSAGE_SELF
#define RUSAGE_SELF 0 /* FIXME controllare che sia giusto */
#endif
#ifndef RUSAGE_CHILDREN
#define RUSAGE_CHILDREN -1 /* FIXME controllare che sia giusto */
#endif
#ifndef RUSAGE_THREAD
#define RUSAGE_THREAD 1 /* FIXME controllare che sia giusto */
#endif

#ifdef MINGW

void trp_convert_slash( uns8b *p )
{
    for ( ; *p ; p++ )
        if ( *p == '\\' )
            *p = '/';
}

wchar_t *trp_utf8_to_wc( const uns8b *p )
{
    wchar_t *wp;
    uns32b l;

    if ( ( l = MultiByteToWideChar( CP_UTF8, 0, p, -1, NULL, 0 ) ) == 0 )
        return NULL;
    wp = trp_gc_malloc_atomic( l * sizeof( wchar_t ) );
    if ( MultiByteToWideChar( CP_UTF8, 0, p, -1, wp, l ) == 0 ) {
        trp_gc_free( wp );
        return NULL;
    }
    return wp;
}

wchar_t *trp_utf8_to_wc_path( uns8b *cpath )
{
    uns32b l = strlen( cpath );

    while ( l > 1 ) {
        l--;
        if ( ( cpath[ l ] != '/' ) && ( cpath[ l ] != '\\' ) ) {
            cpath[ l + 1 ] = 0;
            break;
        }
    }
    return trp_utf8_to_wc( cpath );
}

uns8b *trp_wc_to_utf8( const wchar_t *wp )
{
    uns8b *p;
    uns32b l;

    if ( ( l = WideCharToMultiByte( CP_UTF8, 0, wp, -1, NULL, 0, NULL, NULL ) ) == 0 )
        return NULL;
    p = trp_gc_malloc_atomic( l );
    if ( WideCharToMultiByte( CP_UTF8, 0, wp, -1, p, l, NULL, NULL ) == 0 ) {
        trp_gc_free( p );
        return NULL;
    }
    return p;
}

FILE *trp_fopen( const char *path, const char *mode )
{
    wchar_t *wpath, *wmode;
    FILE *fp;

    if ( ( wpath = trp_utf8_to_wc( path ) ) == NULL )
        return NULL;
    if ( ( wmode = trp_utf8_to_wc( mode ) ) == NULL ) {
        trp_gc_free( wpath );
        return NULL;
    }
    fp = _wfopen( wpath, wmode );
    trp_gc_free( wpath );
    trp_gc_free( wmode );
    return fp;
}

int trp_open( const char *path, int oflag )
{
    wchar_t *wpath;
    int fd;

    if ( ( wpath = trp_utf8_to_wc( path ) ) == NULL )
        return -1;
    fd = _wopen( wpath, oflag | _O_BINARY );
    trp_gc_free( wpath );
    return fd;
}

uns8b *trp_get_short_path_name( uns8b *path )
{
    wchar_t *wp, *wq;
    uns8b *p;
    uns32b l;

    if ( ( wp = trp_utf8_to_wc_path( path ) ) == NULL )
        return path;
    if ( ( l = GetShortPathNameW( wp, NULL, 0 ) ) == 0 ) {
        trp_gc_free( wp );
        return path;
    }
    wq = trp_gc_malloc_atomic( l * sizeof( wchar_t ) );
    if ( GetShortPathNameW( wp, wq, l ) == 0 ) {
        trp_gc_free( wp );
        trp_gc_free( wq );
        return path;
    }
    trp_gc_free( wp );
    p = trp_wc_to_utf8( wq );
    trp_gc_free( wq );
    return p ? p : path;
}

#endif

trp_obj_t *trp_uname()
{
#ifdef MINGW
    return trp_cord( "MINGW64_NT" );
#else
    trp_obj_t *res;
    uns8b *buf;
    struct utsname name;

    if ( uname( &name ) )
        return UNDEF;
    buf = trp_gc_malloc_atomic( strlen( name.sysname ) + strlen( name.release ) +
                                strlen( name.version ) + strlen( name.machine ) +
                                6 );
    sprintf( buf, "%s %s %s (%s)", name.sysname, name.machine, name.release, name.version );
    res = trp_cord( buf );
    trp_gc_free( buf );
    return res;
#endif
}

#ifndef MINGW

static trp_obj_t *trp_timeval2seconds( struct timeval *tv )
{
    if ( tv->tv_usec < 0 )
        tv->tv_usec = 0;
    else if ( tv->tv_usec > 999999 )
        tv->tv_usec = 999999;
    return trp_cat( trp_sig64( tv->tv_sec ),
                    trp_math_ratio( trp_sig64( tv->tv_usec ),
                                    trp_sig64( 1000000 ),
                                    NULL ),
                    NULL );
}

static trp_obj_t *trp_getrusage_basic( int who )
{
    struct rusage usage;

    if ( getrusage( who, &usage ) )
        return UNDEF;
    return trp_list( trp_timeval2seconds( &( usage.ru_utime ) ),
                     trp_timeval2seconds( &( usage.ru_stime ) ),
                     NULL );
}

#else

static trp_obj_t *trp_getrusage_basic( int who )
{
    /*
     FIXME
     */
    return UNDEF;
}

#endif

trp_obj_t *trp_getrusage_self()
{
    return trp_getrusage_basic( RUSAGE_SELF );
}

trp_obj_t *trp_getrusage_children()
{
    return trp_getrusage_basic( RUSAGE_CHILDREN );
}

trp_obj_t *trp_getrusage_thread()
{
    return trp_getrusage_basic( RUSAGE_THREAD );
}

trp_obj_t *trp_getuid()
{
#ifdef MINGW
    return UNO;
#else
    return trp_sig64( (sig64b)getuid() );
#endif
}

trp_obj_t *trp_geteuid()
{
#ifdef MINGW
    return UNO;
#else
    return trp_sig64( (sig64b)geteuid() );
#endif
}

#ifndef MINGW

trp_obj_t *trp_realpath( trp_obj_t *obj )
{
    trp_obj_t *res = UNDEF;
    uns8b *cpath = trp_csprint( obj ), *p;

    if ( p = realpath( cpath, NULL ) ) {
        res = trp_cord( p );
        free( p );
    }
    trp_csprint_free( cpath );
    return res;
}

#else

trp_obj_t *trp_realpath( trp_obj_t *obj )
{
    trp_obj_t *res;
    uns8b *cpath = trp_csprint( obj );
    wchar_t *wp, *wq;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL )
        return UNDEF;
    wq = _wfullpath( NULL, wp, 1 );
    trp_gc_free( wp );
    if ( wq == NULL )
        return UNDEF;
    cpath = trp_wc_to_utf8( wq );
    free( wq );
    if ( cpath == NULL )
        return UNDEF;
    trp_convert_slash( cpath );
    res = trp_cord( cpath );
    trp_gc_free( cpath );
    return res;
}

#endif

#ifndef MINGW

trp_obj_t *trp_cwd()
{
    uns8b *p = getcwd( NULL, 0 );
    trp_obj_t *res;

    if ( p == NULL )
        return UNDEF;
    res = trp_cord( p );
    free( p );
    return res;
}

#else

trp_obj_t *trp_cwd()
{
    wchar_t *wp = _wgetcwd( NULL, 0 );
    uns8b *p;
    trp_obj_t *res;

    if ( wp == NULL )
        return UNDEF;
    p = trp_wc_to_utf8( wp );
    free( wp );
    trp_convert_slash( p );
    res = trp_cord( p );
    trp_gc_free( p );
    return res;
}

#endif

#ifndef MINGW

uns8b trp_chdir( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;

    res = chdir( cpath ) ? 1 : 0;
    trp_csprint_free( cpath );
    return res;
}

#else

uns8b trp_chdir( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;
    wchar_t *wp;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL )
        return 1;
    res = _wchdir( wp ) ? 1 : 0;
    trp_gc_free( wp );
    return res;
}

#endif

#ifndef MINGW

uns8b trp_mkdir( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;

    res = mkdir( cpath,
                 S_IRUSR | S_IWUSR | S_IXUSR |
                 S_IRGRP |           S_IXGRP |
                 S_IROTH |           S_IXOTH ) ? 1 : 0;
    trp_csprint_free( cpath );
    return res;
}

#else

uns8b trp_mkdir( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;
    wchar_t *wp;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL )
        return 1;
    res = _wmkdir( wp ) ? 1 : 0;
    trp_gc_free( wp );
    return res;
}

#endif

uns8b trp_mkfifo( trp_obj_t *path )
{
#ifdef MINGW
    return 1;;
#else
    uns8b *cpath = trp_csprint( path ), res;

    res = mkfifo( cpath,
                  S_IRUSR | S_IWUSR | S_IXUSR |
                  S_IRGRP |           S_IXGRP |
                  S_IROTH |           S_IXOTH ) ? 1 : 0;
    trp_csprint_free( cpath );
    return res;
#endif
}

#ifndef MINGW

uns8b trp_remove( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;

    res = remove( cpath ) ? 1 : 0;
    trp_csprint_free( cpath );
    return res;
}

#else

uns8b trp_remove( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), res;
    wchar_t *wp;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL )
        return 1;
    res = _wremove( wp ) ? 1 : 0;
    if ( res )
        res = _wrmdir( wp ) ? 1 : 0;
    trp_gc_free( wp );
    return res;
}

#endif

#ifndef MINGW

uns8b trp_rename( trp_obj_t *oldp, trp_obj_t *newp )
{
    uns8b *opath = trp_csprint( oldp ), *npath = trp_csprint( newp ), res;

    res = rename( opath, npath ) ? 1 : 0;
    trp_csprint_free( npath );
    trp_csprint_free( opath );
    return res;
}

#else

uns8b trp_rename( trp_obj_t *oldp, trp_obj_t *newp )
{
    uns8b *opath = trp_csprint( oldp ), *npath = trp_csprint( newp ), res;
    wchar_t *wo, *wn;

    wo = trp_utf8_to_wc_path( opath );
    wn = trp_utf8_to_wc_path( npath );
    trp_csprint_free( npath );
    trp_csprint_free( opath );
    if ( ( wo == NULL ) || ( wn == NULL ) ) {
        trp_gc_free( wo );
        trp_gc_free( wn );
        return 1;
    }
    res = _wrename( wo, wn ) ? 1 : 0;
    trp_gc_free( wo );
    trp_gc_free( wn );
    return res;
}

#endif

#ifndef MINGW

trp_obj_t *trp_pathexists( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    trp_obj_t *res;
    struct stat st;

    res = lstat( cpath, &st ) ? TRP_FALSE : TRP_TRUE;
    trp_csprint_free( cpath );
    return res;
}

#else

trp_obj_t *trp_pathexists( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    wchar_t *wp;
    struct _stati64 st;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL ) {
        trp_gc_free( wp );
        return TRP_FALSE;
    }
    return _wstati64( wp, &st ) ? TRP_FALSE : TRP_TRUE;
}

#endif

#ifndef MINGW

trp_obj_t *trp_ftime( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    struct stat st;

    if ( lstat( cpath, &st ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
    trp_csprint_free( cpath );
    return trp_date_cal( st.st_mtime );
}

#else

trp_obj_t *trp_ftime( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    wchar_t *wp;
    struct _stati64 st;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL ) {
        trp_gc_free( wp );
        return UNDEF;
    }
    if ( _wstati64( wp, &st ) ) {
        trp_gc_free( wp );
        return UNDEF;
    }
    trp_gc_free( wp );
    return trp_date_cal( st.st_mtime );
}

#endif

#ifndef MINGW

trp_obj_t *trp_fsize( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    struct stat st;

    if ( lstat( cpath, &st ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
    trp_csprint_free( cpath );
    return trp_sig64( st.st_size );
}

#else

trp_obj_t *trp_fsize( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    wchar_t *wp;
    struct _stati64 st;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL ) {
        trp_gc_free( wp );
        return UNDEF;
    }
    if ( _wstati64( wp, &st ) ) {
        trp_gc_free( wp );
        return UNDEF;
    }
    trp_gc_free( wp );
    return trp_sig64( st.st_size );
}

#endif

uns8b trp_utime( trp_obj_t *path, trp_obj_t *actime, trp_obj_t *modtime )
{
    uns8b *cpath;
    struct tm lt;
#ifndef MINGW
    struct utimbuf timebuf;
#else
    struct _utimbuf timebuf;
#endif
    uns8b res;

    if ( modtime == NULL )
        modtime = actime;
    if ( ( actime->tipo != TRP_DATE ) ||
         ( modtime->tipo != TRP_DATE ) )
        return 1;
    if ( ( ((trp_date_t *)actime)->anno < 1900 ) ||
         ( ((trp_date_t *)modtime)->anno < 1900 ) )
        return 1;
    lt.tm_sec = ( ((trp_date_t *)actime)->secondi < 60 ) ? ((trp_date_t *)actime)->secondi : 0;
    lt.tm_min = ( ((trp_date_t *)actime)->minuti < 60 ) ? ((trp_date_t *)actime)->minuti : 0;
    lt.tm_hour = ( ((trp_date_t *)actime)->ore < 24 ) ? ((trp_date_t *)actime)->ore : 0;
    lt.tm_mday = ((trp_date_t *)actime)->giorno ? ((trp_date_t *)actime)->giorno : 1;
    lt.tm_mon = ((trp_date_t *)actime)->mese ? ((trp_date_t *)actime)->mese - 1 : 0;
    lt.tm_year = ((trp_date_t *)actime)->anno - 1900;
    lt.tm_wday = 0;
    lt.tm_yday = 0;
    lt.tm_isdst = -1;
    if ( ( timebuf.actime = mktime( &lt ) ) == (time_t)(-1) )
        return 1;
    lt.tm_sec = ( ((trp_date_t *)modtime)->secondi < 60 ) ? ((trp_date_t *)modtime)->secondi : 0;
    lt.tm_min = ( ((trp_date_t *)modtime)->minuti < 60 ) ? ((trp_date_t *)modtime)->minuti : 0;
    lt.tm_hour = ( ((trp_date_t *)modtime)->ore < 24 ) ? ((trp_date_t *)modtime)->ore : 0;
    lt.tm_mday = ((trp_date_t *)modtime)->giorno ? ((trp_date_t *)modtime)->giorno : 1;
    lt.tm_mon = ((trp_date_t *)modtime)->mese ? ((trp_date_t *)modtime)->mese - 1 : 0;
    lt.tm_year = ((trp_date_t *)modtime)->anno - 1900;
    lt.tm_wday = 0;
    lt.tm_yday = 0;
    lt.tm_isdst = -1;
    if ( ( timebuf.modtime = mktime( &lt ) ) == (time_t)(-1) )
        return 1;
    cpath = trp_csprint( path );
#ifndef MINGW
    res =  utime( cpath, &timebuf ) ? 1 : 0;
#else
    {
        wchar_t *wp = trp_utf8_to_wc_path( cpath );

        if ( wp == NULL ) {
            trp_csprint_free( cpath );
            return 1;
        }
        res = _wutime( wp, &timebuf ) ? 1 : 0;
        trp_gc_free( wp );
    }
#endif
    trp_csprint_free( cpath );
    return res;
}

#ifndef MINGW

trp_obj_t *trp_directory( trp_obj_t *obj )
{
    trp_obj_t *res;
    DIR *d;
    struct dirent *de;

    if ( obj ) {
        uns8b *cpath = trp_csprint( obj );

        if ( *cpath ) {
            d = opendir( cpath );
            trp_csprint_free( cpath );
        } else {
            return UNDEF;
        }
    } else {
        d = opendir( "." );
    }
    if ( d == NULL )
        return UNDEF;
    res = trp_queue();
    while ( de = readdir( d ) )
        trp_queue_put( res, trp_cord( de->d_name ) );
    closedir( d );
    return res;
}

trp_obj_t *trp_directory_ext( trp_obj_t *obj )
{
    trp_obj_t *res;
    DIR *d;
    struct dirent *de;

    if ( obj ) {
        uns8b *cpath = trp_csprint( obj );

        if ( *cpath ) {
            d = opendir( cpath );
            trp_csprint_free( cpath );
        } else {
            return UNDEF;
        }
    } else {
        d = opendir( "." );
    }
    if ( d == NULL )
        return UNDEF;
    res = trp_queue();
    while ( de = readdir( d ) )
        trp_queue_put( res, trp_list( trp_sig64( de->d_ino ),
                                      trp_sig64( de->d_type ),
                                      trp_cord( de->d_name ),
                                      NULL ) );
    closedir( d );
    return res;
}

#else

trp_obj_t *trp_directory( trp_obj_t *obj )
{
    trp_obj_t *res;
    _WDIR *d;
    struct _wdirent *de;
    uns8b *p;
    wchar_t *wp;

    if ( obj ) {
        uns8b *cpath = trp_csprint( obj );

        if ( ( wp = trp_utf8_to_wc_path( cpath ) ) == NULL ) {
            trp_csprint_free( cpath );
            return UNDEF;
        }
        trp_csprint_free( cpath );
        d = _wopendir( wp );
        trp_gc_free( wp );
    } else {
        if ( ( wp = trp_utf8_to_wc( "." ) ) == NULL )
            return UNDEF;
        d = _wopendir( wp );
        trp_gc_free( wp );
    }
    if ( d == NULL )
        return UNDEF;
    res = trp_queue();
    while ( de = _wreaddir( d ) )
        if ( p = trp_wc_to_utf8( de->d_name ) ) {
            trp_queue_put( res, trp_cord( p ) );
            trp_gc_free( p );
        }
    _wclosedir( d );
    return res;
}

trp_obj_t *trp_directory_ext( trp_obj_t *obj )
{
    trp_obj_t *res;
    _WDIR *d;
    struct _wdirent *de;
    uns8b *p;
    wchar_t *wp;

    if ( obj ) {
        uns8b *cpath = trp_csprint( obj );

        if ( ( wp = trp_utf8_to_wc_path( cpath ) ) == NULL ) {
            trp_csprint_free( cpath );
            return UNDEF;
        }
        trp_csprint_free( cpath );
        d = _wopendir( wp );
        trp_gc_free( wp );
    } else {
        if ( ( wp = trp_utf8_to_wc( "." ) ) == NULL )
            return UNDEF;
        d = _wopendir( wp );
        trp_gc_free( wp );
    }
    if ( d == NULL )
        return UNDEF;
    res = trp_queue();
    while ( de = _wreaddir( d ) )
        if ( p = trp_wc_to_utf8( de->d_name ) ) {
            trp_queue_put( res, trp_list( trp_sig64( de->d_ino ), /* sempre 0 */
                                          ZERO, /* DT_UNKNOWN */
                                          trp_cord( p ),
                                          NULL ) );
            trp_gc_free( p );
        }
    _wclosedir( d );
    return res;
}

#endif

trp_obj_t *trp_getenv( trp_obj_t *obj )
{
    uns8b *c = trp_csprint( obj ), *p;

    p = getenv( c );
    trp_csprint_free( c );
    if ( p == NULL )
        return UNDEF;
    return trp_cord( p );
}

#ifndef MINGW

uns8b trp_sleep( trp_obj_t *sec )
{
    trp_obj_t *n = trp_math_floor( sec );
    struct timespec a, b, *req, *rem, *t;
    uns8b res;

    if ( n->tipo != TRP_SIG64 )
        return 1;
    if ( ((trp_sig64_t *)n)->val < 0 )
        return 1;
    a.tv_sec = (time_t)( ((trp_sig64_t *)n)->val );
    n = trp_math_floor( trp_math_times( trp_math_minus( sec, n, NULL ),
                                        trp_sig64( 1000000000 ), NULL ) );
    a.tv_nsec = (long)( ((trp_sig64_t *)n)->val );
    for ( req = &a, rem = &b ; ; t = req, req = rem, rem = t ) {
        if ( nanosleep( req, rem ) == 0 ) {
            res = 0;
            break;
        }
        if ( errno != EINTR ) {
            res = 1;
            break;
        }
    }
    return res;
}

#else

uns8b trp_sleep( trp_obj_t *sec )
{
    uns32b msec;

    if ( trp_cast_uns32b( trp_math_floor( trp_math_times( sec, trp_sig64( 1000 ), NULL ) ), &msec ) )
        return 1;
    if ( msec == 0 )
        return 0;
    if ( msec == INFINITE )
        return 1;
    Sleep( msec );
    return 0;
}

#endif

#ifndef MINGW

static uns8b trp_path_mode( trp_obj_t *path, mode_t *mode )
/*
 rende st.st_mode reso da lstat;
 */
{
    uns8b *cpath = trp_csprint( path ), res;
    struct stat st;

    if ( lstat( cpath, &st ) )
        res = 1;
    else {
        res = 0;
        *mode = st.st_mode;
    }
    trp_csprint_free( cpath );
    return res;
}

#else

static uns8b trp_path_mode( trp_obj_t *path, mode_t *mode )
/*
 rende st.st_mode reso da lstat;
 */
{
    uns8b *cpath = trp_csprint( path ), res;
    wchar_t *wp;
    struct _stati64 st;

    wp = trp_utf8_to_wc_path( cpath );
    trp_csprint_free( cpath );
    if ( wp == NULL ) {
        trp_gc_free( wp );
        return 1;
    }
    if ( _wstati64( wp, &st ) )
        res = 1;
    else {
        res = 0;
        *mode = st.st_mode;
    }
    trp_gc_free( wp );
    return res;
}

#endif

trp_obj_t *trp_lstat_mode( trp_obj_t *path )
{
    mode_t mode;

    if ( trp_path_mode( path, &mode ) )
        return UNDEF;
    return trp_sig64( (sig64b)mode );
}

trp_obj_t *trp_isreg( trp_obj_t *path )
{
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISREG( mode ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_isdir( trp_obj_t *path )
{
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISDIR( mode ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_ischr( trp_obj_t *path )
{
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISCHR( mode ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_isblk( trp_obj_t *path )
{
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISBLK( mode ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_isfifo( trp_obj_t *path )
{
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISFIFO( mode ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_islnk( trp_obj_t *path )
{
#ifdef MINGW
    return TRP_FALSE;
#else
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISLNK( mode ) ? TRP_TRUE : TRP_FALSE;
#endif
}

trp_obj_t *trp_issock( trp_obj_t *path )
{
#ifdef MINGW
    return TRP_FALSE;
#else
    mode_t mode;

    if ( path->tipo == TRP_SIG64 )
        mode = (mode_t)( ((trp_sig64_t *)path)->val );
    else if ( trp_path_mode( path, &mode ) )
        return TRP_FALSE;
    return S_ISSOCK( mode ) ? TRP_TRUE : TRP_FALSE;
#endif
}

trp_obj_t *trp_inode( trp_obj_t *path )
{
#ifdef MINGW
    return UNDEF;
#else
    uns8b *cpath = trp_csprint( path );
    struct stat st;

    if ( lstat( cpath, &st ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
    trp_csprint_free( cpath );
    return trp_cons( trp_sig64( (sig64b)(st.st_dev) ),
                     trp_sig64( (sig64b)(st.st_ino) ) );
#endif
}

trp_obj_t *trp_gc_version_major()
{
#ifdef GC_VERSION_MAJOR
    return trp_sig64( GC_VERSION_MAJOR );
#else
    return UNDEF;
#endif
}

trp_obj_t *trp_gc_version_minor()
{
#ifdef GC_VERSION_MINOR
    return trp_sig64( GC_VERSION_MINOR );
#else
    return UNDEF;
#endif
}

trp_obj_t *trp_readlink( trp_obj_t *path )
{
#ifdef MINGW
    return UNDEF;
#else
    uns8b *cpath = trp_csprint( path );
    char buf[ 65536 ];
    int res;

    res = readlink( cpath, buf, 65536 );
    trp_csprint_free( cpath );
    if ( res < 0 )
        return UNDEF;
    if ( res == 65536 )
        --res;
    buf[ res ] = 0;
    return trp_cord( buf );
#endif
}

uns8b trp_link( trp_obj_t *path1, trp_obj_t *path2 )
{
#ifdef MINGW
    return 1;
#else
    uns8b *cpath1 = trp_csprint( path1 ), *cpath2 = trp_csprint( path2 );
    int res;

    res = link( cpath1, cpath2 );
    trp_csprint_free( cpath1 );
    trp_csprint_free( cpath2 );
    return res ? 1 : 0;
#endif
}

uns8b trp_symlink( trp_obj_t *path1, trp_obj_t *path2 )
{
#ifdef MINGW
    return 1;
#else
    uns8b *cpath1 = trp_csprint( path1 ), *cpath2 = trp_csprint( path2 );
    int res;

    res = symlink( cpath1, cpath2 );
    trp_csprint_free( cpath1 );
    trp_csprint_free( cpath2 );
    return res ? 1 : 0;
#endif
}

void trp_sync()
{
#ifndef MINGW
    sync();
#endif
}

trp_obj_t *trp_ipv4_address()
{
#ifdef MINGW
    return UNDEF;
#else
    int fd;
    struct ifreq ifr;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy( ifr.ifr_name, "enp3s0", IFNAMSIZ-1 );
    ioctl( fd, SIOCGIFADDR, &ifr );
    close( fd );
    return trp_cord( inet_ntoa( ( ( struct sockaddr_in *)&ifr.ifr_addr )->sin_addr ) );
#endif
}

trp_obj_t *trp_system( trp_obj_t *obj, ... )
{
#ifdef MINGW
    int res;
    uns8b *p;
    va_list args;

    va_start( args, obj );
    p = trp_csprint_multi( obj, args );
    va_end( args );
    GC_gcollect();
    res = system( p );
    trp_csprint_free( p );
    return trp_sig64( res );
#else
    int res, pid;

    GC_gcollect();
    GC_atfork_prepare();
    pid = fork();
    if ( pid == 0 ) {
        uns8b *sh = "/bin/sh", *p;
        va_list args;

        GC_atfork_child();
        va_start( args, obj );
        p = trp_csprint_multi( obj, args );
        va_end( args );
        execl( sh, sh, "-c", p, NULL );
        exit( -1 );
    }
    GC_atfork_parent();
    if ( pid == -1 )
        res = -1;
    else
        waitpid( pid, &res, 0 );
    return trp_sig64( res );
#endif
}

trp_obj_t *trp_getpid()
{
    return trp_sig64( getpid() );
}

trp_obj_t *trp_fork()
{
#ifdef MINGW
    return trp_sig64( -1 );
#else
    int pid;

    GC_gcollect();
    GC_atfork_prepare();
    pid = fork();
    if ( pid )
        GC_atfork_parent();
    else
        GC_atfork_child();
    return trp_sig64( pid );
#endif
}

trp_obj_t *trp_sysinfo()
{
#ifdef MINGW
    return UNDEF;
#else
    struct sysinfo info;

    if ( sysinfo( &info ) )
        return UNDEF;
    return trp_list( trp_sig64( info.uptime ),
                     trp_list( trp_sig64( info.loads[ 0 ] ),
                               trp_sig64( info.loads[ 1 ] ),
                               trp_sig64( info.loads[ 2 ] ),
                               NULL ),
                     trp_sig64( info.totalram ),
                     trp_sig64( info.freeram ),
                     trp_sig64( info.sharedram ),
                     trp_sig64( info.bufferram ),
                     trp_sig64( info.totalswap ),
                     trp_sig64( info.freeswap ),
                     trp_sig64( info.procs ),
                     trp_sig64( info.totalhigh ),
                     trp_sig64( info.freehigh ),
                     trp_sig64( info.mem_unit ),
                     NULL );
#endif
}

trp_obj_t *trp_ratio2uns64b( trp_obj_t *obj )
{
    uns64b val;

    if ( trp_cast_double( obj, (flt64b *)( &val ) ) )
        return UNDEF;
    if ( val <= 0x7fffffffffffffffLL )
        return trp_sig64( val );
    return trp_cat( TRP_MAXINT, trp_sig64( val - 0x7fffffffffffffffLL ), NULL );
}

void trp_print_rusage_diff( char *msg )
{
    static trp_obj_t *st = NULL;
    static uns8b lev = 0;
    trp_obj_t *tmsg, *act;
    uns8b i;

    if ( st == NULL )
        st = NIL;
    tmsg = trp_cord( msg );
    act = trp_car( trp_getrusage_self() );
    if ( trp_equal( tmsg, trp_car( trp_car( st ) ) ) == TRP_TRUE ) {
        lev--;
        for ( i = lev ; i ; i-- )
            fprintf( stderr, "  " );
        fprintf( stderr, "### %s (fine): ", msg );
        trp_fprint( TRP_STDERR, trp_math_minus( act, trp_cdr( trp_car( st ) ), NULL ), NULL );
        fprintf( stderr, " secondi\n" );
        st = trp_cdr( st );
    } else {
        for ( i = lev ; i ; i-- )
            fprintf( stderr, "  " );
        lev++;
        fprintf( stderr, "### %s (inizio)\n", msg );
        st = trp_cons( trp_cons( tmsg, act ), st );
    }
}

