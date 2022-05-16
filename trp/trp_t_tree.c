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

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );
extern void trp_queue_init_internal( trp_queue_t *q );

static uns8b trp_tree_print_level( trp_print_t *p, trp_tree_t *obj, uns16b level );
static trp_obj_t *trp_tree_internal( trp_obj_t *root );

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

uns8b trp_tree_print( trp_print_t *p, trp_tree_t *obj )
{
    return trp_tree_print_level( p, obj, 0 );
}

static uns8b trp_tree_print_level( trp_print_t *p, trp_tree_t *obj, uns16b level )
{
    trp_queue_elem *elem;
    uns16b i;

    for ( i = 0 ; i < level ; i++ )
        if ( trp_print_char_star( p, "    " ) )
            return 1;
    if ( trp_print_obj( p, obj->root ) )
        return 1;
    for ( elem = (trp_queue_elem *)( obj->children.first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) ) {
        if ( trp_print_char( p, '\n' ) )
            return 1;
        if ( trp_tree_print_level( p, (trp_tree_t *)( elem->val ), level + 1 ) )
            return 1;
    }
    return 0;
}

uns32b trp_tree_size( trp_tree_t *obj )
{
    return 1 + trp_size_internal( obj->root ) +
        trp_size_internal( (trp_obj_t *)( &( obj->children ) ) );
}

void trp_tree_encode( trp_tree_t *obj, uns8b **buf )
{
    **buf = TRP_TREE;
    ++(*buf);
    trp_encode_internal( obj->root, buf );
    trp_encode_internal( (trp_obj_t *)( &( obj->children ) ), buf );
}

trp_obj_t *trp_tree_decode( uns8b **buf )
{
    uns32b len;
    trp_obj_t *res = trp_tree_internal( trp_decode_internal( buf ) );

    ++(*buf); /* TRP_QUEUE */
    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    for ( ; len ; len-- )
        trp_queue_put( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ),
                       trp_decode_internal( buf ) );
    return res;
}

trp_obj_t *trp_tree_equal( trp_tree_t *o1, trp_tree_t *o2 )
{
    if ( trp_equal( o1->root, o2->root ) == TRP_FALSE )
        return TRP_FALSE;
    return trp_queue_equal( &( o1->children ), &( o2->children ) );
}

trp_obj_t *trp_tree_less( trp_tree_t *o1, trp_tree_t *o2 )
{
    return ( o1->children.len < o2->children.len ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_tree_length( trp_tree_t *t )
{
    return trp_sig64( t->children.len );
}

trp_obj_t *trp_tree_nth( uns32b n, trp_tree_t *obj )
{
    return trp_queue_nth( n, &( obj->children ) );
}

static trp_obj_t *trp_tree_internal( trp_obj_t *root )
{
    trp_tree_t *res = trp_gc_malloc( sizeof( trp_tree_t ) );

    res->tipo = TRP_TREE;
    res->root = root;
    trp_queue_init_internal( &( res->children ) );
    return (trp_obj_t *)res;
}

trp_obj_t *trp_tree( trp_obj_t *root, trp_obj_t *child, ... )
{
    trp_obj_t *res = trp_tree_internal( root );
    va_list args;

    va_start( args, child );
    for ( ; child ; child = va_arg( args, trp_obj_t * ) ) {
        if ( child->tipo != TRP_TREE ) {
            while ( ((trp_tree_t *)res)->children.len )
                (void)trp_queue_get( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ) );
            va_end( args );
            trp_gc_free( res );
            return UNDEF;
        }
        trp_queue_put( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ), child );
    }
    va_end( args );
    return res;
}

trp_obj_t *trp_tree_list( trp_obj_t *root, trp_obj_t *children )
{
    trp_obj_t *res = trp_tree_internal( root ), *child;
    trp_queue_elem *elem;
    int fail = 0;

    switch ( children->tipo ) {
    case TRP_CONS:
        for ( ; ;  ) {
            child = ((trp_cons_t *)children)->car;
            if ( child->tipo != TRP_TREE ) {
                fail = 1;
                break;
            }
            trp_queue_put( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ), child );
            children = ((trp_cons_t *)children)->cdr;
            if ( children->tipo != TRP_CONS ) {
                if ( children != NIL )
                    fail = 1;
                break;
            }
        }
        break;
    case TRP_QUEUE:
        for ( elem = (trp_queue_elem *)( ((trp_queue_t *)children)->first ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) ) {
            child = elem->val;
            if ( child->tipo != TRP_TREE ) {
                fail = 1;
                break;
            }
            trp_queue_put( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ), child );
        }
        break;
    default:
        if ( children != NIL )
            fail = 1;
        break;
    }
    if ( fail ) {
        while ( ((trp_tree_t *)res)->children.len )
            (void)trp_queue_get( (trp_obj_t *)( &( ((trp_tree_t *)res)->children ) ) );
        trp_gc_free( res );
        res = UNDEF;
    }
    return res;
}

trp_obj_t *trp_tree_root( trp_obj_t *obj )
{
    return ( obj->tipo == TRP_TREE ) ? ((trp_tree_t *)obj)->root : UNDEF;
}

trp_obj_t *trp_tree_swap( trp_obj_t *obj,  trp_obj_t *i,  trp_obj_t *j )
{
    trp_obj_t *children;
    trp_queue_elem *elem, *elemi, *elemj;
    uns32b ii, jj, len;

    if ( ( obj->tipo != TRP_TREE ) ||
         trp_cast_uns32b( i, &ii ) ||
         trp_cast_uns32b( j, &jj ) )
        return UNDEF;
    len = ((trp_tree_t *)obj)->children.len;
    if ( ( ii >= len ) || ( jj >= len ) )
        return UNDEF;
    if ( ii == jj )
        return obj;
    children = trp_queue();
    for ( elem = (trp_queue_elem *)( ((trp_tree_t *)obj)->children.first ), len = 0 ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ), len++ ) {
        (void)trp_queue_put( children, elem->val );
        if ( ii == len ) {
            i = elem->val;
            elemi = (trp_queue_elem *)(((trp_queue_t *)children)->last);
        }
        else if ( jj == len ) {
            j = elem->val;
            elemj = (trp_queue_elem *)(((trp_queue_t *)children)->last);
        }
    }
    elemi->val = j;
    elemj->val = i;
    return trp_tree_list( ((trp_tree_t *)obj)->root, children );
}

