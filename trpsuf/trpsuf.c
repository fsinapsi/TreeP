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

#include "../trp/trp.h"
#include "./trpsuf.h"
#include "sais.h"
// #include "sais-lcp.h"

uns32b trp_size_internal( trp_obj_t *obj );
void trp_encode_internal( trp_obj_t *obj, uns8b **buf );

typedef struct {
    uns8b tipo;
    uns32b len;
    uns8b *T;
    uns32b *SA;
    uns32b *LCP;
} trp_suf_t;

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

static uns8b trp_suf_print( trp_print_t *p, trp_suf_t *obj );
static uns8b trp_suf_close( trp_suf_t *obj );
static uns8b trp_suf_close_basic( uns8b flags, trp_suf_t *obj );
static void trp_suf_finalize( void *obj, void *data );
static uns32b trp_suf_size( trp_suf_t *obj );
static void trp_suf_encode( trp_suf_t *obj, uns8b **buf );
static trp_obj_t *trp_suf_decode( uns8b **buf );
static trp_obj_t *trp_suf_length( trp_suf_t *suf );
static uns32b *trp_suf_lcp_array( uns32b n, uns8b *T, uns32b *SA );
static uns32b *trp_suf_get_lcp_array( trp_suf_t *suf );
static trp_obj_t *trp_suf_lcs_k_low( uns8b flags, trp_obj_t *k, trp_obj_t *s, va_list args1, va_list args2, va_list args3 );

#define TRP_MIN(a,b) (((a)<=(b))?(a):(b))

uns8b trp_suf_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern uns32bfun_t _trp_size_fun[];
    extern voidfun_t _trp_encode_fun[];
    extern objfun_t _trp_decode_fun[];
    extern objfun_t _trp_length_fun[];

    _trp_print_fun[ TRP_SUF ] = trp_suf_print;
    _trp_close_fun[ TRP_SUF ] = trp_suf_close;
    _trp_size_fun[ TRP_SUF ] = trp_suf_size;
    _trp_encode_fun[ TRP_SUF ] = trp_suf_encode;
    _trp_decode_fun[ TRP_SUF ] = trp_suf_decode;
    _trp_length_fun[ TRP_SUF ] = trp_suf_length;
    return 0;
}

static uns8b trp_suf_print( trp_print_t *p, trp_suf_t *obj )
{
    if ( trp_print_char_star( p, "#suf" ) )
        return 1;
    if ( obj->SA ) {
        /*
         FIXME
         */
    } else if ( trp_print_char_star( p, " (closed)" ) )
        return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_suf_close( trp_suf_t *obj )
{
    return trp_suf_close_basic( 1, obj );
}

static uns8b trp_suf_close_basic( uns8b flags, trp_suf_t *obj )
{
    if ( obj->SA ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->T );
        free( obj->SA );
        free( obj->LCP );
        obj->T = NULL;
        obj->SA = NULL;
        obj->LCP = NULL;
    }
    return 0;
}

static void trp_suf_finalize( void *obj, void *data )
{
    trp_suf_close_basic( 0, (trp_suf_t *)obj );
}

static uns32b trp_suf_size( trp_suf_t *obj )
{
    uns32b sz;

    if ( obj->SA )
        sz = 1 + sizeof( uns32b ) + ( 1 + sizeof( uns32b ) ) * obj->len;
    else
        sz = trp_size_internal( UNDEF );
    return sz;
}

static void trp_suf_encode( trp_suf_t *obj, uns8b **buf )
{
    if ( obj->SA ) {
        uns32b i, *p, *q;

        **buf = TRP_SUF;
        ++(*buf);
        p = (uns32b *)(*buf);
        *p = norm32( obj->len );
        (*buf) += 4;
        (void)memcpy( *buf, obj->T, obj->len );
        (*buf) += obj->len;
        p = (uns32b *)(*buf);
        q = obj->SA;
        for ( i = 0 ; i < obj->len ; i++ )
            *p++ = norm32( *q++ );
        *buf = (uns8b *)p;
    } else
        trp_encode_internal( UNDEF, buf );
}

static trp_obj_t *trp_suf_decode( uns8b **buf )
{
    trp_suf_t *obj;
    uns32b n, i, *p, *q;
    uns8b *T;
    uns32b *SA;

    n = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    T = malloc( n * sizeof( uns8b ) );
    SA = malloc( n * sizeof( int ) );
    if ( ( T == NULL ) || ( SA == NULL ) ) {
        free( T );
        free( SA );
        (*buf) += ( 1 + sizeof( uns32b ) ) * n;
        return UNDEF;
    }
    (void)memcpy( T, *buf, n );
    (*buf) += n;
    p = (uns32b *)(*buf);
    q = SA;
    for ( i = 0 ; i < n ; i++ )
        *q++ = norm32( *p++ );
    *buf = (uns8b *)p;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_suf_t ), trp_suf_finalize );
    obj->tipo = TRP_SUF;
    obj->len = n;
    obj->T = T;
    obj->SA = SA;
    obj->LCP = NULL;
    return (trp_obj_t *)obj;
}

static trp_obj_t *trp_suf_length( trp_suf_t *suf )
{
    return trp_sig64( suf->len );
}

static uns32b *trp_suf_lcp_array( uns32b n, uns8b *T, uns32b *SA )
{
    uns32b lcp, v, j;
    uns32b *LCP = malloc( n * sizeof( uns32b ) );
    uns32b *RANK = malloc( ( n - 1 ) * sizeof( uns32b ) );
    uns8b c;

    if ( ( LCP == NULL ) || ( RANK == NULL ) ) {
        free( LCP );
        free( RANK );
        return NULL;
    }
    for ( j = 1 ; j < n ; j++ )
        RANK[ SA[ j ] ] = j;
    n--;
    LCP[ 0 ] = 0;
    lcp = 0;
    for ( j = 0 ; j < n ; j++ ) {
        for ( v = SA[ RANK[ j ] - 1 ] ; ; lcp++ ) {
            c = T[ j + lcp ];
            if ( c != T[ v + lcp ] )
                break;
            if ( c == 0 )
                break;
        }
        LCP[ RANK[ j ] ] = lcp;
        if ( lcp )
            lcp--;
    }
    /*
    for ( j = 0 ; j <= n ; j++ ) {
        if ( j < n )
            printf( "# SA[%2u] = %2d, RANK = %2u, LCP = %2u  ", j, SA[ j ], RANK[ j ], LCP[ j ] );
        else
            printf( "# SA[%2u] = %2d, RANK = <>, LCP = %2u  ", j, SA[ j ], LCP[ j ] );
        for ( v = (uns32b)( SA[ j ] ) ; v < n ; v++ )
            if ( T[ v ] )
                printf( "%c", T[ v ] );
            else
                printf( "$" );
        printf( "\n" );
    }
    */
    free( RANK );
    return LCP;
}

static uns32b *trp_suf_get_lcp_array( trp_suf_t *suf )
{
    if ( suf->LCP == NULL )
        if ( suf->SA )
            suf->LCP = trp_suf_lcp_array( suf->len, suf->T, suf->SA );
    return suf->LCP;
}

static sig32b _compare( const uns8b *T, sig32b Tsize,
                        const uns8b *P, sig32b Psize,
                        sig32b suf, sig32b *match )
{
    sig32b i, j;
    sig32b r;

    for(i = suf + *match, j = *match, r = 0;
        (i < Tsize) && (j < Psize) && ((r = T[i] - P[j]) == 0); ++i, ++j) { }
    *match = j;
    return (r == 0) ? -(j != Psize) : r;
}

/* Search for the pattern P in the string T.
 * @param Tsize The length of the given string.
 * @param T[0..Tsize-1] The input string.
 * @param Psize The length of the given pattern string.
 * @param P[0..Psize-1] The input pattern string.
 * @param SA[0..SAsize-1] The input suffix array.
 * @param idx The output index.
 * @return The count of matches if no error occurred, -1 otherwise.
 */

static sig32b sa_search( sig32b Tsize, const uns8b *T,
                         sig32b Psize, const uns8b *P,
                         const sig32b *SA, sig32b *idx )
{
    sig32b size, lsize, rsize, half;
    sig32b match, lmatch, rmatch;
    sig32b llmatch, lrmatch, rlmatch, rrmatch;
    sig32b i, j, k;
    sig32b r;

    if ( idx )
        *idx = -1;
    if(Tsize == 0)
        return 0;
    if(Psize == 0) {
        if( idx )
            *idx = 0;
        return Tsize;
    }

    for(i = j = k = 0, lmatch = rmatch = 0, size = Tsize, half = size >> 1;
        0 < size;
        size = half, half >>= 1) {
        match = TRP_MIN(lmatch, rmatch);
        r = _compare(T, Tsize, P, Psize, SA[i + half], &match);
        if(r < 0) {
            i += half + 1;
            half -= (size & 1) ^ 1;
            lmatch = match;
        } else if(r > 0) {
            rmatch = match;
        } else {
            lsize = half, j = i, rsize = size - half - 1, k = i + half + 1;

            /* left part */
            for(llmatch = lmatch, lrmatch = match, half = lsize >> 1;
                0 < lsize;
                lsize = half, half >>= 1) {
                lmatch = TRP_MIN(llmatch, lrmatch);
                r = _compare(T, Tsize, P, Psize, SA[j + half], &lmatch);
                if(r < 0) {
                    j += half + 1;
                    half -= (lsize & 1) ^ 1;
                    llmatch = lmatch;
                } else {
                    lrmatch = lmatch;
                }
            }

            /* right part */
            for(rlmatch = match, rrmatch = rmatch, half = rsize >> 1;
                0 < rsize;
                rsize = half, half >>= 1) {
                rmatch = TRP_MIN(rlmatch, rrmatch);
                r = _compare(T, Tsize, P, Psize, SA[k + half], &rmatch);
                if(r <= 0) {
                    k += half + 1;
                    half -= (rsize & 1) ^ 1;
                    rlmatch = rmatch;
                } else {
                    rrmatch = rmatch;
                }
            }

            break;
        }
    }
    if (idx) { *idx = (0 < (k - j)) ? j : i; }
    return k - j;
}

trp_obj_t *trp_suf_sais( trp_obj_t *s )
{
    trp_suf_t *obj;
    uns32b n;
    uns8b *T, *p;
    int *SA;
    CORD_pos i;

    if ( s->tipo != TRP_CORD )
        return UNDEF;
    n = ((trp_cord_t *)s)->len + 1;
    T = malloc( n * sizeof( uns8b ) );
    SA = malloc( n * sizeof( int ) );
    if ( ( T == NULL ) || ( SA == NULL ) ) {
        free( T );
        free( SA );
        return UNDEF;
    }
    for ( CORD_set_pos( i, ((trp_cord_t *)s)->c, 0 ), p = T ;
          CORD_pos_valid( i ) ;
          CORD_next( i ) )
        *p++ = CORD_pos_fetch( i );
    *p = 0;
    if ( sais( T, SA, (int)n ) ) {
        free( T );
        free( SA );
        return UNDEF;
    }
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_suf_t ), trp_suf_finalize );
    obj->tipo = TRP_SUF;
    obj->len = n;
    obj->T = T;
    obj->SA = (uns32b *)SA;
    obj->LCP = NULL;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_suf_sa( trp_obj_t *suf, trp_obj_t *idx )
{
    uns32b i, *SA;

    if ( suf->tipo != TRP_SUF )
        return UNDEF;
    if ( ( SA = ((trp_suf_t *)suf)->SA ) == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, ((trp_suf_t *)suf)->len - 1 ) )
        return UNDEF;
    return trp_sig64( SA[ i ] );
}

trp_obj_t *trp_suf_lcp( trp_obj_t *suf, trp_obj_t *idx )
{
    uns32b i, *LCP;

    if ( suf->tipo != TRP_SUF )
        return UNDEF;
    if ( ( LCP = trp_suf_get_lcp_array( (trp_suf_t *)suf ) ) == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, ((trp_suf_t *)suf)->len - 1 ) )
        return UNDEF;
    return trp_sig64( LCP[ i ] );
}

trp_obj_t *trp_suf_search( trp_obj_t *suf, trp_obj_t *pattern )
{
    sig32b s, idx;
    uns32b len;
    uns32b *SA;
    uns8b *p;

    if ( suf->tipo != TRP_SUF )
        return UNDEF;
    if ( ( SA = ((trp_suf_t *)suf)->SA ) == NULL )
        return UNDEF;
    if ( pattern->tipo == TRP_CORD ) {
        uns8b *q;
        CORD_pos i;

        len = ((trp_cord_t *)pattern)->len;
        if ( ( p = malloc( len ) ) == NULL )
            return UNDEF;
        for ( CORD_set_pos( i, ((trp_cord_t *)pattern)->c, 0 ), q = p ;
              CORD_pos_valid( i ) ;
              CORD_next( i ) )
            *q++ = CORD_pos_fetch( i );
    } else {
        p = trp_csprint( pattern );
        len = strlen( p );
    }
    s = sa_search( ((trp_suf_t *)suf)->len, ((trp_suf_t *)suf)->T, len, p, ((trp_suf_t *)suf)->SA, &idx );
    if ( pattern->tipo == TRP_CORD )
        free( p );
    else
        trp_csprint_free( p );
    if ( s < 0 )
        return UNDEF;
    return trp_cons( trp_sig64( s ), s ? trp_sig64( idx ) : UNDEF );
}

static trp_obj_t *trp_suf_lcs_k_low( uns8b flags, trp_obj_t *k, trp_obj_t *s, va_list args1, va_list args2, va_list args3 )
{
    uns32b n = 0, N, K, max_len, max_idx, max_ofs, occ_cnt, min_lcp, first_j, lcp, idx, saj, saj_1, v, w, j;
    uns8b *T;
    int *SA;
    uns32b *pos;
    uns8b *occ;
    uns8b c;

    if ( s->tipo == TRP_CORD ) {
        trp_obj_t *t = s;

        for ( N = 0 ; ; ) {
            n += ((trp_cord_t *)t)->len;
            N++;
            if ( ( t = va_arg( args1, trp_obj_t * ) ) == NULL )
                break;
            if (  t->tipo != TRP_CORD )
                return UNDEF;
        }
    } else {
        trp_obj_t *t = s;
        trp_queue_elem *elem;

        if ( va_arg( args1, trp_obj_t * ) )
            return UNDEF;
        switch ( t->tipo ) {
        case TRP_CONS:
            for ( N = 0 ; ; ) {
                if ( ((trp_cons_t *)t)->car->tipo != TRP_CORD )
                    return UNDEF;
                n += ((trp_cord_t *)(((trp_cons_t *)t)->car))->len;
                N++;
                if ( ( t = ((trp_cons_t *)t)->cdr ) == NIL )
                    break;
                if ( t->tipo != TRP_CONS )
                    return UNDEF;
            }
            break;
        case TRP_QUEUE:
            N = ((trp_queue_t *)t)->len;
            for ( elem = (trp_queue_elem *)( ((trp_queue_t *)t)->first ) ;
                  elem ;
                  elem = (trp_queue_elem *)( elem->next ) ) {
                if ( elem->val->tipo != TRP_CORD )
                    return UNDEF;
                n += ((trp_cord_t *)(elem->val))->len;
            }
            break;
        case TRP_ARRAY:
            N = ((trp_array_t *)t)->len;
            for ( j = 0 ; j < N ; j++ ) {
                if ( (((trp_array_t *)t)->data[ j ])->tipo != TRP_CORD )
                    return UNDEF;
                n += ((trp_cord_t *)(((trp_array_t *)t)->data[ j ]))->len;
            }
            break;
        default:
            return UNDEF;
        }
    }
    if ( N < 2 )
        return UNDEF;
    if ( k ) {
        if ( trp_cast_uns32b( k, &K ) )
            return UNDEF;
        if ( ( K < 2 ) || ( K > N ) )
            return UNDEF;
    } else
        K = N;
    n += N;
    T = malloc( n * sizeof( uns8b ) );
    SA = malloc( n * sizeof( int ) );
    pos = malloc( N * sizeof( uns32b ) );
    occ = malloc( N );
    if( ( T == NULL ) || ( SA == NULL ) || ( pos == NULL ) || ( occ == NULL ) ) {
        free( T );
        free( SA );
        free( pos );
        free( occ );
        return UNDEF;
    }
    if ( s->tipo == TRP_CORD ) {
        trp_obj_t *t = s;
        unsigned char *p = T;
        CORD_pos i;

        for ( j = 0 ; ; ) {
            pos[ j++ ] = (uns32b)( p - T );
            for ( CORD_set_pos( i, ((trp_cord_t *)t)->c, 0 ) ;
                  CORD_pos_valid( i ) ;
                  CORD_next( i ) )
                *p++ = CORD_pos_fetch( i );
            *p++ = 0;
            if ( ( t = va_arg( args2, trp_obj_t * ) ) == NULL )
                break;
        }
    } else {
        trp_obj_t *t = s;
        trp_queue_elem *elem;
        unsigned char *p = T;
        CORD_pos i;

        switch ( t->tipo ) {
        case TRP_CONS:
            for ( j = 0 ; ; ) {
                pos[ j++ ] = (uns32b)( p - T );
                for ( CORD_set_pos( i, ((trp_cord_t *)(((trp_cons_t *)t)->car))->c, 0 ) ;
                      CORD_pos_valid( i ) ;
                      CORD_next( i ) )
                    *p++ = CORD_pos_fetch( i );
                *p++ = 0;
                if ( ( t = ((trp_cons_t *)t)->cdr ) == NIL )
                    break;
            }
            break;
        case TRP_QUEUE:
            for ( elem = (trp_queue_elem *)( ((trp_queue_t *)t)->first ), j = 0 ; ; ) {
                pos[ j++ ] = (uns32b)( p - T );
                for ( CORD_set_pos( i, ((trp_cord_t *)(elem->val))->c, 0 ) ;
                      CORD_pos_valid( i ) ;
                      CORD_next( i ) )
                    *p++ = CORD_pos_fetch( i );
                *p++ = 0;
                if ( ( elem = (trp_queue_elem *)( elem->next ) ) == NULL )
                    break;
            }
            break;
        case TRP_ARRAY:
            for ( j = 0 ; ; ) {
                pos[ j ] = (uns32b)( p - T );
                for ( CORD_set_pos( i, ((trp_cord_t *)((trp_cord_t *)(((trp_array_t *)t)->data[ j ])))->c, 0 ) ;
                      CORD_pos_valid( i ) ;
                      CORD_next( i ) )
                    *p++ = CORD_pos_fetch( i );
                *p++ = 0;
                if ( ++j == N )
                    break;
            }
            break;
        }
    }
    if ( sais( T, SA, (int)n ) ) {
        free( T );
        free( SA );
        free( pos );
        free( occ );
        return UNDEF;
    }
    /*
    for ( j = 0 ; j < n ; j++ ) {
        printf( "# SA[%3u] = %3d  ", j, SA[ j ] );
        for ( v = SA[ j ] ; v < n ; v++ )
            if ( T[ v ] )
                printf( "%c", T[ v ] );
            else
                printf( "$" );
        printf( "\n" );
    }
    */
    max_len = occ_cnt = 0;
    memset( occ, 0, N );
    saj_1 = (uns32b)( SA[ 0 ] ); /* sempre n-1 */
    if ( flags & 1 ) {
        for ( j = 1 ; j < n ; j++ ) {
            saj = (uns32b)( SA[ j ] );
            /*
             FIXME
             vautare se è possibile, nel cacolo di lcp,
             tenere conto anche dei caratteri UTF-8...
             */
            for ( lcp = 0 ; ; lcp++ ) {
                c = T[ saj + lcp ];
                if ( c != T[ saj_1 + lcp ] )
                    break;
                if ( c == 0 )
                    break;
            }
            if ( lcp <= max_len ) {
                if ( occ_cnt ) {
                    occ_cnt = 0;
                    memset( occ, 0, N );
                }
            } else {
                if ( occ_cnt ) {
                    if ( min_lcp > lcp )
                        min_lcp = lcp;
                } else {
                    for ( idx = 0, v = N ; v - idx > 1 ; ) {
                        w = ( v + idx ) >> 1;
                        if ( saj_1 >= pos[ w ] )
                            idx = w;
                        else
                            v = w;
                    }
                    occ[ idx ] = 1;
                    occ_cnt = 1;
                    min_lcp = lcp;
                    first_j = j;
                }
                for ( idx = 0, v = N ; v - idx > 1 ; ) {
                    w = ( v + idx ) >> 1;
                    if ( saj >= pos[ w ] )
                        idx = w;
                    else
                        v = w;
                }
                if ( occ[ idx ] == 0 ) {
                    occ[ idx ] = 1;
                    occ_cnt++;
                    if ( occ_cnt == K ) {
                        max_idx = idx;
                        max_ofs = saj - pos[ idx ];
                        max_len = min_lcp;
                        occ_cnt = 0;
                        memset( occ, 0, N );
                        j = first_j;
                        saj = (uns32b)( SA[ j ] );
                    }
                }
            }
            saj_1 = saj;
        }
        free( T );
    } else {
        /*
         versione O(n), che però usa più spazio
         */
        uns32b *LCP = trp_suf_lcp_array( n, T, (uns32b *)SA );

        free ( T );
        if ( LCP == NULL ) {
            free( SA );
            free( pos );
            free( occ );
            return UNDEF;
        }
        for ( j = 1 ; j < n ; j++ ) {
            saj = (uns32b)( SA[ j ] );
            lcp = LCP[ j ];
            if ( lcp <= max_len ) {
                if ( occ_cnt ) {
                    occ_cnt = 0;
                    memset( occ, 0, N );
                }
            } else {
                if ( occ_cnt ) {
                    if ( min_lcp > lcp )
                        min_lcp = lcp;
                } else {
                    for ( idx = 0, v = N ; v - idx > 1 ; ) {
                        w = ( v + idx ) >> 1;
                        if ( saj_1 >= pos[ w ] )
                            idx = w;
                        else
                            v = w;
                    }
                    occ[ idx ] = 1;
                    occ_cnt = 1;
                    min_lcp = lcp;
                    first_j = j;
                }
                for ( idx = 0, v = N ; v - idx > 1 ; ) {
                    w = ( v + idx ) >> 1;
                    if ( saj >= pos[ w ] )
                        idx = w;
                    else
                        v = w;
                }
                if ( occ[ idx ] == 0 ) {
                    occ[ idx ] = 1;
                    occ_cnt++;
                    if ( occ_cnt == K ) {
                        max_idx = idx;
                        max_ofs = saj - pos[ idx ];
                        max_len = min_lcp;
                        occ_cnt = 0;
                        memset( occ, 0, N );
                        j = first_j;
                        saj = (uns32b)( SA[ j ] );
                    }
                }
            }
            saj_1 = saj;
        }
        free( LCP );
    }
    free( SA );
    free( pos );
    free( occ );
    if ( max_len == 0 )
        return EMPTYCORD;
    if ( s->tipo == TRP_CORD ) {
        for ( ; max_idx ; max_idx-- )
            s = va_arg( args3, trp_obj_t * );
    } else {
        trp_queue_elem *elem;

        switch ( s->tipo ) {
        case TRP_CONS:
            for ( ; max_idx ; max_idx-- )
                s = ((trp_cons_t *)s)->cdr;
            s = ((trp_cons_t *)s)->car;
            break;
        case TRP_QUEUE:
            for ( elem = (trp_queue_elem *)( ((trp_queue_t *)s)->first ) ; max_idx ; max_idx-- )
                elem = (trp_queue_elem *)( elem->next );
            s = elem->val;
            break;
        case TRP_ARRAY:
            s = ((trp_array_t *)s)->data[ max_idx ];
            break;
        }
    }
    return trp_cord_cons( CORD_substr( ((trp_cord_t *)s)->c, max_ofs, max_len ), max_len );
}

trp_obj_t *trp_suf_lcs( trp_obj_t *s, ... )
{
    va_list args1, args2, args3;

    va_start( args1, s );
    va_start( args2, s );
    va_start( args3, s );
    s = trp_suf_lcs_k_low( 0, NULL, s, args1, args2, args3 );
    va_end( args1 );
    va_end( args2 );
    va_end( args3 );
    return s;
}

trp_obj_t *trp_suf_lcs_alt( trp_obj_t *s, ... )
{
    va_list args1, args2, args3;

    va_start( args1, s );
    va_start( args2, s );
    va_start( args3, s );
    s = trp_suf_lcs_k_low( 1, NULL, s, args1, args2, args3 );
    va_end( args1 );
    va_end( args2 );
    va_end( args3 );
    return s;
}

trp_obj_t *trp_suf_lcs_k( trp_obj_t *k, trp_obj_t *s, ... )
{
    va_list args1, args2, args3;

    va_start( args1, s );
    va_start( args2, s );
    va_start( args3, s );
    s = trp_suf_lcs_k_low( 0, k, s, args1, args2, args3 );
    va_end( args1 );
    va_end( args2 );
    va_end( args3 );
    return s;
}

trp_obj_t *trp_suf_lcs_k_alt( trp_obj_t *k, trp_obj_t *s, ... )
{
    va_list args1, args2, args3;

    va_start( args1, s );
    va_start( args2, s );
    va_start( args3, s );
    s = trp_suf_lcs_k_low( 1, k, s, args1, args2, args3 );
    va_end( args1 );
    va_end( args2 );
    va_end( args3 );
    return s;
}

