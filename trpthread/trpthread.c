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

#include "../trp/trp.h"
#include "./trpthread.h"

typedef struct {
    uns8b tipo;
    uns8b stato;
    trp_obj_t *env_stack;
    trp_obj_t *msg;
    trp_obj_t *msg_mitt;
    trp_queue_t msgq;
    trp_queue_t mitt;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t th;
} trp_thread_t;

#define TRP_THREAD_STATE_STOPPED 0
#define TRP_THREAD_STATE_RUNNING 1
#define TRP_THREAD_STATE_WAITING_ON_RECEIVE 2
#define TRP_THREAD_STATE_WAITING_ON_SEND 3
#define TRP_THREAD_STATE_WAITING_ON_SEND_TERM_DEST 4

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

typedef struct {
    trp_thread_t *mitt;
    trp_obj_t *msg;
    uns8b mitt_is_susp;
} trp_thread_msg_t;

typedef struct {
    uns32b priority;
    uns32b retcode;
    trp_obj_t **obj;
    trp_obj_t **mitt;
    trp_queue_t mittq;
} trp_thread_alternative_t;

static trp_obj_t *_trp_thread_main;
static uns32b _trp_thread_max = 1;
static trp_obj_t *_trp_thread_q;
static pthread_mutex_t _trp_thread_q_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t _trp_thread_key;

static uns8b trp_thread_print( trp_print_t *p, trp_thread_t *obj );
static trp_obj_t *trp_thread_length( trp_thread_t *obj );
static trp_obj_t *trp_thread_env_stack();
static trp_obj_t *trp_thread_create_internal();
static void trp_thread_setspecific( trp_thread_t *th );
static void *trp_thread_start_routine( void *arg );
static trp_obj_t *trp_thread_case_cmp( trp_thread_alternative_t *x, trp_thread_alternative_t *y );
#ifdef TRP_FORCE_FREE
static void trp_thread_case_free_array( trp_array_t *a );
#else
#define trp_thread_case_free_array(a)
#endif
#define trp_thread_private_lock(th) (void)pthread_mutex_lock(&(((trp_thread_t *)(th))->mutex))
#define trp_thread_private_unlock(th) (void)pthread_mutex_unlock(&(((trp_thread_t *)(th))->mutex))
#define trp_thread_private_wait(th_cond,th_mutex) (void)pthread_cond_wait(&(((trp_thread_t *)(th_cond))->cond),&(((trp_thread_t *)(th_mutex))->mutex))
#define trp_thread_private_signal(th) (void)pthread_cond_signal(&(((trp_thread_t *)(th))->cond))
#define trp_thread_q_lock() (void)pthread_mutex_lock(&_trp_thread_q_mutex)
#define trp_thread_q_unlock() (void)pthread_mutex_unlock(&_trp_thread_q_mutex)

uns8b trp_thread_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_env_stack;

    if ( pthread_key_create( &_trp_thread_key, NULL ) ) {
        fprintf( stderr, "Initialization of pthread key failed\n" );
        return 1;
    }
    _trp_thread_q = trp_queue();
    _trp_print_fun[ TRP_THREAD ] = trp_thread_print;
    _trp_length_fun[ TRP_THREAD ] = trp_thread_length;
    _trp_env_stack = trp_thread_env_stack;
    _trp_thread_main = trp_thread_create_internal();
    trp_thread_setspecific( (trp_thread_t *)_trp_thread_main );
    return 0;
}

void trp_thread_quit()
{
    /*
     FIXME
     */
}

static uns8b trp_thread_print( trp_print_t *p, trp_thread_t *obj )
{
    if ( trp_print_char_star( p, "#thread " ) )
        return 1;
    if ( trp_print_obj( p, trp_sig64( (sig64b)( obj->th ) ) ) )
        return 1;
    if ( obj->stato == TRP_THREAD_STATE_STOPPED )
        if ( trp_print_char_star( p, " (stopped)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static trp_obj_t *trp_thread_length( trp_thread_t *obj )
{
    return trp_sig64( obj->msgq.len );
}

static trp_obj_t *trp_thread_env_stack()
{
    return ((trp_thread_t *)pthread_getspecific( _trp_thread_key ))->env_stack;
}

static trp_obj_t *trp_thread_create_internal()
{
    extern void trp_queue_init_internal( trp_queue_t *q );
    trp_thread_t *th;

    th = trp_gc_malloc( sizeof( trp_thread_t ) );
    th->tipo = TRP_THREAD;
    th->stato = TRP_THREAD_STATE_RUNNING;
    th->env_stack = trp_stack();
    th->msg = NULL;
    th->msg_mitt = NULL;
    trp_queue_init_internal( &( th->msgq ) );
    trp_queue_init_internal( &( th->mitt ) );
    (void)pthread_mutex_init( &( th->mutex ), NULL );
    (void)pthread_cond_init( &( th->cond ), NULL );
    trp_thread_q_lock();
    (void)trp_queue_put( _trp_thread_q, (trp_obj_t *)th );
    trp_thread_q_unlock();
    return (trp_obj_t *)th;
}

static void trp_thread_setspecific( trp_thread_t *th )
{
    th->th = pthread_self();
    (void)pthread_setspecific( _trp_thread_key, (void *)th );
}

static void *trp_thread_start_routine( void *arg )
{
    trp_obj_t **p = (trp_obj_t **)arg;
    trp_thread_t *me;
    uns8bfun_t f;
    uns8b nargs;

    f = ((trp_netptr_t *)( p[ 0 ] ))->f;
    nargs = ((trp_netptr_t *)( p[ 0 ] ))->nargs;
    me = (trp_thread_t *)( p[ nargs + 1 ] );
    trp_thread_setspecific( me );
    switch ( nargs ) {
    case 0:
        (void)( (f)() );
        break;
    case 1:
        (void)( (f)( p[ 1 ] ) );
        break;
    case 2:
        (void)( (f)( p[ 1 ], p[ 2 ] ) );
        break;
    case 3:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ]) );
        break;
    case 4:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ] ) );
        break;
    case 5:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ] ) );
        break;
    case 6:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ] ) );
        break;
    case 7:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ] ) );
        break;
    case 8:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ] ) );
        break;
    case 9:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ] ) );
        break;
    case 10:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ] ) );
        break;
    case 11:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ] ) );
        break;
    case 12:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ] ) );
        break;
    case 13:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ] ) );
        break;
    case 14:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ] ) );
        break;
    case 15:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ] ) );
        break;
    case 16:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ],
                     p[ 16 ] ) );
        break;
    case 17:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ],
                     p[ 16 ], p[ 17 ] ) );
        break;
    case 18:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ],
                     p[ 16 ], p[ 17 ], p[ 18 ] ) );
        break;
    case 19:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ],
                     p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ] ) );
        break;
    case 20:
        (void)( (f)( p[ 1 ], p[ 2 ], p[ 3 ], p[ 4 ], p[ 5 ],
                     p[ 6 ], p[ 7 ], p[ 8 ], p[ 9 ], p[ 10 ],
                     p[ 11 ], p[ 12 ], p[ 13 ], p[ 14 ], p[ 15 ],
                     p[ 16 ], p[ 17 ], p[ 18 ], p[ 19 ], p[ 20 ] ) );
        break;
    }
    trp_gc_free( arg );
    me->stato = TRP_THREAD_STATE_STOPPED;
    (void)pthread_setspecific( _trp_thread_key, NULL );
    /*
     dobbiamo svegliare tutti quelli che ci hanno mandato
     messaggi che non abbiamo consumato e che sono in wait
     */
    {
        extern void trp_queue_init_internal( trp_queue_t *q );
        trp_thread_msg_t *msg;
        trp_queue_t msgq;

        trp_thread_private_lock( me );
        (void)memcpy( (void *)( &msgq ), (void *)( &( me->msgq ) ), sizeof( trp_queue_t ) );
        trp_queue_init_internal( &( me->msgq ) );
        trp_thread_private_unlock( me );
        while ( msgq.len ) {
            msg = (trp_thread_msg_t *)trp_queue_get( (trp_obj_t *)( &msgq ) );
            if ( msg->mitt_is_susp ) {
                msg->mitt->stato = TRP_THREAD_STATE_WAITING_ON_SEND_TERM_DEST;
                trp_thread_private_signal( msg->mitt );
            }
            trp_gc_free( msg );
        }
    }
    /*
     dobbiamo toglierci dalla coda dei thread e dalla coda dei mittenti
     di tutti quelli che aspettano messaggi da noi, ed eventualmente
     svegliarli...
     */
    {
        trp_queue_elem *elem, *prev = NULL, *elem2, *prev2;
        void *tmp;
        uns8b sveglia;

        trp_thread_q_lock();
        for ( elem = (trp_queue_elem *)( ((trp_queue_t *)_trp_thread_q)->first ) ;
              elem ; ) {
            if ( (trp_thread_t *)( elem->val ) == me ) {
                if ( prev == NULL )
                    ((trp_queue_t *)_trp_thread_q)->first = elem->next;
                else
                    prev->next = elem->next;
                if ( elem->next == NULL )
                    ((trp_queue_t *)_trp_thread_q)->last = prev;
                prev = elem;
                elem = (trp_queue_elem *)( elem->next );
                trp_gc_free( prev );
            } else {
                sveglia = 0;
                trp_thread_private_lock( elem->val );
                if ( ( (trp_thread_t *)( elem->val ) )->stato == TRP_THREAD_STATE_WAITING_ON_RECEIVE )
                    if ( ( (trp_thread_t *)( elem->val ) )->msg == NULL ) {
                        prev2 = NULL;
                        for ( elem2 = (trp_queue_elem *)( ( (trp_thread_t *)( elem->val ) )->mitt.first ) ;
                              elem2 ; ) {
                            if ( (trp_thread_t *)(elem2->val) == me ) {
                                if ( prev2 == NULL )
                                    ( (trp_thread_t *)( elem->val ) )->mitt.first = elem2->next;
                                else
                                    prev2->next = elem2->next;
                                if ( elem2->next == NULL )
                                    ( (trp_thread_t *)( elem->val ) )->mitt.last = prev2;
                                tmp = (void *)elem2;
                                elem2 = (trp_queue_elem *)( elem2->next );
                                trp_gc_free( tmp );
                                ( (trp_thread_t *)( elem->val ) )->mitt.len--;
                                if ( ( (trp_thread_t *)( elem->val ) )->mitt.len == 0 )
                                    sveglia = 1;
                            } else {
                                prev2 = elem2;
                                elem2 = (trp_queue_elem *)( elem2->next );
                            }
                        }
                    }
                trp_thread_private_unlock( elem->val );
                if ( sveglia )
                    trp_thread_private_signal( elem->val );
                prev = elem;
                elem = (trp_queue_elem *)( elem->next );
            }
        }
        ((trp_queue_t *)_trp_thread_q)->len--;
        trp_thread_q_unlock();
    }
    /*
     in teoria andrebbero chiamate anche
     pthread_mutex_destroy e pthread_cond_destroy...
     ma dovrebbe non essere necessario: nelle
     implementazioni Linux dei POSIX threads non e'
     sicuramente necessario, e probabilmente non lo e'
     nemmeno in tutte le altre implementazioni
     */
    return NULL;
}

trp_obj_t *trp_thread_create( trp_obj_t *net, ... )
{
    trp_obj_t **arg;
    pthread_t th;
    va_list args;
    uns8b nargs, i;

    if ( net->tipo != TRP_NETPTR )
        return UNDEF;
    nargs = ((trp_netptr_t *)net)->nargs;
    if ( nargs > 20 )
        return UNDEF;
    /*
     qui non si puo' usare trp_gc_malloc_atomic...
     */
    arg = (trp_obj_t **)trp_gc_malloc( ( 2 + nargs ) * sizeof( trp_obj_t * ) );
    va_start( args, net );
    for ( i = 0 ; net && ( i <= nargs ) ; net = va_arg( args, trp_obj_t * ) )
        arg[ i++ ] = net;
    va_end( args );
    if ( net || ( i <= nargs ) ) {
        trp_gc_free( arg );
        return UNDEF;
    }
    arg[ i ] = net = trp_thread_create_internal();
    if ( pthread_create( &th, NULL, trp_thread_start_routine, (void *)arg ) ) {
        trp_queue_elem *elem, *prev = NULL;

        trp_thread_q_lock();
        for ( elem = (trp_queue_elem *)( ((trp_queue_t *)_trp_thread_q)->first ) ;
              elem->val != net ;
              elem = (trp_queue_elem *)( elem->next ) )
            prev = elem;
        if ( prev == NULL )
            ((trp_queue_t *)_trp_thread_q)->first = elem->next;
        else
            prev->next = elem->next;
        if ( elem->next == NULL )
            ((trp_queue_t *)_trp_thread_q)->last = prev;
        ((trp_queue_t *)_trp_thread_q)->len--;
        trp_thread_q_unlock();
        trp_gc_free( net );
        trp_gc_free( arg );
        return UNDEF;
    }
    ((trp_thread_t *)net)->th = th;
    trp_thread_q_lock();
    if ( ((trp_queue_t *)_trp_thread_q)->len > _trp_thread_max )
        _trp_thread_max = ((trp_queue_t *)_trp_thread_q)->len;
    trp_thread_q_unlock();
    return net;
}

uns8b trp_thread_join( trp_obj_t *th )
{
    if ( th->tipo != TRP_THREAD )
        return 1;
    return pthread_join( ((trp_thread_t *)th)->th, NULL ) ? 1 : 0;
}

trp_obj_t *trp_thread_self()
{
    return (trp_obj_t *)pthread_getspecific( _trp_thread_key );
}

trp_obj_t *trp_thread_stopped( trp_obj_t *th )
{
    if ( th->tipo != TRP_THREAD )
        return UNDEF;
    return ( ((trp_thread_t *)th)->stato == TRP_THREAD_STATE_STOPPED ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_thread_cur()
{
    trp_obj_t *res;

    trp_thread_q_lock();
    res = trp_sig64( (sig64b)( ((trp_queue_t *)_trp_thread_q)->len ) );
    trp_thread_q_unlock();
    return res;
}

trp_obj_t *trp_thread_max()
{
    trp_obj_t *res;

    trp_thread_q_lock();
    res = trp_sig64( (sig64b)_trp_thread_max );
    trp_thread_q_unlock();
    return res;
}

trp_obj_t *trp_thread_list()
{
    trp_obj_t *res = NIL;
    trp_queue_elem *elem;

    trp_thread_q_lock();
    for ( elem = (trp_queue_elem *)( ((trp_queue_t *)_trp_thread_q)->first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) ) {
        res = trp_cons( elem->val, res );
    }
    trp_thread_q_unlock();
    return res;
}

uns8b trp_thread_send( uns8b flags, trp_obj_t *bmax, trp_obj_t *obj, trp_obj_t *th )
{
    trp_thread_t *me;
    trp_thread_msg_t *msg;
    uns32b bsizemax;
    uns8b res;

    if ( ( th->tipo != TRP_THREAD ) ||
         trp_cast_uns32b( bmax, &bsizemax ) )
        return 1;
    me = (trp_thread_t *)pthread_getspecific( _trp_thread_key );
    if ( (trp_thread_t *)th == me )
        return 1;
    trp_thread_private_lock( th );
    if ( ((trp_thread_t *)th)->stato == TRP_THREAD_STATE_WAITING_ON_RECEIVE )
        if ( ((trp_thread_t *)th)->msg == NULL ) {
            /*
             sappiamo che il destinatario e' bloccato in ricezione e che
             finora non lo ha sbloccato nessuno;
             resta da vedere se e' in attesa di un messaggio da noi
             */
            trp_queue_elem *elem;

            for ( elem = (trp_queue_elem *)( ((trp_thread_t *)th)->mitt.first ) ;
                  elem ;
                  elem = (trp_queue_elem *)( elem->next ) )
                if ( (trp_thread_t *)( elem->val ) == me )
                    break;
            if ( elem ) {
                /*
                 possiamo lasciare il messaggio direttamente
                 */
                ((trp_thread_t *)th)->msg = obj;
                ((trp_thread_t *)th)->msg_mitt = (trp_obj_t *)me;
                trp_thread_private_unlock( th );
                trp_thread_private_signal( th );
                return 0;
            }
        }
    if ( ( flags & 1 ) ||
         ( ((trp_thread_t *)th)->stato == TRP_THREAD_STATE_STOPPED ) ) {
        /*
         il destinatario e' terminato oppure la send e' non bloccante
         */
        trp_thread_private_unlock( th );
        return 1;
    }
    /*
     accodiamo il messaggio nella mailbox del destinatario
     */
    msg = (trp_thread_msg_t *)trp_gc_malloc( sizeof( trp_thread_msg_t ) );
    (void)trp_queue_put( (trp_obj_t *)( &(((trp_thread_t *)th)->msgq) ),
                         (trp_obj_t *)msg );
    msg->mitt = me;
    msg->msg = obj;
    if ( ((trp_thread_t *)th)->msgq.len <= bsizemax ) {
        msg->mitt_is_susp = 0;
        trp_thread_private_unlock( th );
        return 0;
    }
    msg->mitt_is_susp = 1;
    me->stato = TRP_THREAD_STATE_WAITING_ON_SEND; /* questo puo' essere fatto non in mutua esclusione */
    trp_thread_private_wait( me, th );
    trp_thread_private_unlock( th );
    /*
     me->stato == TRP_THREAD_STATE_WAITING_ON_SEND_TERM_DEST vuol dire
     che il destinatario e' terminato senza ricevere il nostro msg
     */
    res = ( me->stato == TRP_THREAD_STATE_WAITING_ON_SEND ) ? 0 : 1;
    me->stato = TRP_THREAD_STATE_RUNNING;
    return res;
}

uns8b trp_thread_receive( uns8b flags, trp_obj_t **obj, trp_obj_t **mitt, trp_obj_t *th, ... )
{
    trp_thread_t *me;
    trp_obj_t *mittq;
    trp_queue_elem *elem, *prev = NULL;
    va_list args;
    uns8b res;

    if ( mitt ) {
        uns8b err = 0;

        me = (trp_thread_t *)pthread_getspecific( _trp_thread_key );
        mittq = trp_queue();
        va_start( args, th );
        if ( th ) {
            switch ( th->tipo ) {
            case TRP_QUEUE:
                for ( elem = (trp_queue_elem *)( ((trp_queue_t *)th)->first ) ;
                      elem ;
                      elem = (trp_queue_elem *)( elem->next ) ) {
                    if ( elem->val->tipo != TRP_THREAD ) {
                        err = 1;
                        break;
                    }
                    if ( (trp_thread_t *)( elem->val ) != me )
                        (void)trp_queue_put( mittq, elem->val );
                }
                break;
            case TRP_CONS:
                for ( ; ; ) {
                    if ( ((trp_cons_t *)th)->car->tipo != TRP_THREAD ) {
                        err = 1;
                        break;
                    }
                    if ( (trp_thread_t *)( ((trp_cons_t *)th)->car ) != me )
                        (void)trp_queue_put( mittq, ((trp_cons_t *)th)->car );
                    th = ((trp_cons_t *)th)->cdr;
                    if ( th->tipo != TRP_CONS )
                        break;
                }
                break;
            case TRP_ARRAY:
                {
                    trp_obj_t *o;
                    uns32b i;

                    for ( i = 0 ; i < ((trp_array_t *)th)->len ; i++ ) {
                        o = ((trp_array_t *)th)->data[ i ];
                        if ( o->tipo != TRP_THREAD ) {
                            err = 1;
                            break;
                        }
                        if ( (trp_thread_t *)o != me )
                            (void)trp_queue_put( mittq, o );
                    }
                }
                break;
            default:
                err = 1;
                break;
            }
        } else {
            for ( th = va_arg( args, trp_obj_t * ) ;
                  th ;
                  th = va_arg( args, trp_obj_t * ) ) {
                if ( th->tipo != TRP_THREAD ) {
                    err = 1;
                    break;
                }
                if ( (trp_thread_t *)th != me )
                    (void)trp_queue_put( mittq, th );
            }
        }
        va_end( args );
        if ( err || ( ((trp_queue_t *)mittq)->len == 0 ) ) {
            while ( ((trp_queue_t *)mittq)->len )
                (void)trp_queue_get( mittq );
            trp_gc_free( mittq );
            return 1;
        }
    } else {
        if ( th->tipo != TRP_THREAD )
            return 1;
        me = (trp_thread_t *)pthread_getspecific( _trp_thread_key );
        if ( (trp_thread_t *)th == me )
            return 1;
        mittq = trp_queue();
        (void)trp_queue_put( mittq, th );
    }
    /*
     inizia la sezione critica
     */
    trp_thread_private_lock( me );
    /*
     dobbiamo controllare se nella nostra mailbox c'e' gia'
     un messaggio proveniente da uno dei mittenti
     */
    {
        trp_queue_elem *elem2;

        for ( elem = (trp_queue_elem *)( me->msgq.first ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) ) {
            th = (trp_obj_t *)( ((trp_thread_msg_t *)( elem->val ))->mitt );
            for ( elem2 = (trp_queue_elem *)( ((trp_queue_t *)mittq)->first ) ;
                  elem2 ;
                  elem2 = (trp_queue_elem *)( elem2->next ) )
                if ( elem2->val == th )
                    break;
            if ( elem2 )
                break;
            prev = elem;
        }
    }
    if ( elem ) {
        /*
         possiamo consumare il messaggio e risvegliare il mittente
         */
        *obj = ((trp_thread_msg_t *)( elem->val ))->msg;
        if ( mitt )
            *mitt = th;
        if ( prev == NULL )
            me->msgq.first = elem->next;
        else
            prev->next = elem->next;
        if ( elem->next == NULL )
            me->msgq.last = prev;
        me->msgq.len--;
        trp_thread_private_unlock( me );
        if ( ((trp_thread_msg_t *)( elem->val ))->mitt_is_susp )
            trp_thread_private_signal( th );
        trp_gc_free( elem->val );
        trp_gc_free( elem );
        while ( ((trp_queue_t *)mittq)->len )
            (void)trp_queue_get( mittq );
        trp_gc_free( mittq );
        return 0;
    }
    if ( flags & 1 ) {
        trp_thread_private_unlock( me );
        while ( ((trp_queue_t *)mittq)->len )
            (void)trp_queue_get( mittq );
        trp_gc_free( mittq );
        return 1;
    }
    /*
     costruiamo la coda dei mittenti (che e' vuota)
     */
    while ( ((trp_queue_t *)mittq)->len ) {
        th = trp_queue_get( mittq );
        /*
         questo dovrebbe essere corretto perche' siamo
         in mutua esclusione su me...
         */
        if ( ((trp_thread_t *)th)->stato != TRP_THREAD_STATE_STOPPED )
            (void)trp_queue_put( (trp_obj_t *)( &(me->mitt) ), th );
    }
    trp_gc_free( mittq );
    if ( me->mitt.len == 0 ) {
        trp_thread_private_unlock( me );
        return 1;
    }
    /*
     aggiorniamo lo stato
     */
    me->stato = TRP_THREAD_STATE_WAITING_ON_RECEIVE;
    /*
     e aspettiamo che uno dei mittenti ci mandi un messaggio
     */
    trp_thread_private_wait( me, me );
    /*
     possiamo svuotare la coda dei mittenti
     */
    while ( me->mitt.len )
        (void)trp_queue_get( (trp_obj_t *)( &(me->mitt) ) );
    if ( me->msg ) {
        /*
         siamo stati risvegliati da uno dei mittenti
         */
        *obj = me->msg;
        if ( mitt )
            *mitt = me->msg_mitt;
        me->msg = NULL;
        me->msg_mitt = NULL;
        res = 0;
    } else {
        /*
         tutti i mittenti sono terminati senza mandarci messaggi
         */
        res = 1;
    }
    me->stato = TRP_THREAD_STATE_RUNNING;
    trp_thread_private_unlock( me );
    return res;
}

static trp_obj_t *trp_thread_case_cmp( trp_thread_alternative_t *x, trp_thread_alternative_t *y )
{
    return ( x->priority > y->priority ) ? TRP_TRUE : TRP_FALSE;
}

#ifdef TRP_FORCE_FREE
static void trp_thread_case_free_array( trp_array_t *a )
{
    trp_obj_t *obj;
    uns32b n;

    for ( n = 0 ; n < a->len ; n++ ) {
        obj = (trp_obj_t *)( &( ( (trp_thread_alternative_t *)( a->data[ n ] ) )->mittq ) );
        while ( ((trp_queue_t *)obj)->len )
            (void)trp_queue_get( obj );
        trp_gc_free( a->data[ n ] );
    }
    trp_gc_free( a->data );
    trp_gc_free( a );
}
#endif

trp_obj_t *trp_thread_case( trp_obj_t *obj, ... )
{
    extern void trp_queue_init_internal( trp_queue_t *q );
    extern uns8b trp_array_sort_internal( trp_array_t *a, objfun_t *cmp );

    trp_thread_t *me;
    trp_array_t *a;
    trp_thread_alternative_t *alt;
    trp_queue_elem *elem, *prev;
    uns32b n, i;
    va_list args;
    uns8b err = 0;

    me = (trp_thread_t *)pthread_getspecific( _trp_thread_key );
    a = (trp_array_t *)trp_array_multi( UNDEF, ZERO, NULL );
    va_start( args, obj );
    for ( n = 1 ; ; n++ ) {
        if ( trp_cast_uns32b( obj, &i ) ) {
            err = 1;
            break;
        }
        obj = va_arg( args, trp_obj_t * );
        if ( obj == TRP_TRUE ) {
            alt = trp_gc_malloc( sizeof( trp_thread_alternative_t ) );
            trp_queue_init_internal( &( alt->mittq ) );
            alt->priority = i;
            alt->retcode = n;
            alt->obj = va_arg( args, trp_obj_t ** );
            if ( alt->obj ) {
                alt->mitt = va_arg( args, trp_obj_t ** );
                obj = va_arg( args, trp_obj_t * );
                if ( alt->mitt ) {
                    if ( obj ) {
                        switch ( obj->tipo ) {
                        case TRP_QUEUE:
                            for ( elem = (trp_queue_elem *)( ((trp_queue_t *)obj)->first ) ;
                                  elem ;
                                  elem = (trp_queue_elem *)( elem->next ) ) {
                                if ( elem->val->tipo != TRP_THREAD ) {
                                    err = 1;
                                    break;
                                }
                                if ( (trp_thread_t *)( elem->val ) != me )
                                    (void)trp_queue_put( (trp_obj_t *)( &( alt->mittq ) ), elem->val );
                            }
                            break;
                        case TRP_CONS:
                            for ( ; ; ) {
                                if ( ((trp_cons_t *)obj)->car->tipo != TRP_THREAD ) {
                                    err = 1;
                                    break;
                                }
                                if ( (trp_thread_t *)( ((trp_cons_t *)obj)->car ) != me )
                                    (void)trp_queue_put( (trp_obj_t *)( &( alt->mittq ) ), ((trp_cons_t *)obj)->car );
                                obj = ((trp_cons_t *)obj)->cdr;
                                if ( obj->tipo != TRP_CONS )
                                    break;
                            }
                            break;
                        case TRP_ARRAY:
                            {
                                trp_obj_t *o;
                                uns32b i;

                                for ( i = 0 ; i < ((trp_array_t *)obj)->len ; i++ ) {
                                    o = ((trp_array_t *)obj)->data[ i ];
                                    if ( o->tipo != TRP_THREAD ) {
                                        err = 1;
                                        break;
                                    }
                                    if ( (trp_thread_t *)o != me )
                                        (void)trp_queue_put( (trp_obj_t *)( &( alt->mittq ) ), o );
                                }
                            }
                            break;
                        default:
                            err = 1;
                            break;
                        }
                    } else {
                        for ( obj = va_arg( args, trp_obj_t * ) ;
                              obj ;
                              obj = va_arg( args, trp_obj_t * ) ) {
                            if ( obj->tipo != TRP_THREAD ) {
                                err = 1;
                                break;
                            }
                            if ( (trp_thread_t *)obj != me )
                                (void)trp_queue_put( (trp_obj_t *)( &( alt->mittq ) ), obj );
                        }
                    }
                    if ( err ) {
                        while ( alt->mittq.len )
                            (void)trp_queue_get( (trp_obj_t *)( &( alt->mittq ) ) );
                        trp_gc_free( alt );
                        break;
                    }
                } else {
                    if ( obj->tipo != TRP_THREAD ) {
                        trp_gc_free( alt );
                        err = 1;
                        break;
                    }
                    if ( (trp_thread_t *)obj != me )
                        (void)trp_queue_put( (trp_obj_t *)( &( alt->mittq ) ), obj );
                }
                if ( alt->mittq.len )
                    (void)trp_array_insert( (trp_obj_t *)a, NULL, (trp_obj_t *)alt, NULL );
                else
                    trp_gc_free( alt );
            } else {
                alt->mitt = NULL;
                (void)trp_array_insert( (trp_obj_t *)a, NULL, (trp_obj_t *)alt, NULL );
            }

        } else if ( obj == TRP_FALSE ) {
            obj = va_arg( args, trp_obj_t * );
            if ( obj ) {
                obj = va_arg( args, trp_obj_t * );
                if ( obj ) {
                    obj = va_arg( args, trp_obj_t * );
                    if ( obj == NULL )
                        for ( obj = va_arg( args, trp_obj_t * ) ;
                              obj ;
                              obj = va_arg( args, trp_obj_t * ) );
                } else
                    obj = va_arg( args, trp_obj_t * );
            }
        } else {
            err = 1;
            break;
        }
        obj = va_arg( args, trp_obj_t * );
        if ( obj == NULL )
            break;
    }
    va_end( args );
    if ( err || ( a->len == 0 ) ) {
        trp_thread_case_free_array( a );
        return ZERO;
    }
    (void)trp_array_sort_internal( a, (objfun_t *)trp_thread_case_cmp );
    /*
     inizia la sezione critica
     */
    trp_thread_private_lock( me );
    {
        trp_queue_elem *elem2;

        for ( n = 0 ; n < a->len ; n++ ) {
            alt = (trp_thread_alternative_t *)( a->data[ n ] );
            if ( alt->obj == NULL ) {
                trp_thread_private_unlock( me );
                obj = trp_sig64( alt->retcode );
                trp_thread_case_free_array( a );
                return obj;
            }
            for ( prev = NULL, elem = (trp_queue_elem *)( me->msgq.first ) ;
                  elem ;
                  elem = (trp_queue_elem *)( elem->next ) ) {
                obj = (trp_obj_t *)( ((trp_thread_msg_t *)( elem->val ))->mitt );
                for ( elem2 = (trp_queue_elem *)( alt->mittq.first ) ;
                      elem2 ;
                      elem2 = (trp_queue_elem *)( elem2->next ) )
                    if ( elem2->val == obj )
                        break;
                if ( elem2 )
                    break;
                prev = elem;
            }
            if ( elem ) {
                *( alt->obj ) = ((trp_thread_msg_t *)( elem->val ))->msg;
                if ( alt->mitt )
                    *( alt->mitt ) = obj;
                if ( prev == NULL )
                    me->msgq.first = elem->next;
                else
                    prev->next = elem->next;
                if ( elem->next == NULL )
                    me->msgq.last = prev;
                me->msgq.len--;
                trp_thread_private_unlock( me );
                if ( ((trp_thread_msg_t *)( elem->val ))->mitt_is_susp )
                    trp_thread_private_signal( obj );
                trp_gc_free( elem->val );
                trp_gc_free( elem );
                obj = trp_sig64( alt->retcode );
                trp_thread_case_free_array( a );
                return obj;
            }
        }
    }
    for ( n = 0 ; n < a->len ; n++ ) {
        alt = (trp_thread_alternative_t *)( a->data[ n ] );
        for ( elem = (trp_queue_elem *)( alt->mittq.first ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) )
            if ( ((trp_thread_t *)( elem->val ))->stato != TRP_THREAD_STATE_STOPPED )
                (void)trp_queue_put( (trp_obj_t *)( &(me->mitt) ), elem->val );
    }
    if ( me->mitt.len == 0 ) {
        trp_thread_private_unlock( me );
        trp_thread_case_free_array( a );
        return ZERO;
    }
    me->stato = TRP_THREAD_STATE_WAITING_ON_RECEIVE;
    trp_thread_private_wait( me, me );
    while ( me->mitt.len )
        (void)trp_queue_get( (trp_obj_t *)( &(me->mitt) ) );
    if ( me->msg == NULL ) {
        me->stato = TRP_THREAD_STATE_RUNNING;
        trp_thread_private_unlock( me );
        trp_thread_case_free_array( a );
        return ZERO;
    }
    for ( n = 0 ; ; n++ ) {
        alt = (trp_thread_alternative_t *)( a->data[ n ] );
        for ( elem = (trp_queue_elem *)( alt->mittq.first ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) )
            if ( elem->val == me->msg_mitt )
                break;
        if ( elem )
            break;
    }
    *( alt->obj ) = me->msg;
    if ( alt->mitt )
        *( alt->mitt ) = me->msg_mitt;
    me->msg = NULL;
    me->msg_mitt = NULL;
    me->stato = TRP_THREAD_STATE_RUNNING;
    trp_thread_private_unlock( me );
    obj = trp_sig64( alt->retcode );
    trp_thread_case_free_array( a );
    return obj;
}

