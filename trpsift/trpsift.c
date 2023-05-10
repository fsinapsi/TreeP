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

#include "../trp/trp.h"
#include "./trpsift.h"
#include "./libsiftfast.h"
#include "../trppix/trppix_internal.h"

typedef struct {
    float x;
    float y;
    float sigma;
    float theta;
    uns8b descr[ 128 ];
} trp_sift_keypoint_t;

typedef struct {
    uns8b tipo;
    uns32b cnt;
    trp_sift_keypoint_t *kp;
} trp_sift_t;

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

static uns8b trp_sift_print( trp_print_t *p, trp_sift_t *obj );
static uns8b trp_sift_close( trp_sift_t *obj );
static uns8b trp_sift_close_basic( uns8b flags, trp_sift_t *obj );
static void trp_sift_finalize( void *obj, void *data );
static trp_obj_t *trp_sift_length( trp_sift_t *obj );
static trp_obj_t *trp_sift_nth( uns32b n, trp_sift_t *obj );
static trp_sift_keypoint_t *trp_sift_keypoints_low( trp_obj_t *obj, uns32b *cnt, uns8b *to_free );
static uns32b trp_sift_euclidean_distance_square( uns8b *a1, uns8b *a2 );
static void trp_sift_keypoints_two_min( trp_sift_keypoint_t *kp1, uns32b cnt1,
                                        trp_sift_keypoint_t *kp2, uns32b cnt2,
                                        uns32b i, uns32b *dA, uns32b *dB, uns32b *iA );

uns8b trp_sift_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_nth_fun[];

    _trp_print_fun[ TRP_SIFT ] = trp_sift_print;
    _trp_close_fun[ TRP_SIFT ] = trp_sift_close;
    _trp_length_fun[ TRP_SIFT ] = trp_sift_length;
    _trp_nth_fun[ TRP_SIFT ] = trp_sift_nth;
    return 0;
}

static uns8b trp_sift_print( trp_print_t *p, trp_sift_t *obj )
{
    if ( trp_print_char_star( p, "#sift" ) )
        return 1;
    if ( obj->kp == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_sift_close( trp_sift_t *obj )
{
//    fprintf( stderr, "### sift_close\n" );
    return trp_sift_close_basic( 1, obj );
}

static uns8b trp_sift_close_basic( uns8b flags, trp_sift_t *obj )
{
    if ( obj->kp ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->kp );
        obj->kp = NULL;
    }
    return 0;
}

static void trp_sift_finalize( void *obj, void *data )
{
//    fprintf( stderr, "### sift_finalize\n" );
    trp_sift_close_basic( 0, (trp_sift_t *)obj );
}

static trp_obj_t *trp_sift_length( trp_sift_t *obj )
{
    return trp_sig64( obj->cnt );
}

static trp_obj_t *trp_sift_nth( uns32b n, trp_sift_t *obj )
{
    trp_sift_keypoint_t *kp;

    if ( n >= obj->cnt )
        return UNDEF;
    kp = obj->kp + n;
    return trp_list( trp_double( kp->x ), trp_double( kp->y ),
                     trp_double( kp->sigma ), trp_double( kp->theta ),
                     NULL );
}

static trp_sift_keypoint_t *trp_sift_keypoints_low( trp_obj_t *obj, uns32b *cnt, uns8b *to_free )
{
    static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    trp_pix_color_t *c;
    Image image;
    Keypoint kpt, k;
    trp_sift_keypoint_t *kp;
    int intdesc;
    uns32b w, h, i, j;

    if ( obj->tipo == TRP_SIFT ) {
        *cnt = ((trp_sift_t *)obj)->cnt;
        *to_free = 0;
        return ((trp_sift_t *)obj)->kp;
    }
    if ( ( c = trp_pix_get_mapc( obj ) ) == NULL )
        return NULL;
    w = ((trp_pix_t *)obj)->w;
    h = ((trp_pix_t *)obj)->h;
    pthread_mutex_lock( &mut );
    if ( ( image = CreateImage( h, w ) ) == NULL ) {
        pthread_mutex_unlock( &mut );
        return NULL;
    }
    for ( j = 0 ; j < h ; j++ )
        for ( i = 0 ; i < w ; i++, c++ )
            image->pixels[ j * image->stride + i ] =
                ( 0.212639005871510 * (float)( c->red ) +
                  0.715168678767756 * (float)( c->green ) +
                  0.072192315360734 * (float)( c->blue ) ) / 255.0;
    kpt = GetKeypoints( image );
    DestroyAllResources();
    pthread_mutex_unlock( &mut );
    if ( kpt == NULL )
        return NULL;
    for ( k = kpt, *cnt = 0 ; k ; k = k->next, (*cnt)++ );
    if ( *cnt == 0 ) {
        FreeKeypoints( kpt );
        return NULL;
    }
    if ( ( kp = malloc( *cnt * sizeof( trp_sift_keypoint_t ) ) ) == NULL ) {
        FreeKeypoints( kpt );
        return NULL;
    }
    for ( k = kpt, j = 0 ; k ; k = k->next, j++ ) {
        kp[ j ].x = k->col;
        kp[ j ].y = k->row;
        kp[ j ].sigma = k->scale;
        kp[ j ].theta = k->ori;
        for ( i = 0 ; i < 128 ; i++ ) {
            intdesc = (int)( k->descrip[ i ] * 512.0 );
            if ( intdesc > 255 )
                 intdesc = 255;
            kp[ j ].descr[ i ] = (uns8b)intdesc;
        }
    }
    FreeKeypoints( kpt );
    *to_free = 1;
    return kp;
}

static uns32b trp_sift_euclidean_distance_square( uns8b *a1, uns8b *a2 )
{
    int i, t;
    uns32b d = 0;

    for ( i = 0 ; i < 128 ; i++ ) {
        t = ( ((int)( *a1++ )) - ((int)( *a2++ )) );
        d += t * t;
    }
    return d;
}

static void trp_sift_keypoints_two_min( trp_sift_keypoint_t *kp1, uns32b cnt1,
                                        trp_sift_keypoint_t *kp2, uns32b cnt2,
                                        uns32b i, uns32b *dA, uns32b *dB, uns32b *iA )
{
    uns8b *v;
    uns32b j, d;

    v = kp1[ i ].descr;
    *dA = trp_sift_euclidean_distance_square( v, kp2[ 0 ].descr );
    *dB = trp_sift_euclidean_distance_square( v, kp2[ 1 ].descr );
    if ( *dA < *dB ) {
        *iA = 0;
    } else {
        d = *dA;
        *dA = *dB;
        *dB = d;
        *iA = 1;
    }
    for ( j = 2 ; j < cnt2 ; j++ ) {
        d = trp_sift_euclidean_distance_square( v, kp2[ j ].descr );
        if ( d < *dB ) {
            if ( d < *dA ) {
                *dB = *dA;
                *dA = d;
                *iA = j;
            } else {
                *dB = d;
            }
        }
    }
}

trp_obj_t *trp_sift_features( trp_obj_t *pix )
{
    trp_sift_t *res;
    trp_sift_keypoint_t *kp;
    uns32b cnt;
    uns8b to_free;

    if ( ( kp = trp_sift_keypoints_low( pix, &cnt, &to_free ) ) == NULL )
        return UNDEF;
    if ( to_free == 0 )
        return pix;
    res = trp_gc_malloc_atomic_finalize( sizeof( trp_sift_t ), trp_sift_finalize );
    res->tipo = TRP_SIFT;
    res->cnt = cnt;
    res->kp = kp;
    return (trp_obj_t *)res;
}

trp_obj_t *trp_sift_match( trp_obj_t *obj1, trp_obj_t *obj2, trp_obj_t *threshold )
{
    trp_obj_t *res;
    trp_sift_keypoint_t *kp1, *kp2;
    flt64b thresh;
    uns32b cnt1, cnt2;
    uns32b i, iA, iTest;
    uns32b dA, dB;
    uns8b to_free1, to_free2;

    if ( threshold ) {
        if ( trp_cast_flt64b_range( threshold, &thresh, 0.0, 10.0 ) )
            return UNDEF;
    } else
        thresh = 0.68;
    if ( ( kp1 = trp_sift_keypoints_low( obj1, &cnt1, &to_free1 ) ) == NULL )
        return UNDEF;
    if ( ( kp2 = trp_sift_keypoints_low( obj2, &cnt2, &to_free2 ) ) == NULL ) {
        if ( to_free1 )
            free( kp1 );
        return UNDEF;
    }
    res = trp_queue();
    if ( ( cnt1 >= 2 ) && ( cnt2 >= 2 ) ) {
        thresh *= thresh;
        for ( i = 0 ; i < cnt1 ; i++ ) {
            trp_sift_keypoints_two_min( kp1, cnt1, kp2, cnt2, i, &dA, &dB, &iA );
            if ( ((flt64b)dA) / ((flt64b)dB) <= thresh  ) {
                /* un secondo test rende simmetrica (e più attendibile) la funzione match (idea mia) */
                trp_sift_keypoints_two_min( kp2, cnt2, kp1, cnt1, iA, &dA, &dB, &iTest );
                if ( ( iTest == i ) && ( ((flt64b)dA) / ((flt64b)dB) <= thresh ) )
                    trp_queue_put( res,
                                   trp_cons( trp_cons( trp_double( kp1[ i ].x ),
                                                       trp_double( kp1[ i ].y ) ),
                                             trp_cons( trp_double( kp2[ iA ].x ),
                                                       trp_double( kp2[ iA ].y ) ) ) );
            }
        }
    }
    if ( to_free1 )
        free( kp1 );
    if ( to_free2 )
        free( kp2 );
    return res;
}

#define TRP_SIFT_ANALYZE_MIN 6.0

trp_obj_t *trp_sift_analyze( trp_obj_t *m )
{
    trp_queue_elem *elem1, *elem2;
    flt64b x1, y1, x2, y2, dx1, dy1, dx2, dy2, u;
    flt64b sumx[ 100 ], sumy[ 100 ], deltax[ 100 ], deltay[ 100 ];
    uns32b cntx[ 100 ], cnty[ 100 ], nx, ny, bnx, bny, i, j;

    if ( m->tipo != TRP_QUEUE )
        return UNDEF;
    for ( i = 0 ; i < 100 ; i++ ) {
        sumx[ i ] = sumy[ i ] = deltax[ i ] = deltay[ i ] = 0.0;
        cntx[ i ] = cnty[ i ] = 0;
    }
    for ( elem1 = (trp_queue_elem *)( ((trp_queue_t *)m)->first );
          elem1 ;
          elem1 = (trp_queue_elem *)( elem1->next ) ) {
        if ( trp_cast_flt64b( trp_car( trp_car ( elem1->val ) ), &x1 ) ||
             trp_cast_flt64b( trp_cdr( trp_car ( elem1->val ) ), &y1 ) ||
             trp_cast_flt64b( trp_car( trp_cdr ( elem1->val ) ), &x2 ) ||
             trp_cast_flt64b( trp_cdr( trp_cdr ( elem1->val ) ), &y2 ) )
            return UNDEF;
        for ( elem2 = (trp_queue_elem *)( elem1->next );
              elem2 ;
              elem2 = (trp_queue_elem *)( elem2->next ) ) {
            if ( trp_cast_flt64b( trp_car( trp_car ( elem2->val ) ), &dx1 ) ||
                 trp_cast_flt64b( trp_cdr( trp_car ( elem2->val ) ), &dy1 ) ||
                 trp_cast_flt64b( trp_car( trp_cdr ( elem2->val ) ), &dx2 ) ||
                 trp_cast_flt64b( trp_cdr( trp_cdr ( elem2->val ) ), &dy2 ) )
                return UNDEF;
            dx1 -= x1;
            dx2 -= x2;
            if ( ( ( dx1 < -TRP_SIFT_ANALYZE_MIN ) && ( dx2 < -TRP_SIFT_ANALYZE_MIN ) ) ||
                 ( ( dx1 >  TRP_SIFT_ANALYZE_MIN ) && ( dx2 >  TRP_SIFT_ANALYZE_MIN ) ) ) {
                dx2 /= dx1;
                i = (uns32b)( dx2 * 10.0 + 0.5 );
                i = TRP_MIN( i, 99 );
                sumx[ i ] += dx2;
                deltax[ i ] += ( x2 - dx2 * x1 );
                cntx[ i ]++;
            }
            dy1 -= y1;
            dy2 -= y2;
            if ( ( ( dy1 < -TRP_SIFT_ANALYZE_MIN ) && ( dy2 < -TRP_SIFT_ANALYZE_MIN ) ) ||
                 ( ( dy1 >  TRP_SIFT_ANALYZE_MIN ) && ( dy2 >  TRP_SIFT_ANALYZE_MIN ) ) ) {
                dy2 /= dy1;
                i = (uns32b)( dy2 * 10.0 + 0.5 );
                i = TRP_MIN( i, 99 );
                sumy[ i ] += dy2;
                deltay[ i ] += ( y2 - dy2 * y1 );
                cnty[ i ]++;
            }
        }
    }
    bnx = nx = cntx[ 0 ];
    x2 = sumx[ 0 ];
    dx2 = deltax[ 0 ];
    bny = ny = cnty[ 0 ];
    y2 = sumy[ 0 ];
    dy2 = deltay[ 0 ];
    for ( i = 1 ; i < 100 ; i++ ) {
        nx += cntx[ i ];
        ny += cnty[ i ];
        if ( j = cntx[ i ] ) {
            x1 = sumx[ i ];
            dx1 = deltax[ i ];
            if ( cntx[ i - 1 ] ) {
                u = sumx[ i - 1 ] / (flt64b)( cntx[ i - 1 ] ) - x1 / (flt64b)j;
                if ( TRP_ABS( u ) < 0.1 ) {
                    j += cntx[ i - 1 ];
                    x1 += sumx[ i - 1 ];
                    dx1 += deltax[ i - 1 ];
                }
            }
            if ( j > bnx ) {
                bnx = j;
                x2 = x1;
                dx2 = dx1;
            }
        }
        if ( j = cnty[ i ] ) {
            y1 = sumy[ i ];
            dy1 = deltay[ i ];
            if ( cnty[ i - 1 ] ) {
                u = sumy[ i - 1 ] / (flt64b)( cnty[ i - 1 ] ) - y1 / (flt64b)j;
                if ( TRP_ABS( u ) < 0.1 ) {
                    j += cnty[ i - 1 ];
                    y1 += sumy[ i - 1 ];
                    dy1 += deltay[ i - 1 ];
                }
            }
            if ( j > bny ) {
                bny = j;
                y2 = y1;
                dy2 = dy1;
            }
        }
    }
    return trp_list( trp_sig64( nx ), trp_sig64( ny ),
                     trp_sig64( bnx ), trp_sig64( bny ),
                     trp_double( x2 ), trp_double( y2 ),
                     trp_double( dx2 ), trp_double( dy2 ),
                     NULL );
}

