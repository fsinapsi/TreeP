/*
    TreeP Run Time Support
    Copyright (C) 2008-2022 Frank Sinapsi

    Copyright 1998 Michael H. Buselli <cosine@cosine.org>
    Copyright 2000-2002  Wessel Dankers <wsl@nl.linux.org>
    Copyright 2010 Leszek Dubiel <leszek@dubiel.pl>

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

    Original code for avl trees by Michael H. Buselli <cosine@cosine.org>.

    Modified by Wessel Dankers <wsl@nl.linux.org> to add a bunch of bloat to
    the sourcecode, change the interface and squash a few bugs.

    Finally simplified by Leszek Dubiel <leszek@dubiel.pl>, to store data
    in a form of name-value pairs.

    Modified by Frank Sinapsi <fsinapsi@yahoo.it>
*/

#include "trp.h"

extern uns32b trp_size_internal( trp_obj_t *obj );
extern void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
extern trp_obj_t *trp_decode_internal( uns8b **buf );

static void trp_assoc_balance( struct tree *t, struct node *n );
static void trp_assoc_set_basic_basic( uns8b flags, trp_assoc_t *obj, uns8b *a, trp_obj_t *val );
static uns8b trp_assoc_set_basic( uns8b flags, trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val );

#define NODE_NEW(m,a,v,t) { \
    m = trp_gc_malloc(sizeof(struct node)); \
    m->name = a; \
    m->vlue = (void *)(v); \
    m->tree = t; \
    m->left = m->rght = 0; \
    m->dpth = 1; }
#define NODE_DPTH(n) ((n) ? (n)->dpth : 0)
#define LEFT_DPTH(n) (NODE_DPTH((n)->left))
#define RGHT_DPTH(n) (NODE_DPTH((n)->rght))
#define CALC_DPTH(n) ((LEFT_DPTH(n) > RGHT_DPTH(n) ? LEFT_DPTH(n) : RGHT_DPTH(n)) + 1)

static void trp_assoc_balance( struct tree *t, struct node *n )
{
    struct node *c, *g, *p, **s;	/* child, grand child, parent, slot */

    while ( n ) {

        /* p is parent; s is a place where n is saved */
        p = n->root;
        if (p == 0)
            s = &t->root;
        else
            s = n == p->left ? &p->left : &p->rght;

        /* rebalancing; for cases see wikipedia */
        if (RGHT_DPTH(n) - LEFT_DPTH(n) < -1) {
            c = n->left;
            if (LEFT_DPTH(c) >= RGHT_DPTH(c)) {
                /* left left case */
                n->left = c->rght;
                if ( n->left )
                    n->left->root = n;
                c->rght = n;
                n->root = c;
                *s = c;
                c->root = p;
                n->dpth = CALC_DPTH(n);
                c->dpth = CALC_DPTH(c);
            } else {
                /* left right case */
                g = c->rght;
                n->left = g->rght;
                if ( n->left )
                    n->left->root = n;
                c->rght = g->left;
                if ( c->rght )
                    c->rght->root = c;
                g->rght = n;
                if ( g->rght )
                    g->rght->root = g;
                g->left = c;
                if ( g->left )
                    g->left->root = g;
                *s = g;
                g->root = p;
                n->dpth = CALC_DPTH(n);
                c->dpth = CALC_DPTH(c);
                g->dpth = CALC_DPTH(g);
            }
        } else if (RGHT_DPTH(n) - LEFT_DPTH(n) > 1) {
            c = n->rght;
            if (RGHT_DPTH(c) >= LEFT_DPTH(c)) {
                /* right right case */
                n->rght = c->left;
                if ( n->rght )
                    n->rght->root = n;
                c->left = n;
                n->root = c;
                *s = c;
                c->root = p;
                n->dpth = CALC_DPTH(n);
                c->dpth = CALC_DPTH(c);
            } else {
                /* right left case */
                g = c->left;
                n->rght = g->left;
                if ( n->rght )
                    n->rght->root = n;
                c->left = g->rght;
                if ( c->left )
                    c->left->root = c;
                g->left = n;
                if ( g->left )
                    g->left->root = g;
                g->rght = c;
                if ( g->rght )
                    g->rght->root = g;
                *s = g;
                g->root = p;
                n->dpth = CALC_DPTH(n);
                c->dpth = CALC_DPTH(c);
                g->dpth = CALC_DPTH(g);
            }
        } else {
            n->dpth = CALC_DPTH(n);
        }
        n = p;
    }
}

uns8b trp_assoc_print( trp_print_t *p, trp_assoc_t *obj )
{
    uns8b buf[ 11 ];

    if ( trp_print_char_star( p, "#assoc (length=" ) )
        return 1;
    sprintf( buf, "%u", obj->len );
    if ( trp_print_char_star( p, buf ) )
        return 1;
    if ( trp_print_char_star( p, ", depth=" ) )
        return 1;
    if ( obj->t.root ) {
        sprintf( buf, "%u", obj->t.root->dpth );
        if ( trp_print_char_star( p, buf ) )
            return 1;
    } else {
        if ( trp_print_char( p, '0' ) )
            return 1;
    }
    return trp_print_char_star( p, ")#" );
}

uns32b trp_assoc_size( trp_assoc_t *obj )
{
    struct node *n;
    struct node *stk[ 256 ];
    int d;
    uns32b sz = 1 + 4;

    for ( n = obj->t.root, d = 0 ; ; ) {
        if ( n == NULL ) {
            if ( d == 0 )
                break;
            n = stk[ --d ];
        }
        sz += strlen( n->name ) + 1 +
            trp_size_internal( (trp_obj_t *)( n->vlue ) );
        if ( n->left ) {
            if ( n->rght )
                stk[ d++ ] = n->rght;
            n = n->left;
        } else {
            n = n->rght;
        }
    }
    return sz;
}

void trp_assoc_encode( trp_assoc_t *obj, uns8b **buf )
{
    uns32b *p;
    struct node *n;
    struct node *stk[ 256 ];
    int d;

    **buf = TRP_ASSOC;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    for ( n = obj->t.root, d = 0 ; ; ) {
        if ( n == NULL ) {
            if ( d == 0 )
                break;
            n = stk[ --d ];
        }
        strcpy( *buf, n->name );
        (*buf) += strlen( n->name ) + 1;
        trp_encode_internal( (trp_obj_t *)( n->vlue ), buf );
        if ( n->left ) {
            if ( n->rght )
                stk[ d++ ] = n->rght;
            n = n->left;
        } else {
            n = n->rght;
        }
    }
}

trp_obj_t *trp_assoc_decode( uns8b **buf )
{
    trp_obj_t *obj = trp_assoc();
    uns8b *a;
    uns32b len;
    int l;

    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    for ( ; len ; len-- ) {
        l = strlen( *buf ) + 1;
        a = trp_gc_malloc_atomic( l );
        strcpy( a, *buf );
        (*buf) += l;
        trp_assoc_set_basic_basic( 0, (trp_assoc_t *)obj, a, trp_decode_internal( buf ) );
    }
    return obj;
}

trp_obj_t *trp_assoc_equal( trp_assoc_t *o1, trp_assoc_t *o2 )
{
    struct node *n1, *n2;
    struct node *stk[ 256 ];
    int c, d;

    if ( o1->len != o2->len )
        return TRP_FALSE;
    for ( n1 = o1->t.root, d = 0 ; ; ) {
        if ( n1 == NULL ) {
            if ( d == 0 )
                break;
            n1 = stk[ --d ];
        }
        for ( n2 = o2->t.root ; ; ) {
            c = strcmp( n1->name, n2->name );
            if (c < 0)
                n2 = n2->left;
            else if (c > 0)
                n2 = n2->rght;
            else {
                if ( trp_equal( (trp_obj_t *)( n1->vlue ), (trp_obj_t *)( n2->vlue ) ) != TRP_TRUE )
                    return TRP_FALSE;
                break;
            }
            if ( n2 == NULL )
                return TRP_FALSE;
        }
        if ( n1->left ) {
            if ( n1->rght )
                stk[ d++ ] = n1->rght;
            n1 = n1->left;
        } else {
            n1 = n1->rght;
        }
    }
    return TRP_TRUE;
}

trp_obj_t *trp_assoc_length( trp_assoc_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_assoc_height( trp_assoc_t *obj )
{
    if ( obj->t.root == NULL )
        return ZERO;
    return trp_sig64( obj->t.root->dpth );
}

uns8b trp_assoc_in( trp_obj_t *obj, trp_assoc_t *seq, uns32b *pos, uns32b nth )
{
    struct node *n;
    uns8b *a;
    int c;
    uns32b i;
    uns8b res = 1;

    if ( nth == 0 ) {
        a = trp_csprint( obj );
        for ( n = seq->t.root, i = 0 ; n ; i++ ) {
            c = strcmp( a, n->name );
            if ( c < 0 )
                n = n->left;
            else if ( c > 0 )
                n = n->rght;
            else {
                res = 0;
                *pos = i;
                break;
            }
        }
        trp_csprint_free( a );
    }
    return res;
}

trp_obj_t *trp_assoc()
{
    trp_assoc_t *obj;

    obj = trp_gc_malloc( sizeof( trp_assoc_t ) );
    obj->tipo = TRP_ASSOC;
    obj->len = 0;
    obj->t.root = NULL;
    obj->t.frst = NULL;
    obj->t.last = NULL;
    return (trp_obj_t *)obj;
}

static void trp_assoc_set_basic_basic( uns8b flags, trp_assoc_t *obj, uns8b *a, trp_obj_t *val )
{
    struct tree *t;
    struct node *n, *m;
    int c;

    t = &( obj->t );
    if (t->root == 0) {
        obj->len++;
        NODE_NEW( m, a, val, t )
        m->prev = m->next = m->root = 0;
        t->frst = t->last = t->root = m;
        return;
    }
    for ( n = t->root ; ; ) {
        c = strcmp(a, n->name);
        if (c < 0) {
            if ( n->left ) {
                n = n->left;
            } else {
                NODE_NEW( m, a, val, t )
                m->root = n;
                if ( n->prev ) {
                    m->prev = n->prev;
                    n->prev->next = m;
                } else {
                    m->prev = 0;
                    t->frst = m;
                }
                m->next = n;
                n->prev = m;
                n->left = m;
                break;
            }
        } else if (c > 0) {
            if ( n->rght ) {
                n = n->rght;
            } else {
                NODE_NEW( m, a, val, t )
                m->root = n;
                m->prev = n;
                if ( n->next ) {
                    m->next = n->next;
                    n->next->prev = m;
                } else {
                    m->next = 0;
                    t->last = m;
                }
                n->next = m;
                n->rght = m;
                break;
            }
        } else {
            trp_csprint_free( a );
            if ( flags & 1 ) {
                val = trp_cat( (trp_obj_t *)( n->vlue ), val, NULL );
                if ( val != UNDEF )
                    n->vlue = (void *)val;
            } else {
                n->vlue = (void *)val;
            }
            return;
        }
    }
    ((trp_assoc_t *)obj)->len++;
    trp_assoc_balance(t, n);
}

static uns8b trp_assoc_set_basic( uns8b flags, trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val )
{
    if ( obj->tipo != TRP_ASSOC )
        return 1;
    trp_assoc_set_basic_basic( flags, (trp_assoc_t *)obj, trp_csprint( key ), val );
    return 0;
}

uns8b trp_assoc_set( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val )
{
    if ( val == UNDEF )
        return trp_assoc_clr( obj, key );
    return trp_assoc_set_basic( 0, obj, key, val );
}

uns8b trp_assoc_inc( trp_obj_t *obj, trp_obj_t *key, trp_obj_t *val )
{
    if ( val == NULL )
        val = UNO;
    else if ( val == UNDEF )
        return trp_assoc_clr( obj, key );
    return trp_assoc_set_basic( 1, obj, key, val );
}

uns8b trp_assoc_clr( trp_obj_t *obj, trp_obj_t *key )
{
    struct tree *t;
    struct node *n, *m, **s;
    uns8b *a;
    int c;

    if ( obj->tipo != TRP_ASSOC )
        return 1;
    t = &( ((trp_assoc_t *)obj)->t );
    a = trp_csprint( key );

    /* locate node n with name a */
    for ( n = t->root ; n ; ) {
        c = strcmp(a, n->name);
        if (c < 0)
            n = n->left;
        else if (c > 0)
            n = n->rght;
        else
            break;
    }
    trp_csprint_free( a );
    if ( n ) {

        ((trp_assoc_t *)obj)->len--;

        /* set s to point to place where n is saved */
        if (n->root == 0)
            s = &t->root;
        else
            s = n == n->root->left ? &n->root->left : &n->root->rght;

        /* remove node n from list next, prev */
        if ( n->prev )
            n->prev->next = n->next;
        else
            t->frst = n->next;
        if ( n->next )
            n->next->prev = n->prev;
        else
            t->last = n->prev;

        /* remove node from tree */
        if (n->left == 0) {
            *s = n->rght;
            if ( n->rght )
                n->rght->root = n->root;
            if ( n->root )
                trp_assoc_balance(t, n->root);
        } else if (n->rght == 0) {
            *s = n->left;
            n->left->root = n->root;
            if ( n->root )
                trp_assoc_balance(t, n->root);
        } else {
            /* replace node with righmost node of left subtree */
            *s = n->prev;
            if (n->prev == n->left) {
                m = n->prev;
            } else {
                m = n->prev->root;
                m->rght = n->prev->left;
                if ( m->rght )
                    m->rght->root = m;
                n->prev->left = n->left;
                n->left->root = n->prev;
            }
            n->prev->rght = n->rght;
            n->prev->root = n->root;
            n->rght->root = n->prev;
            trp_assoc_balance(t, m);
        }
#ifdef TRP_FORCE_FREE
        trp_gc_free( n->name );
        trp_gc_free( n );
#endif
    }
    return 0;
}

trp_obj_t *trp_assoc_get( trp_obj_t *obj, trp_obj_t *key )
{
    trp_obj_t *val = UNDEF;
    struct node *n;
    uns8b *a;
    int c;

    if ( obj->tipo == TRP_ASSOC ) {
        a = trp_csprint( key );
        for ( n = ((trp_assoc_t *)obj)->t.root ; n ; ) {
            c = strcmp(a, n->name);
            if (c < 0)
                n = n->left;
            else if (c > 0)
                n = n->rght;
            else {
                val = (trp_obj_t *)( n->vlue );
                break;
            }
        }
        trp_csprint_free( a );
    }
    return val;
}

trp_obj_t *trp_assoc_list( trp_obj_t *obj )
{
    trp_obj_t *res;
    struct node *n;
    struct node *stk[ 256 ];
    int d;

    if ( obj->tipo != TRP_ASSOC )
        return UNDEF;
    for ( n = ((trp_assoc_t *)obj)->t.root, res = NIL, d = 0 ; ; ) {
        if ( n == NULL ) {
            if ( d == 0 )
                break;
            n = stk[ --d ];
        }
        res = trp_cons( trp_cons( trp_cord( n->name ), (trp_obj_t *)( n->vlue ) ), res );
        if ( n->left ) {
            if ( n->rght )
                stk[ d++ ] = n->rght;
            n = n->left;
        } else {
            n = n->rght;
        }
    }
    return res;
}

trp_obj_t *trp_assoc_root( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_ASSOC )
        return UNDEF;
    if ( ((trp_assoc_t *)obj)->len == 0 )
        return UNDEF;
    return trp_cord( (((trp_assoc_t *)obj)->t.root)->name );
}

