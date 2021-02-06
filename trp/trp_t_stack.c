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

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_stack_elem;

uns8b trp_stack_print( trp_print_t *p, trp_stack_t *obj )
{
    uns8b buf[ 11 ];

    if ( trp_print_char_star( p, "#stack (" ) )
        return 1;
    sprintf( buf, "%u", obj->len );
    if ( trp_print_char_star( p, buf ) )
        return 1;
    return trp_print_char_star( p, ")#" );
}

uns32b trp_stack_size( trp_stack_t *obj )
{
    uns32b sz = 1 + 4;
    trp_stack_elem *elem;

    for ( elem = (trp_stack_elem *)( obj->data ) ;
          elem ;
          elem = (trp_stack_elem *)( elem->next ) )
        sz += trp_size_internal( elem->val );
    return sz;
}

void trp_stack_encode( trp_stack_t *obj, uns8b **buf )
{
    trp_stack_elem *elem;
    uns32b *p;

    **buf = TRP_STACK;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( elem = (trp_stack_elem *)( obj->data ) ;
          elem ;
          elem = (trp_stack_elem *)( elem->next ) )
        trp_encode_internal( elem->val, buf );
}

trp_obj_t *trp_stack_decode( uns8b **buf )
{
    uns32b len;
    trp_obj_t *res, *tmp;

    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    res = trp_stack();
    tmp = trp_stack();
    for ( ; len ; len-- )
        trp_stack_push( tmp, trp_decode_internal( buf ) );
    while ( ((trp_stack_t *)tmp)->len )
        trp_stack_push( res, trp_stack_pop( tmp ) );
    trp_gc_free( tmp );
    return res;
}

trp_obj_t *trp_stack_equal( trp_stack_t *o1, trp_stack_t *o2 )
{
    trp_stack_elem *elem1, *elem2;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( elem1 = (trp_stack_elem *)( o1->data ), elem2 = (trp_stack_elem *)( o2->data ) ;
          elem1 ;
          elem1 = (trp_stack_elem *)( elem1->next ), elem2 = (trp_stack_elem *)( elem2->next ) )
        if ( trp_equal( elem1->val, elem2->val ) == TRP_FALSE )
            return TRP_FALSE;
    return TRP_TRUE;
}

trp_obj_t *trp_stack_length( trp_stack_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_stack_nth( uns32b n, trp_stack_t *obj )
{
    trp_stack_elem *elem;

    if ( n >= obj->len )
        return UNDEF;
    for ( elem = (trp_stack_elem *)( obj->data ) ;
          n ;
          elem = (trp_stack_elem *)( elem->next ), n-- );
    return elem->val;
}

uns8b trp_stack_in( trp_obj_t *obj, trp_stack_t *seq, uns32b *pos, uns32b nth )
{
    trp_stack_elem *elem;
    uns32b i = 0;
    uns8b res = 1;

    for ( elem = (trp_stack_elem *)( seq->data ) ;
          elem ;
          elem = (trp_stack_elem *)( elem->next ), i++ )
        if ( trp_equal( elem->val, obj ) == TRP_TRUE ) {
            res = 0;
            *pos = i;
            if ( nth == 0 )
                break;
            nth--;
        }
    return res;
}

trp_obj_t *trp_stack()
{
    trp_stack_t *obj;

    obj = trp_gc_malloc( sizeof( trp_stack_t ) );
    obj->tipo = TRP_STACK;
    obj->len = 0;
    obj->data = NULL;
    return (trp_obj_t *)obj;
}

uns8b trp_stack_push( trp_obj_t *stack, trp_obj_t *obj )
{
    trp_stack_elem *p;

    if ( stack->tipo != TRP_STACK )
        return 1;
    p = trp_gc_malloc( sizeof( trp_stack_elem ) );
    p->val = obj;
    p->next = ((trp_stack_t *)stack)->data;
    ((trp_stack_t *)stack)->data = (void *)p;
    ((trp_stack_t *)stack)->len++;
    return 0;
}

trp_obj_t *trp_stack_pop( trp_obj_t *stack )
{
    trp_obj_t *res;
    trp_stack_elem *p;

    if ( stack->tipo != TRP_STACK )
        return UNDEF;
    if ( ((trp_stack_t *)stack)->len == 0 )
        return UNDEF;
    ((trp_stack_t *)stack)->len--;
    p = (trp_stack_elem *)(((trp_stack_t *)stack)->data);
    res = p->val;
    ((trp_stack_t *)stack)->data = p->next;
    trp_gc_free( p );
    return res;
}

