/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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

static trp_char_t *_trp_char = NULL;

uns8b trp_char_print( trp_print_t *p, trp_char_t *obj )
{
    return trp_print_char( p, obj->c );
}

uns32b trp_char_size( trp_char_t *obj )
{
    return 1 + 1;
}

void trp_char_encode( trp_char_t *obj, uns8b **buf )
{
    **buf = TRP_CHAR;
    ++(*buf);
    **buf = obj->c;
    ++(*buf);
}

trp_obj_t *trp_char_decode( uns8b **buf )
{
    trp_obj_t *res = trp_char( **buf );
    ++(*buf);
    return res;
}

trp_obj_t *trp_char_equal( trp_char_t *o1, trp_char_t *o2 )
{
    return ( o1->c == o2->c ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_char_less( trp_char_t *o1, trp_char_t *o2 )
{
    return ( o1->c < o2->c ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_char_length( trp_char_t *obj )
{
    return trp_sig64( obj->c );
}

trp_obj_t *trp_char_cat( trp_char_t *c, va_list args )
{
    trp_obj_t *obj;
    sig64b v = c->c;

    obj = va_arg( args, trp_obj_t * );
    obj = trp_math_cat( obj, args );
    if ( ( obj->tipo != TRP_SIG64 ) )
        return UNDEF;
    v += ((trp_sig64_t *)obj)->val;
    if ( ( v < 0 ) || ( v > 0xff ) )
        return UNDEF;
    return trp_char( (uns8b)v );
}

void trp_char_init()
{
    if ( _trp_char == NULL ) {
        uns16b i;

        _trp_char = trp_gc_malloc_atomic( sizeof( trp_char_t ) * 256 );
        for ( i = 0 ; i < 256 ; i++ ) {
            _trp_char[ i ].tipo = TRP_CHAR;
            _trp_char[ i ].c = (uns8b)i;
        }
    }
}

trp_obj_t *trp_char( uns8b c )
{
    return (trp_obj_t *)( &( _trp_char[ c ] ) );
}

trp_obj_t *trp_nl()
{
    return (trp_obj_t *)( &( _trp_char[ '\n' ] ) );
}

trp_obj_t *trp_int2char( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_SIG64 )
        return UNDEF;
    if ( ( ((trp_sig64_t *)obj)->val < 0 ) ||
         ( ((trp_sig64_t *)obj)->val > 0xff ) )
        return UNDEF;
    return trp_char( (uns8b)( ((trp_sig64_t *)obj)->val ) );
}

