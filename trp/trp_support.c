/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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

static uns32b      _trp_const_n = 0;
static trp_raw_t  *_trp_const_r = NULL;
static uns8b     **_trp_const_c = NULL;
static trp_obj_t **_trp_const = NULL;
static uns32b      _trp_glb_n = 0;
static trp_obj_t **_trp_glb = NULL;

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

typedef struct {
    trp_obj_t **var;
    trp_obj_t *act;
    trp_obj_t *target;
    trp_obj_t *step;
    trp_obj_t *queue;
    trp_obj_t *stack;
    CORD_pos *cordpos;
    voidfun_t cordnxt;
    uns32b max;
    uns32b pos;
    sig16b chstep;
} trp_for_t;

void trp_const_init( uns32b n, trp_raw_t r[], uns8b *c[], uns64b cst_totsize )
{
    void *root = (void *)( c[ 0 ] );

    _trp_const = trp_gc_malloc( sizeof( trp_obj_t * ) * n );
    _trp_const_n = n;
    _trp_const_r = r;
    _trp_const_c = c;
    GC_exclude_static_roots( root, root + cst_totsize );
    /*
     FIXME
     se si verificassero problemi strani, provare a togliere
     GC_exclude_static_roots...

     considerare se si possono escludere altre aree...
     GC_exclude_static_roots(low_address,high_address_plus_1)
     */
}

void trp_glb_init( uns32b n, trp_obj_t *glb[] )
{
    trp_obj_t *defval = UNDEF;

    _trp_glb_n = n;
    _trp_glb = glb;
    while ( n )
        _trp_glb[ --n ] = defval;
}

void trp_gc()
{
    uns32b i;

    for ( i = 0 ; i < _trp_const_n ; i++ ) {
        /*
         FIXME
         prima di chiamare GC_gcollect() si potrebbe valutare
         se vale la pena azzerare qualche costante...
         */
    }
    GC_gcollect();
}

trp_obj_t *trp_const( uns32b i )
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    trp_obj_t *res = _trp_const[ i ];

    if ( res ) {
        /*
         FIXME
         qui bisognerebbe incrementare un contatore associato alla costante
         per valutare statisticamente il suo uso prima di chiamare GC_gcollect()
         */
    } else {
        /*
         potrebbe succedere che una costante venga decompressa da piu' thread
         contemporaneamente, ma cio' non comporta ulteriori problemi; mettere
         un lock per evitarlo, sarebbe un rimedio peggiore del male...
         */
        /*
         questo assegnamento non da' mai problemi
         */
        _trp_const_r[ i ].data = _trp_const_c[ i ];
        res = trp_uncompress( (trp_obj_t *)( &( _trp_const_r[ i ] ) ) );
        /*
         se la costante e' abbastanza grossa (_trp_const_r[ i ].len), potrebbe
         essere piu' conveniente lasciare _trp_const[ i ] a NULL e rendere
         direttamente trp_uncompress().
         Cio' comporta piu' decompressioni, in caso di successive
         valutazioni, ma lascia la possibilita' che l'oggetto decompresso
         diventi garbage e venga liberato nel corso dell'esecuzione...
         */
        /*
         questo assegnamento, pero', deve essere atomico!
         */
        pthread_mutex_lock( &mutex );
        _trp_const[ i ] = res;
        pthread_mutex_unlock( &mutex );
    }
    return res;
}

/*
 supporto al meccanismo di salvataggio dell'ambiente per
 l'implementazione dei costrutti non deterministici (alt, opt, opt*)
 FIXME
 nota: se la macro TRP_ENV_STACK_GLB non e' definita, le variabili
 globali non vengono salvate...
 il ripristino delle variabili globali in ambito multithreading
 da parte della funzione trp_pop_env() e' problematico: un
 thread potrebbe eseguire un costrutto non deterministico con
 fallimento e ripristinare una globale che era stata legittimamente
 modificata da un altro thread (nell'intervallo di tempo compreso
 tra le chiamate trp_push_env() e trp_pop_env()), con effetti
 disastrosi...
 */

#define TRP_ENV_STACK_GLB

void trp_push_env( trp_obj_t *obj, ... )
{
    extern objfun_t _trp_env_stack;
    trp_obj_t *stack;
    va_list args;

    stack = (_trp_env_stack)();
#ifdef TRP_ENV_STACK_GLB
    {
        uns32b n;

        for ( n = 0 ; n < _trp_glb_n ; n++ )
            trp_stack_push( stack, _trp_glb[ n ] );
    }
#endif
    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        trp_stack_push( stack, obj );
    va_end( args );
}

void trp_pop_env( trp_obj_t **obj, ... )
{
    extern objfun_t _trp_env_stack;
    trp_obj_t *stack;
    va_list args;

    stack = (_trp_env_stack)();
    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t ** ) )
        *obj = trp_stack_pop( stack );
    va_end( args );
#ifdef TRP_ENV_STACK_GLB
    {
        uns32b n;

        for ( n = _trp_glb_n ; n ; )
            _trp_glb[ --n ] = trp_stack_pop( stack );
    }
#endif
}

void trp_pop_env_void( uns32b n )
{
    extern objfun_t _trp_env_stack;
    trp_obj_t *stack;

    stack = (_trp_env_stack)();
#ifdef TRP_ENV_STACK_GLB
    n += _trp_glb_n;
#endif
    for ( ; n ; n-- )
        (void)trp_stack_pop( stack );
}

/*
 supporto al costrutto 'for'
 */

uns8b trp_for_init( trp_obj_t **fst, trp_obj_t **var, trp_obj_t *from, trp_obj_t *to, trp_obj_t *step, uns8b rev )
{
    trp_for_t *f;

    if ( to ) {
        if ( step == ZERO )
            return 1;
        if ( ( ( from->tipo != TRP_SIG64 ) && ( from->tipo != TRP_MPI ) ) ||
             ( ( to->tipo != TRP_SIG64 ) && ( to->tipo != TRP_MPI ) ) ||
             ( ( step->tipo != TRP_SIG64 ) && ( step->tipo != TRP_MPI ) ) ) {
            uns8b a, b;

            if ( ( from->tipo != TRP_CHAR ) ||
                 ( to->tipo != TRP_CHAR ) ||
                 ( step->tipo != TRP_SIG64 ) )
                return 1;
            if ( ((trp_sig64_t *)step)->val < 1 )
                return 1;
            a = ((trp_char_t *)from)->c;
            b = ((trp_char_t *)to)->c;
            if ( b < a )
                return 1;
            b = a + ( ( b - a ) / ((trp_sig64_t *)step)->val ) * ((trp_sig64_t *)step)->val;
            f = trp_gc_malloc( sizeof( trp_for_t ) );
            f->step = NULL;
            f->queue = NULL;
            f->stack = NULL;
            f->cordpos = NULL;
            f->chstep = ( a + ((trp_sig64_t *)step)->val < b ) ? ((trp_sig64_t *)step)->val : b - a;
            if ( rev ) {
                f->act = trp_char( b );
                f->target = trp_char( a );
                f->chstep = -f->chstep;
            } else {
                f->act = trp_char( a );
                f->target = trp_char( b );
            }
        } else {
            if ( trp_less( to, from ) == TRP_TRUE )
                return 1;
            if ( trp_less( ZERO, step ) == TRP_FALSE )
                return 1;
            if ( step != UNO )
                to = trp_cat( from,
                              trp_math_times( step,
                                              trp_math_div( trp_math_minus( to,
                                                                            from,
                                                                            NULL ),
                                                            step ),
                                              NULL ),
                              NULL );
            f = trp_gc_malloc( sizeof( trp_for_t ) );
            f->queue = NULL;
            f->stack = NULL;
            f->cordpos = NULL;
            if ( rev ) {
                f->act = to;
                f->step = trp_math_minus( ZERO, step, NULL );
                f->target = from;
            } else {
                f->act = from;
                f->step = step;
                f->target = to;
            }
        }
    } else {
        switch ( from->tipo ) {
        case TRP_CONS:
            f = trp_gc_malloc( sizeof( trp_for_t ) );
            f->step = NULL;
            f->cordpos = NULL;
            if ( rev ) {
                f->queue = NULL;
                f->stack = trp_stack();
                for ( ; ; ) {
                    to = ((trp_cons_t *)from)->car;
                    from = ((trp_cons_t *)from)->cdr;
                    if ( from->tipo != TRP_CONS )
                        break;
                    trp_stack_push( f->stack, to );
                }
                f->act = to;
            } else {
                f->stack = NULL;
                f->queue = trp_queue();
                f->act = ((trp_cons_t *)from)->car;
                for ( ; ; ) {
                    from = ((trp_cons_t *)from)->cdr;
                    if ( from->tipo != TRP_CONS )
                        break;
                    trp_queue_put( f->queue, ((trp_cons_t *)from)->car );
                }
            }
            break;
        case TRP_QUEUE:
            if ( ((trp_queue_t *)from)->len == 0 )
                return 1;
            f = trp_gc_malloc( sizeof( trp_for_t ) );
            f->step = NULL;
            f->cordpos = NULL;
            if ( rev ) {
                trp_queue_elem *elem = (trp_queue_elem *)( ((trp_queue_t *)from)->first );

                f->queue = NULL;
                f->stack = trp_stack();
                for ( ; ; ) {
                    to = elem->val;
                    elem = (trp_queue_elem *)( elem->next );
                    if ( elem == NULL )
                        break;
                    trp_stack_push( f->stack, to );
                }
                f->act = to;
            } else {
                trp_queue_elem *elem = (trp_queue_elem *)( ((trp_queue_t *)from)->first );

                f->stack = NULL;
                f->queue = trp_queue();
                f->act = elem->val;
                for ( ; ; ) {
                    elem = (trp_queue_elem *)( elem->next );
                    if ( elem == NULL )
                        break;
                    trp_queue_put( f->queue, elem->val );
                }
            }
            break;
        case TRP_ARRAY:
            if ( ((trp_array_t *)from)->len == 0 )
                return 1;
            else {
                uns32b i, l = ((trp_array_t *)from)->len;

                f = trp_gc_malloc( sizeof( trp_for_t ) );
                f->step = NULL;
                f->queue = trp_queue();
                f->stack = NULL;
                f->cordpos = NULL;
                if ( rev ) {
                    f->act = ((trp_array_t *)from)->data[ l - 1 ];
                    for ( i = 1 ; i < l ; i++ )
                        trp_queue_put( f->queue, ((trp_array_t *)from)->data[ l - i - 1 ] );
                } else {
                    f->act = ((trp_array_t *)from)->data[ 0 ];
                    for ( i = 1 ; i < l ; i++ )
                        trp_queue_put( f->queue, ((trp_array_t *)from)->data[ i ] );
                }
            }
            break;
        case TRP_ASSOC:
            if ( ((trp_assoc_t *)from)->len == 0 )
                return 1;
            else {
                struct node *n = ((trp_assoc_t *)from)->t.root;
                struct node *stk[ 256 ];
                int d = 0;

                f = trp_gc_malloc( sizeof( trp_for_t ) );
                f->step = NULL;
                f->queue = trp_queue();
                f->stack = NULL;
                f->cordpos = NULL;
                for ( f->act = NULL ; ; ) {
                    if ( n == NULL ) {
                        if ( d == 0 )
                            break;
                        n = stk[ --d ];
                    }
                    if ( f->act )
                        trp_queue_put( f->queue, trp_cons( trp_cord( n->name ), (trp_obj_t *)( n->vlue ) ) );
                    else
                        f->act = trp_cons( trp_cord( n->name ), (trp_obj_t *)( n->vlue ) );
                    if ( rev ) {
                        if ( n->rght ) {
                            if ( n->left )
                                stk[ d++ ] = n->left;
                            n = n->rght;
                        } else {
                            n = n->left;
                        }
                    } else {
                        if ( n->left ) {
                            if ( n->rght )
                                stk[ d++ ] = n->rght;
                            n = n->left;
                        } else {
                            n = n->rght;
                        }
                    }
                }
            }
            break;
        default:
            if ( from == NIL )
                return 1;
            if ( from->tipo != TRP_CORD )
                from = trp_sprint( from, NULL );
            if ( ((trp_cord_t *)from)->len == 0 )
                return 1;
            f = trp_gc_malloc( sizeof( trp_for_t ) );
            f->step = NULL;
            f->queue = NULL;
            f->stack = NULL;
            f->max = ((trp_cord_t *)from)->len - 1;
            f->cordpos = trp_gc_malloc( sizeof( CORD_pos ) );
            f->cordnxt = rev ? CORD_prev : CORD_next;
            CORD_set_pos( *(f->cordpos), ((trp_cord_t *)from)->c, rev ? f->max : 0 );
            f->act = trp_char( CORD_pos_fetch( *(f->cordpos) ) );
            break;
        }
    }
    *var = f->act;
    f->var = var;
    f->pos = 0;
    *fst = (trp_obj_t *)f;
    return 0;
}

uns8b trp_for_next( trp_obj_t **fst )
{
    trp_for_t *f = *( (trp_for_t **)fst );

    if ( f->step ) {
        if ( trp_equal( f->act, f->target ) == TRP_FALSE ) {
            *( f->var ) = f->act = trp_cat( f->act, f->step, NULL );
            f->pos++;
            return 1;
        }
    } else if ( f->queue ) {
        if ( ((trp_queue_t *)(f->queue))->len  ) {
            *( f->var ) = f->act = trp_queue_get( f->queue );
            f->pos++;
            return 1;
        }
        trp_gc_free( f->queue );
    } else if ( f->stack ) {
        if ( ((trp_stack_t *)(f->stack))->len  ) {
            *( f->var ) = f->act = trp_stack_pop( f->stack );
            f->pos++;
            return 1;
        }
        trp_gc_free( f->stack );
    } else if ( f->cordpos ) {
        if ( f->pos < f->max ) {
            char c;

            (f->cordnxt)( *(f->cordpos) );
            *( f->var ) = f->act = trp_char( c = CORD_pos_fetch( *(f->cordpos) ) );
            f->pos++;
            return 1;
        }
    } else {
        if ( f->act != f->target ) {
            *( f->var ) = f->act = trp_char( ((trp_char_t *)(f->act))->c + f->chstep );
            f->pos++;
            return 1;
        }
    }
    *( f->var ) = f->act;
    trp_gc_free( f->cordpos );
    trp_gc_free( f );
    f = NULL;
    *fst = UNDEF;
    return 0;
}

void trp_for_break( trp_obj_t **fst )
{
    trp_for_t *f = *( (trp_for_t **)fst );

    if ( f->queue ) {
        while ( ((trp_queue_t *)(f->queue))->len )
            (void)trp_queue_get( f->queue );
        trp_gc_free( f->queue );
    }
    if ( f->stack ) {
        while ( ((trp_stack_t *)(f->stack))->len )
            (void)trp_stack_pop( f->stack );
        trp_gc_free( f->stack );
    }
    *( f->var ) = f->act;
    trp_gc_free( f->cordpos );
    trp_gc_free( f );
    f = NULL;
    *fst = UNDEF;
}

trp_obj_t *trp_for_pos( trp_obj_t *fst )
{
    return trp_sig64( ((trp_for_t *)fst)->pos );
}

