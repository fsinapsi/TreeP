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

#include "trp.h"
#ifdef MINGW
#include <windows.h>
#include <shellapi.h>
#endif

static trp_obj_t *trp_arg_internal( uns8b flags, uns32b argc, char *argv[] );

#ifndef MINGW

static trp_obj_t *trp_arg_internal( uns8b flags, uns32b argc, char *argv[] )
{
    static uns32b n = 0;
    static trp_cord_t *arg = NULL;

    if ( flags & 1 ) {
        if ( arg == NULL ) {
            CORD c;

            n = argc;
            arg = trp_gc_malloc( sizeof( trp_cord_t ) * n );
            for ( argc = 0 ; argc < n ; argc++ ) {
                c = CORD_from_char_star( argv[ argc ] );
                arg[ argc ].tipo = TRP_CORD;
                arg[ argc ].len = CORD_len( c );
                arg[ argc ].c = c;
            }
        }
        return UNDEF;
    }
    if ( flags & 2 )
        return trp_sig64( n );
    if ( flags & 4 ) {
        if ( argc >= n )
            return UNDEF;
        return (trp_obj_t *)( &( arg[ argc ] ) );
    }
    return UNDEF;
}

#else

static trp_obj_t *trp_arg_internal( uns8b flags, uns32b argc, char *argv[] )
{
    static uns32b n = 0;
    static trp_cord_t *arg = NULL;

    if ( flags & 1 ) {
        if ( arg == NULL ) {
            LPWSTR *szArglist;
            int nArgs;
            CORD c;
            uns8b *p;

            if ( szArglist = CommandLineToArgvW( GetCommandLineW(), &nArgs ) ) {
                n = (uns32b)nArgs;
                arg = trp_gc_malloc( sizeof( trp_cord_t ) * n );
                for ( argc = 0 ; argc < n ; argc++ ) {
                    if ( ( p = trp_wc_to_utf8( szArglist[ argc ] ) ) == NULL ) {
                        n = 0;
                        trp_gc_free( arg );
                        arg = NULL;
                        break;
                    }
                    if ( argc == 0 )
                        trp_convert_slash( p );
                    c = CORD_from_char_star( p );
                    trp_gc_free( p );
                    arg[ argc ].tipo = TRP_CORD;
                    arg[ argc ].len = CORD_len( c );
                    arg[ argc ].c = c;
                }
                LocalFree( szArglist );
            }
        }
        return UNDEF;
    }
    if ( flags & 2 )
        return trp_sig64( n );
    if ( flags & 4 ) {
        if ( argc >= n )
            return UNDEF;
        return (trp_obj_t *)( &( arg[ argc ] ) );
    }
    return UNDEF;
}

#endif

void trp_arg_init( int argc, char *argv[] )
{
    if ( argc > 0 )
        (void)trp_arg_internal( 1, (uns32b)argc, argv );
}

trp_obj_t *trp_argc()
{
    return trp_arg_internal( 2, 0, NULL );
}

trp_obj_t *trp_argv( trp_obj_t *obj )
{
    uns32b n;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    return trp_arg_internal( 4, n, NULL );
}

