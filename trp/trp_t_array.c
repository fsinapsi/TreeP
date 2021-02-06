/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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

#define TRP_DEFAULT_SORT_ALG trp_array_quicksort_internal

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );

static uns32b trp_array_max( trp_array_t *a );
static uns8b trp_array_pos_is_invalid( trp_obj_t *a, trp_obj_t *pos, uns32b *val, uns8b shift );
static trp_obj_t *trp_array_ext_internal( trp_obj_t *default_val, uns32b incr, uns32b len );
static trp_obj_t *trp_array_multi_rec( trp_obj_t *default_val, trp_obj_t *l );
static uns8b trp_array_incdec_multi_low( trp_obj_t *a, trp_obj_t *pos, va_list args, objfun_t fun );
static uns8b trp_array_sort_low( trp_obj_t *a, trp_obj_t *cmp, uns8bfun_t alg );
uns8b trp_array_sort_internal( trp_array_t *a, objfun_t *cmp );
static uns8b trp_array_sort_low_low( trp_array_t *a, objfun_t *cmp, uns8bfun_t alg );
static uns8b trp_array_quicksort_internal( trp_array_t *a, objfun_t cmp );
static void trp_array_quicksort_rec( trp_array_t *a, objfun_t cmp, uns32b fr, uns32b to );
static void trp_array_quicksort_perno( trp_array_t *a, objfun_t cmp, uns32b fr, uns32b to, uns32b *pos );
static uns8b trp_array_heapsort_internal( trp_array_t *a, objfun_t cmp );
static uns8b trp_array_mergesort_internal( trp_array_t *a, objfun_t cmp );

uns8b trp_array_print( trp_print_t *p, trp_array_t *obj )
{
    uns32b i;

    if ( trp_print_char( p, '<' ) )
        return 1;
    for ( i = 0 ; i < obj->len ; i++ ) {
        if ( i )
            if ( trp_print_char( p, ' ' ) )
                return 1;
        if ( trp_print_obj( p, obj->data[ i ] ) )
            return 1;
    }
    return trp_print_char( p, '>' );
}

uns32b trp_array_size( trp_array_t *obj )
{
    uns32b sz = 1 + 4 + 4, i;

    for ( i = 0 ; i < obj->len ; i++ )
        sz += trp_size_internal( obj->data[ i ] );
    return sz;
}

void trp_array_encode( trp_array_t *obj, uns8b **buf )
{
    uns32b *p, i;

    **buf = TRP_ARRAY;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->incr );
    (*buf) += 4;
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( i = 0 ; i < obj->len ; i++ )
        trp_encode_internal( obj->data[ i ], buf );
}

trp_obj_t *trp_array_decode( uns8b **buf )
{
    uns32b incr, len, i;
    trp_obj_t *res;

    incr = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    res = trp_array_ext_internal( UNDEF, incr, len );
    for ( i = 0 ; i < len ; i++ )
        ((trp_array_t *)res)->data[ i ] = trp_decode_internal( buf );
    return res;
}

trp_obj_t *trp_array_equal( trp_array_t *o1, trp_array_t *o2 )
{
    uns32b i;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( i = 0 ; i < o1->len ; i++ )
        if ( trp_equal( o1->data[ i ], o2->data[ i ] ) != TRP_TRUE )
            return TRP_FALSE;
    return TRP_TRUE;
}

trp_obj_t *trp_array_less( trp_array_t *o1, trp_array_t *o2 )
{
    return ( o1->len < o2->len ) ? TRP_TRUE : TRP_FALSE;
}

uns8b trp_array_close( trp_array_t *obj )
{
    if ( obj->data ) {
        obj->len = 0;
        trp_gc_free( obj->data );
        obj->data = NULL;
    }
    return 0;
}

trp_obj_t *trp_array_length( trp_array_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_array_nth( uns32b n, trp_array_t *obj )
{
    return ( n < obj->len ) ? obj->data[ n ] : UNDEF;
}

trp_obj_t *trp_array_sub( uns32b start, uns32b len, trp_array_t *obj )
{
    uns32b n, i;
    trp_obj_t *res;

    if ( start > obj->len )
        return UNDEF;
    n = obj->len - start;
    if ( n > len )
        n = len;
    res = trp_array_ext_internal( UNDEF, obj->incr, n );
    for ( i = 0 ; i < n ; )
        ((trp_array_t *)res)->data[ i++ ] = obj->data[ start++ ];
    return res;
}

uns8b trp_array_in( trp_obj_t *obj, trp_array_t *seq, uns32b *pos, uns32b nth )
{
    uns32b i = 0;
    uns8b res = 1;

    for ( ; i < seq->len ; i++ )
        if ( trp_equal( seq->data[ i ], obj ) == TRP_TRUE ) {
            res = 0;
            *pos = i;
            if ( nth == 0 )
                break;
            nth--;
        }
    return res;
}

static uns32b trp_array_max( trp_array_t *a )
{
    return ( ( a->len + a->incr - 1 ) / a->incr ) * a->incr;
}

static uns8b trp_array_pos_is_invalid( trp_obj_t *a, trp_obj_t *pos, uns32b *val, uns8b shift )
{
    sig64b l;

    if ( ( a->tipo != TRP_ARRAY ) || ( pos->tipo != TRP_SIG64 ) )
        return 1;
    l = ((trp_sig64_t *)pos)->val;
    if ( ( l < 0 ) || ( l - shift >= ((trp_array_t *)a)->len ) )
        return 1;
    *val = (uns32b)l;
    return 0;
}

static trp_obj_t *trp_array_ext_internal( trp_obj_t *default_val, uns32b incr, uns32b len )
{
    trp_array_t *obj;

    obj = trp_gc_malloc( sizeof( trp_array_t ) );
    obj->tipo = TRP_ARRAY;
    obj->incr = incr;
    obj->len = len;
    if ( len ) {
        obj->data = trp_gc_malloc( sizeof( trp_obj_t * ) * trp_array_max( obj ) );
        while ( len )
            obj->data[ --len ] = default_val;
    } else {
        obj->data = NULL;
    }
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_array_ext( trp_obj_t *default_val, trp_obj_t *incr, trp_obj_t *len )
{
    uns32b i, l;

    if ( trp_cast_uns32b( incr, &i ) ||
         trp_cast_uns32b( len, &l ) )
        return UNDEF;
    return ( i == 0 ) ? UNDEF : trp_array_ext_internal( default_val, i, l );
}

trp_obj_t *trp_array_multi( trp_obj_t *default_val, trp_obj_t *len, ... )
{
    uns32b i;
    int has_zero = 0;
    trp_obj_t *l, *lnew, *last;
    va_list args;

    va_start( args, len );
    for ( l = NIL ; len ; len = va_arg( args, trp_obj_t * ) ) {
        if ( trp_cast_uns32b( len, &i ) ) {
            va_end( args );
            trp_free_list( l );
            return UNDEF;
        }
        if ( i == 0 )
            has_zero = 1;
        lnew = trp_cons( len, NIL );
        if ( l == NIL )
            l = lnew;
        else
            ((trp_cons_t *)last)->cdr = lnew;
        last = lnew;
    }
    va_end( args );
    if ( l == NIL )
        return UNDEF;
    if ( ((trp_cons_t *)l)->cdr == NIL ) {
        (void)trp_cast_uns32b( ((trp_cons_t *)l)->car, &i );
        last = trp_array_ext_internal( default_val, 10, i );
    } else if ( has_zero ) {
        last = UNDEF;
    } else {
        last = trp_array_multi_rec( default_val, l );
    }
    trp_free_list( l );
    return last;
}

static trp_obj_t *trp_array_multi_rec( trp_obj_t *default_val, trp_obj_t *l )
{
    uns32b i;
    trp_obj_t *len, *a;

    len = ((trp_cons_t *)l)->car;
    l = ((trp_cons_t *)l)->cdr;
    a = trp_array_ext_internal( default_val, 1, (uns32b)( ((trp_sig64_t *)len)->val ) );
    if ( l != NIL )
        for ( i = 0 ; i < ((trp_sig64_t *)len)->val ; i++ )
            trp_array_set( a, trp_sig64( i ),
                           trp_array_multi_rec( default_val, l ) );
    return a;
}

uns8b trp_array_insert( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *obj, ... )
{
    uns32b n, c, l, m;
    va_list args;

    if ( pos ) {
        if ( trp_array_pos_is_invalid( a, pos, &n, 1 ) )
            return 1;
    } else {
        if ( a->tipo != TRP_ARRAY )
            return 1;
        n = ((trp_array_t *)a)->len;
    }
    va_start( args, obj );
    c = trp_nargs( args );
    va_end( args );
    l = ((trp_array_t *)a)->len;
    m = trp_array_max( ((trp_array_t *)a) );
    ((trp_array_t *)a)->len += c;
    if ( l ) {
        if ( c > m - l )
            ((trp_array_t *)a)->data = trp_gc_realloc( ((trp_array_t *)a)->data,
                                                       sizeof( trp_obj_t * ) * trp_array_max( ((trp_array_t *)a) ) );
    } else {
        ((trp_array_t *)a)->data = trp_gc_malloc( sizeof( trp_obj_t * ) * trp_array_max( ((trp_array_t *)a) ) );
    }
    if ( n < l )
        memmove( ((void *)((trp_array_t *)a)->data) + ( n + c ) * sizeof( trp_obj_t * ),
                 ((void *)((trp_array_t *)a)->data) + n * sizeof( trp_obj_t * ),
                 ( l - n ) * sizeof( trp_obj_t * ) );
    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        ((trp_array_t *)a)->data[ n++ ] = obj;
    va_end( args );
    return 0;
}

uns8b trp_array_remove( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *cnt )
{
    uns32b n, c, r, m;

    if ( cnt == NULL )
        cnt = UNO;
    if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) ||
         ( cnt->tipo != TRP_SIG64 ) )
        return 1;
    if ( ((trp_sig64_t *)cnt)->val < 0 )
        return 1;
    if ( ((trp_sig64_t *)cnt)->val == 0 )
        return 0;
    c = ((trp_array_t *)a)->len - n;
    if ( ((trp_sig64_t *)cnt)->val < c ) {
        r = c - ((trp_sig64_t *)cnt)->val;
        c = (uns32b)(((trp_sig64_t *)cnt)->val);
    } else {
        r = 0;
    }
    m = trp_array_max( ((trp_array_t *)a) );
    ((trp_array_t *)a)->len -= c;
    if ( ((trp_array_t *)a)->len ) {
        memmove( ((void *)((trp_array_t *)a)->data) + n * sizeof( trp_obj_t * ),
                 ((void *)((trp_array_t *)a)->data) + ( n + c ) * sizeof( trp_obj_t * ),
                 r * sizeof( trp_obj_t * ) );
        memset( ((void *)((trp_array_t *)a)->data) + ( n + r ) * sizeof( trp_obj_t * ),
                0, c * sizeof( trp_obj_t * ) );
        if ( trp_array_max( ((trp_array_t *)a) ) < m )
            ((trp_array_t *)a)->data = trp_gc_realloc( ((trp_array_t *)a)->data,
                                                       sizeof( trp_obj_t * ) * trp_array_max( ((trp_array_t *)a) ) );
    } else {
        trp_gc_free( ((trp_array_t *)a)->data );
        ((trp_array_t *)a)->data = NULL;
    }
    return 0;
}

uns8b trp_array_set( trp_obj_t *a, trp_obj_t *pos, trp_obj_t *obj )
{
    uns32b n;

    if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) )
        return 1;
    ((trp_array_t *)a)->data[ n ] = obj;
    return 0;
}

uns8b trp_array_set_multi( trp_obj_t *a, trp_obj_t *pos, ... )
{
    uns32b n;
    trp_obj_t *obj, *nxt;
    va_list args;

    va_start( args, pos );
    obj = va_arg( args, trp_obj_t * );
    for ( ; nxt = va_arg( args, trp_obj_t * ) ; obj = nxt ) {
        switch ( a->tipo ) {
        case TRP_ARRAY:
            if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) ) {
                va_end( args );
                return 1;
            }
            a = ((trp_array_t *)a)->data[ n ];
            break;
        case TRP_ASSOC:
            a = trp_assoc_get( a, pos );
            break;
        default:
            va_end( args );
            return 1;
        }
        pos = obj;
    }
    va_end( args );
    switch ( a->tipo ) {
    case TRP_ARRAY:
        if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) )
            return 1;
        ((trp_array_t *)a)->data[ n ] = obj;
        break;
    case TRP_ASSOC:
        return trp_assoc_set( a, pos, obj );
    case TRP_RAW:
        {
            uns32b l = ((trp_raw_t *)a)->len, v;

            if ( l == 0 )
                return 1;
            if ( trp_cast_uns32b_range( pos, &n, 0, l - 1 ) || trp_cast_uns32b_range( obj, &v, 0, 255 ) )
                return 1;
            ((trp_raw_t *)a)->data[ n ] = (uns8b)v;
        }
        break;
    default:
        return 1;
    }
    return 0;
}

static uns8b trp_array_incdec_multi_low( trp_obj_t *a, trp_obj_t *pos, va_list args, objfun_t fun )
{
    uns32b n;
    trp_obj_t *obj, *i;

    for ( ; obj = va_arg( args, trp_obj_t * ) ; pos = obj ) {
        switch ( a->tipo ) {
        case TRP_ARRAY:
            if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) )
                return 1;
            a = ((trp_array_t *)a)->data[ n ];
            break;
        case TRP_ASSOC:
            a = trp_assoc_get( a, pos );
            break;
        default:
            return 1;
        }
    }
    switch ( a->tipo ) {
    case TRP_ARRAY:
        if ( trp_array_pos_is_invalid( a, pos, &n, 0 ) )
            return 1;
        obj = ((trp_array_t *)a)->data[ n ];
        break;
    case TRP_ASSOC:
        obj = trp_assoc_get( a, pos );
        if ( obj == UNDEF )
            obj = ZERO;
        break;
    default:
        return 1;
    }
    if ( i = va_arg( args, trp_obj_t * ) )
        for ( ; ; ) {
            obj = (*fun)( obj, i, NULL );
            if ( ( i = va_arg( args, trp_obj_t * ) ) == NULL )
                break;
        }
    else
        obj = (*fun)( obj, UNO, NULL );
    switch ( a->tipo ) {
    case TRP_ARRAY:
        ((trp_array_t *)a)->data[ n ] = obj;
        break;
    case TRP_ASSOC:
        return trp_assoc_set( a, pos, obj );
    }
    return 0;
}

uns8b trp_array_inc_multi( trp_obj_t *a, trp_obj_t *pos, ... )
{
    va_list args;
    uns8b res;

    va_start( args, pos );
    res = trp_array_incdec_multi_low( a, pos, args, (objfun_t)trp_cat );
    va_end( args );
    return res;
}

uns8b trp_array_dec_multi( trp_obj_t *a, trp_obj_t *pos, ... )
{
    va_list args;
    uns8b res;

    va_start( args, pos );
    res = trp_array_incdec_multi_low( a, pos, args, (objfun_t)trp_math_minus );
    va_end( args );
    return res;
}

uns8b trp_array_sort( trp_obj_t *a, trp_obj_t *cmp )
{
    return trp_array_sort_low( a, cmp, NULL );
}

uns8b trp_array_quicksort( trp_obj_t *a, trp_obj_t *cmp )
{
    return trp_array_sort_low( a, cmp, trp_array_quicksort_internal );
}

uns8b trp_array_heapsort( trp_obj_t *a, trp_obj_t *cmp )
{
    return trp_array_sort_low( a, cmp, trp_array_heapsort_internal );
}

uns8b trp_array_mergesort( trp_obj_t *a, trp_obj_t *cmp )
{
    return trp_array_sort_low( a, cmp, trp_array_mergesort_internal );
}

static uns8b trp_array_sort_low( trp_obj_t *a, trp_obj_t *cmp, uns8bfun_t alg )
{
    if ( a->tipo != TRP_ARRAY )
        return 1;
    if ( cmp ) {
        if ( cmp->tipo != TRP_FUNPTR )
            return 1;
        if ( ((trp_funptr_t *)cmp)->nargs != 2 )
            return 1;
    } else {
        cmp = trp_funptr_less_obj();
    }
    return trp_array_sort_low_low( (trp_array_t *)a, (objfun_t *)( ((trp_funptr_t *)cmp)->f ), alg );
}

uns8b trp_array_sort_internal( trp_array_t *a, objfun_t *cmp )
{
    return trp_array_sort_low_low( a, cmp, NULL );
}

static uns8b trp_array_sort_low_low( trp_array_t *a, objfun_t *cmp, uns8bfun_t alg )
{
    if ( alg == NULL )
        alg = TRP_DEFAULT_SORT_ALG;
    return (alg)( a, cmp );
}

static uns8b trp_array_quicksort_internal( trp_array_t *a, objfun_t cmp )
{
    uns32b n;

    n = a->len;
    if ( n <= 1 )
        return 0;
    trp_array_quicksort_rec( a, cmp, 0, n - 1 );
    return 0;
}

static void trp_array_quicksort_rec( trp_array_t *a, objfun_t cmp, uns32b fr, uns32b to )
{
    uns32b pos;

    trp_array_quicksort_perno( a, cmp, fr, to, &pos );
    if ( pos - fr > 1 )
        trp_array_quicksort_rec( a, cmp, fr, pos - 1 );
    if ( to - pos > 1 )
        trp_array_quicksort_rec( a, cmp, pos + 1, to );
}

static void trp_array_quicksort_perno( trp_array_t *a, objfun_t cmp, uns32b fr, uns32b to, uns32b *pos )
{
    uns32b i1, i2;
    trp_obj_t *tmp;

    i2 = ( fr + to ) >> 1;
    tmp = a->data[ fr ];
    a->data[ fr ] = a->data[ i2 ];
    a->data[ i2 ] = tmp;
    for ( i1 = fr + 1, i2 = to ; ; i1++, i2-- ) {
        for ( ; i1 < i2 ; i1++ )
            if ( (cmp)( a->data[ fr ], a->data[ i1 ] ) == TRP_TRUE )
                break;
        for ( ; i1 <= i2 ; i2-- )
            if ( (cmp)( a->data[ i2 ], a->data[ fr ] ) == TRP_TRUE )
                break;
        if ( i1 >= i2 )
            break;
        tmp = a->data[ i1 ];
        a->data[ i1 ] = a->data[ i2 ];
        a->data[ i2 ] = tmp;
    }
    tmp = a->data[ fr ];
    a->data[ fr ] = a->data[ i2 ];
    a->data[ i2 ] = tmp;
    *pos = i2;
}

static uns8b trp_array_heapsort_internal( trp_array_t *a, objfun_t cmp )
{
    uns32b n, i, j, k;
    trp_obj_t *tmp;

    n = a->len;
    if ( n <= 1 )
        return 0;

    /* costruzione dello heap (O(n)) */

    for ( i = 1 ; i < n ; i++ )
        for ( j = i ; j ; ) {
            k = ( j - 1 ) >> 1;
            if ( (cmp)( a->data[ k ], a->data[ j ] ) == TRP_FALSE )
                break;
            tmp = a->data[ k ];
            a->data[ k ] = a->data[ j ];
            a->data[ j ] = tmp;
            j = k;
        }

    /* riordinamento (O(n log n)) */

    while ( n > 1 ) {
        n--;
        tmp = a->data[ 0 ];
        a->data[ 0 ] = a->data[ n ];
        a->data[ n ] = tmp;
        for ( i = 0 ; ; ) {
            j = ( i << 1 ) + 1;
            if ( j >= n )
                break;
            if ( j + 1 < n )
                if ( (cmp)( a->data[ j ], a->data[ j + 1 ] ) == TRP_TRUE )
                    j++;
            if ( (cmp)( a->data[ i ], a->data[ j ] ) == TRP_FALSE )
                break;
            tmp = a->data[ i ];
            a->data[ i ] = a->data[ j ];
            a->data[ j ] = tmp;
            i = j;
        }
    }
    return 0;
}

static uns8b trp_array_mergesort_internal( trp_array_t *a, objfun_t cmp )
{
    uns32b n, nn, sz, x, y, max, i, j, k;
    trp_obj_t **v, **p1, **p2, **p3;

    n = a->len;
    if ( n <= 1 )
        return 0;
    nn = sizeof( trp_obj_t * ) * n;
    if ( ( v = malloc( nn ) ) == NULL )
        return 1;
    for ( p1 = a->data, p2 = v, sz = 1 ; ; ) {
        for ( x = 0, k = 0 ; ; x = max ) {
            if ( ( y = x + sz ) > n )
                y = n;
            if ( ( max = y + sz ) > n )
                max = n;
            for ( i = x, j = y ; ( i < y ) && ( j < max ) ; ++k )
                if ( (cmp)( p1[ i ], p1[ j ] ) == TRP_TRUE )
                    p2[ k ] = p1[ i++ ];
                else
                    p2[ k ] = p1[ j++ ];
            while ( i < y )
                p2[ k++ ] = p1[ i++ ];
            while ( j < max )
                p2[ k++ ] = p1[ j++ ];
            if ( max == n )
                break;
        }
        sz <<= 1;
        if ( sz >= n )
            break;
        p3 = p1;
        p1 = p2;
        p2 = p3;
    }
    if ( p2 == v )
        memcpy( a->data, v, nn );
    free( v );
    return 0;
}

