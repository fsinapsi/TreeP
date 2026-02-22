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
#include "../trppix/trppix_internal.h"

#define FLOAT_DEFAULT_PRECISION 128
#define RATIO_DECIMAL_DIGITS 60

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );
extern trp_obj_t *trp_date_minus_args( trp_date_t *d, va_list args );

#define trp_math_rand_lock() (void)pthread_mutex_lock(&_trp_math_rand_mutex)
#define trp_math_rand_unlock() (void)pthread_mutex_unlock(&_trp_math_rand_mutex)
static pthread_mutex_t _trp_math_rand_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned long int trp_math_prec( uns8b flags, unsigned long int prec );
static uns8b trp_math_randseed( trp_obj_t *inseed, gmp_randstate_t **outstate, trp_obj_t **outseed );
static gmp_randstate_t *trp_math_randstate();
static trp_obj_t *trp_math_is_negative_internal( trp_obj_t *obj );
void trp_math_sig64_to_mpz( sig64b val, mpz_t mp );
static void trp_math_obj_to_mpq( trp_obj_t *obj, mpq_t mq );
static trp_obj_t *trp_math_result_from_mpz_ext( mpz_t mp, int must_clear );
#define trp_math_result_from_mpz(mp) trp_math_result_from_mpz_ext(mp,1)
static trp_obj_t *trp_math_result_from_mpq( mpq_t mq );
static trp_obj_t *trp_math_result_from_complex( mpq_t re, mpq_t im );
static trp_obj_t *trp_pix_minus_args( trp_pix_t *d, va_list args );
static void trp_math_times_internal( mpq_t rop1, mpq_t rop2, mpq_t op1, mpq_t op2, trp_obj_t *obj, mpz_t ztmp, mpq_t qtmp );
static void trp_math_sqrt_internal( mpq_t op, mpq_t res );
#define PRECISION trp_math_prec(0,0)
#define LLMAXINT 0x7fffffffffffffffLL
#define LLMININT 0x8000000000000000LL

uns8b trp_sig64_print( trp_print_t *p, trp_sig64_t *obj )
{
    return trp_print_sig64( p, obj->val );
}

uns8b trp_mpi_print( trp_print_t *p, trp_mpi_t *obj )
{
    int len;
    uns8b *buf;
    uns8b res;

    len = gmp_asprintf( (char **)&buf, "%Zd", obj->val );
    res = trp_print_chars( p, buf, len );
    trp_gc_free( buf );
    return res;
}

uns8b trp_ratio_print( trp_print_t *p, trp_ratio_t *obj )
{
    mpz_t mp, mr;
    int len, decdigit = RATIO_DECIMAL_DIGITS;
    uns8b *buf;
    uns8b exact = 1;

    mpz_init( mp );
    mpz_set( mp, mpq_numref( obj->val ) );
    if ( mp[ 0 ]._mp_size < 0 ) {
        if ( trp_print_char( p, '-' ) ) {
            mpz_clear( mp );
            return 1;
        }
        mp[ 0 ]._mp_size = -mp[ 0 ]._mp_size;
    }
    mpz_init( mr );
    mpz_ui_pow_ui( mr, 10, decdigit + 1 );
    mpz_mul( mp, mp, mr );
    mpz_fdiv_qr( mp, mr, mp, mpq_denref( obj->val ) );
    if ( mr[ 0 ]._mp_size )
        exact = 0;
    mpz_add_ui( mp, mp, 5 );
    if ( mpz_fdiv_q_ui( mp, mp, 10 ) != 5 )
        exact = 0;
    if ( mp[ 0 ]._mp_size == 0 ) {
        mpz_clear( mr );
        mpz_clear( mp );
        if ( trp_print_char_star( p, "0." ) )
            return 1;
        for ( ; decdigit ; decdigit-- )
            if ( trp_print_char( p, '0' ) )
                return 1;
        return trp_print_char( p, '&' );
    }
    if ( exact )
        while ( mpz_fdiv_q_ui( mr, mp, 10 ) == 0 ) {
            mpz_set( mp, mr );
            decdigit--;
        }
    mpz_clear( mr );
    len = gmp_asprintf( (char **)&buf, "%Zd", mp );
    mpz_clear( mp );
    if ( len > decdigit ) {
        if ( trp_print_chars( p, buf, len - decdigit ) ) {
            trp_gc_free( buf );
            return 1;
        }
        if ( trp_print_char( p, '.' ) ) {
            trp_gc_free( buf );
            return 1;
        }
        if ( trp_print_chars( p, buf + len - decdigit, decdigit ) ) {
            trp_gc_free( buf );
            return 1;
        }
    } else {
        if ( trp_print_char_star( p, "0." ) ) {
            trp_gc_free( buf );
            return 1;
        }
        for ( ; len < decdigit ; decdigit-- )
            if ( trp_print_char( p, '0' ) ) {
                trp_gc_free( buf );
                return 1;
            }
        if ( trp_print_chars( p, buf, len ) ) {
            trp_gc_free( buf );
            return 1;
        }
    }
    trp_gc_free( buf );
    if ( !exact )
        if ( trp_print_char( p, '&' ) )
            return 1;
    return 0;
}

uns8b trp_complex_print( trp_print_t *p, trp_complex_t *obj )
{
    if ( obj->re != ZERO ) {
        if ( trp_print_obj( p, obj->re ) )
            return 1;
        if ( trp_math_is_negative_internal( obj->im ) == TRP_FALSE )
            if ( trp_print_char( p, '+' ) )
                return 1;
    }
    if ( obj->im != UNO ) {
        uns8b toprint = 1;
        if ( obj->im->tipo == TRP_SIG64 )
            if ( ((trp_sig64_t *)obj->im)->val == -1 ) {
                if ( trp_print_char( p, '-' ) )
                    return 1;
                toprint = 0;
            }
        if ( toprint )
            if ( trp_print_obj( p, obj->im ) )
                return 1;
    }
    if ( trp_print_char( p, 'i' ) )
        return 1;
    return 0;
}

uns32b trp_sig64_size( trp_sig64_t *obj )
{
    return 1 + 8;
}

uns32b trp_mpi_size( trp_mpi_t *obj )
{
    size_t cnt;

    trp_gc_free( mpz_export( NULL, &cnt, 1, 1, 1, 0, obj->val ) );
    return 1 + 1 + 4 + cnt;
}

uns32b trp_ratio_size( trp_ratio_t *obj )
{
    size_t cnt1, cnt2;

    trp_gc_free( mpz_export( NULL, &cnt1, 1, 1, 1, 0, mpq_numref( obj->val ) ) );
    trp_gc_free( mpz_export( NULL, &cnt2, 1, 1, 1, 0, mpq_denref( obj->val ) ) );
    return 1 + 1 + 4 + cnt1 + 4 + cnt2;
}

uns32b trp_complex_size( trp_complex_t *obj )
{
    return 1 + trp_size_internal( obj->re ) + trp_size_internal( obj->im );
}

void trp_sig64_encode( trp_sig64_t *obj, uns8b **buf )
{
    sig64b *p;

    **buf = TRP_SIG64;
    ++(*buf);
    p = (sig64b *)(*buf);
    *p = (sig64b)norm64( obj->val );
    (*buf) += 8;
}

void trp_mpi_encode( trp_mpi_t *obj, uns8b **buf )
{
    size_t cnt;
    uns32b *p;

    **buf = TRP_MPI;
    ++(*buf);
    **buf = ( obj->val[ 0 ]._mp_size < 0 ) ? 1 : 0;
    ++(*buf);
    p = (uns32b *)(*buf);
    (*buf) += 4; /* riservo lo spazio per cnt... */
    mpz_export( (void *)*buf, &cnt, 1, 1, 1, 0, obj->val );
    (*buf) += cnt;
    *p = norm32( cnt );
}

void trp_ratio_encode( trp_ratio_t *obj, uns8b **buf )
{
    size_t cnt;
    uns32b *p;

    **buf = TRP_RATIO;
    ++(*buf);
    **buf = ( obj->val[ 0 ]._mp_num._mp_size < 0 ) ? 1 : 0;
    ++(*buf);
    p = (uns32b *)(*buf);
    (*buf) += 4; /* riservo lo spazio per cnt... */
    mpz_export( (void *)*buf, &cnt, 1, 1, 1, 0, mpq_numref( obj->val ) );
    (*buf) += cnt;
    *p = norm32( cnt );
    p = (uns32b *)(*buf);
    (*buf) += 4; /* riservo lo spazio per cnt... */
    mpz_export( (void *)*buf, &cnt, 1, 1, 1, 0, mpq_denref( obj->val ) );
    (*buf) += cnt;
    *p = norm32( cnt );
}

void trp_complex_encode( trp_complex_t *obj, uns8b **buf )
{
    **buf = TRP_COMPLEX;
    ++(*buf);
    trp_encode_internal( obj->re, buf );
    trp_encode_internal( obj->im, buf );
}

trp_obj_t *trp_sig64_decode( uns8b **buf )
{
    trp_obj_t *res = trp_sig64( (sig64b)norm64( *((uns64b *)(*buf)) ) );
    (*buf) += 8;
    return res;
}

trp_obj_t *trp_mpi_decode( uns8b **buf )
{
    size_t cnt;
    uns32b *p;
    mpz_t mp;
    uns8b neg;

    neg = **buf;
    ++(*buf);
    p = (uns32b *)(*buf);
    cnt = (size_t)norm32( *p );
    (*buf) += 4;
    mpz_init( mp );
    mpz_import( mp, cnt, 1, 1, 1, 0, (void *)*buf );
    (*buf) += cnt;
    if ( neg )
        mp[ 0 ]._mp_size = -mp[ 0 ]._mp_size;
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_ratio_decode( uns8b **buf )
{
    size_t cnt;
    uns32b *p;
    mpz_t mpnum, mpden;
    mpq_t mq, mqtmp;
    uns8b neg;

    neg = **buf;
    ++(*buf);
    p = (uns32b *)(*buf);
    cnt = (size_t)norm32( *p );
    (*buf) += 4;
    mpz_init( mpnum );
    mpz_import( mpnum, cnt, 1, 1, 1, 0, (void *)*buf );
    (*buf) += cnt;
    if ( neg )
        mpnum[ 0 ]._mp_size = -mpnum[ 0 ]._mp_size;
    p = (uns32b *)(*buf);
    cnt = (size_t)norm32( *p );
    (*buf) += 4;
    mpz_init( mpden );
    mpz_import( mpden, cnt, 1, 1, 1, 0, (void *)*buf );
    (*buf) += cnt;
    mpq_init( mq );
    mpq_set_z( mq, mpnum );
    mpz_clear( mpnum );
    mpq_init( mqtmp );
    mpq_set_z( mqtmp, mpden );
    mpz_clear( mpden );
    mpq_div( mq, mq, mqtmp );
    mpq_clear( mqtmp );
    return trp_math_result_from_mpq( mq );
}

trp_obj_t *trp_complex_decode( uns8b **buf )
{
    trp_obj_t *re, *im;

    re = trp_decode_internal( buf );
    im = trp_decode_internal( buf );
    return trp_complex( re, im );
}

trp_obj_t *trp_sig64_equal( trp_sig64_t *o1, trp_sig64_t *o2 )
{
    return ( o1->val == o2->val ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_mpi_equal( trp_mpi_t *o1, trp_mpi_t *o2 )
{
    return ( mpz_cmp( o1->val, o2->val ) == 0 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_ratio_equal( trp_ratio_t *o1, trp_ratio_t *o2 )
{
    return mpq_equal( o1->val, o2->val ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_complex_equal( trp_complex_t *o1, trp_complex_t *o2 )
{
    if ( trp_equal( o1->re, o2->re ) != TRP_TRUE )
        return TRP_FALSE;
    return trp_equal( o1->im, o2->im );
}

trp_obj_t *trp_sig64_less( trp_sig64_t *o1, trp_sig64_t *o2 )
{
    return ( o1->val < o2->val ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_mpi_less( trp_mpi_t *o1, trp_mpi_t *o2 )
{
    return ( mpz_cmp( o1->val, o2->val ) < 0 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_ratio_less( trp_ratio_t *o1, trp_ratio_t *o2 )
{
    return ( mpq_cmp( o1->val, o2->val ) < 0 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_complex_less( trp_complex_t *o1, trp_complex_t *o2 )
{
    return trp_less( trp_cat( trp_math_times( o1->re, o1->re, NULL ),
                              trp_math_times( o1->im, o1->im, NULL ),
                              NULL ),
                     trp_cat( trp_math_times( o2->re, o2->re, NULL ),
                              trp_math_times( o2->im, o2->im, NULL ),
                              NULL ) );
}

trp_obj_t *trp_math_less( trp_obj_t *o1, trp_obj_t *o2 )
{
    if ( o1->tipo == TRP_COMPLEX ) {
        o1 = trp_cat( trp_math_times( ((trp_complex_t *)o1)->re,
                                      ((trp_complex_t *)o1)->re,
                                      NULL ),
                      trp_math_times( ((trp_complex_t *)o1)->im,
                                      ((trp_complex_t *)o1)->im,
                                      NULL ),
                      NULL );
        o2 = trp_math_times( o2, o2, NULL );
    } else if ( o2->tipo == TRP_COMPLEX ) {
        o1 = trp_math_times( o1, o1, NULL );
        o2 = trp_cat( trp_math_times( ((trp_complex_t *)o2)->re,
                                      ((trp_complex_t *)o2)->re,
                                      NULL ),
                      trp_math_times( ((trp_complex_t *)o2)->im,
                                      ((trp_complex_t *)o2)->im,
                                      NULL ),
                      NULL );
    }
    return trp_math_is_negative_internal( trp_math_minus( o1, o2, NULL ) );
}

trp_obj_t *trp_sig64_length( trp_sig64_t *obj )
{
    return ( obj->val < 0 )
        ? ( ( (uns64b)( obj->val ) == LLMININT )
            ? trp_cat( TRP_MAXINT, UNO, NULL )
            : trp_sig64( -( obj->val ) ) )
        : (trp_obj_t *)obj;
}

trp_obj_t *trp_mpi_length( trp_mpi_t *obj )
{
    if ( obj->val[ 0 ]._mp_size < 0 ) {
        mpz_t t;
        mpz_init( t );
        mpz_set( t, obj->val );
        t[ 0 ]._mp_size = -t[ 0 ]._mp_size;
        return trp_math_result_from_mpz( t );
    }
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_ratio_length( trp_ratio_t *obj )
{
    if ( obj->val[ 0 ]._mp_num._mp_size < 0 ) {
        mpq_t t;
        mpq_init( t );
        mpq_set( t, obj->val );
        t[ 0 ]._mp_num._mp_size = -t[ 0 ]._mp_num._mp_size;
        return trp_math_result_from_mpq( t );
    }
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_complex_length( trp_complex_t *obj )
{
    if ( obj->re == ZERO )
        return trp_length( obj->im );
    if ( obj->im == ZERO )
        return trp_length( obj->re );
    return trp_math_sqrt( trp_cat( trp_math_times( obj->re,
                                                   obj->re,
                                                   NULL ),
                                   trp_math_times( obj->im,
                                                   obj->im,
                                                   NULL ),
                                   NULL ) );
}

uns8b trp_math_set_prec( trp_obj_t *obj )
{
    uns32b prec;

    if ( trp_cast_uns32b( obj, &prec ) )
        return 1;
    (void)trp_math_prec( 1, (unsigned long int)prec );
    return 0;
}

trp_obj_t *trp_math_get_prec()
{
    return trp_sig64( PRECISION );
}

uns8b trp_math_set_seed( trp_obj_t *obj )
{
    trp_obj_t *seed;
    gmp_randstate_t *state;
    uns8b res;

    if ( ( res = trp_math_randseed( obj, &state, &seed ) ) == 0 )
        trp_math_rand_unlock();
    return res;
}

trp_obj_t *trp_math_get_seed()
{
    trp_obj_t *res;
    gmp_randstate_t *state;

    (void)trp_math_randseed( NULL, &state, &res );
    trp_math_rand_unlock();
    return res;
}

trp_obj_t *trp_math_gmp_version()
{
    return trp_cord( (uns8b *)gmp_version );
}

trp_obj_t *trp_sig64( sig64b val )
{
    trp_sig64_t *obj;

    if ( val == 0 )
        return ZERO;
    if ( val == 1 )
        return UNO;
    if ( val == 10 )
        return DIECI;
    obj = trp_gc_malloc_atomic( sizeof( trp_sig64_t ) );
    obj->tipo = TRP_SIG64;
    obj->val = val;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_uns64( uns64b val )
{
    if ( val & 0x8000000000000000LL )
        return trp_cat( trp_sig64( (sig64b)( val & 0x7fffffffffffffffLL ) ), TRP_MAXINT, UNO, NULL );
    return trp_sig64( (sig64b)val );
}

trp_obj_t *trp_double( double val )
{
    mpq_t mq;

    mpq_init( mq );
    mpq_set_d( mq, val );
    return trp_math_result_from_mpq( mq );
}

trp_obj_t *trp_complex( trp_obj_t *re, trp_obj_t *im )
{
    trp_complex_t *obj;

    obj = trp_gc_malloc( sizeof( trp_complex_t ) );
    obj->tipo = TRP_COMPLEX;
    obj->re = re;
    obj->im = im;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_zero()
{
    static trp_obj_t *zero = NULL;

    if ( zero == NULL ) {
        zero = trp_gc_malloc_atomic( sizeof( trp_sig64_t ) );
        zero->tipo = TRP_SIG64;
        ((trp_sig64_t *)zero)->val = 0;
    }
    return zero;
}

trp_obj_t *trp_uno()
{
    static trp_obj_t *uno = NULL;

    if ( uno == NULL ) {
        uno = trp_gc_malloc_atomic( sizeof( trp_sig64_t ) );
        uno->tipo = TRP_SIG64;
        ((trp_sig64_t *)uno)->val = 1;
    }
    return uno;
}

trp_obj_t *trp_dieci()
{
    static trp_obj_t *dieci = NULL;

    if ( dieci == NULL ) {
        dieci = trp_gc_malloc_atomic( sizeof( trp_sig64_t ) );
        dieci->tipo = TRP_SIG64;
        ((trp_sig64_t *)dieci)->val = 10;
    }
    return dieci;
}

trp_obj_t *trp_maxint()
{
    static trp_obj_t *maxint = NULL;

    if ( maxint == NULL )
        maxint = trp_sig64( LLMAXINT );
    return maxint;
}

trp_obj_t *trp_minint()
{
    static trp_obj_t *minint = NULL;

    if ( minint == NULL )
        minint = trp_sig64( LLMININT );
    return minint;
}

static unsigned long int trp_math_prec( uns8b flags, unsigned long int prec )
{
    static unsigned long int p = FLOAT_DEFAULT_PRECISION;

    if ( flags & 1 )
        p = prec;
    return p;
}

static uns8b trp_math_randseed( trp_obj_t *inseed, gmp_randstate_t **outstate, trp_obj_t **outseed )
{
    static gmp_randstate_t state;
    static trp_obj_t *seed = NULL;

    if ( inseed ) {
        mpz_t mp;

        switch ( inseed->tipo ) {
        case TRP_SIG64:
            mpz_init( mp );
            trp_math_sig64_to_mpz( ((trp_sig64_t *)inseed)->val, mp );
            trp_math_rand_lock();
            if ( seed )
                gmp_randclear( state );
            gmp_randinit_default( state );
            gmp_randseed( state, mp );
            mpz_clear( mp );
            break;
        case TRP_MPI:
            trp_math_rand_lock();
            if ( seed )
                gmp_randclear( state );
            gmp_randinit_default( state );
            gmp_randseed( state, ((trp_mpi_t *)inseed)->val );
            break;
        default:
            return 1;
        }
        seed = inseed;
    } else {
        trp_math_rand_lock();
        if ( seed == NULL ) {
            struct timeval tv;
            mpz_t mp;

            (void)gettimeofday( &tv, NULL );
            mpz_init( mp );
            mpz_set_ui( mp, (unsigned long)( tv.tv_usec ) );
            mpz_mul_2exp( mp, mp, 64 );
            mpz_add_ui( mp, mp, (unsigned long)( tv.tv_sec ) );
            gmp_randinit_default( state );
            gmp_randseed( state, mp );
            seed = trp_math_result_from_mpz( mp );
        }
    }
    *outstate = &state;
    *outseed = seed;
    return 0;
}

static gmp_randstate_t *trp_math_randstate()
{
    trp_obj_t *res;
    gmp_randstate_t *state;

    (void)trp_math_randseed( NULL, &state, &res );
    return state;
}

static trp_obj_t *trp_math_is_negative_internal( trp_obj_t *obj )
{
    trp_obj_t *res;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        res = ( ((trp_sig64_t *)obj)->val < 0 ) ? TRP_TRUE : TRP_FALSE;
        break;
    case TRP_MPI:
        res = ( ((trp_mpi_t *)obj)->val[ 0 ]._mp_size < 0 ) ? TRP_TRUE : TRP_FALSE;
        break;
    case TRP_RATIO:
        res = ( ((trp_ratio_t *)obj)->val[ 0 ]._mp_num._mp_size < 0 ) ? TRP_TRUE : TRP_FALSE;
        break;
    default:
        res = TRP_FALSE;
        break;
    }
    return res;
}

void trp_math_sig64_to_mpz( sig64b val, mpz_t mp )
{
    uns64b v;
    uns8b neg;

    if ( val & LLMININT ) {
        v = ~( (uns64b)val ) + 1;
        neg = 1;
    } else {
        v = (uns64b)val;
        neg = 0;
    }
    mpz_set_ui( mp, (uns32b)( v >> 32 ) );
    mpz_mul_2exp( mp, mp, 32 );
    mpz_add_ui( mp, mp, (uns32b)( v & 0xffffffff ) );
    if ( neg )
        mp[ 0 ]._mp_size = -mp[ 0 ]._mp_size;
}

static void trp_math_obj_to_mpq( trp_obj_t *obj, mpq_t mq )
{
    mpz_t ztmp;

    switch( obj->tipo ) {
    case TRP_SIG64:
        mpz_init( ztmp );
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
        mpq_set_z( mq, ztmp );
        mpz_clear( ztmp );
        break;
    case TRP_MPI:
        mpq_set_z( mq, ((trp_mpi_t *)obj)->val );
        break;
    case TRP_RATIO:
        mpq_set( mq, ((trp_ratio_t *)obj)->val );
        break;
    }
}

static trp_obj_t *trp_math_result_from_mpz_ext( mpz_t mp, int must_clear )
{
    static mpz_t mpmax, mpmin;
    static uns8b first = 1;
    int i;
    trp_mpi_t *obj;

    if ( first ) {
        mpz_init( mpmax );
        mpz_init( mpmin );
        trp_math_sig64_to_mpz( LLMAXINT, mpmax );
        trp_math_sig64_to_mpz( LLMININT, mpmin );
        first = 0;
    }
    i = mp[ 0 ]._mp_size;
    if ( i == 0 ) {
        if ( must_clear )
            mpz_clear( mp );
        return ZERO;
    }
    if ( i < 0 ) {
        i = mpz_cmp( mp, mpmin );
        if ( i == 0 ) {
            if ( must_clear )
                mpz_clear( mp );
            return TRP_MININT;
        }
        if ( i > 0 )  {
            uns64b v;

            v = (uns64b)( mpz_get_ui( mp ) & 0xffffffff );
            mpz_tdiv_q_2exp( mp, mp, 32 );
            v |= ( ( (uns64b)( mpz_get_ui( mp ) & 0xffffffff ) ) << 32 );
            if ( must_clear )
                mpz_clear( mp );
            return trp_sig64( (sig64b)( ~( v - 1 ) ) );
        }
    } else {
        i = mpz_cmp( mp, mpmax );
        if ( i == 0 ) {
            if ( must_clear )
                mpz_clear( mp );
            return TRP_MAXINT;
        }
        if ( i < 0 )  {
            uns64b v;

            v = (uns64b)( mpz_get_ui( mp ) & 0xffffffff );
            mpz_tdiv_q_2exp( mp, mp, 32 );
            v |= ( ( (uns64b)( mpz_get_ui( mp ) & 0xffffffff ) ) << 32 );
            if ( must_clear )
                mpz_clear( mp );
            return trp_sig64( (sig64b)v );
        }
    }
    obj = trp_gc_malloc( sizeof( trp_mpi_t ) );
    obj->tipo = TRP_MPI;
    obj->val[ 0 ]._mp_alloc = mp[ 0 ]._mp_alloc;
    obj->val[ 0 ]._mp_size = mp[ 0 ]._mp_size;
    obj->val[ 0 ]._mp_d = mp[ 0 ]._mp_d;
    mp[ 0 ]._mp_d = NULL;
    if ( must_clear )
        mpz_clear( mp );
    return (trp_obj_t *)obj;
}

static trp_obj_t *trp_math_result_from_mpq( mpq_t mq )
{
    trp_ratio_t *obj;

    if ( mq[ 0 ]._mp_den._mp_size == 1 )
        if ( mq[ 0 ]._mp_den._mp_d[ 0 ] == 1 ) {
            trp_obj_t *res = trp_math_result_from_mpz_ext( mpq_numref( mq ), 0 );
            mpq_clear( mq );
            return res;
        }
    obj = trp_gc_malloc( sizeof( trp_ratio_t ) );
    obj->tipo = TRP_RATIO;
    obj->val[ 0 ] = mq[ 0 ];
    return (trp_obj_t *)obj;
}

static trp_obj_t *trp_math_result_from_complex( mpq_t re, mpq_t im )
{
    if ( im[ 0 ]._mp_num._mp_size == 0 ) {
        mpq_clear( im );
        return trp_math_result_from_mpq( re );
    }
    return trp_complex( trp_math_result_from_mpq( re ),
                        trp_math_result_from_mpq( im ) );
}

trp_obj_t *trp_math_approximate( trp_obj_t *obj )
{
    if ( obj->tipo == TRP_RATIO )
        obj = trp_double( mpq_get_d( ((trp_ratio_t *)obj)->val ) );
    return obj;
}

trp_obj_t *trp_math_num( trp_obj_t *obj )
{
    mpz_t t;

    if ( ( obj->tipo == TRP_SIG64 ) ||
         ( obj->tipo == TRP_MPI ) )
        return obj;
    if ( obj->tipo != TRP_RATIO )
        return UNDEF;
    mpz_init( t );
    mpz_set( t, mpq_numref( ((trp_ratio_t *)obj)->val ) );
    return trp_math_result_from_mpz( t );
}

trp_obj_t *trp_math_den( trp_obj_t *obj )
{
    mpz_t t;

    if ( ( obj->tipo == TRP_SIG64 ) ||
         ( obj->tipo == TRP_MPI ) )
        return UNO;
    if ( obj->tipo != TRP_RATIO )
        return UNDEF;
    mpz_init( t );
    mpz_set( t, mpq_denref( ((trp_ratio_t *)obj)->val ) );
    return trp_math_result_from_mpz( t );
}

trp_obj_t *trp_math_re( trp_obj_t *obj )
{
    if ( ( obj->tipo == TRP_SIG64 ) ||
         ( obj->tipo == TRP_MPI ) ||
         ( obj->tipo == TRP_RATIO ) )
        return obj;
    return ( obj->tipo == TRP_COMPLEX ) ? ((trp_complex_t *)obj)->re : UNDEF;
}

trp_obj_t *trp_math_im( trp_obj_t *obj )
{
    if ( ( obj->tipo == TRP_SIG64 ) ||
         ( obj->tipo == TRP_MPI ) ||
         ( obj->tipo == TRP_RATIO ) )
        return ZERO;
    return ( obj->tipo == TRP_COMPLEX ) ? ((trp_complex_t *)obj)->im : UNDEF;
}

trp_obj_t *trp_math_floor( trp_obj_t *obj )
{
    mpz_t res;

    if ( ( obj->tipo == TRP_SIG64 ) || ( obj->tipo == TRP_MPI ) )
        return obj;
    if ( obj->tipo != TRP_RATIO )
        return UNDEF;
    mpz_init( res );
    mpz_fdiv_q( res,
                mpq_numref( ((trp_ratio_t *)obj)->val ),
                mpq_denref( ((trp_ratio_t *)obj)->val ) );
    return trp_math_result_from_mpz( res );
}

trp_obj_t *trp_math_ceil( trp_obj_t *obj )
{
    mpz_t res;

    if ( ( obj->tipo == TRP_SIG64 ) || ( obj->tipo == TRP_MPI ) )
        return obj;
    if ( obj->tipo != TRP_RATIO )
        return UNDEF;
    mpz_init( res );
    mpz_cdiv_q( res,
                mpq_numref( ((trp_ratio_t *)obj)->val ),
                mpq_denref( ((trp_ratio_t *)obj)->val ) );
    return trp_math_result_from_mpz( res );
}

trp_obj_t *trp_math_rint( trp_obj_t *obj )
{
    mpz_t res;

    if ( ( obj->tipo == TRP_SIG64 ) || ( obj->tipo == TRP_MPI ) )
        return obj;
    if ( obj->tipo != TRP_RATIO )
        return UNDEF;
    mpz_init( res );
    mpz_set( res, mpq_denref( ((trp_ratio_t *)obj)->val ) );
    mpz_fdiv_q_2exp( res, res, 1 );
    mpz_add( res, res, mpq_numref( ((trp_ratio_t *)obj)->val ) );
    mpz_fdiv_q( res, res, mpq_denref( ((trp_ratio_t *)obj)->val ) );
    return trp_math_result_from_mpz( res );
}

trp_obj_t *trp_math_gcd( trp_obj_t *obj, ... )
{
    va_list args;
    mpz_t res;

    mpz_init( res );
    mpz_set_ui( res, 0 );
    va_start( args, obj );
    for ( ; ; ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            {
                mpz_t mp;
                mpz_init( mp );
                trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
                mpz_gcd( res, res, mp );
                mpz_clear( mp );
            }
            break;
        case TRP_MPI:
            mpz_gcd( res, res, ((trp_mpi_t *)obj)->val );
            break;
        default:
            va_end( args );
            mpz_clear( res );
            return UNDEF;
        }
        if ( ( obj = va_arg( args, trp_obj_t * ) ) == NULL )
            break;
    }
    va_end( args );
    return trp_math_result_from_mpz( res );
}

trp_obj_t *trp_math_lcm( trp_obj_t *obj, ... )
{
    va_list args;
    mpz_t res;

    mpz_init( res );
    mpz_set_ui( res, 1 );
    va_start( args, obj );
    for ( ; ; ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            {
                mpz_t mp;
                mpz_init( mp );
                trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
                mpz_lcm( res, res, mp );
                mpz_clear( mp );
            }
            break;
        case TRP_MPI:
            mpz_lcm( res, res, ((trp_mpi_t *)obj)->val );
            break;
        default:
            va_end( args );
            mpz_clear( res );
            return UNDEF;
        }
        if ( ( obj = va_arg( args, trp_obj_t * ) ) == NULL )
            break;
    }
    va_end( args );
    return trp_math_result_from_mpz( res );
}

trp_obj_t *trp_math_fac( trp_obj_t *obj )
{
    uns32b n;
    mpz_t mp;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    mpz_init( mp );
    mpz_fac_ui( mp, n );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_mfac( trp_obj_t *obj, trp_obj_t *mobj )
{
    uns32b n, m;
    mpz_t mp;

    if ( ( trp_cast_uns32b( obj, &n ) ) ||
         ( trp_cast_uns32b( mobj, &m ) ) )
        return UNDEF;
    mpz_init( mp );
    mpz_mfac_uiui( mp, n, m );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_primorial( trp_obj_t *obj )
{
    uns32b n;
    mpz_t mp;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    mpz_init( mp );
    mpz_primorial_ui( mp, n );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_bin( trp_obj_t *objn, trp_obj_t *objk )
{
    uns32b k;
    mpz_t mp;

    if ( trp_cast_uns32b( objk, &k ) )
        return UNDEF;
    switch ( objn->tipo ) {
    case TRP_SIG64:
        mpz_init( mp );
        trp_math_sig64_to_mpz( ((trp_sig64_t *)objn)->val, mp );
        mpz_bin_ui( mp, mp, k );
        break;
    case TRP_MPI:
        mpz_init( mp );
        mpz_bin_ui( mp, ((trp_mpi_t *)objn)->val, k );
        break;
    default:
        return UNDEF;
    }
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_fib( trp_obj_t *obj )
{
    uns32b n;
    mpz_t mp;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    mpz_init( mp );
    mpz_fib_ui( mp, n );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_lucnum( trp_obj_t *obj )
{
    uns32b n;
    mpz_t mp;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    mpz_init( mp );
    mpz_lucnum_ui( mp, n );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_probab_isprime( trp_obj_t *obj, trp_obj_t *reps )
{
    int prime;
    uns32b nreps;

    if ( reps ) {
        if ( trp_cast_uns32b_range( reps, &nreps, 15, 50 ) )
            return UNDEF;
    } else
        nreps = 25;
    switch ( obj->tipo ) {
    case TRP_SIG64:
        {
            mpz_t mp;
            mpz_init( mp );
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
            prime = mpz_probab_prime_p( mp, (int)nreps );
            mpz_clear( mp );
        }
        break;
    case TRP_MPI:
        prime = mpz_probab_prime_p( ((trp_mpi_t *)obj)->val, (int)nreps );
        break;
    default:
        return UNDEF;
    }
    return trp_sig64( (sig64b)prime );
}

static int trp_math_isprime_low( mpz_t mp )
{
    extern int aks( ... );
    int prime;

    if ( mpz_sgn( mp ) < 0 )
        return 0;
    if ( ( prime = mpz_probab_prime_p( mp, 25 ) ) == 1 )
        prime = aks( mp );
    return prime ? 1 : 0;
}

trp_obj_t *trp_math_isprime( trp_obj_t *obj )
{
    int prime;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        {
            mpz_t mp;
            mpz_init( mp );
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
            prime = trp_math_isprime_low( mp );
            mpz_clear( mp );
        }
        break;
    case TRP_MPI:
        prime = trp_math_isprime_low( ((trp_mpi_t *)obj)->val );
        break;
    default:
        return UNDEF;
    }
    return prime ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_math_nextprime( trp_obj_t *obj )
{
    mpz_t mp;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        mpz_init( mp );
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
        mpz_nextprime( mp, mp );
        break;
    case TRP_MPI:
        mpz_init( mp );
        mpz_nextprime( mp, ((trp_mpi_t *)obj)->val );
        break;
    default:
        return UNDEF;
    }
//    while ( trp_math_isprime_low( mp ) == 0 )
//        mpz_nextprime( mp, mp );
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_perfect_power( trp_obj_t *obj )
{
    int perfect;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        {
            mpz_t mp;
            mpz_init( mp );
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
            perfect = mpz_perfect_power_p( mp );
            mpz_clear( mp );
        }
        break;
    case TRP_MPI:
        perfect = mpz_perfect_power_p( ((trp_mpi_t *)obj)->val );
        break;
    default:
        return UNDEF;
    }
    return perfect ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_math_perfect_square( trp_obj_t *obj )
{
    int perfect;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        {
            mpz_t mp;
            mpz_init( mp );
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp );
            perfect = mpz_perfect_square_p( mp );
            mpz_clear( mp );
        }
        break;
    case TRP_MPI:
        perfect = mpz_perfect_square_p( ((trp_mpi_t *)obj)->val );
        break;
    default:
        return UNDEF;
    }
    return perfect ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_math_random( trp_obj_t *obj )
{
    mpz_t mp, mp2;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        if ( ((trp_sig64_t *)obj)->val <= 0 )
            return UNDEF;
        mpz_init( mp2 );
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, mp2 );
        mpz_init( mp );
        mpz_urandomm( mp, *trp_math_randstate(), mp2 );
        trp_math_rand_unlock();
        mpz_clear( mp2 );
        break;
    case TRP_MPI:
        if ( ((trp_mpi_t *)obj)->val[ 0 ]._mp_size < 0 )
            return UNDEF;
        mpz_init( mp );
        mpz_urandomm( mp, *trp_math_randstate(), ((trp_mpi_t *)obj)->val );
        trp_math_rand_unlock();
        break;
    default:
        return UNDEF;
    }
    return trp_math_result_from_mpz( mp );
}

trp_obj_t *trp_math_cat( trp_obj_t *obj, va_list args )
{
    mpq_t re, im, qtmp;
    mpz_t ztmp;

    mpq_init( re );
    mpq_init( im );
    mpq_init( qtmp );
    mpz_init( ztmp );
    while ( obj ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
            mpq_set_z( qtmp, ztmp );
            mpq_add( re, re, qtmp );
            obj = va_arg( args, trp_obj_t * );
            break;
        case TRP_MPI:
            mpq_set_z( qtmp, ((trp_mpi_t *)obj)->val );
            mpq_add( re, re, qtmp );
            obj = va_arg( args, trp_obj_t * );
            break;
        case TRP_RATIO:
            mpq_add( re, re, ((trp_ratio_t *)obj)->val );
            obj = va_arg( args, trp_obj_t * );
            break;
        case TRP_COMPLEX:
            switch ( ((trp_complex_t *)obj)->im->tipo ) {
            case TRP_SIG64:
                trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->im))->val, ztmp );
                mpq_set_z( qtmp, ztmp );
                mpq_add( im, im, qtmp );
                break;
            case TRP_MPI:
                mpq_set_z( qtmp, ((trp_mpi_t *)(((trp_complex_t *)obj)->im))->val );
                mpq_add( im, im, qtmp );
                break;
            case TRP_RATIO:
                mpq_add( im, im, ((trp_ratio_t *)(((trp_complex_t *)obj)->im))->val );
                break;
            }
            obj = ((trp_complex_t *)obj)->re;
            break;
        default:
            mpz_clear( ztmp );
            mpq_clear( qtmp );
            mpq_clear( im );
            mpq_clear( re );
            return UNDEF;
        }
    }
    mpz_clear( ztmp );
    mpq_clear( qtmp );
    return trp_math_result_from_complex( re, im );
}

trp_obj_t *trp_math_minus( trp_obj_t *obj, ... )
{
    mpq_t re, im, qtmp;
    mpz_t ztmp;
    va_list args;
    voidfun_t op;

    va_start( args, obj );
    if ( obj->tipo == TRP_DATE ) {
        obj = trp_date_minus_args( (trp_date_t *)obj, args );
        va_end( args );
        return obj;
    }
    if ( obj->tipo == TRP_PIX ) {
        obj = trp_pix_minus_args( (trp_pix_t *)obj, args );
        va_end( args );
        return obj;
    }
    mpq_init( re );
    mpq_init( im );
    mpq_init( qtmp );
    mpz_init( ztmp );
    op = mpq_add;
    while ( obj ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
            mpq_set_z( qtmp, ztmp );
            (op)( re, re, qtmp );
            obj = va_arg( args, trp_obj_t * );
            op = mpq_sub;
            break;
        case TRP_MPI:
            mpq_set_z( qtmp, ((trp_mpi_t *)obj)->val );
            (op)( re, re, qtmp );
            obj = va_arg( args, trp_obj_t * );
            op = mpq_sub;
            break;
        case TRP_RATIO:
            (op)( re, re, ((trp_ratio_t *)obj)->val );
            obj = va_arg( args, trp_obj_t * );
            op = mpq_sub;
            break;
        case TRP_COMPLEX:
            switch ( ((trp_complex_t *)obj)->im->tipo ) {
            case TRP_SIG64:
                trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->im))->val, ztmp );
                mpq_set_z( qtmp, ztmp );
                (op)( im, im, qtmp );
                break;
            case TRP_MPI:
                mpq_set_z( qtmp, ((trp_mpi_t *)(((trp_complex_t *)obj)->im))->val );
                (op)( im, im, qtmp );
                break;
            case TRP_RATIO:
                (op)( im, im, ((trp_ratio_t *)(((trp_complex_t *)obj)->im))->val );
                break;
            }
            obj = ((trp_complex_t *)obj)->re;
            break;
        default:
            va_end( args );
            mpz_clear( ztmp );
            mpq_clear( qtmp );
            mpq_clear( im );
            mpq_clear( re );
            return UNDEF;
        }
    }
    va_end( args );
    mpz_clear( ztmp );
    mpq_clear( qtmp );
    return trp_math_result_from_complex( re, im );
}

#define trp_diff(x,y) (((x)>=(y))?((x)-(y)):((y)-(x)))
#define colorval(v) (((v)+128)/257)

static trp_obj_t *trp_pix_minus_args( trp_pix_t *d, va_list args )
{
    trp_pix_t *obj = va_arg( args, trp_pix_t * );
    double res;
    uns16b i;

    if ( obj == NULL )
        return UNDEF;
    if ( ( obj->tipo != TRP_PIX ) || va_arg( args, void * ) )
        return UNDEF;
    if ( d->sottotipo || obj->sottotipo )
        return UNDEF;
    res = 0.0;
    i = colorval( trp_diff( d->color.red, obj->color.red ) );
    res += (double)( i * i );
    i = colorval( trp_diff( d->color.green, obj->color.green ) );
    res += (double)( i * i );
    i = colorval( trp_diff( d->color.blue, obj->color.blue ) );
    res += (double)( i * i );
    return trp_double( sqrt( res / 3.0 ) );
}

static void trp_math_times_internal( mpq_t rop1, mpq_t rop2, mpq_t op1, mpq_t op2, trp_obj_t *obj, mpz_t ztmp, mpq_t qtmp )
{
    switch ( obj->tipo ) {
    case TRP_SIG64:
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
        mpq_set_z( qtmp, ztmp );
        mpq_mul( rop1, op1, qtmp );
        mpq_mul( rop2, op2, qtmp );
        break;
    case TRP_MPI:
        mpq_set_z( qtmp, ((trp_mpi_t *)obj)->val );
        mpq_mul( rop1, op1, qtmp );
        mpq_mul( rop2, op2, qtmp );
        break;
    case TRP_RATIO:
        mpq_mul( rop1, op1, ((trp_ratio_t *)obj)->val );
        mpq_mul( rop2, op2, ((trp_ratio_t *)obj)->val );
        break;
    }
}

trp_obj_t *trp_math_times( trp_obj_t *obj, ... )
{
    mpq_t re, im, qtmp, qaux1, qaux2;
    mpz_t ztmp;
    va_list args;

    mpq_init( re );
    mpq_init( im );
    mpq_init( qtmp );
    mpq_init( qaux1 );
    mpq_init( qaux2 );
    mpz_init( ztmp );
    mpq_set_ui( re, 1, 1 );
    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
            mpq_set_z( qtmp, ztmp );
            mpq_mul( re, re, qtmp );
            mpq_mul( im, im, qtmp );
            break;
        case TRP_MPI:
            mpq_set_z( qtmp, ((trp_mpi_t *)obj)->val );
            mpq_mul( re, re, qtmp );
            mpq_mul( im, im, qtmp );
            break;
        case TRP_RATIO:
            mpq_mul( re, re, ((trp_ratio_t *)obj)->val );
            mpq_mul( im, im, ((trp_ratio_t *)obj)->val );
            break;
        case TRP_COMPLEX:
            trp_math_times_internal( qaux1, qaux2, re, im,
                                     ((trp_complex_t *)obj)->im,
                                     ztmp, qtmp );
            trp_math_times_internal( re, im, re, im,
                                     ((trp_complex_t *)obj)->re,
                                     ztmp, qtmp );
            mpq_sub( re, re, qaux2 );
            mpq_add( im, im, qaux1 );
            break;
        default:
            va_end( args );
            mpz_clear( ztmp );
            mpq_clear( qaux2 );
            mpq_clear( qaux1 );
            mpq_clear( qtmp );
            mpq_clear( im );
            mpq_clear( re );
            return UNDEF;
        }
    }
    va_end( args );
    mpz_clear( ztmp );
    mpq_clear( qaux2 );
    mpq_clear( qaux1 );
    mpq_clear( qtmp );
    return trp_math_result_from_complex( re, im );
}

trp_obj_t *trp_math_ratio( trp_obj_t *obj, ... )
{
    mpq_t re, im, qtmp, qaux1, qaux2, qaux3;
    mpz_t ztmp;
    va_list args;

    mpq_init( re );
    mpq_init( im );
    mpz_init( ztmp );
    switch ( obj->tipo ) {
    case TRP_SIG64:
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
        mpq_set_z( re, ztmp );
        break;
    case TRP_MPI:
        mpq_set_z( re, ((trp_mpi_t *)obj)->val );
        break;
    case TRP_RATIO:
        mpq_set( re, ((trp_ratio_t *)obj)->val );
        break;
    case TRP_COMPLEX:
        switch ( ((trp_complex_t *)obj)->re->tipo ) {
        case TRP_SIG64:
            trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->re))->val, ztmp );
            mpq_set_z( re, ztmp );
            break;
        case TRP_MPI:
            mpq_set_z( re, ((trp_mpi_t *)(((trp_complex_t *)obj)->re))->val );
            break;
        case TRP_RATIO:
            mpq_set( re, ((trp_ratio_t *)(((trp_complex_t *)obj)->re))->val );
            break;
        }
        switch ( ((trp_complex_t *)obj)->im->tipo ) {
        case TRP_SIG64:
            trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->im))->val, ztmp );
            mpq_set_z( im, ztmp );
            break;
        case TRP_MPI:
            mpq_set_z( im, ((trp_mpi_t *)(((trp_complex_t *)obj)->im))->val );
            break;
        case TRP_RATIO:
            mpq_set( im, ((trp_ratio_t *)(((trp_complex_t *)obj)->im))->val );
            break;
        }
        break;
    default:
        mpz_clear( ztmp );
        mpq_clear( im );
        mpq_clear( re );
        return UNDEF;
    }
    mpq_init( qtmp );
    mpq_init( qaux1 );
    mpq_init( qaux2 );
    mpq_init( qaux3 );
    va_start( args, obj );
    for ( obj = va_arg( args, trp_obj_t * ) ;
          obj ;
          obj = va_arg( args, trp_obj_t * ) ) {
        switch ( obj->tipo ) {
        case TRP_SIG64:
            if ( obj == ZERO ) {
                va_end( args );
                mpq_clear( qaux3 );
                mpq_clear( qaux2 );
                mpq_clear( qaux1 );
                mpq_clear( qtmp );
                mpz_clear( ztmp );
                mpq_clear( im );
                mpq_clear( re );
                return UNDEF;
            }
            trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
            mpq_set_z( qtmp, ztmp );
            mpq_div( re, re, qtmp );
            mpq_div( im, im, qtmp );
            break;
        case TRP_MPI:
            mpq_set_z( qtmp, ((trp_mpi_t *)obj)->val );
            mpq_div( re, re, qtmp );
            mpq_div( im, im, qtmp );
            break;
        case TRP_RATIO:
            mpq_div( re, re, ((trp_ratio_t *)obj)->val );
            mpq_div( im, im, ((trp_ratio_t *)obj)->val );
            break;
        case TRP_COMPLEX:
            switch ( ((trp_complex_t *)obj)->im->tipo ) {
            case TRP_SIG64:
                trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->im))->val, ztmp );
                mpq_set_z( qaux2, ztmp );
                break;
            case TRP_MPI:
                mpq_set_z( qaux2, ((trp_mpi_t *)(((trp_complex_t *)obj)->im))->val );
                break;
            case TRP_RATIO:
                mpq_set( qaux2, ((trp_ratio_t *)(((trp_complex_t *)obj)->im))->val );
                break;
            }
            if ( ((trp_complex_t *)obj)->re == ZERO ) {
                mpq_set( qtmp, re );
                qtmp[ 0 ]._mp_num._mp_size = -qtmp[ 0 ]._mp_num._mp_size;
                mpq_div( re, im, qaux2 );
                mpq_div( im, qtmp, qaux2 );
            } else {
                switch ( ((trp_complex_t *)obj)->re->tipo ) {
                case TRP_SIG64:
                    trp_math_sig64_to_mpz( ((trp_sig64_t *)(((trp_complex_t *)obj)->re))->val, ztmp );
                    mpq_set_z( qaux1, ztmp );
                    break;
                case TRP_MPI:
                    mpq_set_z( qaux1, ((trp_mpi_t *)(((trp_complex_t *)obj)->re))->val );
                    break;
                case TRP_RATIO:
                    mpq_set( qaux1, ((trp_ratio_t *)(((trp_complex_t *)obj)->re))->val );
                    break;
                }
                mpq_mul( qaux3, qaux1, qaux1 );
                mpq_mul( qtmp, qaux2, qaux2 );
                mpq_add( qtmp, qtmp, qaux3 );
                mpq_div( qaux1, qaux1, qtmp );
                mpq_div( qaux2, qaux2, qtmp );
                mpq_mul( qaux3, re, qaux1 );
                mpq_mul( qtmp, re, qaux2 );
                mpq_mul( re, im, qaux2 );
                mpq_add( re, re, qaux3 );
                mpq_mul( im, im, qaux1 );
                mpq_sub( im, im, qtmp );
            }
            break;
        default:
            va_end( args );
            mpq_clear( qaux3 );
            mpq_clear( qaux2 );
            mpq_clear( qaux1 );
            mpq_clear( qtmp );
            mpz_clear( ztmp );
            mpq_clear( im );
            mpq_clear( re );
            return UNDEF;
        }
    }
    va_end( args );
    mpq_clear( qaux3 );
    mpq_clear( qaux2 );
    mpq_clear( qaux1 );
    mpq_clear( qtmp );
    mpz_clear( ztmp );
    return trp_math_result_from_complex( re, im );
}

trp_obj_t *trp_math_div( trp_obj_t *o1, trp_obj_t *o2 )
{
    /*
     FIXME
     andrebbe riscritto meglio
     */
    return trp_math_floor( trp_math_ratio( o1, o2, NULL ) );
}

trp_obj_t *trp_math_mod( trp_obj_t *o1, trp_obj_t *o2 )
{
    /*
     FIXME
     andrebbe riscritto meglio
     */
    return trp_math_minus( o1,
                           trp_math_times( trp_math_div( o1, o2 ),
                                           o2, NULL ),
                           NULL );
}

static void trp_math_sqrt_internal( mpq_t op, mpq_t res )
{
    mpf_t ftmp;

    if ( mpz_perfect_square_p( mpq_numref( op ) ) )
        if ( mpz_perfect_square_p( mpq_denref( op ) ) ) {
            mpz_t num, den;
            mpq_t qtmp;
            mpz_init( num );
            mpz_sqrt( num, mpq_numref( op ) );
            mpz_init( den );
            mpz_sqrt( den, mpq_denref( op ) );
            mpq_set_z( res, num );
            mpz_clear( num );
            mpq_init( qtmp );
            mpq_set_z( qtmp, den );
            mpz_clear( den );
            mpq_div( res, res, qtmp );
            mpq_clear( qtmp );
            return;
        }
    mpf_init2( ftmp, PRECISION );
    mpf_set_q( ftmp, op );
    mpf_sqrt( ftmp, ftmp );
    mpq_set_f( res, ftmp );
    mpf_clear( ftmp );
}

trp_obj_t *trp_math_sqrt( trp_obj_t *obj )
{
    mpq_t re, im, t1, t2;
    mpz_t ztmp;

    switch ( obj->tipo ) {
    case TRP_SIG64:
        mpq_init( re );
        mpq_init( im );
        mpz_init( ztmp );
        trp_math_sig64_to_mpz( ((trp_sig64_t *)obj)->val, ztmp );
        if ( ((trp_sig64_t *)obj)->val < 0 ) {
            ztmp[ 0 ]._mp_size = -ztmp[ 0 ]._mp_size;
            mpq_set_z( im, ztmp );
            trp_math_sqrt_internal( im, im );
        } else {
            mpq_set_z( re, ztmp );
            trp_math_sqrt_internal( re, re );
        }
        mpz_clear( ztmp );
        break;
    case TRP_MPI:
        mpq_init( re );
        mpq_init( im );
        if ( ((trp_mpi_t *)obj)->val[ 0 ]._mp_size < 0 ) {
            mpq_set_z( im, ((trp_mpi_t *)obj)->val );
            im[ 0 ]._mp_num._mp_size = -im[ 0 ]._mp_num._mp_size;
            trp_math_sqrt_internal( im, im );
        } else {
            mpq_set_z( re, ((trp_mpi_t *)obj)->val );
            trp_math_sqrt_internal( re, re );
        }
        break;
    case TRP_RATIO:
        mpq_init( re );
        mpq_init( im );
        if ( ((trp_ratio_t *)obj)->val[ 0 ]._mp_num._mp_size < 0 ) {
            mpq_set( im, ((trp_ratio_t *)obj)->val );
            im[ 0 ]._mp_num._mp_size = -im[ 0 ]._mp_num._mp_size;
            trp_math_sqrt_internal( im, im );
        } else {
            trp_math_sqrt_internal( ((trp_ratio_t *)obj)->val, re );
        }
        break;
    case TRP_COMPLEX:
        mpq_init( t1 );
        mpq_init( t2 );
        mpq_init( re );
        mpq_init( im );
        trp_math_obj_to_mpq( ((trp_complex_t *)obj)->re, re );
        trp_math_obj_to_mpq( ((trp_complex_t *)obj)->im, im );
        mpq_mul( t1, re, re );
        mpq_mul( t2, im, im );
        mpq_add( t1, t1, t2 );
        mpq_clear( t2 );
        trp_math_sqrt_internal( t1, t1 );
        mpq_add( re, re, t1 );
        mpq_clear( t1 );
        mpq_div_2exp( re, re, 1 );
        trp_math_sqrt_internal( re, re );
        mpq_div( im, im, re );
        mpq_div_2exp( im, im, 1 );
        break;
    default:
        return UNDEF;
    }
    return trp_math_result_from_complex( re, im );
}

trp_obj_t *trp_math_pow( trp_obj_t *n, trp_obj_t *m )
{
    if ( n == ZERO )
        return ( m == ZERO ) ? UNO : ZERO;
    if ( ( n == UNO ) || ( m == ZERO ) )
        return UNO;
    if ( m == UNO )
        return n;
    if ( m->tipo == TRP_RATIO ) {
        if ( mpq_cmp_ui( ((trp_ratio_t *)m)->val, 1, 2 ) == 0 )
            return trp_math_sqrt( n );
        if ( mpq_cmp_si( ((trp_ratio_t *)m)->val, -1, 2 ) == 0 )
            return trp_math_ratio( UNO, trp_math_sqrt( n ), NULL );
        /*
         * FIXME
         */
        {
            double nn, mm;

            if ( trp_cast_flt64b( n, &nn ) || trp_cast_flt64b( m, &mm ) )
                return UNDEF;
            return trp_double( pow( nn, mm ) );
        }
    }
    if ( m->tipo != TRP_SIG64 ) {
        /* FIXME */
        return UNDEF;
    }
    switch ( n->tipo ) {
    case TRP_SIG64:
    case TRP_MPI:
        if ( ((trp_sig64_t *)m)->val > 0 ) {
            unsigned long int mm = (unsigned long int)( ((trp_sig64_t *)m)->val );
            if ( ((sig64b)mm) == ((trp_sig64_t *)m)->val ) {
                mpz_t mp;

                mpz_init( mp );
                if ( n->tipo == TRP_SIG64 )
                    trp_math_sig64_to_mpz( ((trp_sig64_t *)n)->val, mp );
                else
                    mpz_set( mp, ((trp_mpi_t *)n)->val );
                mpz_pow_ui( mp, mp, mm );
                return trp_math_result_from_mpz( mp );
            }
        } else {
            unsigned long int mm = (unsigned long int)( -(((trp_sig64_t *)m)->val) );
            if ( -((sig64b)mm) == ((trp_sig64_t *)m)->val ) {
                mpz_t mp;
                mpq_t mq;

                mpz_init( mp );
                if ( n->tipo == TRP_SIG64 )
                    trp_math_sig64_to_mpz( ((trp_sig64_t *)n)->val, mp );
                else
                    mpz_set( mp, ((trp_mpi_t *)n)->val );
                mpz_pow_ui( mp, mp, mm );
                mpq_init( mq );
                mpq_set_z( mq, mp );
                mpz_clear( mp );
                mpq_inv( mq, mq );
                return trp_math_result_from_mpq( mq );
            }
        }
        return UNDEF;
    case TRP_RATIO:
        if ( ((trp_sig64_t *)m)->val > 0 ) {
            unsigned long int mm = (unsigned long int)( ((trp_sig64_t *)m)->val );
            if ( ((sig64b)mm) == ((trp_sig64_t *)m)->val ) {
                mpq_t mq;

                mpq_init( mq );
                mpq_set ( mq, ((trp_ratio_t *)n)->val );
                mpz_pow_ui( mpq_numref ( mq ), mpq_numref ( mq ), mm );
                mpz_pow_ui( mpq_denref ( mq ), mpq_denref ( mq ), mm );
                mpq_canonicalize( mq );
                return trp_math_result_from_mpq( mq );
            }
        } else {
            unsigned long int mm = (unsigned long int)( -(((trp_sig64_t *)m)->val) );
            if ( -((sig64b)mm) == ((trp_sig64_t *)m)->val ) {
                mpq_t mq;

                mpq_init( mq );
                mpq_set ( mq, ((trp_ratio_t *)n)->val );
                mpz_pow_ui( mpq_numref ( mq ), mpq_numref ( mq ), mm );
                mpz_pow_ui( mpq_denref ( mq ), mpq_denref ( mq ), mm );
                mpq_canonicalize( mq );
                mpq_inv( mq, mq );
                return trp_math_result_from_mpq( mq );
            }
        }
        return UNDEF;
    default:
        /*
         FIXME
         */
        return UNDEF;
    }
    return UNDEF;
}

trp_obj_t *trp_math_powm( trp_obj_t *n, trp_obj_t *m, trp_obj_t *mod )
{
    /*
     FIXME
     */
    return UNDEF;
}

trp_obj_t *trp_math_exp( trp_obj_t *n )
{
    /*
     FIXME
     */
    return UNDEF;
}

trp_obj_t *trp_math_ln( trp_obj_t *n )
{
    /*
     FIXME
     */
    return UNDEF;
}

/*
 FIXME
 */

trp_obj_t *trp_math_log( trp_obj_t *base, trp_obj_t *n )
{
    double bbase, nn;

    if ( trp_cast_flt64b( base, &bbase ) || trp_cast_flt64b( n, &nn ) )
        return UNDEF;
    if ( ( bbase <= 0.0 ) || ( bbase == 1.0 ) || ( nn <= 0.0 ) )
        return UNDEF;
    return trp_double( log( nn ) / log( bbase ) );
}

trp_obj_t *trp_math_atan( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    return trp_double( atan( dn ) );
}

trp_obj_t *trp_math_asin( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    if ( ( dn > 1.0 ) || ( dn < -1.0 ) )
        return UNDEF;
    return trp_double( asin( dn ) );
}

trp_obj_t *trp_math_acos( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    if ( ( dn > 1.0 ) || ( dn < -1.0 ) )
        return UNDEF;
    return trp_double( acos( dn ) );
}

trp_obj_t *trp_math_tan( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    /*
     FIXME
     */
    return trp_double( tan( dn ) );
}

trp_obj_t *trp_math_sin( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    return trp_double( sin( dn ) );
}

trp_obj_t *trp_math_cos( trp_obj_t *n )
{
    double dn;

    if ( trp_cast_flt64b( n, &dn ) )
        return UNDEF;
    return trp_double( cos( dn ) );
}

trp_obj_t *trp_math_lyapunov( trp_obj_t *seq, trp_obj_t *a, trp_obj_t *b, trp_obj_t *iter )
{
    uns8b *c = trp_csprint( seq ), *d;
    uns32b i, n;
    double ab[ 2 ], x = 0.4, col = 0.0, r;

    if ( trp_cast_flt64b( a, &( ab[ 0 ] ) ) || trp_cast_flt64b( b, &( ab[ 1 ] ) ) )
        return UNDEF;
    if ( iter ) {
        if ( trp_cast_uns32b( iter, &n ) )
            return UNDEF;
    } else
        n = 2000;
    for ( i = 0 ; i < n ; i++ ) {
        for ( d = c ; *d ; d++ ) {
            r = ab[ ( *d == 'a' ) ? 0 : 1 ];
            x = r * x * ( 1 - x );
            if ( x == 0.5 ) {
                trp_csprint_free( c );
                return UNO;
            }
            col += log( fabs( r - 2 * r * x ) );
        }
    }
    trp_csprint_free( c );
    if ( col > 0 )
        return ZERO;
    col = 0.5 - col / ( ( (double)n ) / 100.0 );
    if ( col > 254 )
        return UNO;
    return (trp_sig64)( 255 - (uns8b)col );
}

