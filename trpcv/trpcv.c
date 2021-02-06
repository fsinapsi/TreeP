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
#include "./trpcv.h"
#include "../trppix/trppix_internal.h"
#include "sift/imgfeatures.h"
#include "sift/minpq.h"
#include "sift/utils.h"
#include "sift/kdtree.h"
#include "sift/sift.h"
#include "sift/xform.h"
// #include <highgui.h>
#include <opencv2/highgui/highgui_c.h>

/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200

/* threshold on squared ratio of distances between NN and 2nd NN */
/* #define NN_SQ_DIST_RATIO_THR 0.49 */
#define NN_SQ_DIST_RATIO_THR 0.3

#define trp_abs(x) (((x)>=0)?(x):(-(x)))
#define TRP_PI 3.1415926535897932384626433832795029L

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    uns32b n;
    struct feature *feat;
} trp_cv_t;

uns32b trp_size_internal( trp_obj_t *obj );
void trp_encode_internal( trp_obj_t *obj, uns8b **buf );

static uns8b trp_cv_print( trp_print_t *p, trp_cv_t *obj );
static uns8b trp_cv_close( trp_cv_t *obj );
static uns8b trp_cv_close_basic( uns8b flags, trp_cv_t *obj );
static void trp_cv_finalize( void *obj, void *data );
static uns32b trp_cv_size( trp_cv_t *obj );
static void trp_cv_encode( trp_cv_t *obj, uns8b **buf );
static trp_obj_t *trp_cv_decode( uns8b **buf );
static trp_obj_t *trp_cv_equal( trp_cv_t *obj1, trp_cv_t *obj2 );
static trp_obj_t *trp_cv_length( trp_cv_t *obj );
static IplImage *trp_cv_pix2IplImage( trp_obj_t *pix );
static IplImage *trp_cv_pix2IplImage_alpha( trp_obj_t *pix );
static void trp_cv_copy_im2pix( IplImage *im, trp_obj_t *pix );
static uns8b trp_pix_load_cv( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static trp_obj_t *trp_cv_sift_features_basic( uns8b flags, trp_obj_t *pix );

uns8b trp_cv_init()
{
    extern uns8bfun_t _trp_pix_load_cv;
    extern objfun_t _trp_pix_rotate_cv;
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern uns32bfun_t _trp_size_fun[];
    extern voidfun_t _trp_encode_fun[];
    extern objfun_t _trp_decode_fun[];
    extern objfun_t _trp_equal_fun[];
    extern objfun_t _trp_length_fun[];

    _trp_pix_load_cv = trp_pix_load_cv;
    _trp_pix_rotate_cv = trp_cv_pix_rotate;
    _trp_print_fun[ TRP_OPENCV ] = trp_cv_print;
    _trp_close_fun[ TRP_OPENCV ] = trp_cv_close;
    _trp_size_fun[ TRP_OPENCV ] = trp_cv_size;
    _trp_encode_fun[ TRP_OPENCV ] = trp_cv_encode;
    _trp_decode_fun[ TRP_OPENCV ] = trp_cv_decode;
    _trp_equal_fun[ TRP_OPENCV ] = trp_cv_equal;
    _trp_length_fun[ TRP_OPENCV ] = trp_cv_length;
    return 0;
}

static uns8b trp_cv_print( trp_print_t *p, trp_cv_t *obj )
{
    if ( trp_print_char_star( p, "#opencv" ) )
        return 1;
    if ( obj->feat == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_cv_close( trp_cv_t *obj )
{
    return trp_cv_close_basic( 1, obj );
}

static uns8b trp_cv_close_basic( uns8b flags, trp_cv_t *obj )
{
    if ( obj->feat ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->feat );
        obj->feat = NULL;
    }
    return 0;
}

static void trp_cv_finalize( void *obj, void *data )
{
    trp_cv_close_basic( 0, (trp_cv_t *)obj );
}

static uns32b trp_cv_size( trp_cv_t *obj )
{
    uns32b sz;

    /*
     FIXME
     */
    if ( obj->feat )
        sz = 1 + 1 + 4 + obj->n * sizeof( struct feature );
    else
        sz = trp_size_internal( UNDEF );
    return sz;
}

static void trp_cv_encode( trp_cv_t *obj, uns8b **buf )
{
    /*
     FIXME
     */
}

static trp_obj_t *trp_cv_decode( uns8b **buf )
{
    /*
     FIXME
     */
    return UNDEF;
}

static trp_obj_t *trp_cv_equal( trp_cv_t *obj1, trp_cv_t *obj2 )
{
    /*
     FIXME
     */
    return TRP_FALSE;
}

static trp_obj_t *trp_cv_length( trp_cv_t *obj )
{
    return obj->feat ? trp_sig64( obj->n ) : UNDEF;
}

trp_obj_t *trp_cv_version()
{
    uns8b buf[ 12 ];

    sprintf( buf, "%d.%d.%d",
             CV_MAJOR_VERSION,
             CV_MINOR_VERSION,
             CV_SUBMINOR_VERSION );
    return trp_cord( buf );
}

static IplImage *trp_cv_pix2IplImage( trp_obj_t *pix )
{
    IplImage *im;
    trp_pix_color_t *c;
    uns8b *p;
    uns32b w, h, i, j;

    if ( pix->tipo != TRP_PIX )
        return NULL;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( im = cvCreateImage( cvSize( w, h ), IPL_DEPTH_8U, 3 ) ) == NULL )
        return NULL;
    for ( i = 0 ; i < h ; i++ )
        for ( p = im->imageData + i * im->widthStep, j = 0 ; j < w ; j++, c++ ) {
            *p++ = c->blue;
            *p++ = c->green;
            *p++ = c->red;
        }
    return im;
}

static IplImage *trp_cv_pix2IplImage_alpha( trp_obj_t *pix )
{
    IplImage *im;
    trp_pix_color_t *c;
    uns32b w, h;

    if ( pix->tipo != TRP_PIX )
        return NULL;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( im = cvCreateImage( cvSize( w, h ), IPL_DEPTH_8U, 4 ) ) == NULL )
        return NULL;
    memcpy( (void *)( im->imageData ), (void *)c, ( w * h ) << 2 );
    return im;
}

static void trp_cv_copy_im2pix( IplImage *im, trp_obj_t *pix )
{
    trp_pix_color_t *c = ((trp_pix_t *)pix)->map.c;
    uns8b *p;
    uns32b w = ((trp_pix_t *)pix)->w, h = ((trp_pix_t *)pix)->h, i, j;

    switch ( im->nChannels ) {
    case 1:
        for ( i = 0 ; i < h ; i++ )
            for ( p = im->imageData + i * im->widthStep, j = 0 ; j < w ; j++, c++ ) {
                c->red = c->green = c->blue = *p++;
            }
        break;
    case 3:
        for ( i = 0 ; i < h ; i++ )
            for ( p = im->imageData + i * im->widthStep, j = 0 ; j < w ; j++, c++ ) {
                c->blue = *p++;
                c->green = *p++;
                c->red = *p++;
            }
        break;
    case 4:
        memcpy( (void *)c, (void *)( im->imageData ), ( w * h ) << 2 );
        break;
    }
}

static uns8b trp_pix_load_cv( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    IplImage *im = cvLoadImage( cpath, 1 );
    trp_pix_color_t *c;
    uns8b *p;
    uns32b i, j;

    if ( im == NULL )
        return 1;
    /*
    printf( "#supp: %d x %d, depth=%d, ch=%d, alpha=%d\n",
            im->width, im->height, im->depth, im->nChannels, im->alphaChannel );
     */
    if ( ( im->depth != 8 ) || ( im->nChannels != 3 ) ) {
        cvReleaseImage( &im );
        return 1;
    }
    *w = im->width;
    *h = im->height;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL ) {
        cvReleaseImage( &im );
        return 1;
    }
    c = (trp_pix_color_t *)( *data );
    for ( i = 0 ; i < *h ; i++ )
        for ( p = im->imageData + i * im->widthStep, j = 0 ; j < *w ; j++, c++ ) {
            c->alpha = 0xff;
            c->blue = *p++;
            c->green = *p++;
            c->red = *p++;
        }
    cvReleaseImage( &im );
    return 0;
}

trp_obj_t *trp_cv_pix_load( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), *data;
    uns32b w, h;

    if ( trp_pix_load_cv( cpath, &w, &h, &data ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
    trp_csprint_free( cpath );
    return trp_pix_create_image_from_data( 0, w, h, data );
}

uns8b trp_cv_pix_gray( trp_obj_t *pix )
{
    IplImage *im = trp_cv_pix2IplImage( pix ), *gim;

    if ( im == NULL )
        return 1;
    if ( gim = cvCreateImage( cvGetSize( im ), IPL_DEPTH_8U, 1 ) )
        cvCvtColor( im, gim, CV_BGR2GRAY );
    cvReleaseImage( &im );
    if ( gim == NULL )
        return 1;
    trp_cv_copy_im2pix( gim, pix );
    cvReleaseImage( &gim );
    return 0;
}

uns8b trp_cv_pix_smooth( trp_obj_t *pix, trp_obj_t *type, trp_obj_t *size1, trp_obj_t *size2, trp_obj_t *sigma1, trp_obj_t *sigma2 )
{
    IplImage *im;
    uns32b ttype, ssize1, ssize2;
    double ssigma1, ssigma2;

    if ( trp_cast_uns32b( type, &ttype ) ||
         trp_cast_uns32b_range( size1, &ssize1, 1, 0xffff ) ||
         trp_cast_uns32b_range( size2, &ssize2, 1, 0xffff ) ||
         trp_cast_double( sigma1, &ssigma1 ) ||
         trp_cast_double( sigma2, &ssigma2 ) )
        return 1;
    if ( ( im = trp_cv_pix2IplImage( pix ) ) == NULL )
        return 1;
    cvSmooth( im, im, ttype, ssize1, ssize2, ssigma1, ssigma2 );
    trp_cv_copy_im2pix( im, pix );
    cvReleaseImage( &im );
    return 0;
}

trp_obj_t *trp_cv_pix_rotate( trp_obj_t *pix, trp_obj_t *angle, trp_obj_t *flags )
{
    uns32b fflags;
    IplImage *imi, *imo;
    CvPoint2D32f srcTri[ 3 ], dstTri[ 3 ];
    CvMat *rot;
    uns32b w, h;
    double x1, x2, y1, y2;

    if ( flags ) {
        if ( trp_cast_uns32b( flags, &fflags ) )
            return UNDEF;
    } else
        fflags = CV_INTER_CUBIC;
    if ( trp_cast_double( angle, &y2 ) )
        return UNDEF;
    if ( ( imi = trp_cv_pix2IplImage_alpha( pix ) ) == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    y2 *= (TRP_PI/180.0);
    y1 = sin( y2 );
    y2 = cos( y2 );
    if ( ( pix = trp_pix_create_basic( (uns32b)( trp_abs( y2 * (double)w ) + trp_abs( y1 * (double)h ) + 0.5 ),
                                       (uns32b)( trp_abs( y1 * (double)w ) + trp_abs( y2 * (double)h ) + 0.5 )
                                     ) ) == UNDEF ) {
        cvReleaseImage( &imi );
        return UNDEF;
    }
    memset( ((trp_pix_t *)pix)->map.t, 0, ( ((trp_pix_t *)pix)->w * ((trp_pix_t *)pix)->h ) << 2 );
    if ( ( imo = trp_cv_pix2IplImage_alpha( pix ) ) == NULL ) {
        cvReleaseImage( &imi );
        trp_pix_close( (trp_pix_t *)pix );
        return UNDEF;
    }
    --w;
    --h;
    x1 = y2 * (double)w;
    x2 = y1 * (double)h;
    y1 *= (double)w;
    y2 *= (double)h;
    srcTri[0].x = 0.0;
    srcTri[0].y = 0.0;
    srcTri[1].x = w;
    srcTri[1].y = 0.0;
    srcTri[2].x = 0.0;
    srcTri[2].y = h;
    if ( ( y1 >= 0.0 ) && ( y2 >= 0.0 ) ) {
        dstTri[0].x = 0.0;
        dstTri[0].y = y1;
        dstTri[1].x = x1;
        dstTri[1].y = 0.0;
        dstTri[2].x = x2;
        dstTri[2].y = y1 + y2;
    } else if ( ( y1 >= 0.0 ) && ( y2 <= 0.0 ) ) {
        dstTri[0].x = -x1;
        dstTri[0].y = y1 - y2;
        dstTri[1].x = 0.0;
        dstTri[1].y = -y2;
        dstTri[2].x = -x1 + x2;
        dstTri[2].y = y1;
    } else if ( ( y1 <= 0.0 ) && ( y2 <= 0.0 ) ) {
        dstTri[0].x = -x1 -x2;
        dstTri[0].y = -y2;
        dstTri[1].x = -x2;
        dstTri[1].y = -y1 - y2;
        dstTri[2].x = -x1;
        dstTri[2].y = 0.0;
    } else { /* y1 < 0.0 && y2 > 0.0 */
        dstTri[0].x = -x2;
        dstTri[0].y = 0.0;
        dstTri[1].x = x1 - x2;
        dstTri[1].y = -y1;
        dstTri[2].x = 0.0;
        dstTri[2].y = y2;
    }
    rot = cvCreateMat( 2, 3, CV_32FC1 );
    cvGetAffineTransform( srcTri, dstTri, rot );
    cvWarpAffine( imi, imo, rot, (int)fflags, cvScalarAll( 0 ) );
    cvReleaseImage( &imi );
    cvReleaseMat( &rot );
    trp_cv_copy_im2pix( imo, pix );
    cvReleaseImage( &imo );
    return pix;
}

trp_obj_t *trp_cv_get_affine_transform( trp_obj_t *sx1, trp_obj_t *sy1, trp_obj_t *dx1, trp_obj_t *dy1, trp_obj_t *sx2, trp_obj_t *sy2, trp_obj_t *dx2, trp_obj_t *dy2, trp_obj_t *sx3, trp_obj_t *sy3, trp_obj_t *dx3, trp_obj_t *dy3 )
{
    trp_obj_t *res;
    double ssx1, ssy1, ddx1, ddy1, ssx2, ssy2, ddx2, ddy2, ssx3, ssy3, ddx3, ddy3;
    CvPoint2D32f srcTri[ 3 ], dstTri[ 3 ];
    CvMat *mat;

    if ( trp_cast_double( sx1, &ssx1 ) ||
         trp_cast_double( sy1, &ssy1 ) ||
         trp_cast_double( dx1, &ddx1 ) ||
         trp_cast_double( dy1, &ddy1 ) ||
         trp_cast_double( sx2, &ssx2 ) ||
         trp_cast_double( sy2, &ssy2 ) ||
         trp_cast_double( dx2, &ddx2 ) ||
         trp_cast_double( dy2, &ddy2 ) ||
         trp_cast_double( sx3, &ssx3 ) ||
         trp_cast_double( sy3, &ssy3 ) ||
         trp_cast_double( dx3, &ddx3 ) ||
         trp_cast_double( dy3, &ddy3 ) )
        return UNDEF;
    srcTri[0].x = ssx1;
    srcTri[0].y = ssy1;
    dstTri[0].x = ddx1;
    dstTri[0].y = ddy1;
    srcTri[1].x = ssx2;
    srcTri[1].y = ssy2;
    dstTri[1].x = ddx2;
    dstTri[1].y = ddy2;
    srcTri[2].x = ssx3;
    srcTri[2].y = ssy3;
    dstTri[2].x = ddx3;
    dstTri[2].y = ddy3;
    mat = cvCreateMat( 2, 3, CV_32FC1 );
    cvGetAffineTransform( srcTri, dstTri, mat );
    res = trp_list( trp_list( trp_double( CV_MAT_ELEM( *mat, float, 0, 0 ) ),
                              trp_double( CV_MAT_ELEM( *mat, float, 0, 1 ) ),
                              trp_double( CV_MAT_ELEM( *mat, float, 0, 2 ) ),
                              NULL ),
                    trp_list( trp_double( CV_MAT_ELEM( *mat, float, 1, 0 ) ),
                              trp_double( CV_MAT_ELEM( *mat, float, 1, 1 ) ),
                              trp_double( CV_MAT_ELEM( *mat, float, 1, 2 ) ),
                              NULL ),
                    NULL );
    cvReleaseMat( &mat );
    return res;
}

uns8b trp_cv_pix_warp_affine( trp_obj_t *pixi, trp_obj_t *pixo, trp_obj_t *aff_mat, trp_obj_t *flags )
{
    uns32b fflags;
    double m00, m01, m02, m10, m11, m12;
    trp_obj_t *c;
    IplImage *imi, *imo;
    CvMat *mat;

    if ( flags ) {
        if ( trp_cast_uns32b( flags, &fflags ) )
            return 1;
    } else
        fflags = CV_INTER_CUBIC;
    if ( aff_mat->tipo != TRP_CONS )
        return 1;
    if ( ((trp_cons_t *)aff_mat)->cdr->tipo != TRP_CONS )
        return 1;
    if ( ((trp_cons_t *)((trp_cons_t *)aff_mat)->cdr)->cdr != NIL )
        return 1;
    c = ((trp_cons_t *)aff_mat)->car;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m00 ) )
        return 1;
    c = ((trp_cons_t *)c)->cdr;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m01 ) )
        return 1;
    c = ((trp_cons_t *)c)->cdr;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m02 ) ||
         ( ((trp_cons_t *)c)->cdr != NIL ) )
        return 1;
    c = ((trp_cons_t *)((trp_cons_t *)aff_mat)->cdr)->car;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m10 ) )
        return 1;
    c = ((trp_cons_t *)c)->cdr;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m11 ) )
        return 1;
    c = ((trp_cons_t *)c)->cdr;
    if ( c->tipo != TRP_CONS )
        return 1;
    if ( trp_cast_double( ((trp_cons_t *)c)->car, &m12 ) ||
         ( ((trp_cons_t *)c)->cdr != NIL ) )
        return 1;
    mat = cvCreateMat( 2, 3, CV_32FC1 );
    *( (float *)CV_MAT_ELEM_PTR( *mat, 0, 0 ) ) = m00;
    *( (float *)CV_MAT_ELEM_PTR( *mat, 0, 1 ) ) = m01;
    *( (float *)CV_MAT_ELEM_PTR( *mat, 0, 2 ) ) = m02;
    *( (float *)CV_MAT_ELEM_PTR( *mat, 1, 0 ) ) = m10;
    *( (float *)CV_MAT_ELEM_PTR( *mat, 1, 1 ) ) = m11;
    *( (float *)CV_MAT_ELEM_PTR( *mat, 1, 2 ) ) = m12;
    if ( ( imi = trp_cv_pix2IplImage( pixi ) ) == NULL ) {
        cvReleaseMat( &mat );
        return 1;
    }
    if ( ( imo = trp_cv_pix2IplImage( pixo ) ) == NULL ) {
        cvReleaseImage( &imi );
        cvReleaseMat( &mat );
        return 1;
    }
    cvWarpAffine( imi, imo, mat, (int)fflags, cvScalarAll( 0 ) );
    cvReleaseImage( &imi );
    cvReleaseMat( &mat );
    trp_cv_copy_im2pix( imo, pixo );
    cvReleaseImage( &imo );
    return 0;
}

static trp_obj_t *trp_cv_sift_features_basic( uns8b flags, trp_obj_t *pix )
{
    IplImage *im = trp_cv_pix2IplImage( pix );
    struct feature *feat;
    uns32b n;

    if ( im == NULL )
        return UNDEF;
    n = sift_features( im, &feat );
    if ( ( flags & 1 ) && n ) {
        draw_features( im, feat, n );
        trp_cv_copy_im2pix( im, pix );
    }
    cvReleaseImage( &im );
    if ( n == 0 )
        return UNDEF;
    pix = trp_gc_malloc_atomic_finalize( sizeof( trp_cv_t ), trp_cv_finalize );
    pix->tipo = TRP_OPENCV;
    ((trp_cv_t *)pix)->sottotipo = 0;
    ((trp_cv_t *)pix)->n = n;
    ((trp_cv_t *)pix)->feat = feat;
    return pix;
}

trp_obj_t *trp_cv_sift_features( trp_obj_t *pix )
{
    return trp_cv_sift_features_basic( 0, pix );
}

trp_obj_t *trp_cv_sift_features_draw( trp_obj_t *pix )
{
    return trp_cv_sift_features_basic( 1, pix );
}

trp_obj_t *trp_cv_sift_match( trp_obj_t *o1, trp_obj_t *o2, trp_obj_t *thr )
{
    trp_obj_t *res;
    struct feature *feat1, *feat2, *feat, **nbrs;
    struct kd_node *kd_root;
    double dthr;
    uns32b n1, n2, i, k;

    if ( thr == NULL )
        dthr = NN_SQ_DIST_RATIO_THR;
    else if ( trp_cast_double_range( thr, &dthr, 0.0, 1.0 ) )
        return UNDEF;
    if ( ( o1->tipo != TRP_OPENCV ) || ( o2->tipo != TRP_OPENCV ) )
        return UNDEF;
    if ( ( ( feat1 = ((trp_cv_t *)o1)->feat ) == NULL ) ||
         ( ( feat2 = ((trp_cv_t *)o2)->feat ) == NULL ) )
        return UNDEF;
    res = trp_queue();
    n1 = ((trp_cv_t *)o1)->n;
    n2 = ((trp_cv_t *)o2)->n;
    if ( ( n1 < 2 ) || ( n2 < 2 ) )
        return res;
    kd_root = kdtree_build( feat2, n2 );
    for ( i = 0 ; i < n1 ; i++ ) {
        feat = feat1 + i;
        k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
        if( k == 2 ) {
            if ( descr_dist_sq( feat, nbrs[0] ) < descr_dist_sq( feat, nbrs[1] ) * dthr ) {
                trp_queue_put( res, trp_cons( trp_cons( trp_double( feat->x ), trp_double( feat->y ) ),
                                              trp_cons( trp_double( nbrs[0]->x ), trp_double( nbrs[0]->y ) ) ) );
                feat1[i].fwd_match = nbrs[0];
            }
        }
        free( nbrs );
    }
    kdtree_release( kd_root );
    return res;
}

