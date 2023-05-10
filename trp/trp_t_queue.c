/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

void trp_queue_init_internal( trp_queue_t *q );

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

uns8b trp_queue_print( trp_print_t *p, trp_queue_t *obj )
{
    uns8b buf[ 11 ];

    if ( trp_print_char_star( p, "#queue (" ) )
        return 1;
    sprintf( buf, "%u", obj->len );
    if ( trp_print_char_star( p, buf ) )
        return 1;
    return trp_print_char_star( p, ")#" );
}

uns32b trp_queue_size( trp_queue_t *obj )
{
    uns32b sz = 1 + 4;
    trp_queue_elem *elem;

    for ( elem = (trp_queue_elem *)( obj->first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) )
        sz += trp_size_internal( elem->val );
    return sz;
}

void trp_queue_encode( trp_queue_t *obj, uns8b **buf )
{
    trp_queue_elem *elem;
    uns32b *p;

    **buf = TRP_QUEUE;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( elem = (trp_queue_elem *)( obj->first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) )
        trp_encode_internal( elem->val, buf );
}

trp_obj_t *trp_queue_decode( uns8b **buf )
{
    uns32b len;
    trp_obj_t *res = trp_queue();

    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    for ( ; len ; len-- )
        trp_queue_put( res, trp_decode_internal( buf ) );
    return res;
}

trp_obj_t *trp_queue_equal( trp_queue_t *o1, trp_queue_t *o2 )
{
    trp_queue_elem *elem1, *elem2;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( elem1 = (trp_queue_elem *)( o1->first ), elem2 = (trp_queue_elem *)( o2->first ) ;
          elem1 ;
          elem1 = (trp_queue_elem *)( elem1->next ), elem2 = (trp_queue_elem *)( elem2->next ) )
        if ( trp_equal( elem1->val, elem2->val ) == TRP_FALSE )
            return TRP_FALSE;
    return TRP_TRUE;
}

trp_obj_t *trp_queue_less( trp_queue_t *o1, trp_queue_t *o2 )
{
    return ( o1->len < o2->len ) ? TRP_TRUE : TRP_FALSE;
}

uns8b trp_queue_close( trp_queue_t *obj )
{
    while ( obj->len )
        trp_queue_get( (trp_obj_t *)obj );
    return 0;
}

trp_obj_t *trp_queue_length( trp_queue_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_queue_nth( uns32b n, trp_queue_t *obj )
{
    trp_queue_elem *elem;

    if ( n >= obj->len )
        return UNDEF;
    for ( elem = (trp_queue_elem *)( obj->first ) ;
          n ;
          elem = (trp_queue_elem *)( elem->next ), n-- );
    return elem->val;
}

uns8b trp_queue_in( trp_obj_t *obj, trp_queue_t *seq, uns32b *pos, uns32b nth )
{
    trp_queue_elem *elem;
    uns32b i = 0;
    uns8b res = 1;

    for ( elem = (trp_queue_elem *)( seq->first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ), i++ )
        if ( trp_equal( elem->val, obj ) == TRP_TRUE ) {
            res = 0;
            *pos = i;
            if ( nth == 0 )
                break;
            nth--;
        }
    return res;
}

void trp_queue_init_internal( trp_queue_t *q )
{
    q->tipo = TRP_QUEUE;
    q->len = 0;
    q->first = NULL;
    q->last = NULL;
}

trp_obj_t *trp_queue()
{
    trp_queue_t *obj;

    obj = trp_gc_malloc( sizeof( trp_queue_t ) );
    trp_queue_init_internal( obj );
    return (trp_obj_t *)obj;
}

uns8b trp_queue_put( trp_obj_t *queue, trp_obj_t *obj )
{
    trp_queue_elem *p;

    if ( queue->tipo != TRP_QUEUE )
        return 1;
    p = trp_gc_malloc( sizeof( trp_queue_elem ) );
    p->val = obj;
    p->next = NULL;
    if ( ((trp_queue_t *)queue)->len )
        ((trp_queue_elem *)(((trp_queue_t *)queue)->last))->next = p;
    else
        ((trp_queue_t *)queue)->first = p;
    ((trp_queue_t *)queue)->last = p;
    ((trp_queue_t *)queue)->len++;
    return 0;
}

trp_obj_t *trp_queue_get( trp_obj_t *queue )
{
    trp_obj_t *res;
    trp_queue_elem *p;

    if ( queue->tipo != TRP_QUEUE )
        return UNDEF;
    if ( ((trp_queue_t *)queue)->len == 0 )
        return UNDEF;
    ((trp_queue_t *)queue)->len--;
    p = (trp_queue_elem *)(((trp_queue_t *)queue)->first);
    res = p->val;
    ((trp_queue_t *)queue)->first = p->next;
    if ( ((trp_queue_t *)queue)->len == 0 )
        ((trp_queue_t *)queue)->last = NULL;
    trp_gc_free( p );
    return res;
}

uns8b trp_queue_swap( trp_obj_t *obj,  trp_obj_t *i,  trp_obj_t *j )
{
    uns32b ii, jj;

    if ( ( obj->tipo != TRP_QUEUE ) ||
         trp_cast_uns32b( i, &ii ) ||
         trp_cast_uns32b( j, &jj ) )
        return 1;
    if ( ( ii >= ((trp_queue_t *)obj)->len ) ||
         ( jj >= ((trp_queue_t *)obj)->len ) )
        return 1;
    if ( ii != jj ) {
        trp_queue_elem *p1, *p2;

        if ( jj < ii ) {
            uns32b k = ii;
            ii = jj;
            jj = k;
        }
        jj -= ii;
        for ( p1 = (trp_queue_elem *)(((trp_queue_t *)obj)->first) ;
              ii ;
              p1 = (trp_queue_elem *)( p1->next ), ii-- );
        for ( p2 = p1 ;
              jj ;
              p2 = (trp_queue_elem *)( p2->next ), jj-- );
        obj = p1->val;
        p1->val = p2->val;
        p2->val = obj;
    }
    return 0;
}

