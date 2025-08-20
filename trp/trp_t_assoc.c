/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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
    uns8b *key;
    trp_obj_t *val;
} trp_assoc_item_t;

static int trp_assoc_less_i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 );
static int trp_assoc_less_l( const void *p, const struct avl_tree_node *node );

uns8b trp_assoc_print( trp_print_t *p, trp_assoc_t *obj )
{
    if ( trp_print_char_star( p, "#assoc (length=" ) )
        return 1;
    if ( trp_print_sig64( p, obj->len ) )
        return 1;
    return trp_print_char_star( p, ")#" );
}

uns32b trp_assoc_size( trp_assoc_t *obj )
{
    extern uns32b trp_size_internal( trp_obj_t * );
    struct avl_tree_node *node;
    uns32b sz = 1 + 4;

    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(obj->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        sz += strlen( ((trp_assoc_item_t *)(node->dummy))->key ) + 1 +
              trp_size_internal( ((trp_assoc_item_t *)(node->dummy))->val );
    return sz;
}

void trp_assoc_encode( trp_assoc_t *obj, uns8b **buf )
{
    extern void trp_encode_internal( trp_obj_t *, uns8b ** );
    uns32b *p;
    struct avl_tree_node *node;

    **buf = TRP_ASSOC;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(obj->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) ) {
        strcpy( *buf, ((trp_assoc_item_t *)(node->dummy))->key );
        (*buf) += strlen( ((trp_assoc_item_t *)(node->dummy))->key ) + 1;
        trp_encode_internal( ((trp_assoc_item_t *)(node->dummy))->val, buf );
    }
}

trp_obj_t *trp_assoc_decode( uns8b **buf )
{
    extern trp_obj_t *trp_decode_internal( uns8b ** );
    trp_obj_t *obj = trp_assoc();
    trp_assoc_item_t *item;
    uns8b *c;
    uns32b len;
    int l;

    ((trp_assoc_t *)obj)->len = len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    for ( ; len ; len-- ) {
        l = strlen( *buf ) + 1;
        c = trp_gc_malloc_atomic( l );
        strcpy( c, *buf );
        (*buf) += l;
        item = trp_gc_malloc( sizeof( trp_assoc_item_t ) );
        item->node.dummy = item;
        item->key = c;
        item->val = trp_decode_internal( buf );
        avl_tree_insert( (struct avl_tree_node **)(&(((trp_assoc_t *)obj)->root)), &(item->node), trp_assoc_less_i );
    }
    return obj;
}

trp_obj_t *trp_assoc_equal( trp_assoc_t *o1, trp_assoc_t *o2 )
{
    struct avl_tree_node *node1, *node2;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( node1 = avl_tree_first_in_order( (struct avl_tree_node *)(o1->root) ) ;
          node1 ;
          node1 = avl_tree_next_in_order( node1 ) ) {
        node2 = avl_tree_lookup( (struct avl_tree_node *)(o2->root), (void *)(((trp_assoc_item_t *)(node1->dummy))->key), trp_assoc_less_l );
        if ( node2 == NULL )
            return TRP_FALSE;
        if ( trp_equal( ((trp_assoc_item_t *)(node1->dummy))->val, ((trp_assoc_item_t *)(node2->dummy))->val ) != TRP_TRUE )
            return TRP_FALSE;
    }
    return TRP_TRUE;
}

trp_obj_t *trp_assoc_length( trp_assoc_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_assoc_height( trp_assoc_t *obj )
{
    return ZERO;
}

uns8b trp_assoc_in( trp_obj_t *obj, trp_assoc_t *seq, uns32b *pos, uns32b nth )
{
    uns8b *c;
    struct avl_tree_node *node;

    if ( nth || ( seq->len == 0 ) )
        return 1;
    c = trp_csprint( obj );
    node = avl_tree_lookup( (struct avl_tree_node *)(seq->root), (void *)c, trp_assoc_less_l );
    trp_csprint_free( c );
    if ( node == NULL )
        return 1;
    *pos = 0;
    return 0;
}

static int trp_assoc_less_i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 )
{
    return strcmp( ((trp_assoc_item_t *)(node1->dummy))->key, ((trp_assoc_item_t *)(node2->dummy))->key );
}

static int trp_assoc_less_l( const void *p, const struct avl_tree_node *node )
{
    return strcmp( (uns8b *)p, ((trp_assoc_item_t *)(node->dummy))->key );
}

trp_obj_t *trp_assoc()
{
    trp_assoc_t *obj;

    obj = trp_gc_malloc( sizeof( trp_assoc_t ) );
    obj->tipo = TRP_ASSOC;
    obj->len = 0;
    obj->root = NULL;
    return (trp_obj_t *)obj;
}

uns8b trp_assoc_set( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val )
{
    uns8b *c;
    trp_assoc_item_t *item;

    if ( val == UNDEF )
        return trp_assoc_clr( obj, key );
    if ( obj->tipo != TRP_ASSOC )
        return 1;
    c = trp_csprint( key );
    if ( ((trp_assoc_t *)obj)->len ) {
        struct avl_tree_node *node = avl_tree_lookup( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root), (void *)c, trp_assoc_less_l );

        if ( node ) {
            ((trp_assoc_item_t *)(node->dummy))->val = val;
            trp_csprint_free( c );
            return 0;
        }
    }
    item = trp_gc_malloc( sizeof( trp_assoc_item_t ) );
    item->node.dummy = item;
    item->key = c;
    item->val = val;
    if ( avl_tree_insert( (struct avl_tree_node **)(&(((trp_assoc_t *)obj)->root)), &(item->node), trp_assoc_less_i ) ) {
        trp_gc_free( item );
        trp_csprint_free( c );
        return 1;
    }
    ((trp_assoc_t *)obj)->len++;
    return 0;
}

uns8b trp_assoc_inc( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val )
{
    uns8b *c;
    struct avl_tree_node *node;

    if ( val == UNDEF )
        return trp_assoc_clr( obj, key );
    if ( val == NULL )
        val = UNO;
    if ( ((trp_assoc_t *)obj)->len == 0 )
        return trp_assoc_set( obj, key, val );
    c = trp_csprint( key );
    node = avl_tree_lookup( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root), (void *)c, trp_assoc_less_l );
    trp_csprint_free( c );
    if ( node == NULL )
        return trp_assoc_set( obj, key, val );
    val = trp_cat( ((trp_assoc_item_t *)(node->dummy))->val, val, NULL );
    if ( val != UNDEF )
        ((trp_assoc_item_t *)(node->dummy))->val = val;
    return 0;
}

uns8b trp_assoc_clr( trp_obj_t *obj, trp_obj_t *key )
{
    uns8b *c;
    struct avl_tree_node *node;

    if ( obj->tipo != TRP_ASSOC )
        return 1;
    if ( ((trp_assoc_t *)obj)->len == 0 )
        return 0;
    c = trp_csprint( key );
    node = avl_tree_lookup( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root), (void *)c, trp_assoc_less_l );
    trp_csprint_free( c );
    if ( node == NULL )
        return 0;
    avl_tree_remove( (struct avl_tree_node **)(&(((trp_assoc_t *)obj)->root)), node );
    ((trp_assoc_t *)obj)->len--;
    return 0;
}

trp_obj_t *trp_assoc_get( trp_obj_t *obj, trp_obj_t *key )
{
    uns8b *c;
    struct avl_tree_node *node;

    if ( obj->tipo != TRP_ASSOC )
        return UNDEF;
    if ( ((trp_assoc_t *)obj)->len == 0 )
        return UNDEF;
    c = trp_csprint( key );
    node = avl_tree_lookup( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root), (void *)c, trp_assoc_less_l );
    trp_csprint_free( c );
    if ( node == NULL )
        return UNDEF;
    return ((trp_assoc_item_t *)(node->dummy))->val;
}

trp_obj_t *trp_assoc_queue( trp_obj_t *obj )
{
    trp_obj_t *res;
    struct avl_tree_node *node;

    if ( obj->tipo != TRP_ASSOC )
        return UNDEF;
    res= trp_queue();
    for ( node = avl_tree_first_in_order( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root) ) ;
          node ;
          node = avl_tree_next_in_order( node ) )
        trp_queue_put( res, trp_cons( trp_cord( ((trp_assoc_item_t *)(node->dummy))->key ), ((trp_assoc_item_t *)(node->dummy))->val ) );
    return res;
}

trp_obj_t *trp_assoc_root( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_ASSOC )
        return UNDEF;
    if ( ((trp_assoc_t *)obj)->len == 0 )
        return UNDEF;
    return trp_cord( ((trp_assoc_item_t *)(avl_tree_first_in_order( (struct avl_tree_node *)(((trp_assoc_t *)obj)->root) )->dummy))->key );
}

