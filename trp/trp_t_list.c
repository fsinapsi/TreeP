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

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );

uns8b trp_list_print( trp_print_t *p, trp_cons_t *obj )
{
    trp_obj_t *o;

    if ( trp_print_char( p, '[' ) )
        return 1;
    if ( trp_print_obj( p, obj->car ) )
        return 1;
    for ( ; ; ) {
        o = obj->cdr;
        if ( o->tipo != TRP_CONS )
            break;
        obj = (trp_cons_t *)o;
        if ( trp_print_char( p, ' ' ) )
            return 1;
        if ( trp_print_obj( p, obj->car ) )
            return 1;
    }
    if ( o != trp_nil() ) {
        if ( trp_print_char_star( p, " . " ) )
            return 1;
        if ( trp_print_obj( p, o ) )
            return 1;
    }
    return trp_print_char( p, ']' );
}

uns32b trp_list_size( trp_cons_t *obj )
{
    return 1 + trp_size_internal( obj->car ) + trp_size_internal( obj->cdr );
}

void trp_list_encode( trp_cons_t *obj, uns8b **buf )
{
    **buf = TRP_CONS;
    ++(*buf);
    trp_encode_internal( obj->car, buf );
    trp_encode_internal( obj->cdr, buf );
}

trp_obj_t *trp_list_decode( uns8b **buf )
{
    trp_obj_t *car, *cdr;

    car = trp_decode_internal( buf );
    cdr = trp_decode_internal( buf );
    return trp_cons( car, cdr );
}

trp_obj_t *trp_list_equal( trp_cons_t *o1, trp_cons_t *o2 )
{
    if ( trp_equal( o1->car, o2->car ) != TRP_TRUE )
        return TRP_FALSE;
    return trp_equal( o1->cdr, o2->cdr );
}

trp_obj_t *trp_list_less( trp_cons_t *o1, trp_cons_t *o2 )
{
    return trp_less( trp_list_length( o1 ), trp_list_length( o2 ) );
}

trp_obj_t *trp_list_length( trp_cons_t *obj )
{
    uns32b len = 1;

    for ( ; ; len++ ) {
        obj = (trp_cons_t *)( obj->cdr );
        if ( obj->tipo != TRP_CONS )
            break;
    }
    return trp_sig64( len );
}

trp_obj_t *trp_list_nth( uns32b n, trp_cons_t *obj )
{
    for ( ; n ; n-- ) {
        obj = (trp_cons_t *)( obj->cdr );
        if ( obj->tipo != TRP_CONS )
            return UNDEF;
    }
    return obj->car;
}

trp_obj_t *trp_list_sub( uns32b start, uns32b len, trp_cons_t *obj )
{
    trp_obj_t *t = NIL, *tt, *cc, *pt;

    for ( ; start ; start-- ) {
        if ( obj->tipo != TRP_CONS )
            return UNDEF;
        obj = (trp_cons_t *)( obj->cdr );
    }
    pt = (trp_obj_t *)obj;
    for ( ; len ; len-- ) {
        if ( (trp_obj_t *)obj == NIL ) {
            trp_free_list( t );
            return pt;
        }
        if ( obj->tipo != TRP_CONS ) {
            trp_free_list( t );
            return UNDEF;
        }
        cc = trp_cons( obj->car, NIL );
        if ( t == NIL )
            t = cc;
        else
            ((trp_cons_t *)tt)->cdr = cc;
        tt = cc;
        obj = (trp_cons_t *)( obj->cdr );
    }
    return t;
}

trp_obj_t *trp_list_cat( trp_obj_t *obj, va_list args )
{
    trp_obj_t *t = NIL, *tt, *cc, *next;

    for ( ; ; ) {
        next = va_arg( args, trp_obj_t * );
        if ( next == NULL ) {
            if ( ( obj->tipo != TRP_CONS ) && ( obj != NIL ) ) {
                trp_free_list( t );
                return UNDEF;
            }
            if ( t == NIL ) {
                t = obj;
            } else {
                ((trp_cons_t *)tt)->cdr = obj;
            }
            break;
        }
        for ( ; obj != NIL ; obj = ((trp_cons_t *)obj)->cdr ) {
            if ( obj->tipo != TRP_CONS ) {
                trp_free_list( t );
                return UNDEF;
            }
            cc = trp_cons( ((trp_cons_t *)obj)->car, NIL );
            if ( t == NIL )
                t = cc;
            else
                ((trp_cons_t *)tt)->cdr = cc;
            tt = cc;
        }
        obj = next;
    }
    return t;
}

uns8b trp_list_in( trp_obj_t *obj, trp_cons_t *seq, uns32b *pos, uns32b nth )
{
    uns32b i = 0;
    uns8b res = 1;

    for ( ; ; i++ ) {
        if ( trp_equal( seq->car, obj ) == TRP_TRUE ) {
            res = 0;
            *pos = i;
            if ( nth == 0 )
                break;
            nth--;
        }
        seq = (trp_cons_t *)( seq->cdr );
        if ( seq->tipo != TRP_CONS )
            break;
    }
    return res;
}

trp_obj_t *trp_cons( trp_obj_t *car, trp_obj_t *cdr )
{
    trp_cons_t *obj;

    obj = trp_gc_malloc( sizeof( trp_cons_t ) );
    obj->tipo = TRP_CONS;
    obj->car = car;
    obj->cdr = cdr;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_car( trp_obj_t *obj )
{
    return ( obj->tipo == TRP_CONS ) ? ((trp_cons_t *)obj)->car : UNDEF;
}

trp_obj_t *trp_cdr( trp_obj_t *obj )
{
    return ( obj->tipo == TRP_CONS ) ? ((trp_cons_t *)obj)->cdr : UNDEF;
}

trp_obj_t *trp_list( trp_obj_t *obj, ... )
{
    trp_obj_t *res = NULL;
    trp_cons_t *cell, *cellprec;
    va_list args;

    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) ) {
        cell = (trp_cons_t *)trp_cons( obj, NIL );
        if ( res ) {
            cellprec->cdr = (trp_obj_t *)cell;
        } else {
            res = (trp_obj_t *)cell;
        }
        cellprec = cell;
    }
    va_end( args );
    return res ? res : NIL;
}

trp_obj_t *trp_list_reverse( trp_obj_t *obj )
{
    trp_obj_t *t = NIL;

    for ( ; ; obj = ((trp_cons_t *)obj)->cdr ) {
        if ( obj == NIL )
            break;
        if ( obj->tipo != TRP_CONS ) {
            trp_free_list( t );
            return UNDEF;
        }
        t = trp_cons( ((trp_cons_t *)obj)->car, t );
    }
    return t;
}



