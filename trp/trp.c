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
#include <signal.h>

extern void trp_char_init();
extern void trp_compiler_exit();

static void trp_signal_handler( int sig );
static void trp_init_check();
static void trp_init_error( char *msg );
static void trp_gc_warn_proc( char *msg, GC_word arg );
static void *trp_mp_realloc( void *p, size_t osz, size_t nsz );
static void trp_mp_free( void *p, size_t sz );
static trp_obj_t *trp_env_stack();
static void trp_exit_internal( int code );

objfun_t _trp_env_stack = NULL;

static voidfun_t _trp_gc_warn_proc;

void trp_init( int argc, char *argv[] )
{
    extern trp_obj_t *trp_date_19700101();

#ifndef MINGW
    GC_set_handle_fork( -1 );
#endif
    GC_INIT();
//    _trp_gc_warn_proc =
    GC_set_warn_proc( trp_gc_warn_proc );
    if ( _trp_env_stack == NULL )
        _trp_env_stack = trp_env_stack;
    trp_init_check();
    trp_arg_init( argc, argv );
    trp_char_init();
    trp_special_init();
    /*
     modifica il sistema di allocazione di gmp
     */
    mp_set_memory_functions( trp_gc_malloc, trp_mp_realloc, trp_mp_free );
    /*
     costruisce gli oggetti speciali
     (non è strettamente necessario, ma così siamo certi,
     in ambito multithreading, che non verranno costruiti
     più di una volta)
     */
    (void)EMPTYCORD;
    (void)trp_double( 0.0 );
    (void)UNO;
    (void)DIECI;
    (void)TRP_MAXINT;
    (void)TRP_MININT;
    (void)TRP_EQUAL;
    (void)TRP_LESS;
    (void)TRP_STDIN;
    (void)TRP_STDOUT;
    (void)TRP_STDERR;
    (void)trp_date_19700101();

    signal( SIGSEGV, trp_signal_handler );
#ifndef MINGW
    signal( SIGBUS, trp_signal_handler );
#endif
    signal( SIGINT, trp_signal_handler );
    signal( SIGTERM, trp_signal_handler );
    signal( SIGABRT, trp_signal_handler );
#ifndef MINGW
    signal( SIGHUP, trp_signal_handler );
    signal( SIGQUIT, trp_signal_handler );
    signal( SIGPIPE, trp_signal_handler );
#endif
    signal( SIGILL, trp_signal_handler );
    signal( SIGFPE, trp_signal_handler );
}

static void trp_signal_handler( int sig )
{
    signal( sig, trp_signal_handler );
    switch ( sig ) {
    case SIGSEGV:
#ifndef MINGW
    case SIGBUS:
#endif
        fprintf( stderr, "Segmentation fault!\n" );
        trp_exit_internal( -1 );
        break;
    case SIGINT:
    case SIGTERM:
    case SIGABRT:
        fprintf( stderr, "\nReceived signal int/term/abrt. Premature exit.\n" );
        trp_exit_internal( -1 );
        break;
#ifndef MINGW
    case SIGHUP:
        fprintf( stderr, "\nReceived signal hup (ignored)\n" );
        break;
    case SIGQUIT:
        fprintf( stderr, "\nReceived signal quit (ignored)\n" );
        break;
    case SIGPIPE:
        fprintf( stderr, "\nReceived signal pipe (ignored)\n" );
        break;
#endif
    case SIGILL:
        fprintf( stderr, "\nReceived signal ill (ignored)\n" );
        break;
    case SIGFPE:
        fprintf( stderr, "\nReceived signal fpe (ignored)\n" );
        break;
    }
}

static void trp_init_check()
{
    uns16b i = 1;

    if ( ( sizeof( uns8b ) != 1 ) || ( sizeof( sig8b ) != 1 ) )
        trp_init_error( "ridefinisci le macro uns8b e sig8b in trp.h" );
    if ( ( sizeof( uns16b ) != 2 ) || ( sizeof( sig16b ) != 2 ) )
        trp_init_error( "ridefinisci le macro uns16b e sig16b in trp.h" );
    if ( ( sizeof( uns32b ) != 4 ) || ( sizeof( sig32b ) != 4 ) )
        trp_init_error( "ridefinisci le macro uns32b e sig32b in trp.h" );
    if ( ( sizeof( uns64b ) != 8 ) || ( sizeof( sig64b ) != 8 ) )
        trp_init_error( "ridefinisci le macro uns64b e sig64b in trp.h" );
    if ( sizeof( flt32b ) != 4 )
        trp_init_error( "ridefinisci la macro flt32b in trp.h" );
    if ( sizeof( flt64b ) != 8 )
        trp_init_error( "ridefinisci la macro flt64b in trp.h" );
    if ( sizeof( int ) != 4 )
        trp_init_error( "modifica la codifica di mpi e ratio" );
#ifdef TRP_LITTLE_ENDIAN
    if ( *((uns8b *)(&i)) == 0 )
        trp_init_error( "definisci TRP_BIG_ENDIAN e commenta TRP_LITTLE_ENDIAN in trp.h" );
#endif
#ifndef TRP_BIG_ENDIAN
    if ( *((uns8b *)(&i)) == 0 )
        trp_init_error( "definisci TRP_BIG_ENDIAN e commenta TRP_LITTLE_ENDIAN in trp.h" );
#endif
#ifdef TRP_BIG_ENDIAN
    if ( *((uns8b *)(&i)) == 1 )
        trp_init_error( "definisci TRP_LITTLE_ENDIAN e commenta TRP_BIG_ENDIAN in trp.h" );
#endif
#ifndef TRP_LITTLE_ENDIAN
    if ( *((uns8b *)(&i)) == 1 )
        trp_init_error( "definisci TRP_LITTLE_ENDIAN e commenta TRP_BIG_ENDIAN in trp.h" );
#endif
}

static void trp_init_error( char *msg )
{
    fprintf( stderr, "%s\n", msg );
    exit( -1 );
}

static void trp_gc_warn_proc( char *msg, GC_word arg )
{
    /*
    fprintf( stderr, "###GC warn proc:\n" );
    _trp_gc_warn_proc( msg, arg );
    */
}

static void *trp_mp_realloc( void *p, size_t osz, size_t nsz )
{
    return trp_gc_realloc( p, nsz );
}

static void trp_mp_free( void *p, size_t sz )
{
    trp_gc_free( p );
}

static trp_obj_t *trp_env_stack()
{
    static trp_obj_t *stack = NULL;

    if ( stack == NULL )
        stack = trp_stack();
    return stack;
}

static void trp_exit_internal( int code )
{
    GC_gcollect();
    trp_compiler_exit();
    exit( code );
}

void trp_exit( trp_obj_t *obj )
{
    int code = 0;

    if ( obj )
        if ( obj->tipo == TRP_SIG64 )
            code = (int)( ((trp_sig64_t *)obj)->val );
    trp_exit_internal( code );
}

trp_obj_t *trp_heap_size()
{
    return trp_sig64( GC_get_heap_size() );
}

trp_obj_t *trp_free_bytes()
{
    return trp_sig64( GC_get_free_bytes() );
}

trp_obj_t *trp_endianness()
{
#ifdef TRP_LITTLE_ENDIAN
    return TRP_TRUE;
#else
    return TRP_FALSE;
#endif
}

char *trp_gc_strdup( const char *s )
{
    char *t;
    size_t size = strlen( s ) + 1;

    if ( ( t = GC_malloc( size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    memcpy( t, s, size );
    return t;
}

void *trp_gc_calloc( size_t nmemb, size_t size )
{
    size_t n = nmemb * size;
    void *p;

    if ( ( p = GC_malloc( n ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    memset( p, 0, n );
    return p;
}

void *trp_gc_malloc( size_t size )
{
    void *p;

    if ( ( p = GC_malloc( size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    return p;
}

void *trp_gc_malloc_finalize( size_t size, GC_finalization_proc fn )
{
    void *p;

    p = trp_gc_malloc( size );
    GC_register_finalizer( p, fn, NULL, NULL, NULL );
    return p;
}

void *trp_gc_malloc_atomic( size_t size )
{
    void *p;

    if ( ( p = GC_malloc_atomic( size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    return p;
}

void *trp_gc_malloc_atomic_finalize( size_t size, GC_finalization_proc fn )
{
    void *p;

    p = trp_gc_malloc_atomic( size );
    GC_register_finalizer( p, fn, NULL, NULL, NULL );
    return p;
}

void *trp_gc_realloc( void *p, size_t size )
{
    if ( ( p = GC_realloc( p, size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    return p;
}

void trp_gc_remove_finalizer( trp_obj_t *obj )
{
    GC_register_finalizer( (void *)obj, 0, NULL, NULL, NULL );
}

void *trp_malloc( size_t size )
{
    void *p;

    if ( ( p = malloc( size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    return p;
}

void *trp_realloc( void *p, size_t size )
{
    if ( ( p = realloc( p, size ) ) == NULL )
        trp_init_error( "aborted (malloc)\n" );
    return p;
}

#ifdef TRP_FORCE_FREE
void trp_free_list( trp_obj_t *l )
{
    trp_obj_t *obj;

    while ( l != NIL ) {
        obj = ((trp_cons_t *)l)->cdr;
        trp_gc_free( l );
        l = obj;
    }
}
#endif

uns16b trp_swap_endian16( uns16b n )
{
    uns16b m;
    uns8b *c = (uns8b *)( &n ), *d = (uns8b *)( &m );
    d[ 0 ] = c[ 1 ];
    d[ 1 ] = c[ 0 ];
    return m;
}

uns32b trp_swap_endian32( uns32b n )
{
    uns32b m;
    uns8b *c = (uns8b *)( &n ), *d = (uns8b *)( &m );
    d[ 0 ] = c[ 3 ];
    d[ 1 ] = c[ 2 ];
    d[ 2 ] = c[ 1 ];
    d[ 3 ] = c[ 0 ];
    return m;
}

uns64b trp_swap_endian64( uns64b n )
{
    uns64b m;
    uns8b *c = (uns8b *)( &n ), *d = (uns8b *)( &m );
    d[ 0 ] = c[ 7 ];
    d[ 1 ] = c[ 6 ];
    d[ 2 ] = c[ 5 ];
    d[ 3 ] = c[ 4 ];
    d[ 4 ] = c[ 3 ];
    d[ 5 ] = c[ 2 ];
    d[ 6 ] = c[ 1 ];
    d[ 7 ] = c[ 0 ];
    return m;
}

uns32b trp_nargs( va_list args )
{
    uns32b n = 1;
    trp_obj_t *obj;

    for ( ; ; ) {
        obj = va_arg( args, trp_obj_t * );
        if ( obj == NULL )
            break;
        n++;
    }
    return n;
}

uns8b trp_upcase( uns8b c )
{
    if ( ( c >= 'a' ) && ( c <= 'z' ) )
        c = ( c - 'a' ) + 'A';
    return c;
}

uns8b trp_downcase( uns8b c )
{
    if ( ( c >= 'A' ) && ( c <= 'Z' ) )
        c = ( c - 'A' ) + 'a';
    return c;
}

void trp_skip( trp_obj_t *obj )
{
    return;
}

void trp_segfault()
{
    uns8b *p = NULL;

    *p = 0;
}

#include "../.ccver"

trp_obj_t *trp_cc_version()
{
    return trp_cord( _trp_cc_ver );
}

