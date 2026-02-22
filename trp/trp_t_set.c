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
#include "avl_tree.h"

typedef struct {
    struct avl_tree_node node;
    trp_obj_t *val;
} trp_set_item_t;

static int trp_set_less_i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 );
static int trp_set_less_l( const void *p, const struct avl_tree_node *node );
static void trp_set_insert_low( trp_set_t *s, trp_obj_t *x );

uns8b trp_set_print( trp_print_t *p, trp_set_t *obj )
{
    if ( trp_print_char_star( p, "#set (length=" ) )
        return 1;
    if ( trp_print_sig64( p, obj->len ) )
        return 1;
    return trp_print_char_star( p, ")#" );
}

uns32b trp_set_size( trp_set_t *obj )
{
    extern uns32b trp_size_internal( trp_obj_t * );
    struct avl_tree_node *node;
    uns32b sz = 1 + 4;

    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(obj->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        sz += trp_size_internal( ((trp_set_item_t *)(node->dummy))->val );
    return sz;
}

void trp_set_encode( trp_set_t *obj, uns8b **buf )
{
    extern void trp_encode_internal( trp_obj_t *, uns8b ** );
    uns32b *p;
    struct avl_tree_node *node;

    **buf = TRP_SET;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(obj->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        trp_encode_internal( ((trp_set_item_t *)(node->dummy))->val, buf );
}

trp_obj_t *trp_set_decode( uns8b **buf )
{
    extern trp_obj_t *trp_decode_internal( uns8b ** );
    trp_obj_t *obj = trp_set( NULL );
    trp_set_item_t *item;
    uns32b len;

    ((trp_set_t *)obj)->len = len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    for ( ; len ; len-- ) {
        item = trp_gc_malloc( sizeof( trp_set_item_t ) );
        item->node.dummy = item;
        item->val = trp_decode_internal( buf );
        avl_tree_insert( (struct avl_tree_node **)(&(((trp_set_t *)obj)->root)), &(item->node), trp_set_less_i );
    }
    return obj;
}

trp_obj_t *trp_set_equal( trp_set_t *o1, trp_set_t *o2 )
{
    struct avl_tree_node *node;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(o1->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        if ( avl_tree_lookup( (struct avl_tree_node *)(o2->root), (void *)(((trp_set_item_t *)(node->dummy))->val), trp_set_less_l ) == NULL )
            return TRP_FALSE;
    return TRP_TRUE;
}

trp_obj_t *trp_set_length( trp_set_t *obj )
{
    return trp_sig64( obj->len );
}

uns8b trp_set_in( trp_obj_t *obj, trp_set_t *seq, uns32b *pos, uns32b nth )
{
    if ( nth || ( seq->len == 0 ) )
        return 1;
    if ( avl_tree_lookup( (struct avl_tree_node *)(seq->root), (void *)obj, trp_set_less_l ) == NULL )
        return 1;
    *pos = 0;
    return 0;
}

trp_obj_t *trp_set_cat( trp_set_t *obj, va_list args )
{
    trp_obj_t *s = trp_set( NULL );
    struct avl_tree_node *node;

    for ( ; obj ; obj = va_arg( args, trp_set_t * ) ) {
        if ( obj->tipo != TRP_SET )
            return UNDEF;
        for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(obj->root) ) ;
              node ;
              node = avl_tree_next_in_order( node ) )
            trp_set_insert_low( (trp_set_t *)s, ((trp_set_item_t *)(node->dummy))->val );
    }
    return s;
}

static int trp_set_less_i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 )
{
    if ( trp_equal( ((trp_set_item_t *)(node1->dummy))->val, ((trp_set_item_t *)(node2->dummy))->val ) == TRP_TRUE )
        return 0;
    return ( trp_less( ((trp_set_item_t *)(node1->dummy))->val, ((trp_set_item_t *)(node2->dummy))->val ) == TRP_TRUE ) ? -1 : 1;
}

static int trp_set_less_l( const void *p, const struct avl_tree_node *node )
{
    if ( trp_equal( (trp_obj_t *)p, ((trp_set_item_t *)(node->dummy))->val ) == TRP_TRUE )
        return 0;
    return ( trp_less( (trp_obj_t *)p, ((trp_set_item_t *)(node->dummy))->val ) == TRP_TRUE ) ? -1 : 1;
}

trp_obj_t *trp_set( trp_obj_t *x, ... )
{
    trp_set_t *obj;
    va_list args;

    obj = trp_gc_malloc( sizeof( trp_set_t ) );
    obj->tipo = TRP_SET;
    obj->len = 0;
    obj->root = NULL;
    va_start( args, x );
    for ( ; x ; x = va_arg( args, trp_obj_t * ) )
        trp_set_insert_low( obj, x );
    va_end( args );
    return (trp_obj_t *)obj;
}

static void trp_set_insert_low( trp_set_t *s, trp_obj_t *x )
{
    if ( avl_tree_lookup( (struct avl_tree_node *)(s->root), (void *)x, trp_set_less_l ) == NULL ) {
        trp_set_item_t *item;

        item = trp_gc_malloc( sizeof( trp_set_item_t ) );
        item->node.dummy = item;
        item->val = x;
        avl_tree_insert( (struct avl_tree_node **)(&(s->root)), &(item->node), trp_set_less_i );
        s->len++;
    }
}

uns8b trp_set_insert( trp_obj_t *s, ... )
{
    trp_obj_t *x;
    va_list args;

    if ( s->tipo != TRP_SET )
        return 1;
    va_start( args, s );
    for ( x = va_arg( args, trp_obj_t * ) ; x ; x = va_arg( args, trp_obj_t * ) )
        trp_set_insert_low( (trp_set_t *)s, x );
    va_end( args );
    return 0;
}

trp_obj_t *trp_set_queue( trp_obj_t *s )
{
    trp_obj_t *res;
    struct avl_tree_node *node;

    if ( s->tipo != TRP_SET )
        return UNDEF;
    res= trp_queue();
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_set_t *)s)->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        trp_queue_put( res, ((trp_set_item_t *)(node->dummy))->val );
    return res;
}

uns8b trp_set_remove( trp_obj_t *s, trp_obj_t *x )
{
    struct avl_tree_node *node;

    if ( s->tipo != TRP_SET )
        return 1;
    node = avl_tree_lookup( (struct avl_tree_node *)(((trp_set_t *)s)->root), (void *)x, trp_set_less_l );
    if ( node == NULL )
        return 1;
    avl_tree_remove( (struct avl_tree_node **)(&(((trp_set_t *)s)->root)), node );
    ((trp_set_t *)s)->len--;
    return 0;
}

trp_obj_t *trp_set_intersection( trp_obj_t *s, ... )
{
    trp_obj_t *res, *sm = NULL;
    va_list args;
    uns32b m = 0xffffffff;

    va_start( args, s );
    for ( res = s ; res ; res = va_arg( args, trp_obj_t * ) ) {
        if ( res->tipo != TRP_SET ) {
            va_end( args );
            return UNDEF;
        }
        if ( ((trp_set_t *)res)->len < m ) {
            m = ((trp_set_t *)res)->len;
            sm = res;
        }
    }
    va_end( args );
    res = trp_set( NULL );
    if ( sm ) {
        trp_obj_t *obj, *t;
        struct avl_tree_node *node;

        for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_set_t *)sm)->root) ) ;
              node ;
              node = avl_tree_next_in_order( node ) ) {
            obj = ((trp_set_item_t *)(node->dummy))->val;
            va_start( args, s );
            for ( t = s ; ; t = va_arg( args, trp_obj_t * ) ) {
                if ( t == NULL ) {
                    trp_set_insert_low( (trp_set_t *)res, obj );
                    break;
                }
                if ( t == sm )
                    continue;
                if ( avl_tree_lookup( (struct avl_tree_node *)(((trp_set_t *)t)->root), (void *)obj, trp_set_less_l ) == NULL )
                    break;
            }
            va_end( args );
        }
    }
    return res;
}

trp_obj_t *trp_set_difference( trp_obj_t *s1, trp_obj_t *s2 )
{
    trp_obj_t *s;
    struct avl_tree_node *node;

    if ( ( s1->tipo != TRP_SET ) || ( s2->tipo != TRP_SET ) )
        return UNDEF;
    s = trp_set( NULL );
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_set_t *)s1)->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        if ( avl_tree_lookup( (struct avl_tree_node *)(((trp_set_t *)s2)->root),
                              (void *)(((trp_set_item_t *)(node->dummy))->val),
                              trp_set_less_l ) == NULL )
            trp_set_insert_low( (trp_set_t *)s, ((trp_set_item_t *)(node->dummy))->val );
    return s;
}

trp_obj_t *trp_set_are_disjoint( trp_obj_t *s1, trp_obj_t *s2 )
{
    struct avl_tree_node *node;

    if ( ( s1->tipo != TRP_SET ) || ( s2->tipo != TRP_SET ) )
        return UNDEF;
    if ( ((trp_set_t *)s2)->len < ((trp_set_t *)s1)->len ) {
        trp_obj_t *tmp = s1;
        s1 = s2;
        s2 = tmp;
    }
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_set_t *)s1)->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        if ( avl_tree_lookup( (struct avl_tree_node *)(((trp_set_t *)s2)->root),
                              (void *)(((trp_set_item_t *)(node->dummy))->val),
                              trp_set_less_l ) )
            return TRP_FALSE;
    return TRP_TRUE;
}

