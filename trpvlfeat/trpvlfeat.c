/*
    TreeP Run Time Support
    Copyright (C) 2008-2020 Frank Sinapsi

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
#include "./trpvlfeat.h"
#include "../trppix/trppix_internal.h"
#include "./vl/sift.h"
#include "./vl/kdtree.h"

#define NN_SQ_DIST_RATIO_THR 0.4 /* 0.2 ? */

typedef struct {
    float x;
    float y;
} trp_vlpoint_t;

typedef struct {
    uns8b tipo;
    VlSiftFilt *filt;
    vl_sift_pix *descr;
    trp_vlpoint_t *ndx;
    int ndescr;
} trp_vlfeat_t;

static uns8b trp_vl_print( trp_print_t *p, trp_vlfeat_t *obj );
static uns8b trp_vl_close( trp_vlfeat_t *obj );
static uns8b trp_vl_close_basic( uns8b flags, trp_vlfeat_t *obj );
static void trp_vl_finalize( void *obj, void *data );
static VlSiftFilt *trp_vl_filt( trp_obj_t *obj );
static trp_obj_t *trp_vl_length( trp_vlfeat_t *obj );
static trp_obj_t *trp_vl_width( trp_vlfeat_t *obj );
static trp_obj_t *trp_vl_height( trp_vlfeat_t *obj );
static uns8b trp_vl_sift_set_basic( trp_obj_t *f, trp_obj_t *val, voidfun_t fun );
static trp_obj_t *trp_vl_sift_get_int( trp_obj_t *f, intfun_t fun );
static trp_obj_t *trp_vl_sift_get_double( trp_obj_t *f, doublefun_t fun );

uns8b trp_vl_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_VLFEAT ] = trp_vl_print;
    _trp_close_fun[ TRP_VLFEAT ] = trp_vl_close;
    _trp_length_fun[ TRP_VLFEAT ] = trp_vl_length;
    _trp_width_fun[ TRP_VLFEAT ] = trp_vl_width;
    _trp_height_fun[ TRP_VLFEAT ] = trp_vl_height;
    return 0;
}

static uns8b trp_vl_print( trp_print_t *p, trp_vlfeat_t *obj )
{
    if ( trp_print_char_star( p, "#vlfeat" ) )
        return 1;
    if ( obj->filt == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_vl_close( trp_vlfeat_t *obj )
{
    return trp_vl_close_basic( 1, obj );
}

static uns8b trp_vl_close_basic( uns8b flags, trp_vlfeat_t *obj )
{
    if ( obj->filt ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        vl_sift_delete( obj->filt );
        obj->filt = NULL;
        if ( obj->descr ) {
            free( obj->descr );
            obj->descr = NULL;
        }
        if ( obj->ndx ) {
            free( obj->ndx );
            obj->ndx = NULL;
        }
    }
    return 0;
}

static void trp_vl_finalize( void *obj, void *data )
{
    trp_vl_close_basic( 0, (trp_vlfeat_t *)obj );
}

static VlSiftFilt *trp_vl_filt( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_VLFEAT )
        return NULL;
    return ((trp_vlfeat_t *)obj)->filt;
}

static trp_obj_t *trp_vl_length( trp_vlfeat_t *obj )
{
    if ( obj->filt == NULL )
        return UNDEF;
    return trp_sig64( obj->ndescr );
}

static trp_obj_t *trp_vl_width( trp_vlfeat_t *obj )
{
    if ( obj->filt == NULL )
        return UNDEF;
    return trp_sig64( obj->filt->width );
}

static trp_obj_t *trp_vl_height( trp_vlfeat_t *obj )
{
    if ( obj->filt == NULL )
        return UNDEF;
    return trp_sig64( obj->filt->height );
}

static uns8b trp_vl_sift_set_basic( trp_obj_t *f, trp_obj_t *val, voidfun_t fun )
{
    VlSiftFilt *filt = trp_vl_filt( f );
    double dval;

    if ( ( filt == NULL ) || trp_cast_double( val, &dval ) )
        return 1;
    (fun)( filt, dval );
    return 0;
}

static trp_obj_t *trp_vl_sift_get_int( trp_obj_t *f, intfun_t fun )
{
    VlSiftFilt *filt = trp_vl_filt( f );

    if ( filt == NULL )
        return UNDEF;
    return trp_sig64( (fun)( filt ) );
}

static trp_obj_t *trp_vl_sift_get_double( trp_obj_t *f, doublefun_t fun )
{
    VlSiftFilt *filt = trp_vl_filt( f );

    if ( filt == NULL )
        return UNDEF;
    return trp_double( (fun)( filt ) );
}

trp_obj_t *trp_vl_version()
{
    return trp_cord( VL_VERSION_STRING );
}

trp_obj_t *trp_vl_sift_new( trp_obj_t *w, trp_obj_t *h, trp_obj_t *octaves, trp_obj_t *levels, trp_obj_t *o_min )
{
    trp_vlfeat_t *obj;
    VlSiftFilt *filt;

    if ( ( w->tipo != TRP_SIG64 ) || ( h->tipo != TRP_SIG64 ) ||
         ( octaves->tipo != TRP_SIG64 ) || ( levels->tipo != TRP_SIG64 ) ||
         ( o_min->tipo != TRP_SIG64 ) )
        return UNDEF;
    if ( ( filt = vl_sift_new( ((trp_sig64_t *)w)->val, ((trp_sig64_t *)h)->val,
                               ((trp_sig64_t *)octaves)->val, ((trp_sig64_t *)levels)->val,
                               ((trp_sig64_t *)o_min)->val ) ) == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_vlfeat_t ), trp_vl_finalize );
    obj->tipo = TRP_VLFEAT;
    obj->filt = filt;
    obj->descr = NULL;
    obj->ndx = NULL;
    obj->ndescr = 0;
    return (trp_obj_t *)obj;
}

uns8b trp_vl_sift_set_peak_thresh( trp_obj_t *f, trp_obj_t *val )
{
    return trp_vl_sift_set_basic( f, val, vl_sift_set_peak_thresh );
}

uns8b trp_vl_sift_set_edge_thresh( trp_obj_t *f, trp_obj_t *val )
{
    return trp_vl_sift_set_basic( f, val, vl_sift_set_edge_thresh );
}

uns8b trp_vl_sift_set_norm_thresh( trp_obj_t *f, trp_obj_t *val )
{
    return trp_vl_sift_set_basic( f, val, vl_sift_set_norm_thresh );
}

uns8b trp_vl_sift_set_magnif( trp_obj_t *f, trp_obj_t *val )
{
    return trp_vl_sift_set_basic( f, val, vl_sift_set_magnif );
}

uns8b trp_vl_sift_set_window_size( trp_obj_t *f, trp_obj_t *val )
{
    return trp_vl_sift_set_basic( f, val, vl_sift_set_window_size );
}

uns8b trp_vl_sift_process_first_octave( trp_obj_t *f, trp_obj_t *pix )
{
    VlSiftFilt *filt = trp_vl_filt( f );
    trp_pix_color_t *c;
    vl_sift_pix *fdata, *d;
    int n;

    if ( ( filt == NULL ) || ( pix->tipo != TRP_PIX ) )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    if ( ( filt->width != ((trp_pix_t *)pix)->w ) ||
         ( filt->height != ((trp_pix_t *)pix)->h ) )
        return 1;
    n = filt->width * filt->height;
    if ( ( fdata = malloc( n * sizeof( vl_sift_pix ) ) ) == NULL )
        return 1;
    for ( d = fdata ; n ; n--, c++ )
        *d++ =  ( (double)( ((uns32b)( c->red )) * 299 +
                            ((uns32b)( c->green )) * 587 +
                            ((uns32b)( c->blue )) * 114 ) ) / 1000.0;
    n = vl_sift_process_first_octave( filt, fdata );
    free( fdata );
    return n ? 1 : 0;
}

uns8b trp_vl_sift_process_next_octave( trp_obj_t *f )
{
    VlSiftFilt *filt = trp_vl_filt( f );

    if ( filt == NULL )
        return 1;
    return vl_sift_process_next_octave( filt ) ? 1 : 0;
}

uns8b trp_vl_sift_detect( trp_obj_t *f )
{
    VlSiftFilt *filt = trp_vl_filt( f );

    if ( filt == NULL )
        return 1;
    vl_sift_detect( filt );
    return 0;
}

trp_obj_t *trp_vl_sift_get_octave_index( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_octave_index );
}

trp_obj_t *trp_vl_sift_get_noctaves( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_noctaves );
}

trp_obj_t *trp_vl_sift_get_octave_first( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_octave_first );
}

trp_obj_t *trp_vl_sift_get_octave_width( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_octave_width );
}

trp_obj_t *trp_vl_sift_get_octave_height( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_octave_height );
}

trp_obj_t *trp_vl_sift_get_nlevels( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_nlevels );
}

trp_obj_t *trp_vl_sift_get_nkeypoints( trp_obj_t *f )
{
    return trp_vl_sift_get_int( f, vl_sift_get_nkeypoints );
}

trp_obj_t *trp_vl_sift_get_peak_thresh( trp_obj_t *f )
{
    return trp_vl_sift_get_double( f, vl_sift_get_peak_thresh );
}

trp_obj_t *trp_vl_sift_get_edge_thresh( trp_obj_t *f )
{
    return trp_vl_sift_get_double( f, vl_sift_get_edge_thresh );
}

trp_obj_t *trp_vl_sift_get_norm_thresh( trp_obj_t *f )
{
    return trp_vl_sift_get_double( f, vl_sift_get_norm_thresh );
}

trp_obj_t *trp_vl_sift_get_magnif( trp_obj_t *f )
{
    return trp_vl_sift_get_double( f, vl_sift_get_magnif );
}

trp_obj_t *trp_vl_sift_get_window_size( trp_obj_t *f )
{
    return trp_vl_sift_get_double( f, vl_sift_get_window_size );
}

trp_obj_t *trp_vl_sift_get_gss( trp_obj_t *f, trp_obj_t *level )
{
    VlSiftFilt *filt = trp_vl_filt( f );
    vl_sift_pix *pt;
    trp_pix_color_t *c;
    uns8b *data;
    uns32b lv, w, h;
    int n;

    if ( ( filt == NULL ) || trp_cast_uns32b( level, &lv ) )
        return UNDEF;
    if ( lv >= vl_sift_get_nlevels( filt ) )
        return UNDEF;
    w = vl_sift_get_octave_width( filt );
    h = vl_sift_get_octave_height( filt );
    n = w * h;
    if ( n == 0 )
        return UNDEF;
    if ( ( data = malloc( n << 2 ) ) == NULL )
        return UNDEF;
    for ( pt = vl_sift_get_octave( filt, lv ), c = (trp_pix_color_t *)data ; n ; n--, c++ ) {
        c->red = c->green = c->blue = (uns8b)( *pt++ );
        c->alpha = 0xff;
    }
    return trp_pix_create_image_from_data( 0, w, h, data );
}

uns8b trp_vl_sift_match_descr( trp_obj_t *f, trp_obj_t *pix )
{
    VlSiftFilt *filt = trp_vl_filt( f );
    trp_pix_color_t *c;
    vl_sift_pix *descr, *ddescr;
    trp_vlpoint_t *ndx, *nndx;
    VlSiftKeypoint const *kp;
    int nkeys, nangles, i, j, ndescr, allocated, end;
    double angles[ 4 ];

    if ( ( filt == NULL ) || ( pix->tipo != TRP_PIX ) )
        return 1;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return 1;
    if ( ( filt->width != ((trp_pix_t *)pix)->w ) ||
         ( filt->height != ((trp_pix_t *)pix)->h ) )
        return 1;
    i = filt->width * filt->height;
    if ( ( descr = malloc( i * sizeof( vl_sift_pix ) ) ) == NULL )
        return 1;
    for ( ddescr = descr ; i ; i--, c++ )
        *ddescr++ =  ( (double)( ((uns32b)( c->red )) * 299 +
                                 ((uns32b)( c->green )) * 587 +
                                 ((uns32b)( c->blue )) * 114 ) ) / 1000.0;
    end = vl_sift_process_first_octave( filt, descr );
    free( descr );
    for ( descr = NULL, ndx = NULL, ndescr = 0, allocated = 0 ; end != VL_ERR_EOF ; end = vl_sift_process_next_octave( filt ) ) {
        vl_sift_detect( filt );
        kp = vl_sift_get_keypoints( filt );
        nkeys = vl_sift_get_nkeypoints( filt );
        for ( i = 0 ; i < nkeys ; i++, kp++ ) {
            nangles = vl_sift_calc_keypoint_orientations( filt, angles, kp );
            for ( j = 0 ; j < nangles ; j++, ndescr++ ) {
                if ( ndescr == allocated ) {
                    allocated += 100;
                    ddescr = realloc( descr, allocated * ( sizeof( vl_sift_pix ) << 7 ) );
                    nndx = realloc( ndx, allocated * sizeof( trp_vlpoint_t ) );
                    if ( ( ddescr == NULL ) || ( nndx == NULL ) ) {
                        free( descr );
                        free( ndx );
                        return 1;
                    }
                    descr = ddescr;
                    ndx = nndx;
                }
                vl_sift_calc_keypoint_descriptor( filt, descr + ( ndescr << 7 ), kp, angles[ j ] );
                nndx = ndx + ndescr;
                nndx->x = kp->x;
                nndx->y = kp->y;
            }
        }
    }
    ((trp_vlfeat_t *)f)->descr = descr;
    ((trp_vlfeat_t *)f)->ndx = ndx;
    ((trp_vlfeat_t *)f)->ndescr = ndescr;
    return 0;
}

trp_obj_t *trp_vl_sift_match( trp_obj_t *f1, trp_obj_t *f2, trp_obj_t *cmp, trp_obj_t *thr )
{
    VlSiftFilt *filt1 = trp_vl_filt( f1 ), *filt2 = trp_vl_filt( f2 );
    vl_sift_pix *d1, *d2;
    trp_vlpoint_t *ndx1, *ndx2;
    trp_obj_t *res;
    int nd1, nd2;
    uns32b ccmp;
    double dthr;

    if ( trp_cast_uns32b( cmp, &ccmp ) || ( filt1 == NULL ) || ( filt2 == NULL ) )
        return UNDEF;
    if ( thr ) {
        if ( trp_cast_double_range( thr, &dthr, 0.0, 1.0 ) )
            return UNDEF;
    } else
        dthr = NN_SQ_DIST_RATIO_THR;
    d1 = ( (trp_vlfeat_t *)f1 )->descr;
    d2 = ( (trp_vlfeat_t *)f2 )->descr;
    ndx1 = ( (trp_vlfeat_t *)f1 )->ndx;
    ndx2 = ( (trp_vlfeat_t *)f2 )->ndx;
    if ( ( d1 == NULL ) || ( d2 == NULL ) || ( ndx1 == NULL ) || ( ndx2 == NULL ) )
        return UNDEF;
    nd1 = ( (trp_vlfeat_t *)f1 )->ndescr;
    nd2 = ( (trp_vlfeat_t *)f2 )->ndescr;
    res = trp_queue();
    if ( ( nd1 >= 2 ) && ( nd2 >= 2 ) ) {
        VlKDForest *kd;
        trp_vlpoint_t *p1, *p2;
        VlKDForestNeighbor nb[ 2 ];
        int i;

        if ( ( kd = vl_kdforest_new( VL_TYPE_FLOAT, 128, 1, VlDistanceL1 ) ) == NULL )
            return UNDEF;
        vl_kdforest_set_max_num_comparisons( kd, ccmp );
        vl_kdforest_build( kd, nd2, d2 );
        for ( i = 0 ; i < nd1 ; i++, d1 += 128 ) {
            vl_kdforest_query( kd, nb, 2, d1 );
            if ( nb[ 0 ].distance < nb[ 1 ].distance * dthr ) {
                p1 = ndx1 + i;
                p2 = ndx2 + nb[ 0 ].index;
                trp_queue_put( res, trp_cons( trp_cons( trp_double( p1->x ),
                                                        trp_double( p1->y ) ),
                                              trp_cons( trp_double( p2->x ),
                                                        trp_double( p2->y ) ) ) );
            }
        }
        vl_kdforest_delete( kd );
    }
    return res;
}



