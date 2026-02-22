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

#include "trp.h"
#include "avl_tree.h"
#ifdef MINGW
#include <windows.h>
#endif

#define TRP_PRINT_BUF_SIZE 512

#define trp_print_lock(f) (void)pthread_mutex_lock( &( ((trp_file_t *)(f))->mutex ) )
#define trp_print_unlock(f) (void)pthread_mutex_unlock( &( ((trp_file_t *)(f))->mutex ) )

static uns8b trp_print_flush( trp_print_t *p );

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

typedef struct {
    struct avl_tree_node node;
    uns8b *key;
    trp_obj_t *val;
} trp_assoc_item_t;

typedef struct {
    struct avl_tree_node node;
    trp_obj_t *val;
} trp_set_item_t;

uns8b trp_print( trp_obj_t *obj, ... )
{
    trp_print_t p;
    va_list args;
    uns8b buf[ TRP_PRINT_BUF_SIZE ], res = 0;

    p.flags = 0;
    p.fp = stdout;
    p.buf = buf;
    p.cnt = 0;
    va_start( args, obj );
    trp_print_lock( TRP_STDOUT );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        if ( trp_print_obj( &p, obj ) ) {
            res = 1;
            break;
        }
    if ( res == 0 )
        res = trp_print_flush( &p );
    trp_print_unlock( TRP_STDOUT );
    va_end( args );
    /*
     * è meglio che print non fallisca mai...
     * return res;
     */
    return 0;
}

uns8b trp_fprint( trp_obj_t *stream, trp_obj_t *obj, ... )
{
    trp_print_t p;
    va_list args;
    uns8b buf[ TRP_PRINT_BUF_SIZE ], res = 0;

    if ( ( p.fp = trp_file_writable_fp( stream ) ) == NULL )
        return 1;
    p.flags = ( (((trp_file_t *)stream)->flags) & 4 ) ? 1 : 0;
    p.buf = buf;
    p.cnt = 0;
    va_start( args, obj );
    trp_print_lock( stream );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        if ( trp_print_obj( &p, obj ) ) {
            res = 1;
            break;
        }
    if ( res == 0 )
        res = trp_print_flush( &p );
    trp_print_unlock( stream );
    va_end( args );
    return res;
}

trp_obj_t *trp_sprint( trp_obj_t *obj, ... )
{
    trp_print_t p;
    va_list args;

    p.flags = 0;
    p.buf = NULL;
    p.cnt = 0;
    CORD_ec_init( p.x );
    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        trp_print_obj( &p, obj );
    va_end( args );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( p.x ) ), p.cnt );
}

trp_obj_t *trp_sprint_list( trp_obj_t *l, trp_obj_t *divider )
{
    uns32b i;
    trp_queue_elem *elem;
    trp_print_t p;
    uns8b div = 0;

    if ( divider == NULL )
        divider = EMPTYCORD;
    p.flags = 0;
    p.buf = NULL;
    p.cnt = 0;
    CORD_ec_init( p.x );
    switch ( l->tipo ) {
    case TRP_CONS:
        for ( ; ;  ) {
            trp_print_obj( &p, ((trp_cons_t *)l)->car );
            l = ((trp_cons_t *)l)->cdr;
            if ( l->tipo != TRP_CONS ) {
                if ( l != NIL ) {
                    trp_print_obj( &p, divider );
                    trp_print_obj( &p, l );
                }
                break;
            }
            trp_print_obj( &p, divider );
        }
        break;
    case TRP_QUEUE:
        for ( elem = (trp_queue_elem *)( ((trp_queue_t *)l)->first ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) ) {
            if ( div )
                trp_print_obj( &p, divider );
            trp_print_obj( &p, elem->val );
            div = 1;
        }
        break;
    case TRP_ARRAY:
        for ( i = 0 ; i < ((trp_array_t *)l)->len ; i++ ) {
            if ( i )
                trp_print_obj( &p, divider );
            trp_print_obj( &p, ((trp_array_t *)l)->data[ i ] );
        }
        break;
    case TRP_ASSOC:
        {
            struct avl_tree_node *node;

            node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_assoc_t *)l)->root) );
            for ( i = 0 ; i < ((trp_assoc_t *)l)->len ; i++ ) {
                if ( i )
                    trp_print_obj( &p, divider );
                trp_print_chars( &p, "[", 1 );
                trp_print_chars( &p, ((trp_assoc_item_t *)(node->dummy))->key,
                                     strlen( ((trp_assoc_item_t *)(node->dummy))->key ) );
                trp_print_chars( &p, " . ", 3 );
                trp_print_obj( &p, ((trp_assoc_item_t *)(node->dummy))->val );
                trp_print_chars( &p, "]", 1 );
                node = avl_tree_next_in_order( node );
            }
        }
        break;
    case TRP_SET:
        {
            struct avl_tree_node *node;

            node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_set_t *)l)->root) );
            for ( i = 0 ; i < ((trp_set_t *)l)->len ; i++ ) {
                if ( i )
                    trp_print_obj( &p, divider );
                trp_print_obj( &p, ((trp_set_item_t *)(node->dummy))->val );
                node = avl_tree_next_in_order( node );
            }
        }
        break;
    default:
        return ( l == NIL ) ? EMPTYCORD : UNDEF;
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( p.x ) ), p.cnt );
}

uns8b *trp_csprint( trp_obj_t *obj )
{
    trp_print_t p;

    p.flags = 0;
    p.buf = NULL;
    p.cnt = 0;
    CORD_ec_init( p.x );
    trp_print_obj( &p, obj );
    if ( p.cnt == 0 )
        return "";
    return (uns8b *)CORD_to_char_star( CORD_ec_to_cord( p.x ) );
}

uns8b *trp_csprint_multi( trp_obj_t *obj, va_list args )
{
    trp_print_t p;

    p.flags = 0;
    p.buf = NULL;
    p.cnt = 0;
    CORD_ec_init( p.x );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        trp_print_obj( &p, obj );
    if ( p.cnt == 0 )
        return "";
    return (uns8b *)CORD_to_char_star( CORD_ec_to_cord( p.x ) );
}

#ifdef TRP_FORCE_FREE
void trp_csprint_free( uns8b *p )
{
    if ( *p )
        trp_gc_free( p );
}
#endif

uns8b trp_print_dump_raw( trp_obj_t *stream, trp_obj_t *raw )
{
    uns32b i;
    trp_print_t p;
    uns8b buf[ TRP_PRINT_BUF_SIZE ], h[ 5 ];

    if ( ( ( p.fp = trp_file_writable_fp( stream ) ) == NULL ) ||
         ( raw->tipo != TRP_RAW ) )
        return 1;
    p.flags = 0;
    p.buf = buf;
    p.cnt = 0;
    h[ 0 ] = '0';
    h[ 1 ] = 'x';
    if ( trp_print_char( &p, '{' ) )
        return 1;
    for ( i = 0 ; i < ((trp_raw_t *)raw)->len ; i++ ) {
        if ( i ) {
            if ( trp_print_char( &p, ',' ) )
                return 1;
            if ( ( i % 12 ) == 0 )
                if ( trp_print_char_star( &p, "\n " ) )
                    return 1;
        }
        sprintf( h + 2, "%02x", (int)( ((trp_raw_t *)raw)->data[ i ] ) );
        if ( trp_print_char_star( &p, h ) )
            return 1;
    }
    if ( trp_print_char_star( &p, "};\n" ) )
        return 1;
    return trp_print_flush( &p );
}

uns8b trp_print_obj( trp_print_t *p, trp_obj_t *obj )
{
    extern uns8bfun_t _trp_print_fun[];
    return (_trp_print_fun[ obj->tipo ])( p, obj );
}

uns8b trp_print_chars( trp_print_t *p, uns8b *s, uns32b cnt )
{
    for ( ; cnt ; cnt-- )
        if ( trp_print_char( p, *s++ ) )
            return 1;
    return 0;
}

uns8b trp_print_char_star( trp_print_t *p, uns8b *s )
{
    while ( *s )
        if ( trp_print_char( p, *s++ ) )
            return 1;
    return 0;
}

uns8b trp_print_sig64( trp_print_t *p, sig64b val )
{
    extern void trp_math_sig64_to_mpz( sig64b val, mpz_t mp );
    mpz_t mp;
    int len;
    uns8b *buf;
    uns8b res;

    mpz_init( mp );
    trp_math_sig64_to_mpz( val, mp );
    len = gmp_asprintf( (char **)&buf, "%Zd", mp );
    mpz_clear( mp );
    res = trp_print_chars( p, buf, len );
    trp_gc_free( buf );
    return res;
}

uns8b trp_print_char( trp_print_t *p, uns8b c )
{
    if ( p->buf ) {
        if ( p->cnt == TRP_PRINT_BUF_SIZE )
            if ( trp_print_flush( p ) )
                return 1;
        p->buf[ p->cnt ] = c;
    } else {
        if ( c ) {
            CORD_ec_append( p->x, c );
        } else {
            CORD_ec_flush_buf( p->x );
            p->x[ 0 ].ec_cord = CORD_cat( p->x[ 0 ].ec_cord, CORD_nul( 1 ) );
        }
    }
    p->cnt++;
    return 0;
}

static uns8b trp_print_flush( trp_print_t *p )
{
    int i, off = 0;

    while ( p->cnt ) {
        if ( p->flags & 1 ) {
            i = write( fileno( p->fp ), p->buf + off, p->cnt );
            if ( i == 0 ) {
#ifdef MINGW
                return 1;
#else
                struct pollfd ufds[ 1 ];

                ufds[ 0 ].fd = fileno( p->fp );
                ufds[ 0 ].events = POLLHUP;
                if ( poll( ufds, 1, -1 ) == 1 )
                    if ( ufds[ 0 ].revents & POLLHUP )
                        return 1;
#endif
            }
        } else
#ifdef MINGW
            if ( ( p->fp == stdout ) || ( p->fp == stderr ) ) {
                HANDLE h;
                DWORD written = 0;

                if ( p->fp == stdout )
                    h = GetStdHandle( STD_OUTPUT_HANDLE );
                else
                    h = GetStdHandle( STD_ERROR_HANDLE );
                if ( ( h == 0 ) || ( h == INVALID_HANDLE_VALUE ) )
                    return 1;
                if ( p->cnt == TRP_PRINT_BUF_SIZE ) {
                    while ( off + 4 <= p->cnt ) {
                        i = trp_cord_utf8_next( p->buf + off );
                        if ( i < 1 )
                            break;
                        off += i;
                    }
                    if ( off == 0 )
                        return 1;
                } else
                    off = p->cnt;
                WriteConsoleA( h, p->buf, off, &written, NULL );
                if ( ( written <= 0 ) || ( written > p->cnt ) )
                    return 1;
                p->cnt -= written;
                if ( p->cnt )
                    memmove( p->buf, p->buf + written, p->cnt );
                return 0;
            }
            i = fwrite( p->buf + off, 1, p->cnt, p->fp );
#else
            i = fwrite( p->buf + off, 1, p->cnt, p->fp );
#endif
        if ( i < 0 )
            return 1;
        p->cnt -= i;
        off += i;
    }
    return 0;
}

