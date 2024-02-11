/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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
    uns8b tipo;
    uns8b sottotipo;
    void *dgraph;
    trp_obj_t *val;
    trp_obj_t *name;
    uns32b len_in;
    uns32b len_out;
    struct avl_tree_node *root_out;
    struct avl_tree_node *root_in;
    struct avl_tree_node *node;
} trp_dgraph_node_t;

typedef struct {
    struct avl_tree_node node;
    trp_obj_t *n;
} trp_dgraph_link_out_t;

typedef struct {
    struct avl_tree_node node;
    trp_obj_t *n;
    trp_obj_t *val;
} trp_dgraph_link_in_t;

#define trp_dgraph_root_nodes(g) ((struct avl_tree_node *)(((trp_dgraph_t *)(g))->root_nodes))
#define trp_dgraph_root_nodes_ptr(g) ((struct avl_tree_node **)&(((trp_dgraph_t *)(g))->root_nodes))
#define trp_dgraph_link_out_is_member(n,r) (avl_tree_lookup((struct avl_tree_node *)(r),(void *)(n),trp_dgraph_less2l))
#define trp_dgraph_link_out_is_not_member(n,r) (avl_tree_lookup((struct avl_tree_node *)(r),(void *)(n),trp_dgraph_less2l)==NULL)
static uns8b trp_dgraph_check( trp_obj_t *g );
static uns8b trp_dgraph_node_check( trp_obj_t *g );
static int trp_dgraph_less1i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 );
/* static int trp_dgraph_less1l( const void *p, const struct avl_tree_node *node ); */
static int trp_dgraph_less2i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 );
static int trp_dgraph_less2l( const void *p, const struct avl_tree_node *node );
static void trp_dgraph_link_out_insert_item( trp_dgraph_node_t *n, struct avl_tree_node **root );
static uns8b trp_dgraph_link_low( trp_dgraph_t *g, trp_dgraph_node_t *n1, trp_dgraph_node_t *n2, trp_obj_t *val );
static uns8b trp_dgraph_can_reach_low( trp_dgraph_node_t *n1, trp_dgraph_node_t *n2 );
static uns8b trp_dgraph_can_reach_low_low( trp_dgraph_node_t *n1, trp_dgraph_node_t *n2, struct avl_tree_node **root );
static void trp_dgraph_detach_link_low( trp_obj_t *g, trp_obj_t *n1, trp_obj_t *n2, struct avl_tree_node *node1, struct avl_tree_node *node2 );
static uns8b trp_dgraph_detach_link( trp_obj_t *n1, trp_obj_t *n2 );
static uns8b trp_dgraph_detach_node( trp_obj_t *n );
static uns32b trp_dgraph_succ_cnt_low( trp_dgraph_node_t *n );
static uns32b trp_dgraph_succ_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root );
static uns32b trp_dgraph_pred_cnt_low( trp_dgraph_node_t *n );
static uns32b trp_dgraph_pred_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root );
static trp_obj_t *trp_dgraph_root_if_is_tree_low( trp_dgraph_t *g );
static trp_obj_t *trp_dgraph_queue_succ_low( trp_dgraph_node_t *n );
static void trp_dgraph_queue_succ_low_low( trp_obj_t *q, trp_dgraph_node_t *n, struct avl_tree_node **root );
static trp_obj_t *trp_dgraph_queue_pred_low( trp_dgraph_node_t *n );
static void trp_dgraph_queue_pred_low_low( trp_obj_t *q, trp_dgraph_node_t *n, struct avl_tree_node **root );
static uns32b trp_dgraph_connected_cnt_low( trp_dgraph_node_t *n );
static uns32b trp_dgraph_connected_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root );
static uns8b trp_dgraph_detect_a_cycle( uns8b flags, trp_dgraph_node_t *n, struct avl_tree_node **root_local, struct avl_tree_node **root_global );

uns8b trp_dgraph_print( trp_print_t *p, trp_dgraph_t *obj )
{
    if ( trp_print_char_star( p, "#dgraph " ) )
        return 1;
    if ( obj->sottotipo ) {
        trp_dgraph_t *g = (trp_dgraph_t *)(((trp_dgraph_node_t *)obj)->dgraph);

        if ( trp_print_char_star( p, "node" ) )
            return 1;
        if ( g ) {
            if ( trp_print_char_star( p, " in: " ) )
                return 1;
            if ( trp_print_sig64( p, ((trp_dgraph_node_t *)obj)->len_in ) )
                return 1;
            if ( trp_print_char_star( p, " out: " ) )
                return 1;
            if ( trp_print_sig64( p, ((trp_dgraph_node_t *)obj)->len_out ) )
                return 1;
            if ( trp_print_char_star( p, " (member of " ) )
                return 1;
            if ( trp_dgraph_print( p, g ) )
                return 1;
        } else {
            if ( trp_print_char_star( p, " (detached" ) )
                return 1;
        }
    } else {
        if ( trp_print_char_star( p, "(nodes=" ) )
            return 1;
        if ( trp_print_sig64( p, obj->len ) )
            return 1;
        if ( trp_print_char_star( p, ", links=" ) )
            return 1;
        if ( trp_print_sig64( p, obj->link_cnt ) )
            return 1;
        if ( trp_print_char_star( p, ", names=" ) )
            return 1;
        if ( obj->names ) {
            if ( trp_print_sig64( p, ((trp_assoc_t *)(obj->names))->len ) )
                return 1;
        } else {
            if ( trp_print_char_star( p, "0" ) )
                return 1;
        }
    }
    return trp_print_char_star( p, ")#" );
}

uns32b trp_dgraph_size( trp_dgraph_t *obj )
{
    extern uns32b trp_size_internal( trp_obj_t * );
    uns32b sz;

    if ( obj->sottotipo )
        return trp_special_size( (trp_special_t *)UNDEF );
    sz = 1 + 4;
    if ( obj->len ) {
        /*
         * FIXME
         */
    }
    return sz;
}

void trp_dgraph_encode( trp_dgraph_t *obj, uns8b **buf )
{
    extern void trp_encode_internal( trp_obj_t *, uns8b ** );
    uns32b *p;

    if ( obj->sottotipo ) {
        trp_special_encode( (trp_special_t *)UNDEF, buf );
        return;
    }
    **buf = TRP_DGRAPH;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    if ( obj->len ) {
        /*
         * FIXME
         */
    }
}

trp_obj_t *trp_dgraph_decode( uns8b **buf )
{
    extern trp_obj_t *trp_decode_internal( uns8b ** );
    trp_obj_t *res = trp_dgraph();
    uns32b len;

    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    /*
     * FIXME
     */
    return res;
}

trp_obj_t *trp_dgraph_length( trp_dgraph_t *obj )
{
    uns32b len;

    if ( obj->sottotipo )
        len = ((trp_dgraph_node_t *)obj)->len_out;
    else
        len = obj->len;
    return trp_sig64( len );
}

static uns8b trp_dgraph_check( trp_obj_t *g )
{
    if ( g->tipo != TRP_DGRAPH )
        return 1;
    return ( ((trp_dgraph_t *)g)->sottotipo ) ? 1 : 0;
}

static uns8b trp_dgraph_node_check( trp_obj_t *g )
{
    if ( g->tipo != TRP_DGRAPH )
        return 1;
    return ( ((trp_dgraph_t *)g)->sottotipo ) ? 0 : 1;
}

static int trp_dgraph_less1i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 )
{
    void *p = node1->dummy;
    void *q = node2->dummy;

    if ( p < q )
        return -1;
    if ( p > q )
        return 1;
    return 0;
}

/*

static int trp_dgraph_less1l( const void *p, const struct avl_tree_node *node )
{
    void *q = node->dummy;

    if ( p < q )
        return -1;
    if ( p > q )
        return 1;
    return 0;
}

*/

static int trp_dgraph_less2i( const struct avl_tree_node *node1, const struct avl_tree_node *node2 )
{
    void *p = (void *)(((trp_dgraph_link_out_t *)(node1->dummy))->n);
    void *q = (void *)(((trp_dgraph_link_out_t *)(node2->dummy))->n);

    if ( p < q )
        return -1;
    if ( p > q )
        return 1;
    return 0;
}

static int trp_dgraph_less2l( const void *p, const struct avl_tree_node *node )
{
    void *q = (void *)(((trp_dgraph_link_out_t *)(node->dummy))->n);

    if ( p < q )
        return -1;
    if ( p > q )
        return 1;
    return 0;
}

static void trp_dgraph_link_out_insert_item( trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    trp_dgraph_link_out_t *l = trp_gc_malloc( sizeof( trp_dgraph_link_out_t ) );

    l->node.dummy = (void *)l;
    l->n = (trp_obj_t *)n;
    avl_tree_insert( root, &(l->node), trp_dgraph_less2i );
}

trp_obj_t *trp_dgraph()
{
    trp_dgraph_t *g;

    g = trp_gc_malloc( sizeof( trp_dgraph_t ) );
    g->tipo = TRP_DGRAPH;
    g->sottotipo = 0;
    g->len = 0;
    g->link_cnt = 0;
    g->root_nodes = NULL;
    g->names = NULL;
    return (trp_obj_t *)g;
}

trp_obj_t *trp_dgraph_first( trp_obj_t *g )
{
    if ( trp_dgraph_check( g ) )
        return UNDEF;
    if ( ((trp_dgraph_t *)g)->len == 0 )
        return UNDEF;
    return (trp_obj_t *)( avl_tree_first_in_order(trp_dgraph_root_nodes(g))->dummy );
}

trp_obj_t *trp_dgraph_queue( trp_obj_t *g )
{
    trp_obj_t *res;
    struct avl_tree_node *node;

    if ( trp_dgraph_check( g ) ) {
        if ( trp_dgraph_node_check( g ) )
            return UNDEF;
        return trp_dgraph_queue_out( g );
    }
    res= trp_queue();
    for ( node = avl_tree_first_in_order( trp_dgraph_root_nodes( g ) ) ; node ; node = avl_tree_next_in_order( node ) )
        trp_queue_put( res, node->dummy );
    return res;
}

trp_obj_t *trp_dgraph_queue_out( trp_obj_t *n )
{
    trp_obj_t *res;
    struct avl_tree_node *node;

    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    res= trp_queue();
    for ( node = avl_tree_first_in_order( ( (trp_dgraph_node_t *)n)->root_out ) ; node ; node = avl_tree_next_in_order( node ) )
        trp_queue_put( res, ((trp_dgraph_link_out_t *)(node->dummy))->n );
    return res;
}

trp_obj_t *trp_dgraph_queue_in( trp_obj_t *n )
{
    trp_obj_t *res;
    struct avl_tree_node *node;

    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    res= trp_queue();
    for ( node = avl_tree_first_in_order( ( (trp_dgraph_node_t *)n)->root_in ) ; node ; node = avl_tree_next_in_order( node ) )
        trp_queue_put( res, ((trp_dgraph_link_in_t *)(node->dummy))->n );
    return res;
}

/*
 * rende l'oggetto di tipo TRP_DGRAPH
 * di cui fa parte il nodo n
 */

trp_obj_t *trp_dgraph_dgraph( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    n = (trp_obj_t *)( ((trp_dgraph_node_t *)n)->dgraph );
    return n ? n : UNDEF;
}

trp_obj_t *trp_dgraph_node( trp_obj_t *g, trp_obj_t *val, trp_obj_t *name )
{
    trp_dgraph_node_t *n;
    struct avl_tree_node *node;

    if ( trp_dgraph_check( g ) )
        return UNDEF;
    n = trp_gc_malloc( sizeof( trp_dgraph_node_t ) );
    n->tipo = TRP_DGRAPH;
    n->sottotipo = 1;
    n->dgraph = (void *)g;
    n->val = val ? val : UNDEF;
    n->name = UNDEF;
    n->len_in = 0;
    n->len_out = 0;
    n->root_out = NULL;
    n->root_in = NULL;
    if ( name )
        if ( trp_dgraph_set_name( (trp_obj_t *)n, name ) ) {
            trp_gc_free( n );
            return UNDEF;
        }
    node = trp_gc_malloc( sizeof( struct avl_tree_node ) );
    node->dummy = (void *)n;
    n->node = node;
    if ( avl_tree_insert( trp_dgraph_root_nodes_ptr( g ), node, trp_dgraph_less1i ) ) {
        trp_gc_free( node );
        trp_gc_free( n );
        return UNDEF;
    }
    ((trp_dgraph_t *)g)->len++;
    return (trp_obj_t *)n;
}

trp_obj_t *trp_dgraph_is_node( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return TRP_FALSE;
    return TRP_TRUE;
}

trp_obj_t *trp_dgraph_out_cnt( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_sig64( ((trp_dgraph_node_t *)n)->len_out );
}

trp_obj_t *trp_dgraph_in_cnt( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_sig64( ((trp_dgraph_node_t *)n)->len_in );
}

trp_obj_t *trp_dgraph_get_val( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return ((trp_dgraph_node_t *)n)->val;
}

uns8b trp_dgraph_set_val( trp_obj_t *n, trp_obj_t *val )
{
    if ( trp_dgraph_node_check( n ) )
        return 1;
    ((trp_dgraph_node_t *)n)->val = val;
    return 0;
}

trp_obj_t *trp_dgraph_get_name( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return ((trp_dgraph_node_t *)n)->name;
}

uns8b trp_dgraph_set_name( trp_obj_t *n, trp_obj_t *name )
{
    trp_obj_t *g;

    g = trp_dgraph_dgraph( n );
    if ( g == UNDEF )
        return 1;
    if ( trp_equal( name, ((trp_dgraph_node_t *)n)->name ) == TRP_TRUE )
        return 0;
    if ( name == UNDEF ) {
        trp_assoc_clr( ((trp_dgraph_t *)g)->names, ((trp_dgraph_node_t *)n)->name );
        ((trp_dgraph_node_t *)n)->name = UNDEF;
        return 0;
    } else {
        if ( ((trp_dgraph_t *)g)->names == NULL )
            ((trp_dgraph_t *)g)->names = trp_assoc();
        if ( trp_assoc_get( ((trp_dgraph_t *)g)->names, name ) != UNDEF )
            return 1;
        if ( trp_assoc_set( ((trp_dgraph_t *)g)->names, name, n ) )
            return 1;
        if ( ((trp_dgraph_node_t *)n)->name != UNDEF )
            trp_assoc_clr( ((trp_dgraph_t *)g)->names, ((trp_dgraph_node_t *)n)->name );
        ((trp_dgraph_node_t *)n)->name = name;
    }
    return 0;
}

trp_obj_t *trp_dgraph_get_node( trp_obj_t *g, trp_obj_t *name )
{
    if ( trp_dgraph_check( g ) )
        return UNDEF;
    if ( ((trp_dgraph_t *)g)->names == NULL )
        return UNDEF;
    return trp_assoc_get( ((trp_dgraph_t *)g)->names, name );
}

trp_obj_t *trp_dgraph_get_id( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
#if _WIN64 || __x86_64__ || __ppc64__
    return trp_sig64( (sig64b)((uns64b)n) );
#else
    return trp_sig64( (sig64b)((uns32b)n) );
#endif
}

trp_obj_t *trp_dgraph_get_node_by_id( trp_obj_t *g, trp_obj_t *id )
{
    sig64b i;

    if ( trp_dgraph_check( g ) || trp_cast_sig64b( id, &i ) )
        return UNDEF;
#if _WIN64 || __x86_64__ || __ppc64__
    id = (trp_obj_t *)((uns64b)i);
#else
    id = (trp_obj_t *)((uns32b)i);
#endif
    if ( trp_dgraph_node_check( id ) )
        return UNDEF;
    return id;
}

static uns8b trp_dgraph_link_low( trp_dgraph_t *g, trp_dgraph_node_t *n1, trp_dgraph_node_t *n2, trp_obj_t *val )
{
    trp_dgraph_link_out_t *link_out;
    trp_dgraph_link_in_t *link_in;

    /* se sono già linkati, si fallisce */
    if ( avl_tree_lookup( n2->root_in, (void *)n1, trp_dgraph_less2l ) )
        return 1;
    link_out = trp_gc_malloc( sizeof( trp_dgraph_link_out_t ) );
    link_in = trp_gc_malloc( sizeof( trp_dgraph_link_in_t ) );
    link_out->node.dummy = (void *)link_out;
    link_out->n = (trp_obj_t *)n2;
    link_in->node.dummy = (void *)link_in;
    link_in->n = (trp_obj_t *)n1;
    link_in->val = val ? val : UNDEF;
    if ( avl_tree_insert( &(n1->root_out), &(link_out->node), trp_dgraph_less2i ) ) {
        trp_gc_free( link_out );
        trp_gc_free( link_in );
        return 1;
    }
    if ( avl_tree_insert( &(n2->root_in), &(link_in->node), trp_dgraph_less2i ) ) {
        avl_tree_remove( &(n1->root_out), &(link_out->node) );
        trp_gc_free( link_out );
        trp_gc_free( link_in );
        return 1;
    }
    n1->len_out++;
    n2->len_in++;
    g->link_cnt++;
    return 0;
}

uns8b trp_dgraph_link( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val )
{
    trp_obj_t *g;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return 1;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return 1;
    return trp_dgraph_link_low( (trp_dgraph_t *)g, (trp_dgraph_node_t *)n1, (trp_dgraph_node_t *)n2, val );
}

/*
 * rende 1 se da n1 si può arrivare a n2, 0 altrimenti
 */

static uns8b trp_dgraph_can_reach_low( trp_dgraph_node_t *n1, trp_dgraph_node_t *n2 )
{
    struct avl_tree_node *root = NULL;

    if ( n1 == n2 )
        return 1;
    return trp_dgraph_can_reach_low_low( n1, n2, &root );
}

static uns8b trp_dgraph_can_reach_low_low( trp_dgraph_node_t *n1, trp_dgraph_node_t *n2, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;

    trp_dgraph_link_out_insert_item( n1, root );
    for ( node = avl_tree_first_in_order( n1->root_out ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_out_t *)(node->dummy))->n);
        if ( m == n2 )
            return 1;
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            if ( trp_dgraph_can_reach_low_low( m, n2, root ) )
                return 1;
    }
    return 0;
}

/*
 * crea un link tra n1 e n2, ma solo se tale link non introduce un ciclo
 */

uns8b trp_dgraph_link_acyclic( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val )
{
    trp_obj_t *g;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return 1;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return 1;
    if ( trp_dgraph_can_reach_low( (trp_dgraph_node_t *)n2, (trp_dgraph_node_t *)n1 ) )
        return 1;
    return trp_dgraph_link_low( (trp_dgraph_t *)g, (trp_dgraph_node_t *)n1, (trp_dgraph_node_t *)n2, val );
}

trp_obj_t *trp_dgraph_can_reach( trp_obj_t *n1, trp_obj_t *n2 )
{
    trp_obj_t *g;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return TRP_FALSE;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return TRP_FALSE;
    return trp_dgraph_can_reach_low( (trp_dgraph_node_t *)n1, (trp_dgraph_node_t *)n2 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_dgraph_get_link_val( trp_obj_t *n1, trp_obj_t *n2 )
{
    trp_obj_t *g;
    struct avl_tree_node *res;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return UNDEF;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return UNDEF;
    res = avl_tree_lookup( ((trp_dgraph_node_t *)n2)->root_in, (void *)n1, trp_dgraph_less2l );
    if ( res == NULL )
        return UNDEF;
    return ((trp_dgraph_link_in_t *)(res->dummy))->val;
}

uns8b trp_dgraph_set_link_val( trp_obj_t *n1, trp_obj_t *n2, trp_obj_t *val )
{
    trp_obj_t *g;
    struct avl_tree_node *res;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return 1;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return 1;
    res = avl_tree_lookup( ((trp_dgraph_node_t *)n2)->root_in, (void *)n1, trp_dgraph_less2l );
    if ( res == NULL )
        return 1;
    ((trp_dgraph_link_in_t *)(res->dummy))->val = val;
    return 0;
}

static void trp_dgraph_detach_link_low( trp_obj_t *g, trp_obj_t *n1, trp_obj_t *n2, struct avl_tree_node *node1, struct avl_tree_node *node2 )
{
    avl_tree_remove( &(((trp_dgraph_node_t *)n1)->root_out), node1 );
    avl_tree_remove( &(((trp_dgraph_node_t *)n2)->root_in), node2 );
    ((trp_dgraph_node_t *)n1)->len_out--;
    ((trp_dgraph_node_t *)n2)->len_in--;
    ((trp_dgraph_t *)g)->link_cnt--;
}

static uns8b trp_dgraph_detach_link( trp_obj_t *n1, trp_obj_t *n2 )
{
    trp_obj_t *g;
    struct avl_tree_node *node1, *node2;

    g = trp_dgraph_dgraph( n1 );
    if ( g == UNDEF )
        return 1;
    if ( trp_dgraph_dgraph( n2 ) != g )
        return 1;
    node1 = avl_tree_lookup( ((trp_dgraph_node_t *)n1)->root_out, (void *)n2, trp_dgraph_less2l );
    if ( node1 == NULL )
        return 1;
    node2 = avl_tree_lookup( ((trp_dgraph_node_t *)n2)->root_in, (void *)n1, trp_dgraph_less2l );
    if ( node2 == NULL )
        return 1;
    trp_dgraph_detach_link_low( g, n1, n2, node1, node2 );
    return 0;
}

static uns8b trp_dgraph_detach_node( trp_obj_t *n )
{
    trp_obj_t *g = trp_dgraph_dgraph( n ), *m;
    struct avl_tree_node *node;

    if ( g == UNDEF )
        return 1;
    /* si rimuovono i link in uscita */
    while ( ( (trp_dgraph_node_t *)n)->root_out ) {
        node = avl_tree_first_in_order( ( (trp_dgraph_node_t *)n)->root_out );
        m = ((trp_dgraph_link_out_t *)( node->dummy ))->n;
        trp_dgraph_detach_link_low( g, n, m, node, avl_tree_lookup( ((trp_dgraph_node_t *)m)->root_in, (void *)n, trp_dgraph_less2l ) );
    }
    /* si rimuovono i link in ingresso */
    while ( ( (trp_dgraph_node_t *)n)->root_in ) {
        node = avl_tree_first_in_order( ( (trp_dgraph_node_t *)n)->root_in );
        m = ((trp_dgraph_link_in_t *)( node->dummy ))->n;
        trp_dgraph_detach_link_low( g, m, n, avl_tree_lookup( ((trp_dgraph_node_t *)m)->root_out, (void *)n, trp_dgraph_less2l ), node );
    }
    /* si rimuove il nodo dall'avl */
    avl_tree_remove( trp_dgraph_root_nodes_ptr( g ), ((trp_dgraph_node_t *)n)->node );
    ((trp_dgraph_node_t *)n)->dgraph = NULL;
    if ( ((trp_dgraph_node_t *)n)->name != UNDEF ) {
        trp_assoc_clr( ((trp_dgraph_t *)g)->names, ((trp_dgraph_node_t *)n)->name );
        ((trp_dgraph_node_t *)n)->name = UNDEF;
    }
    ((trp_dgraph_t *)g)->len--;
    return 0;
}

uns8b trp_dgraph_detach( trp_obj_t *n1, trp_obj_t *n2 )
{
    uns8b res;

    if ( n2 )
        res = trp_dgraph_detach_link( n1, n2 );
    else
        res = trp_dgraph_detach_node( n1 );
    return res;
}

/*
 * rende il numero di nodi raggiungibili da node (incluso)
 */

static uns32b trp_dgraph_succ_cnt_low( trp_dgraph_node_t *n )
{
    struct avl_tree_node *root = NULL;

    return trp_dgraph_succ_cnt_low_low( n, &root );
}

static uns32b trp_dgraph_succ_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;
    uns32b res = 1;

    trp_dgraph_link_out_insert_item( n, root );
    for ( node = avl_tree_first_in_order( n->root_out ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_out_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            res += trp_dgraph_succ_cnt_low_low( m, root );
    }
    return res;
}

/*
 * rende il numero di nodi che possono raggiungere node (incluso)
 */

static uns32b trp_dgraph_pred_cnt_low( trp_dgraph_node_t *n )
{
    struct avl_tree_node *root = NULL;

    return trp_dgraph_pred_cnt_low_low( n, &root );
}

static uns32b trp_dgraph_pred_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;
    uns32b res = 1;

    trp_dgraph_link_out_insert_item( n, root );
    for ( node = avl_tree_first_in_order( n->root_in ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_in_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            res += trp_dgraph_pred_cnt_low_low( m, root );
    }
    return res;
}

trp_obj_t *trp_dgraph_succ_cnt( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_sig64( trp_dgraph_succ_cnt_low( (trp_dgraph_node_t *)n ) );
}

trp_obj_t *trp_dgraph_pred_cnt( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_sig64( trp_dgraph_pred_cnt_low( (trp_dgraph_node_t *)n ) );
}

/*
 * rende la radice se g è un albero, altrimenti UNDEF
 */

static trp_obj_t *trp_dgraph_root_if_is_tree_low( trp_dgraph_t *g )
{
    trp_dgraph_node_t *n;
    struct avl_tree_node *node;
    uns32b i;

    if ( g->link_cnt + 1 != g->len )
        return UNDEF;
    node = avl_tree_first_in_order( trp_dgraph_root_nodes( g ) );
    n = (trp_dgraph_node_t *)(node->dummy);
    i = 1;
    while ( n->len_in ) {
        /* se i == g->len siamo necessariamente entrati in un ciclo */
        if ( ( n->len_in > 1 ) || ( i == g->len ) )
            return UNDEF;
        node = avl_tree_first_in_order( n->root_in );
        n = (trp_dgraph_node_t *)(((trp_dgraph_link_in_t *)(node->dummy))->n);
        i++;
    }
    return ( trp_dgraph_succ_cnt_low( n ) == g->len ) ? (trp_obj_t *)n : UNDEF;
}

trp_obj_t *trp_dgraph_root_if_is_tree( trp_obj_t *g )
{
    if ( trp_dgraph_check( g ) )
        return UNDEF;
    return trp_dgraph_root_if_is_tree_low( (trp_dgraph_t *)g );
}

trp_obj_t *trp_dgraph_is_tree( trp_obj_t *g )
{
    if ( trp_dgraph_check( g ) )
        return TRP_FALSE;
    return ( trp_dgraph_root_if_is_tree_low( (trp_dgraph_t *)g ) != UNDEF ) ? TRP_TRUE : TRP_FALSE;
}

/*
 * rende la coda dei nodi raggiungibili da node (incluso)
 */

static trp_obj_t *trp_dgraph_queue_succ_low( trp_dgraph_node_t *n )
{
    trp_obj_t *q = trp_queue();
    struct avl_tree_node *root = NULL;

    trp_dgraph_queue_succ_low_low( q, n, &root );
    return q;
}

static void trp_dgraph_queue_succ_low_low( trp_obj_t *q, trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;

    trp_queue_put( q, (trp_obj_t *)n );
    trp_dgraph_link_out_insert_item( n, root );
    for ( node = avl_tree_first_in_order( n->root_out ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_out_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            trp_dgraph_queue_succ_low_low( q, m, root );
    }
}

/*
 * rende la coda dei nodi che possono raggiungere node (incluso)
 */

static trp_obj_t *trp_dgraph_queue_pred_low( trp_dgraph_node_t *n )
{
    trp_obj_t *q = trp_queue();
    struct avl_tree_node *root = NULL;

    trp_dgraph_queue_pred_low_low( q, n, &root );
    return q;
}

static void trp_dgraph_queue_pred_low_low( trp_obj_t *q, trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;

    trp_queue_put( q, (trp_obj_t *)n );
    trp_dgraph_link_out_insert_item( n, root );
    for ( node = avl_tree_first_in_order( n->root_in ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_in_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            trp_dgraph_queue_pred_low_low( q, m, root );
    }
}

trp_obj_t *trp_dgraph_queue_succ( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_dgraph_queue_succ_low( (trp_dgraph_node_t *)n );
}

trp_obj_t *trp_dgraph_queue_pred( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_dgraph_queue_pred_low( (trp_dgraph_node_t *)n );
}

/*
 * rende il numero di nodi connessi in qualsiasi modo a node (incluso)
 */

static uns32b trp_dgraph_connected_cnt_low( trp_dgraph_node_t *n )
{
    struct avl_tree_node *root = NULL;

    return trp_dgraph_connected_cnt_low_low( n, &root );
}

static uns32b trp_dgraph_connected_cnt_low_low( trp_dgraph_node_t *n, struct avl_tree_node **root )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;
    uns32b res = 1;

    trp_dgraph_link_out_insert_item( n, root );
    for ( node = avl_tree_first_in_order( n->root_out ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_out_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            res += trp_dgraph_connected_cnt_low_low( m, root );
    }
    for ( node = avl_tree_first_in_order( n->root_in ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_in_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_not_member( m, *root ) )
            res += trp_dgraph_connected_cnt_low_low( m, root );
    }
    return res;
}

trp_obj_t *trp_dgraph_connected_cnt( trp_obj_t *n )
{
    if ( trp_dgraph_node_check( n ) )
        return UNDEF;
    return trp_sig64( trp_dgraph_connected_cnt_low( (trp_dgraph_node_t *)n ) );
}

trp_obj_t *trp_dgraph_is_connected( trp_obj_t *g )
{
    if ( trp_dgraph_check( g ) )
        return TRP_FALSE;
    if ( ((trp_dgraph_t *)g)->len == 0 )
        return TRP_TRUE;
    return ( trp_dgraph_connected_cnt_low( (trp_dgraph_node_t *)( avl_tree_first_in_order( trp_dgraph_root_nodes( g ) )->dummy ) )
             == ((trp_dgraph_t *)g)->len ) ? TRP_TRUE : TRP_FALSE;
}

/*
 * flags:
 * bit 0: 1=forward, 0=back
 * bit 1: il nodo va inserito in root_global
 * nota: si poteva fare senza flags, visitando il grafo seguendo un
 * solo verso degli archi, ma così è più efficiente nella maggior
 * parte dei casi
 */

static uns8b trp_dgraph_detect_a_cycle( uns8b flags, trp_dgraph_node_t *n, struct avl_tree_node **root_local, struct avl_tree_node **root_global )
{
    struct avl_tree_node *node;
    trp_dgraph_node_t *m;

    trp_dgraph_link_out_insert_item( n, root_local );
    for ( node = avl_tree_first_in_order( ( flags & 1 ) ? n->root_out : n->root_in ) ; node ; node = avl_tree_next_in_order( node ) ) {
        m = (trp_dgraph_node_t *)(((trp_dgraph_link_out_t *)(node->dummy))->n);
        if ( trp_dgraph_link_out_is_member( m, *root_local ) )
            return 1;
        if ( trp_dgraph_link_out_is_not_member( m, *root_global ) )
            if ( trp_dgraph_detect_a_cycle( flags | 2, m, root_local, root_global ) )
                return 1;
    }
    if ( flags & 2 )
        trp_dgraph_link_out_insert_item( n, root_global );
    return 0;
}

trp_obj_t *trp_dgraph_is_acyclic( trp_obj_t *g )
{
    struct avl_tree_node *root_global;
    struct avl_tree_node *root_local;
    struct avl_tree_node *node;
    trp_dgraph_node_t *n;

    if ( trp_dgraph_check( g ) )
        return TRP_FALSE;
    root_global = NULL;
    for ( node = avl_tree_first_in_order( trp_dgraph_root_nodes( g ) ) ; node ; node = avl_tree_next_in_order( node ) ) {
        n = (trp_dgraph_node_t *)(node->dummy);
        if ( trp_dgraph_link_out_is_not_member( n, root_global ) ) {
            root_local = NULL;
            if ( trp_dgraph_detect_a_cycle( 1, n, &root_local, &root_global ) )
                return TRP_FALSE;
            root_local = NULL;
            if ( trp_dgraph_detect_a_cycle( 2, n, &root_local, &root_global ) )
                return TRP_FALSE;
        }
    }
    return TRP_TRUE;
}

