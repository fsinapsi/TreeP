/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

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

static trp_special_t *_trp_special = NULL;

#define TRP_SPECIAL_CNT 4

static uns8b *_trp_special_out[ TRP_SPECIAL_CNT ] = {
    "#undef#",
    "#nil#",
    "#true#",
    "#false#"
};

uns8b trp_special_print( trp_print_t *p, trp_special_t *obj )
{
    uns8b *out;

    out = _trp_special_out[ obj->sottotipo ];
    return trp_print_char_star( p, out );
}

uns32b trp_special_size( trp_special_t *obj )
{
    return 1 + 1;
}

void trp_special_encode( trp_special_t *obj, uns8b **buf )
{
    **buf = TRP_SPECIAL;
    ++(*buf);
    **buf = obj->sottotipo;
    ++(*buf);
}

trp_obj_t *trp_special_decode( uns8b **buf )
{
    trp_obj_t *res = (trp_obj_t *)( &( _trp_special[ **buf ] ) );
    ++(*buf);
    return res;
}

trp_obj_t *trp_special_equal( trp_special_t *o1, trp_special_t *o2 )
{
    return ( o1->sottotipo == o2->sottotipo ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_special_less( trp_special_t *o1, trp_special_t *o2 )
{
    return ( o1->sottotipo < o2->sottotipo ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_special_length( trp_obj_t *obj )
{
    return ( obj == NIL ) ? ZERO : UNDEF;
}

trp_obj_t *trp_special_sub( uns32b start, uns32b len, trp_obj_t *obj )
{
    return ( ( obj == NIL ) && ( start == 0 ) ) ? NIL : UNDEF;
}

void trp_special_init()
{
    if ( _trp_special == NULL ) {
        uns16b i;

        _trp_special = trp_gc_malloc_atomic( sizeof( trp_special_t ) * TRP_SPECIAL_CNT );
        for ( i = 0 ; i < TRP_SPECIAL_CNT ; i++ ) {
            _trp_special[ i ].tipo = TRP_SPECIAL;
            _trp_special[ i ].sottotipo = (uns8b)i;
        }
    }
}

trp_obj_t *trp_undef()
{
    return (trp_obj_t *)( &( _trp_special[ 0 ] ) );
}

trp_obj_t *trp_nil()
{
    return (trp_obj_t *)( &( _trp_special[ 1 ] ) );
}

trp_obj_t *trp_true()
{
    return (trp_obj_t *)( &( _trp_special[ 2 ] ) );
}

trp_obj_t *trp_false()
{
    return (trp_obj_t *)( &( _trp_special[ 3 ] ) );
}

