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
#include "./trpminizip.h"
#include <minizip/unzip.h>
#include <minizip/zip.h>
#ifdef MINGW
#include <io.h>
#include <windows.h>
#include <direct.h>
#include <fcntl.h>
#include <iowin32.h>
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

#ifndef MINGW
#define MYSLASH '/'
#define MYBACKSLASH '\\'
#else
#define MYSLASH '\\'
#define MYBACKSLASH '/'
#endif

#ifdef MINGW
#define trp_off_t sig64b
#define fseeko fseeko64
#define ftello ftello64
#else
#define trp_off_t off_t
#endif

#ifndef MINGW

static void my_get_time( uns8b *path, tm_zip *tmzip )
{
    struct stat st;
    struct tm *filedate;
    time_t tm_t = 0;

    if ( lstat( path, &st ) == 0 )
        tm_t = st.st_mtime;
    filedate = localtime( &tm_t );
    tmzip->tm_sec  = filedate->tm_sec;
    tmzip->tm_min  = filedate->tm_min;
    tmzip->tm_hour = filedate->tm_hour;
    tmzip->tm_mday = filedate->tm_mday;
    tmzip->tm_mon  = filedate->tm_mon ;
    tmzip->tm_year = filedate->tm_year;
}

static void my_set_time( uns8b *path, tm_unz tmu_date )
{
    struct utimbuf ut;
    struct tm newdate;

    newdate.tm_sec  = tmu_date.tm_sec;
    newdate.tm_min  = tmu_date.tm_min;
    newdate.tm_hour = tmu_date.tm_hour;
    newdate.tm_mday = tmu_date.tm_mday;
    newdate.tm_mon  = tmu_date.tm_mon;
    if ( tmu_date.tm_year > 1900 )
        newdate.tm_year = tmu_date.tm_year - 1900;
    else
        newdate.tm_year = tmu_date.tm_year;
    newdate.tm_isdst = -1;
    ut.actime = ut.modtime = mktime( &newdate );
    utime( path, &ut );
}

static uns8b my_mkdir( uns8b *path )
{
    struct stat st;
    uns8b res;

    if ( lstat( path, &st ) == 0 )
        res = S_ISDIR( st.st_mode ) ? 0 : 1;
    else
        res = mkdir( path,
                     S_IRUSR | S_IWUSR | S_IXUSR |
                     S_IRGRP |           S_IXGRP |
                     S_IROTH |           S_IXOTH ) ? 1 : 0;
    return res;
}

#else

static void my_get_time( uns8b *path, tm_zip *tmzip )
{
    wchar_t *wp;
    struct _stati64 st;
    struct tm *filedate;
    time_t tm_t = 0;

    wp = trp_utf8_to_wc( path );
    if ( wp ) {
        if ( _wstati64( wp, &st ) == 0 )
            tm_t = st.st_mtime;
        trp_gc_free( wp );
    }
    filedate = localtime( &tm_t );
    tmzip->tm_sec  = filedate->tm_sec;
    tmzip->tm_min  = filedate->tm_min;
    tmzip->tm_hour = filedate->tm_hour;
    tmzip->tm_mday = filedate->tm_mday;
    tmzip->tm_mon  = filedate->tm_mon ;
    tmzip->tm_year = filedate->tm_year;
}

static void my_set_time( uns8b *path, tm_unz tmu_date )
{
    struct _utimbuf ut;
    struct tm newdate;
    wchar_t *wp;

    wp = trp_utf8_to_wc( path );
    if ( wp == NULL )
        return;
    newdate.tm_sec  = tmu_date.tm_sec;
    newdate.tm_min  = tmu_date.tm_min;
    newdate.tm_hour = tmu_date.tm_hour;
    newdate.tm_mday = tmu_date.tm_mday;
    newdate.tm_mon  = tmu_date.tm_mon;
    if ( tmu_date.tm_year > 1900 )
        newdate.tm_year = tmu_date.tm_year - 1900;
    else
        newdate.tm_year = tmu_date.tm_year;
    newdate.tm_isdst = -1;
    ut.actime = ut.modtime = mktime( &newdate );
    _wutime( wp, &ut );
    trp_gc_free( wp );
}

static uns8b my_mkdir( uns8b *path )
{
    wchar_t *wp;
    struct _stati64 st;
    uns8b res;

    wp = trp_utf8_to_wc( path );
    if ( wp == NULL )
        return 1;
    if ( _wstati64( wp, &st ) == 0 )
        res = S_ISDIR( st.st_mode ) ? 0 : 1;
    else
        res = _wmkdir( wp ) ? 1 : 0;
    trp_gc_free( wp );
    return res;
}

#endif

static uns8b my_mkdir_hier( uns8b *path )
{
    uns8b *p;
    uns8b res = 0;
    uns8b notfirst = 0;

    for ( p = path ; *p ; p++ )
        if ( *p == MYSLASH ) {
            if ( notfirst ) {
                if ( *( p - 1 ) != MYSLASH ) {
                    *p = '\0';
                    res = my_mkdir( path );
                    *p = MYSLASH;
                    if ( res )
                        return 1;
                }
            }
            notfirst = 1;
        }
    return my_mkdir( path );
}

uns8b trp_minizip_zip( trp_obj_t *zip_path, trp_obj_t *src_path, trp_obj_t *str_path, trp_obj_t *level )
{
    zipFile zf = NULL;
    zip_fileinfo zi;
    FILE *fp = NULL;
    uns8b *zpath, *spath, *store, *buf = NULL, *p;
    uns32b compress_level;
    size_t size_buf = 16384, size_read;
    sig64b len;
    int zip64;
    uns8b res = 1;

    if ( level ) {
        if ( trp_cast_uns32b_range( level, &compress_level, 0, 9 ) )
            return 1;
    } else
        compress_level = 9;
    zpath = trp_csprint( zip_path );
    spath = trp_csprint( src_path );
    store = trp_csprint( str_path );

    for ( p = store ; *p ; p++ )
        if ( *p == '\\' )
            *p = '/';

    zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
    zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
    zi.dosDate = 0;
    zi.internal_fa = 0;
    zi.external_fa = 0;
    my_get_time( spath, &zi.tmz_date );

    if ( ( fp = trp_fopen( spath, "rb" ) ) == NULL )
        goto zipexit;
    if ( fseeko( fp, 0, SEEK_END ) )
        goto zipexit;
    if ( ( len = (sig64b)ftello( fp ) ) < 0 )
        goto zipexit;
    if ( fseeko( fp, 0, SEEK_SET ) )
        goto zipexit;
    zip64 = ( len >= 0xffffffff ) ? 1 : 0;

#ifdef MINGW
    {
        zlib_filefunc64_def ffunc;
        uns8b *wpath = (uns8b *)trp_utf8_to_wc_path( zpath );

        trp_csprint_free( zpath );
        zpath = wpath;
        fill_win32_filefunc64W( &ffunc );
        if ( ( zf = zipOpen2_64( zpath, 2, NULL, &ffunc ) ) == NULL )
            if ( ( zf = zipOpen2_64( zpath, 0, NULL, &ffunc ) ) == NULL )
                goto zipexit;
    }
#else
    if ( ( zf = zipOpen64( zpath, 2 ) ) == NULL )
        if ( ( zf = zipOpen64( zpath, 0 ) ) == NULL )
            goto zipexit;
#endif

    if ( ( buf = malloc( size_buf ) ) == NULL )
        goto zipexit;

    if ( zipOpenNewFileInZip3_64( zf, store, &zi,
                                  NULL, 0, NULL, 0, NULL,
                                  ( compress_level ) ? Z_DEFLATED : 0,
                                  (int)compress_level, 0,
                                  -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                                  NULL, 0, zip64 ) != ZIP_OK )
        goto zipexit;

    for ( ; ; ) {
        size_read = fread( buf, 1, size_buf, fp );
        if ( size_read < size_buf )
            if ( feof( fp ) == 0 )
                goto zipexit;
        if ( size_read == 0 )
            break;
        if ( zipWriteInFileInZip( zf, buf, size_read ) < 0 )
            goto zipexit;
    }

    if ( zipCloseFileInZip( zf ) != ZIP_OK )
        goto zipexit;

    res = 0;

zipexit:
    if ( zf )
        if ( zipClose( zf, NULL ) != ZIP_OK )
            res = 1;
    if ( fp )
        fclose( fp );
    if ( buf )
        free( buf );
#ifdef MINGW
    trp_gc_free( zpath );
#else
    trp_csprint_free( zpath );
#endif
    trp_csprint_free( spath );
    trp_csprint_free( store );
    return res;
}

#define MAX_PATH_LEN 512

uns8b trp_minizip_unzip( trp_obj_t *zip_path, trp_obj_t *dst_path )
{
    unzFile uf = NULL;
    unz_global_info64 gi;
    unz_file_info64 file_info;
    FILE *fp = NULL;
    uns8b *zpath, *dpath, *buf = NULL, *dest_path = NULL;
    uns8b *p, *filename_withoutpath;
    size_t size_buf = 8192;
    int err;
    uns32b dpath_len, i;
    uns8b res = 1;

    zpath = trp_csprint( zip_path );
    dpath = trp_csprint( dst_path );
    dpath_len = strlen( dpath );

    if ( dpath_len == 0 )
        goto unzipexit;

#ifdef MINGW
    {
        zlib_filefunc64_def ffunc;
        uns8b *wpath = (uns8b *)trp_utf8_to_wc_path( zpath );

        trp_csprint_free( zpath );
        zpath = wpath;
        fill_win32_filefunc64W( &ffunc );
        if ( ( uf = unzOpen2_64( zpath , &ffunc ) ) == NULL )
            goto unzipexit;
    }
#else
    if ( ( uf = unzOpen64( zpath ) ) == NULL )
        goto unzipexit;
#endif

    if ( unzGetGlobalInfo64( uf, &gi ) != UNZ_OK )
        goto unzipexit;

    if ( ( buf = malloc( size_buf ) ) == NULL )
        goto unzipexit;

    if ( ( dest_path = malloc( dpath_len + MAX_PATH_LEN + 1 ) ) == NULL )
        goto unzipexit;

    strcpy( dest_path, dpath );
    if ( dest_path[ dpath_len - 1 ] == MYBACKSLASH )
        dest_path[ dpath_len - 1 ] = MYSLASH;
    if ( dest_path[ dpath_len - 1 ] != MYSLASH )
        dest_path[ dpath_len++ ] = MYSLASH;

    for ( i = 0 ; i < gi.number_entry ; ) {
        if ( unzGetCurrentFileInfo64( uf, &file_info, dest_path + dpath_len, MAX_PATH_LEN, NULL, 0, NULL, 0 ) != UNZ_OK )
            goto unzipexit;
        for ( p = dest_path ; *p ; p++ ) {
            if ( *p == MYBACKSLASH )
                *p = MYSLASH;
            if ( *p == MYSLASH )
                filename_withoutpath = p + 1;
        }
        p = filename_withoutpath - 1;
        *p = '\0';
        if ( my_mkdir_hier( dest_path ) )
            goto unzipexit;
        *p = MYSLASH;
        if ( *filename_withoutpath ) {
            if ( ( fp = trp_fopen( dest_path, "w+b" ) ) == NULL )
                goto unzipexit;
            if ( unzOpenCurrentFilePassword( uf, NULL ) != UNZ_OK )
                goto unzipexit;
            for ( ; ; ) {
                err = unzReadCurrentFile( uf, buf, size_buf );
                if ( err == 0 )
                    break;
                if ( err < 0 )
                    goto unzipexit;
                if ( fwrite( buf, err, 1, fp ) != 1 )
                    goto unzipexit;
            }
            if ( unzCloseCurrentFile( uf ) != UNZ_OK )
                goto unzipexit;
            fclose( fp );
            fp = NULL;
            my_set_time( dest_path, file_info.tmu_date );
        }
        i++;
        if ( i < gi.number_entry )
            if ( unzGoToNextFile( uf ) != UNZ_OK )
                goto unzipexit;
    }

    res = 0;

unzipexit:
    if ( uf )
        unzClose( uf );
    if ( fp )
        fclose( fp );
    if ( buf )
        free( buf );
    if ( dest_path )
        free( dest_path );
#ifdef MINGW
    trp_gc_free( zpath );
#else
    trp_csprint_free( zpath );
#endif
    trp_csprint_free( dpath );
    return res;
}

