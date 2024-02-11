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

static uns8b trp_tree_print_level( trp_print_t *p, trp_tree_t *obj, uns32b level );
static uns8b trp_tree_in( trp_obj_t *obj, trp_array_t *seq, uns32b *pos );
static uns32b trp_tree_node_cnt_low( trp_obj_t *obj );
static uns8b trp_tree_insert_low( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *child );

uns8b trp_tree_print( trp_print_t *p, trp_tree_t *obj )
{
    return trp_tree_print_level( p, obj, 0 );
}

static uns8b trp_tree_print_level( trp_print_t *p, trp_tree_t *obj, uns32b level )
{
    uns32b i;

    for ( i = level << 2 ; i ; i-- )
        if ( trp_print_char( p, ' ' ) )
            return 1;
    ++level;
    if ( trp_print_obj( p, obj->val ) )
            return 1;
    if ( trp_print_char( p, '\n' ) )
            return 1;
    for ( i = 0 ; i < obj->children->len ; i++ )
        if ( trp_tree_print_level( p, (trp_tree_t *)( obj->children->data[ i ] ), level ) )
                return 1;
    return 0;
}

uns32b trp_tree_size( trp_tree_t *obj )
{
    extern uns32b trp_size_internal( trp_obj_t * );

    return 1 + trp_size_internal( obj->val ) +
        trp_size_internal( (trp_obj_t *)( obj->children ) );
}

void trp_tree_encode( trp_tree_t *obj, uns8b **buf )
{
    extern void trp_encode_internal( trp_obj_t *, uns8b ** );

    **buf = TRP_TREE;
    ++(*buf);
    trp_encode_internal( obj->val, buf );
    trp_encode_internal( (trp_obj_t *)( obj->children ), buf );
}

trp_obj_t *trp_tree_decode( uns8b **buf )
{
    extern trp_obj_t *trp_decode_internal( uns8b ** );
    trp_obj_t *res = trp_tree( trp_decode_internal( buf ), NULL );
    uns32b n;

    ((trp_tree_t *)res)->children = (trp_array_t *)trp_decode_internal( buf );
    for ( n = ((trp_tree_t *)res)->children->len ; n ; )
        ((trp_tree_t *)(((trp_tree_t *)res)->children->data[ --n ]))->parent = res;
    return res;
}

trp_obj_t *trp_tree_equal( trp_tree_t *o1, trp_tree_t *o2 )
{
    if ( trp_equal( o1->val, o2->val ) == TRP_FALSE )
        return TRP_FALSE;
    return trp_array_equal( o1->children, o2->children );
}

trp_obj_t *trp_tree_less( trp_tree_t *o1, trp_tree_t *o2 )
{
    return ( o1->children->len < o2->children->len ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_tree_length( trp_tree_t *obj )
{
    return trp_sig64( obj->children->len );
}

trp_obj_t *trp_tree_nth( uns32b n, trp_tree_t *obj )
{
    return trp_array_nth( n, obj->children );
}

static uns8b trp_tree_in( trp_obj_t *obj, trp_array_t *seq, uns32b *pos )
{
    uns32b i;
    uns8b res = 1;

    for ( i = 0 ; i < seq->len ; i++ )
        if ( seq->data[ i ] == obj ) {
            res = 0;
            *pos = i;
            break;
        }
    return res;
}

trp_obj_t *trp_tree( trp_obj_t *val, ... )
{
    trp_tree_t *obj;
    va_list args;
    uns8b res;

    va_start( args, val );
    obj = trp_gc_malloc( sizeof( trp_tree_t ) );
    obj->tipo = TRP_TREE;
    obj->val = val ? val : UNDEF;
    obj->parent = UNDEF;
    obj->children = (trp_array_t *)trp_array_ext_internal( UNDEF, 10, 0 );
    if ( val )
        val = va_arg( args, trp_obj_t * );
    for ( res = 0 ; val ; val = va_arg( args, trp_obj_t * ) ) {
        res = trp_tree_insert_low( (trp_obj_t *)obj, NULL, val );
        if ( res )
            break;
    }
    va_end( args );
    return res ? UNDEF : (trp_obj_t *)obj;
}

trp_obj_t *trp_tree_get( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    return ((trp_tree_t *)obj)->val;
}

trp_obj_t *trp_tree_root( trp_obj_t *obj )
{
    trp_obj_t *parent;

    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    for ( ; ; ) {
        parent = ((trp_tree_t *)obj)->parent;
        if ( parent == UNDEF )
            break;
        obj = parent;
    }
    return obj;
}

trp_obj_t *trp_tree_level( trp_obj_t *obj )
{
    uns32b level = 0;

    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    for ( ; ; ) {
        obj = ((trp_tree_t *)obj)->parent;
        if ( obj == UNDEF )
            break;
        level++;
    }
    return trp_sig64( level );
}

trp_obj_t *trp_tree_parent( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    return ((trp_tree_t *)obj)->parent;
}

trp_obj_t *trp_tree_children( trp_obj_t *obj )
{
    trp_obj_t *res;
    uns32b n;

    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    res = trp_queue();
    for ( n = 0 ; n < ((trp_tree_t *)obj)->children->len ; n++ )
        trp_queue_put( res, ((trp_tree_t *)obj)->children->data[ n ] );
    return res;
}

trp_obj_t *trp_tree_pos( trp_obj_t *obj )
{
    trp_obj_t *parent;
    uns32b i;

    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    parent = ((trp_tree_t *)obj)->parent;
    if ( parent == UNDEF )
        return UNDEF;
    if ( trp_tree_in( obj, ((trp_tree_t *)parent)->children, &i ) ) /* non può mai succedere */
        return UNDEF;
    return trp_sig64( i );
}

static uns32b trp_tree_node_cnt_low( trp_obj_t *obj )
{
    trp_array_t *children = ((trp_tree_t *)obj)->children;
    uns32b cnt = 1, i;

    for ( i = children->len ; i ; )
        cnt += trp_tree_node_cnt_low( children->data[ --i ] );
    return cnt;
}

trp_obj_t *trp_tree_node_cnt( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_TREE )
        return UNDEF;
    return trp_sig64( trp_tree_node_cnt_low( obj ) );
}

uns8b trp_tree_set( trp_obj_t *obj, trp_obj_t *val )
{
    if ( obj->tipo != TRP_TREE )
        return 1;
    ((trp_tree_t *)obj)->val = val;
    return 0;
}

static uns8b trp_tree_insert_low( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *child )
{
    if ( ( obj->tipo != TRP_TREE ) || ( child->tipo != TRP_TREE ) )
        return 1;
    if ( ((trp_tree_t *)child)->parent != UNDEF )
        return 1;
    if ( child == trp_tree_root( obj ) )
        return 1;
    if ( trp_array_insert( (trp_obj_t *)((trp_tree_t *)obj)->children, pos, child, NULL ) )
        return 1;
    ((trp_tree_t *)child)->parent = obj;
    return 0;
}

uns8b trp_tree_insert( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *child )
{
    return trp_tree_insert_low( obj, pos, child );
}

uns8b trp_tree_append( trp_obj_t *obj, trp_obj_t *child )
{
    return trp_tree_insert_low( obj, NULL, child );
}

uns8b trp_tree_detach( trp_obj_t *obj, trp_obj_t *pos )
{
    trp_array_t *children;
    uns32b i;

    if ( obj->tipo != TRP_TREE )
        return 1;
    if ( pos ) {
        if ( trp_cast_uns32b( pos, &i ) )
            return 1;
        children = ((trp_tree_t *)obj)->children;
        if ( i >= children->len )
            return 1;
        obj = children->data[ i ];
        if ( trp_array_remove( (trp_obj_t *)children, pos, UNO ) )
            return 1;
    } else {
        trp_obj_t *parent = ((trp_tree_t *)obj)->parent;

        if ( parent == UNDEF )
            return 1;
        children = ((trp_tree_t *)parent)->children;
        if ( trp_tree_in( obj, children, &i ) ) /* non può mai succedere */
            return 1;
        if ( trp_array_remove( (trp_obj_t *)children, trp_sig64( i ), UNO ) )
            return 1;
    }
    ((trp_tree_t *)obj)->parent = UNDEF;
    return 0;
}

uns8b trp_tree_replace( trp_obj_t *obj, trp_obj_t *pos, trp_obj_t *new_obj )
{
    trp_array_t *children;
    uns32b i;

    if ( obj->tipo != TRP_TREE )
        return 1;
    if ( new_obj ) {
        if ( new_obj->tipo != TRP_TREE )
            return 1;
        if ( ( ((trp_tree_t *)new_obj)->parent != UNDEF ) ||
             ( new_obj == trp_tree_root( obj ) ) ||
             ( trp_cast_uns32b( pos, &i ) ) )
            return 1;
        children = ((trp_tree_t *)obj)->children;
        if ( i >= children->len )
            return 1;
        ((trp_tree_t *)new_obj)->parent = obj;
        ((trp_tree_t *)( children->data[ i ] ))->parent = UNDEF;
        children->data[ i ] = new_obj;
    } else {
        trp_obj_t *parent = ((trp_tree_t *)obj)->parent;

        new_obj = pos;
        if ( ( new_obj->tipo != TRP_TREE ) ||
             ( parent == UNDEF ) )
            return 1;
        if ( ( ((trp_tree_t *)new_obj)->parent != UNDEF ) ||
             ( new_obj == trp_tree_root( parent ) ) )
            return 1;
        children = ((trp_tree_t *)parent)->children;
        if ( trp_tree_in( obj, children, &i ) ) /* non può mai succedere */
            return 1;
        children->data[ i ] = new_obj;
        ((trp_tree_t *)new_obj)->parent = parent;
        ((trp_tree_t *)obj)->parent = UNDEF;
    }
    return 0;
}

uns8b trp_tree_swap( trp_obj_t *obj, trp_obj_t *pos1, trp_obj_t *pos2 )
{
    trp_array_t *children;
    uns32b i1, i2;

    if ( ( obj->tipo != TRP_TREE ) ||
         trp_cast_uns32b( pos1, &i1 ) ||
         trp_cast_uns32b( pos2, &i2 ) )
        return 1;
    children = ((trp_tree_t *)obj)->children;
    if ( ( i1 >= children->len ) || ( i2 >= children->len ) )
        return 1;
    obj = children->data[ i1 ];
    children->data[ i1 ] = children->data[ i2 ];
    children->data[ i2 ] = obj;
    return 0;
}

