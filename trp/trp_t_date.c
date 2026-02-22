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

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );

trp_obj_t *trp_date_19700101();
trp_obj_t *trp_date_internal( uns16b anno, uns8b mese, uns8b giorno, uns8b ore, uns8b minuti, uns8b secondi, trp_obj_t *resto, sig32b tz );

static uns32b trp_date_leap_year( uns32b a );
static uns32b trp_date_leap_mult_base( uns32b a1, uns32b a2, uns32b n );
static uns32b trp_date_gma2abs( uns32b g, uns32b m, uns32b a );
static void trp_date_abs2gma( uns32b d, uns32b *g, uns32b *m, uns32b *a );
static trp_obj_t *trp_date_diff_internal( trp_date_t *d1, trp_date_t *d2 );
static trp_obj_t *trp_date_incr_internal( trp_date_t *d, trp_obj_t *incr );
static trp_obj_t *trp_date_change_timezone_internal( trp_obj_t *d, sig32b new_tz );
static sig32b trp_date_timezone_internal();

static uns8b *_trp_mese[] = {
    "gen", "feb", "mar", "apr", "mag", "giu",
    "lug", "ago", "set", "ott", "nov", "dic"
};

static uns8b *_trp_month[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static uns8b *_trp_day_of_week[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static uns16b _trp_mese_offset[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

#ifdef MINGW
/*
 * WIN32 C runtime library had been made thread-safe
 * without affecting the user interface. Provide
 * mappings from the UNIX thread-safe versions to
 * the standard C runtime library calls.
 * Only provide function mappings for functions that
 * actually exist on WIN32.
 */
#ifndef gmtime_r
#define gmtime_r( _clock, _result ) \
    ( *(_result) = *gmtime( (_clock) ), \
    (_result) )
#endif /* !gmtime_r */
#ifndef localtime_r
#define localtime_r( _clock, _result ) \
    ( *(_result) = *localtime( (_clock) ), \
    (_result) )
#endif /* !localtime_r */
#endif /* MINGW */

uns8b trp_date_print( trp_print_t *p, trp_date_t *obj )
{
    uns8b buf[ 8 ];
    uns32b usec;

    if ( obj->mese ) {
        if ( obj->giorno ) {
            sprintf( buf, "%2d ", obj->giorno );
            if ( trp_print_char_star( p, buf ) )
                return 1;
        }
        if ( trp_print_char_star( p, _trp_mese[ obj->mese - 1 ] ) )
            return 1;
        if ( trp_print_char( p, ' ' ) )
            return 1;
    }
    sprintf( buf, "%04d", obj->anno );
    if ( trp_print_char_star( p, buf ) )
        return 1;
    if ( obj->ore < 24 ) {
        sprintf( buf, ", %02d", obj->ore );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( obj->minuti < 60 ) {
            sprintf( buf, ":%02d", obj->minuti );
            if ( trp_print_char_star( p, buf ) )
                return 1;
            if ( obj->secondi < 60 ) {
                sprintf( buf, ":%02d", obj->secondi );
                if ( trp_print_char_star( p, buf ) )
                    return 1;
            }
        }
    }
    if ( obj->resto == ZERO )
        return 0;
    usec = (uns32b)( ( (trp_sig64_t *)trp_math_floor( trp_math_times( obj->resto,
                                                                      trp_sig64( 1000000 ),
                                                                      NULL ) ) )->val );
    if ( usec == 0 )
        return 0;
    sprintf( buf, ".%06u", usec );
    return trp_print_char_star( p, buf );
}

uns32b trp_date_size( trp_date_t *obj )
{
    return 1 + 2 + 1 + 1 + 1 + 1 + 1 + trp_size_internal( obj->resto ) + 4;
}

void trp_date_encode( trp_date_t *obj, uns8b **buf )
{
    sig32b *q;
    uns16b *p;

    **buf = TRP_DATE;
    ++(*buf);
    p = (uns16b *)(*buf);
    *p = norm16( obj->anno );
    (*buf) += 2;
    **buf = obj->mese;
    ++(*buf);
    **buf = obj->giorno;
    ++(*buf);
    **buf = obj->ore;
    ++(*buf);
    **buf = obj->minuti;
    ++(*buf);
    **buf = obj->secondi;
    ++(*buf);
    trp_encode_internal( obj->resto, buf );
    q = (sig32b *)(*buf);
    *q = (sig32b)norm32( obj->tz );
    (*buf) += 4;
}

trp_obj_t *trp_date_decode( uns8b **buf )
{
    trp_obj_t *res;
    sig32b tz;
    uns16b anno;
    uns8b mese, giorno, ore, minuti, secondi;

    anno = norm16( *((uns16b *)(*buf)) );
    (*buf) += 2;
    mese = **buf;
    ++(*buf);
    giorno = **buf;
    ++(*buf);
    ore = **buf;
    ++(*buf);
    minuti = **buf;
    ++(*buf);
    secondi = **buf;
    ++(*buf);
    res = trp_decode_internal( buf );
    tz = (sig32b)norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    return trp_date_internal( anno, mese, giorno, ore, minuti, secondi, res, tz );
}

trp_obj_t *trp_date_equal( trp_date_t *o1, trp_date_t *o2 )
{
    return ( trp_date_diff_internal( o1, o2 ) == ZERO ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_date_less( trp_date_t *o1, trp_date_t *o2 )
{
    return trp_less( trp_date_diff_internal( o1, o2 ), ZERO );
}

trp_obj_t *trp_date_19700101()
{
    static trp_obj_t *ref = NULL;

    if ( ref == NULL )
        ref = trp_date_internal( 1970, 1, 1, 0, 0, 0, ZERO, 0 );
    return ref;
}

trp_obj_t *trp_date_length( trp_date_t *obj )
{
    return trp_date_diff_internal( obj, (trp_date_t *)trp_date_19700101() );
}

trp_obj_t *trp_date_internal( uns16b anno, uns8b mese, uns8b giorno, uns8b ore, uns8b minuti, uns8b secondi, trp_obj_t *resto, sig32b tz )
{
    trp_date_t *obj;

    obj = trp_gc_malloc( sizeof( trp_date_t ) );
    obj->tipo = TRP_DATE;
    obj->anno = anno;
    obj->mese = mese;
    obj->giorno = giorno;
    obj->ore = ore;
    obj->minuti = minuti;
    obj->secondi = secondi;
    obj->resto = resto;
    obj->tz = tz;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_date( trp_obj_t *aa, trp_obj_t *mm, trp_obj_t *gg, trp_obj_t *o, trp_obj_t *m, trp_obj_t *s, trp_obj_t *resto, trp_obj_t *tz )
{
    uns32b anno, mese, giorno, ore, minuti, secondi;
    uns32b dm, dg, ca, cm, cg;

    if ( trp_cast_uns32b( aa, &anno ) ||
         trp_cast_uns32b( mm, &mese ) ||
         trp_cast_uns32b( gg, &giorno ) ||
         trp_cast_uns32b( o, &ore ) ||
         trp_cast_uns32b( m, &minuti ) ||
         trp_cast_uns32b( s, &secondi ) ||
         ( ( resto != ZERO ) && ( resto->tipo != TRP_RATIO ) ) ||
         ( tz->tipo != TRP_SIG64 ) )
        return UNDEF;
    if ( ( anno > 65535 ) ||
         ( mese > 12 ) ||
         ( giorno > 31 ) ||
         ( ore > 24 ) ||
         ( minuti > 60 ) ||
         ( secondi > 60 ) ||
         ( ((trp_sig64_t *)tz)->val < -43200 ) ||
         ( ((trp_sig64_t *)tz)->val > 43200 ) )
        return UNDEF;
    dm = mese;
    if ( dm == 0 )
        dm = 1;
    dg = giorno;
    if ( dg == 0 )
        dg = 1;
    trp_date_abs2gma( trp_date_gma2abs( dg, dm, anno ), &cg, &cm, &ca );
    if ( ( cg != dg ) || ( cm != dm ) || ( ca != anno ) )
        return UNDEF;
    if ( ( trp_less( resto, ZERO ) == TRP_TRUE ) ||
         ( trp_less( resto, UNO ) == TRP_FALSE ) )
        return UNDEF;
    return trp_date_internal( anno, mese, giorno,
                              ore, minuti, secondi,
                              resto,
                              (sig32b)( ((trp_sig64_t *)tz)->val ) );
}

trp_obj_t *trp_date_now()
{
    static trp_date_t *dref = NULL;
    struct timeval tv;

    if ( dref == NULL )
        dref = (trp_date_t *)trp_date_internal( 1970, 1, 1, 0, 0, 0, ZERO,
                                                trp_date_timezone_internal() );
    (void)gettimeofday( &tv, NULL );
    if ( tv.tv_usec < 0 )
        tv.tv_usec = 0;
    if ( tv.tv_usec > 999999 )
        tv.tv_usec = 999999;
    return trp_date_incr_internal( dref,
                                   trp_cat( trp_sig64( tv.tv_sec - trp_date_timezone_internal() ),
                                            trp_math_ratio( trp_sig64( tv.tv_usec ),
                                                            trp_sig64( 1000000 ),
                                                            NULL ),
                                            NULL ) );
}

static uns32b trp_date_leap_year( uns32b a )
{
    return ( ( a % 4 == 0 ) &&
             ( ( a % 100 != 0 ) || ( a % 400 == 0 ) ) )
        ? 1
        : 0;
}

static uns32b trp_date_leap_mult_base( uns32b a1, uns32b a2, uns32b n )
{
    uns32b resto;

    resto = a2 % n;
    if ( resto )
        a2 += ( n - resto );
    return ( a2 >= a1 ) ? 0 : 1 + ( ( a1 - 1 - a2 ) / n );
}

static uns32b trp_date_gma2abs( uns32b g, uns32b m, uns32b a )
{
    if ( ( a < 1582 ) ||
         ( ( a == 1582 ) &&
           ( ( m < 10 ) ||
             ( ( m == 10 ) && g < 5 ) ) ) ) {
        return g - 1 + (uns32b)_trp_mese_offset[ m - 1 ]
            + ( ( ( m > 2 ) && ( a % 4 == 0 ) ) ? 1 : 0 )
            + 365 * a
            + trp_date_leap_mult_base( a, 0, 4 );
    }
    if ( a == 1582 ) {
        return g - 11 + (uns32b)_trp_mese_offset[ m - 1 ]
            + 365 * a
            + trp_date_leap_mult_base( a, 0, 4 );
    }
    return 578180 /* trp_date_gma2abs( 31, 12, 1582 ) */
        + g + (uns32b)_trp_mese_offset[ m - 1 ]
        + ( ( m > 2 ) ? trp_date_leap_year( a ) : 0 )
        + 365 * ( a - 1583 )
        + trp_date_leap_mult_base( a, 1582, 4 )
        - trp_date_leap_mult_base( a, 1582, 100 )
        + trp_date_leap_mult_base( a, 1582, 400 );
}

static void trp_date_abs2gma( uns32b d, uns32b *g, uns32b *m, uns32b *a )
{
    if ( d <= 578102 ) { /* trp_date_gma2abs( 4, 10, 1582 ) */
        *a = ( d * 4 ) / 1461;
        d -= trp_date_gma2abs( 1, 1, *a );
        if ( d < 31 ) {
            *g = d + 1;
            *m = 1;
        } else if ( d < 59 ) {
            *g = d - 30;
            *m = 2;
        } else {
            if ( *a % 4 == 0 ) d--;
            if ( d == 58 ) {
                *g = 29;
                *m = 2;
            } else {
                for ( *m = 3 ; d >= (uns32b)_trp_mese_offset[ *m ] ; (*m)++ );
                *g = d - (uns32b)_trp_mese_offset[ *m - 1 ] + 1;
            }
        }
    } else if ( d <= 578180 ) { /* trp_date_gma2abs( 31, 12, 1582 ) */
        *a = 1582;
        d = d - trp_date_gma2abs( 1, 1, *a ) + 10;
        for ( *m = 10 ; d >= (uns32b)_trp_mese_offset[ *m ] ; (*m)++ );
        *g = d - (uns32b)_trp_mese_offset[ *m - 1 ] + 1;
    } else {
        /*
         FIXME
         si può evitare il ciclo?
         */
        for ( *a = 1583 + ( d - 578181 ) / 366 ;
              trp_date_gma2abs( 1, 1, *a + 1 ) <= d ;
              (*a)++ );
        d -= trp_date_gma2abs( 1, 1, *a );
        if ( d < 31 ) {
            *g = d + 1;
            *m = 1;
        } else if ( d < 59 ) {
            *g = d - 30;
            *m = 2;
        } else {
            d -= trp_date_leap_year( *a );
            if ( d == 58 ) {
                *g = 29;
                *m = 2;
            } else {
                for ( *m = 3 ; d >= (uns32b)_trp_mese_offset[ *m ] ; (*m)++ );
                *g = d - (uns32b)_trp_mese_offset[ *m - 1 ] + 1;
            }
        }
    }
}

static trp_obj_t *trp_date_diff_internal( trp_date_t *d1, trp_date_t *d2 )
{
    uns32b g, m;
    sig64b s;

    g = d2->giorno;
    if ( g == 0 )
        g = 1;
    m = d2->mese;
    if ( m == 0 )
        m = 1;
    s =  trp_date_gma2abs( g, m, d2->anno );
    g = d1->giorno;
    if ( g == 0 )
        g = 1;
    m = d1->mese;
    if ( m == 0 )
        m = 1;
    s -= trp_date_gma2abs( g, m, d1->anno );
    s *= 24;
    s += ( d2->ore ) % 24;
    s -= ( d1->ore ) % 24;
    s *= 60;
    s += ( d2->minuti ) % 60;
    s -= ( d1->minuti ) % 60;
    s *= 60;
    s += ( d2->secondi ) % 60;
    s -= ( d1->secondi ) % 60;
    s -= d1->tz;
    s += d2->tz;
    return trp_math_minus( d1->resto,
                           d2->resto,
                           trp_sig64( s ),
                           NULL );
}

static trp_obj_t *trp_date_incr_internal( trp_date_t *d, trp_obj_t *incr )
{
    uns32b g, m, a, hh, mm, ss;
    sig64b s;
    trp_obj_t *tmp;

    g = d->giorno;
    if ( g == 0 )
        g = 1;
    m = d->mese;
    if ( m == 0 )
        m = 1;
    s  = trp_date_gma2abs( g, m, d->anno );
    s *= 24;
    s += ( d->ore ) % 24;
    s *= 60;
    s += ( d->minuti ) % 60;
    s *= 60;
    s += ( d->secondi ) % 60;
    incr = trp_cat( incr, trp_sig64( s ), d->resto, NULL );
    if ( trp_less( incr, ZERO ) == TRP_TRUE )
        return UNDEF;
    tmp = trp_math_floor( incr );
    if ( tmp->tipo != TRP_SIG64 )
        return UNDEF;
    s = ((trp_sig64_t *)tmp)->val;
    trp_date_abs2gma( (uns32b)( s / 86400 ), &g, &m, &a );
    ss = (uns32b)( s % 86400 );
    hh = ss / 3600;
    ss = ss % 3600;
    mm = ss / 60;
    ss = ss % 60;
    return trp_date_internal( a, m, g, hh, mm, ss,
                              trp_math_minus( incr, tmp, NULL ),
                              d->tz );
}

trp_obj_t *trp_date_cat( trp_date_t *d, va_list args )
{
    trp_obj_t *obj;

    obj = va_arg( args, trp_obj_t * );
    obj = trp_math_cat( obj, args );
    if ( ( obj->tipo != TRP_SIG64 ) &&
         ( obj->tipo != TRP_MPI ) &&
         ( obj->tipo != TRP_RATIO ) )
        return UNDEF;
    return trp_date_incr_internal( d, obj );
}

trp_obj_t *trp_date_minus_args( trp_date_t *d, va_list args )
{
    trp_obj_t *obj = va_arg( args, trp_obj_t * );

    if ( obj == NULL )
        return (trp_obj_t *)d;
    if ( obj->tipo == TRP_DATE ) {
        if ( va_arg( args, trp_obj_t * ) )
            return UNDEF;
        return trp_date_diff_internal( d, (trp_date_t *)obj );
    }
    obj = trp_math_cat( obj, args );
    if ( ( obj->tipo != TRP_SIG64 ) &&
         ( obj->tipo != TRP_MPI ) &&
         ( obj->tipo != TRP_RATIO ) )
        return UNDEF;
    return trp_date_incr_internal( d, trp_math_minus( ZERO, obj, NULL ) );
}

static sig32b trp_date_timezone_internal()
{
    static sig32b tz = 100000;

    if ( tz == 100000 ) {
        trp_obj_t *d;
        time_t t = time( NULL );
        struct tm gt, lt;

        (void)gmtime_r( &t, &gt );
        (void)localtime_r( &t, &lt );
        d = trp_date_diff_internal( (trp_date_t *)trp_date_internal( 1900 + gt.tm_year, 1 + gt.tm_mon, gt.tm_mday,
                                                                     gt.tm_hour, gt.tm_min, gt.tm_sec,
                                                                     ZERO, 0 ),
                                    (trp_date_t *)trp_date_internal( 1900 + lt.tm_year, 1 + lt.tm_mon, lt.tm_mday,
                                                                     lt.tm_hour, lt.tm_min, lt.tm_sec,
                                                                     ZERO, 0 ) );
        if ( d->tipo == TRP_SIG64 )
            tz = ( ( ((trp_sig64_t *)d)->val <= 43200 ) &&
                   ( ((trp_sig64_t *)d)->val >= -43200 ) ) ? ((trp_sig64_t *)d)->val : 0;
        else
            tz = 0;
    }
    return tz;
}

trp_obj_t *trp_date_timezone( trp_obj_t *d )
{
    if ( d == NULL )
        return trp_sig64( trp_date_timezone_internal() );
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->tz );
}

static trp_obj_t *trp_date_change_timezone_internal( trp_obj_t *d, sig32b new_tz )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->tz == new_tz )
        return d;
    d = trp_date_incr_internal( (trp_date_t *)d,
                                trp_sig64( ((trp_date_t *)d)->tz - new_tz ) );
    ((trp_date_t *)d)->tz = new_tz;
    return d;
}

trp_obj_t *trp_date_change_timezone( trp_obj_t *d, trp_obj_t *new_tz )
{
    if ( new_tz->tipo != TRP_SIG64 )
        return UNDEF;
    if ( ( ((trp_sig64_t *)new_tz)->val < -43200 ) ||
         ( ((trp_sig64_t *)new_tz)->val > 43200 ) )
        return UNDEF;
    return trp_date_change_timezone_internal( d, (sig32b)( ((trp_sig64_t *)new_tz)->val ) );
}

trp_obj_t *trp_date_arpa( trp_obj_t *d )
{
    uns32b g, m, a;
    sig32b tz;
    uns8b buf[ 33 ];

    if ( d->tipo != TRP_DATE )
        return UNDEF;

    g = ((trp_date_t *)d)->giorno;
    if ( g == 0 )
        g = 1;
    m = ((trp_date_t *)d)->mese;
    if ( m == 0 )
        m = 1;
    a = ((trp_date_t *)d)->anno;
    tz = ((trp_date_t *)d)->tz;
    sprintf( buf, "%s, %02u %s %04u %02u:%02u:%02u",
             _trp_day_of_week[ ( trp_date_gma2abs( g, m, a ) + 4 ) % 7 ],
             g, _trp_month[ m - 1 ], a,
             ((trp_date_t *)d)->ore % 24,
             ((trp_date_t *)d)->minuti % 60,
             ((trp_date_t *)d)->secondi % 60 );
    if ( tz ) {
        sprintf( buf + 25, " %c%02u%02u",
                 ( tz < 0 ) ? '+' : '-',
                 ( ( tz < 0 ) ? -tz : tz ) / 3600,
                 ( ( ( tz < 0 ) ? -tz : tz ) % 3600 ) / 60 );
    } else {
        sprintf( buf + strlen( buf ), " GMT" );
    }
    return trp_cord( buf );
}

trp_obj_t *trp_date_ctime( trp_obj_t *d )
{
    uns32b g, m, a;
    uns8b buf[ 26 ];

    if ( d->tipo != TRP_DATE )
        return UNDEF;

    g = ((trp_date_t *)d)->giorno;
    if ( g == 0 )
        g = 1;
    m = ((trp_date_t *)d)->mese;
    if ( m == 0 )
        m = 1;
    a = ((trp_date_t *)d)->anno;
    sprintf( buf, "%s %s %2u %02u:%02u:%02u %04u",
             _trp_day_of_week[ ( trp_date_gma2abs( g, m, a ) + 4 ) % 7 ],
             _trp_month[ m - 1 ], g,
             ((trp_date_t *)d)->ore % 24,
             ((trp_date_t *)d)->minuti % 60,
             ((trp_date_t *)d)->secondi % 60, a );
    return trp_cord( buf );
}

trp_obj_t *trp_date_year( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->anno );
}

trp_obj_t *trp_date_month( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->mese == 0 )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->mese );
}

trp_obj_t *trp_date_day( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->giorno == 0 )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->giorno );
}

trp_obj_t *trp_date_hours( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->ore >= 24 )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->ore );
}

trp_obj_t *trp_date_minutes( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->minuti >= 60 )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->minuti );
}

trp_obj_t *trp_date_seconds( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    if ( ((trp_date_t *)d)->secondi >= 60 )
        return UNDEF;
    return trp_sig64( ((trp_date_t *)d)->secondi );
}

trp_obj_t *trp_date_usec( trp_obj_t *d )
{
    if ( d->tipo != TRP_DATE )
        return UNDEF;
    return ((trp_date_t *)d)->resto;
}

trp_obj_t *trp_date_wday( trp_obj_t *d )
{
    uns32b g, m, a;

    if ( d->tipo != TRP_DATE )
        return UNDEF;
    g = ((trp_date_t *)d)->giorno;
    if ( g == 0 )
        g = 1;
    m = ((trp_date_t *)d)->mese;
    if ( m == 0 )
        m = 1;
    a = ((trp_date_t *)d)->anno;
    return trp_sig64( ( trp_date_gma2abs( g, m, a ) + 4 ) % 7 );
}

trp_obj_t *trp_date_s2hhmmss( trp_obj_t *s )
{
    uns32b hh;
    uns8b mm, ss, buf[ 9 ];

    if ( ( s->tipo != TRP_SIG64 ) &&
         ( s->tipo != TRP_MPI ) &&
         ( s->tipo != TRP_RATIO ) )
        return UNDEF;
    if ( trp_less( trp_sig64( 359999 ), s ) == TRP_TRUE )
        hh = 359999;
    else {
        if ( s->tipo == TRP_RATIO )
            s = trp_math_rint( s );
        if ( trp_cast_uns32b( s, &hh ) )
            return UNDEF;
    }
    ss = hh % 60;
    hh /= 60;
    mm = hh % 60;
    hh /= 60;
    sprintf( buf, "%02d:%02d:%02d", (int)hh, (int)mm, (int)ss );
    return trp_cord( buf );
}

trp_obj_t *trp_date_cal( time_t t )
{
    struct tm lt;

    (void)localtime_r( &t, &lt );
    return trp_date_internal( 1900 + lt.tm_year,
                              1 + lt.tm_mon,
                              lt.tm_mday,
                              lt.tm_hour,
                              lt.tm_min,
                              lt.tm_sec,
                              ZERO,
                              trp_date_timezone_internal() );
}

