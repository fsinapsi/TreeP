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

static uns8b trp_regex_close_basic( uns8b flags, trp_regex_t *obj );
static void trp_regex_finalize( void *obj, void *data );

uns8b trp_regex_close( trp_regex_t *obj )
{
    return trp_regex_close_basic( 1, obj );
}

static uns8b trp_regex_close_basic( uns8b flags, trp_regex_t *obj )
{
    regex_t *preg = obj->preg;

    if ( preg ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        regfree( preg );
        free( preg );
        obj->preg = NULL;
    }
    return 0;
}

static void trp_regex_finalize( void *obj, void *data )
{
    trp_regex_close_basic( 0, (trp_regex_t *)obj );
}

trp_obj_t *trp_regex_length( trp_regex_t *obj )
{
    if ( obj->preg == NULL )
        return UNDEF;
    return trp_sig64( obj->preg->re_nsub );
}

trp_obj_t *trp_regsearch( trp_obj_t *key, trp_obj_t *txt, trp_obj_t *flags )
{
    trp_obj_t *res;
    uns32b cflags;
    uns8b *c;
    regex_t preg;

    if ( flags ) {
        if ( trp_cast_uns32b( flags, &cflags ) )
            return UNDEF;
    } else
        cflags = 0;
    c = trp_csprint( key );
    if ( regcomp( &preg, c, REG_NOSUB | (int)cflags ) ) {
        trp_csprint_free( c );
        return UNDEF;
    }
    trp_csprint_free( c );
    c = trp_csprint( txt );
    res = regexec( &preg, c, 0, NULL, 0 ) ? TRP_FALSE : TRP_TRUE;
    trp_csprint_free( c );
    regfree( &preg );
    return res;
}

trp_obj_t *trp_regcomp( trp_obj_t *obj, trp_obj_t *flags )
{
    uns32b cflags;
    uns8b *regex;
    regex_t *preg;

    if ( flags ) {
        if ( trp_cast_uns32b( flags, &cflags ) )
            return UNDEF;
    } else
        cflags = 0;
    if ( ( preg = malloc( sizeof( regex_t ) ) ) == NULL )
        return UNDEF;
    regex = trp_csprint( obj );
    if ( regcomp( preg, regex, (int)cflags ) ) {
        free( preg );
        trp_csprint_free( regex );
        return UNDEF;
    }
    trp_csprint_free( regex );
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_regex_t ), trp_regex_finalize );
    obj->tipo = TRP_REGEX;
    ((trp_regex_t *)obj)->preg = preg;
    return obj;
}

trp_obj_t *trp_regexec( trp_obj_t *re, trp_obj_t *txt, trp_obj_t *flags )
{
    int res;
    uns32b cflags;
    uns8b *c;
    regmatch_t pmatch[ 1 ];

    if ( flags ) {
        if ( trp_cast_uns32b( flags, &cflags ) )
            return UNDEF;
    } else
        cflags = 0;
    if ( re->tipo != TRP_REGEX )
        return UNDEF;
    if ( ((trp_regex_t *)re)->preg == NULL )
        return UNDEF;
    c = trp_csprint( txt );
    res = regexec( ((trp_regex_t *)re)->preg, c, 1, pmatch, (int)cflags );
    trp_csprint_free( c );
    if ( res )
        return NIL;
    /*
     printf( "%d - %d          %d - %d\n", pmatch[ 0 ].rm_so, pmatch[ 0 ].rm_eo, pmatch[ 1 ].rm_so, pmatch[ 1 ].rm_eo );
     FIXME
     */
    return trp_cons( trp_sig64( pmatch[ 0 ].rm_so ), trp_sig64( pmatch[ 0 ].rm_eo ) );
}

uns8b trp_regexec_test( trp_obj_t *re, trp_obj_t *txt )
{
    int res;
    uns8b *c;

    if ( re->tipo != TRP_REGEX )
        return 1;
    if ( ((trp_regex_t *)re)->preg == NULL )
        return 1;
    c = trp_csprint( txt );
    res = regexec( ((trp_regex_t *)re)->preg, c, 0, NULL, 0 );
    trp_csprint_free( c );
    return res ? 1 : 0;
}

