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
#include "./trplept.h"
#include "../trppix/trppix_internal.h"

static PIX *trp_lept_pix2PIX( trp_obj_t *pix );
static PIX *trp_lept_pix2PIX_8bpp( trp_obj_t *pix );
static PIX *trp_lept_pix2PIX_1bpp( trp_obj_t *pix );
static trp_obj_t *trp_lept_PIX2pix( PIX *pixs );
static void trp_lept_copy_PIX2pix( PIX *pixs, trp_obj_t *pix );
static uns8b trp_pix_load_lept( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );

#define TRP_PI 3.1415926535897932384626433832795029L

uns8b trp_lept_init()
{
    extern uns8bfun_t _trp_pix_load_lept;
    extern objfun_t _trp_pix_rotate_lept;

    _trp_pix_load_lept = trp_pix_load_lept;
    _trp_pix_rotate_lept = trp_lept_pix_rotate;
    setMsgSeverity( L_SEVERITY_NONE );
    return 0;
}

trp_obj_t *trp_lept_version()
{
    trp_obj_t *res;
    uns8b *v = getLeptonicaVersion();

    res = trp_cord( v );
    free( v );
    return res;
}

trp_obj_t *trp_lept_cversion()
{
    char v[ 100 ];

    snprintf( v, 100, "leptonica-%d.%d", LIBLEPT_MAJOR_VERSION, LIBLEPT_MINOR_VERSION );
    return trp_cord( v );
}

static PIX *trp_lept_pix2PIX( trp_obj_t *pix )
{
    PIX *pixs;
    trp_pix_color_t *c, *d;
    uns32b w, h, i;

    if ( pix->tipo != TRP_PIX )
        return NULL;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( pixs = pixCreateNoInit( w, h, 32 ) ) == NULL )
        return NULL;
    d = (trp_pix_color_t *)( pixs->data );
    for ( i = w * h ; ; ) {
        i--;
        d[ i ].red = ~( c[ i ].alpha );
        d[ i ].green = c[ i ].blue;
        d[ i ].blue = c[ i ].green;
        d[ i ].alpha = c[ i ].red;
        if ( i == 0 )
            break;
    }
    return pixs;
}

static PIX *trp_lept_pix2PIX_8bpp( trp_obj_t *pix )
{
    PIX *pixs;
    trp_pix_color_t *c;
    uns32b w, h, i, j, buf, cnt;
    uns32b *d;

    if ( pix->tipo != TRP_PIX )
        return NULL;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( pixs = pixCreateNoInit( w, h, 8 ) ) == NULL )
        return NULL;
    for ( i = 0 ; i < h ; i++ ) {
        d = pixs->data + pixs->wpl * i;
        for ( j = 0, buf = 0, cnt = 32 ; j < w ; j++, c++ ) {
            if ( cnt == 0 ) {
                *d++ = buf;
                buf = 0;
                cnt = 32;
            }
            cnt -= 8;
            buf |= ( ( ( (uns32b)( c->red ) * 299 +
                         (uns32b)( c->green ) * 587 +
                         (uns32b)( c->blue ) * 114 + 500 ) / 1000 ) << cnt );
        }
        *d = buf;
    }
    return pixs;
}

static PIX *trp_lept_pix2PIX_1bpp( trp_obj_t *pix )
{
    PIX *pixs;
    trp_pix_color_t *c;
    uns32b w, h, i, j, buf, cnt;
    uns32b *d;

    if ( pix->tipo != TRP_PIX )
        return NULL;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return NULL;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( ( pixs = pixCreateNoInit( w, h, 1 ) ) == NULL )
        return NULL;
    for ( i = 0 ; i < h ; i++ ) {
        d = pixs->data + pixs->wpl * i;
        for ( j = 0, buf = cnt = 0 ; j < w ; j++, c++, ++cnt ) {
            if ( cnt == 32 ) {
                *d++ = buf;
                buf = cnt = 0;
            }
            if ( ( ( (uns32b)( c->red ) * 299 +
                     (uns32b)( c->green ) * 587 +
                     (uns32b)( c->blue ) * 114 + 500 ) / 1000 ) < 128 )
                buf |= ( 0x80000000 >> cnt );
        }
        *d = buf;
    }
    return pixs;
}

static trp_obj_t *trp_lept_PIX2pix( PIX *pixs )
{
    trp_obj_t *res = trp_pix_create_basic( pixs->w, pixs->h );

    if ( res != UNDEF )
        trp_lept_copy_PIX2pix( pixs, res );
    return res;
}

static void trp_lept_copy_PIX2pix( PIX *pixs, trp_obj_t *pix )
{
    trp_pix_color_t *c = ((trp_pix_t *)pix)->map.c;
    uns32b w = ((trp_pix_t *)pix)->w, h = ((trp_pix_t *)pix)->h;

    switch ( pixs->d ) {
    case 1:
        {
            uns32b i, j, buf, cnt;
            uns32b *d;

            for ( i = 0 ; i < h ; i++ ) {
                d = pixs->data + pixs->wpl * i;
                for ( j = 0, cnt = 32 ; j < w ; j++, c++, buf <<= 1, ++cnt ) {
                    if ( cnt == 32 ) {
                        buf = *d++;
                        cnt = 0;
                    }
                    c->red = c->green = c->blue = ( buf & 0x80000000 ) ? 0 : 0xff;
                    c->alpha = 0xff;
                }
            }
        }
        break;
    case 8:
        {
            uns32b i, j, buf, cnt;
            uns32b *d;

            for ( i = 0 ; i < h ; i++ ) {
                d = pixs->data + pixs->wpl * i;
                for ( j = 0, cnt = 4 ; j < w ; j++, c++, buf <<= 8, ++cnt ) {
                    if ( cnt == 4 ) {
                        buf = *d++;
                        cnt = 0;
                    }
                    c->red = c->green = c->blue = (uns8b)( buf >> 24 );
                    c->alpha = 0xff;
                }
            }
        }
        break;
    case 32:
        {
            uns32b i;
            trp_pix_color_t *d = (trp_pix_color_t *)( pixs->data );

            for ( i = w * h ; ; ) {
                i--;
                c[ i ].red = d[ i ].alpha;
                c[ i ].green = d[ i ].blue;
                c[ i ].blue = d[ i ].green;
                c[ i ].alpha = ~( d[ i ].red );
                if ( i == 0 )
                    break;
            }
        }
        break;
    }
}

static uns8b trp_pix_load_lept( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    PIX *pixs;
    trp_pix_color_t *c;

    if ( ( pixs = pixRead( cpath ) ) == NULL )
        return 1;
    *w = pixs->w;
    *h = pixs->h;
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL ) {
        pixDestroy( &pixs );
        return 1;
    }
    c = (trp_pix_color_t *)( *data );
    switch ( pixs->d ) {
    case 1:
        {
            uns32b i, j, buf, cnt;
            uns32b *d;

            for ( i = 0 ; i < *h ; i++ ) {
                d = pixs->data + pixs->wpl * i;
                for ( j = 0, cnt = 32 ; j < *w ; j++, c++, buf <<= 1, ++cnt ) {
                    if ( cnt == 32 ) {
                        buf = *d++;
                        cnt = 0;
                    }
                    c->red = c->green = c->blue = ( buf & 0x80000000 ) ? 0 : 0xff;
                    c->alpha = 0xff;
                }
            }
        }
        break;
    case 8:
        {
            uns32b i, j, buf, cnt;
            uns32b *d;

            for ( i = 0 ; i < *h ; i++ ) {
                d = pixs->data + pixs->wpl * i;
                for ( j = 0, cnt = 4 ; j < *w ; j++, c++, buf <<= 8, ++cnt ) {
                    if ( cnt == 4 ) {
                        buf = *d++;
                        cnt = 0;
                    }
                    c->red = c->green = c->blue = (uns8b)( buf >> 24 );
                    c->alpha = 0xff;
                }
            }
        }
        break;
    case 32:
        {
            uns32b i;
            trp_pix_color_t *d = (trp_pix_color_t *)( pixs->data );

            for ( i = *w * *h ; ; ) {
                i--;
                c[ i ].red = d[ i ].alpha;
                c[ i ].green = d[ i ].blue;
                c[ i ].blue = d[ i ].green;
                c[ i ].alpha = ~( d[ i ].red );
                if ( i == 0 )
                    break;
            }
        }
        break;
    default:
//        printf( "#supp: %d x %d, depth=%d, wpl=%d\n", *w, *h, pixs->d, pixs->wpl );
        free( (void *)( *data ));
        pixDestroy( &pixs );
        return 1;
    }
    pixDestroy( &pixs );
    return 0;
}

trp_obj_t *trp_lept_pix_load( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path ), *data;
    uns32b w, h;

    if ( trp_pix_load_lept( cpath, &w, &h, &data ) ) {
        trp_csprint_free( cpath );
        return UNDEF;
    }
    trp_csprint_free( cpath );
    return trp_pix_create_image_from_data( 0, w, h, data );
}

trp_obj_t *trp_lept_pix_rotate( trp_obj_t *pix, trp_obj_t *angle )
{
    PIX *pixs1, *pixs2, *pixd;
    trp_pix_color_t *c, *d1, *d2;
    uns32b w, h, ww, hh, i;
    double a, si, co;
    uns8b tofree = 0;

    if ( trp_cast_double( angle, &a ) )
        return UNDEF;
    a = -a;
    while ( a >= 360.0 - 45.0 )
        a -= 360.0;
    while ( a < 0.0 - 45.0 )
        a += 360.0;
    if ( a >= 270.0 - 45.0 ) {
        pix = trp_pix_rotate_orthogonal( pix, 90 );
        a -= 270.0;
        tofree = 1;
    } else if ( a >= 180.0 - 45.0 ) {
        pix = trp_pix_rotate_orthogonal( pix, 180 );
        a -= 180.0;
        tofree = 1;
    } else if ( a >= 90.0 - 45.0 ) {
        pix = trp_pix_rotate_orthogonal( pix, 270 );
        a -= 90.0;
        tofree = 1;
    }
    a *= (TRP_PI/180.0);
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    /*
    si = fabs( sin( a ) );
    co = fabs( cos( a ) );
    ww = (double)w * co + (double)h * si + 0.5;
    hh = (double)h * co + (double)w * si + 0.5;
    */
    if ( ( pixs1 = pixCreateNoInit( w, h, 32 ) ) == NULL ) {
        if ( tofree )
            (void)trp_pix_close( (trp_pix_t *)pix );
        return UNDEF;
    }
    if ( ( pixs2 = pixCreateNoInit( w, h, 32 ) ) == NULL ) {
        pixDestroy( &pixs1 );
        if ( tofree )
            (void)trp_pix_close( (trp_pix_t *)pix );
        return UNDEF;
    }
    d1 = (trp_pix_color_t *)( pixs1->data );
    d2 = (trp_pix_color_t *)( pixs2->data );
    for ( i = w * h ; ; ) {
        i--;
        d1[ i ].red = ~( c[ i ].alpha );
        d1[ i ].green = c[ i ].blue;
        d1[ i ].blue = c[ i ].green;
        d1[ i ].alpha = c[ i ].red;
        d2[ i ].red = d2[ i ].green = d2[ i ].blue = d2[ i ].alpha = ~( c[ i ].alpha );
        if ( i == 0 )
            break;
    }
    if ( tofree )
        (void)trp_pix_close( (trp_pix_t *)pix );
    pixd = pixRotate( pixs1, a, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h );
    pixDestroy( &pixs1 );
    if ( pixd == NULL ) {
        pixDestroy( &pixs2 );
        return UNDEF;
    }
    pix = trp_lept_PIX2pix( pixd );
    pixDestroy( &pixd );
    if ( pix == UNDEF ) {
        pixDestroy( &pixs2 );
        return UNDEF;
    }
    pixd = pixRotate( pixs2, a, L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, w, h );
    pixDestroy( &pixs2 );
    if ( pixd == NULL ) {
        (void)trp_pix_close( (trp_pix_t *)pix );
        return UNDEF;
    }
    c = ((trp_pix_t *)pix)->map.c;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    d2 = (trp_pix_color_t *)( pixd->data );
    for ( i = w * h ; ; ) {
        i--;
        c[ i ].alpha = ~( d2[ i ].alpha );
        if ( i == 0 )
            break;
    }
    pixDestroy( &pixd );
    /*
    if ( ( w > ww ) || ( h > hh ) ) {
        printf( "### w = %u, h= %u, ww = %u, hh = %u\n", w, h, ww, hh );
        trp_obj_t *pix2 = trp_pix_crop_low( pix, (double)( ( 1 + w - ww ) >> 1 ), (double)( ( 1 + h - hh ) >> 1 ), (double)ww, (double)hh );
        (void)trp_pix_close( (trp_pix_t *)pix );
        pix = pix2;
    }
    */
    return pix;
}

trp_obj_t *trp_lept_pix_find_skew( trp_obj_t *pix )
{
    PIX *pixs = trp_lept_pix2PIX_1bpp( pix );
    l_float32 pangle, pconf;

    if ( pixs == NULL )
        return UNDEF;
    if ( pixFindSkew( pixs, &pangle, &pconf ) ) {
        pixDestroy( &pixs );
        return UNDEF;
    }
    pixDestroy( &pixs );
    return trp_cons( trp_double( ((double)pangle) ), trp_double( ((double)pconf) ) );
}

uns8b trp_lept_convert_files_to_pdf( trp_obj_t *dirname, trp_obj_t *substr, trp_obj_t *res, trp_obj_t *scalefactor, trp_obj_t *type, trp_obj_t *quality, trp_obj_t *title, trp_obj_t *fileout )
{
    l_int32 i;
    uns32b rres, ttype, qquality;
    double sscalefactor;
    uns8b *ddirname, *ssubstr, *ttitle, *ffileout;

    if ( res == UNDEF )
        rres = 0;
    else
        if ( trp_cast_uns32b_range( res, &rres, 0, 1200 ) )
            return 1;
    if ( scalefactor == UNDEF )
        sscalefactor = 0.0;
    else
        if ( trp_cast_double_range( scalefactor, &sscalefactor, 0.0, 100.0 ) )
            return 1;
    if ( type == UNDEF )
        ttype = 0;
    else
        if ( trp_cast_uns32b_range( type, &ttype, 0, 3 ) )
            return 1;
    if ( quality == UNDEF )
        qquality = 0;
    else
        if ( trp_cast_uns32b_range( quality, &qquality, 0, 100 ) )
            return 1;
    ddirname = trp_csprint( dirname );
    if ( substr == UNDEF )
        ssubstr = NULL;
    else
        ssubstr = trp_csprint( substr );
    if ( title == UNDEF )
        ttitle = NULL;
    else
        ttitle = trp_csprint( title );
    ffileout = trp_csprint( fileout );
    i = convertFilesToPdf( ddirname, ssubstr, rres, sscalefactor, ttype, qquality, ttitle, ffileout );
    trp_csprint_free( ddirname );
    if ( ssubstr )
        trp_csprint_free( ssubstr );
    if ( ttitle )
        trp_csprint_free( ttitle );
    trp_csprint_free( ffileout );
    return i ? 1 : 0;
}

uns8b trp_lept_pix_dither_to_binary( trp_obj_t *pix )
{
    PIX *pixs = trp_lept_pix2PIX_8bpp( pix ), *pixd;

    if ( pixs == NULL )
        return 1;
    pixd = pixDitherToBinary( pixs );
    pixDestroy( &pixs );
    if ( pixd == NULL )
        return 1;
    trp_lept_copy_PIX2pix( pixd, pix );
    pixDestroy( &pixd );
    return 0;
}

uns8b trp_lept_pix_unsharp_masking( trp_obj_t *pix, trp_obj_t *halfwidth, trp_obj_t *fract )
{
    uns32b hhalfwidth;
    double ffract;
    PIX *pixs, *pixd;

    if ( trp_cast_uns32b_range( halfwidth, &hhalfwidth, 0, 1000 ) ||
         trp_cast_double_range( fract, &ffract, 0.0, 10.0 ) )
        return 1;
    if ( ( pixs = trp_lept_pix2PIX( pix ) ) == NULL )
        return 1;
    pixd = pixUnsharpMasking( pixs, hhalfwidth, ffract );
    pixDestroy( &pixs );
    if ( pixd == NULL )
        return 1;
    trp_lept_copy_PIX2pix( pixd, pix );
    pixDestroy( &pixd );
    return 0;
}

uns8b trp_lept_pix_close_gray( trp_obj_t *pix, trp_obj_t *hsize, trp_obj_t *vsize )
{
    uns32b hhsize, vvsize;
    PIX *pixs, *pixd;

    if ( trp_cast_uns32b( hsize, &hhsize ) )
        return 1;
    if ( ( hhsize & 1 ) == 0 )
        return 1;
    if ( vsize ) {
        if ( trp_cast_uns32b( vsize, &vvsize ) )
            return 1;
        if ( ( vvsize & 1 ) == 0 )
            return 1;
    } else
        vvsize = hhsize;
    if ( ( pixs = trp_lept_pix2PIX_8bpp( pix ) ) == NULL )
        return 1;
    pixd = pixCloseGray( pixs, hhsize, vvsize );
    pixDestroy( &pixs );
    if ( pixd == NULL )
        return 1;
    trp_lept_copy_PIX2pix( pixd, pix );
    pixDestroy( &pixd );
    return 0;
}

uns8b trp_lept_pix_blockconv( trp_obj_t *pix, trp_obj_t *wc, trp_obj_t *hc )
{
    uns32b wwc, hhc;
    PIX *pixs, *pixd;

    if ( trp_cast_uns32b( wc, &wwc ) )
        return 1;
    if ( hc ) {
        if ( trp_cast_uns32b( hc, &hhc ) )
            return 1;
    } else
        hhc = wwc;
    if ( ( pixs = trp_lept_pix2PIX( pix ) ) == NULL )
        return 1;
    pixd = pixBlockconv( pixs, wwc, hhc );
    pixDestroy( &pixs );
    if ( pixd == NULL )
        return 1;
    trp_lept_copy_PIX2pix( pixd, pix );
    pixDestroy( &pixd );
    return 0;
}

uns8b trp_lept_pix_subtract( trp_obj_t *pixd, trp_obj_t *pixs )
{
    PIX *ppixd, *ppixs;

    if ( ( ppixd = trp_lept_pix2PIX( pixd ) ) == NULL )
        return 1;
    if ( ( ppixs = trp_lept_pix2PIX( pixs ) ) == NULL ) {
        pixDestroy( &ppixd );
        return 1;
    }
    (void)pixSubtract( ppixd, ppixd, ppixs );
    pixDestroy( &ppixs );
    trp_lept_copy_PIX2pix( ppixd, pixd );
    pixDestroy( &ppixd );
    return 0;
}

uns8b trp_lept_pix_abs_difference( trp_obj_t *pixd, trp_obj_t *pixs )
{
    PIX *ppixd, *ppixs, *p;

    if ( ( ppixd = trp_lept_pix2PIX( pixd ) ) == NULL )
        return 1;
    if ( ( ppixs = trp_lept_pix2PIX( pixs ) ) == NULL ) {
        pixDestroy( &ppixd );
        return 1;
    }
    p = pixAbsDifference( ppixd, ppixs );
    pixDestroy( &ppixd );
    pixDestroy( &ppixs );
    if ( p == NULL )
        return 1;
    trp_lept_copy_PIX2pix( p, pixd );
    pixDestroy( &p );
    return 0;
}

uns8b trp_lept_pix_blend( trp_obj_t *dst, trp_obj_t *x, trp_obj_t *y, trp_obj_t *src, trp_obj_t *fract, trp_obj_t *trans_color )
{

    sig64b xx, yy;
    uns32b ttransparent;
    double ffract;
    trp_pix_color_t ttranspix;
    PIX *pixd, *pixs;

    if ( trp_cast_sig64b_rint_range( x, &xx, -1000000, 1000000 ) ||
         trp_cast_sig64b_rint_range( y, &yy, -1000000, 1000000 ) ||
         trp_cast_double_range( fract, &ffract, 0.0, 1.0 ) )
        return 1;
    if ( trans_color ) {
        uns8b r, g, b, a;

        if ( trp_pix_decode_color_uns8b( trans_color, NULL, &r, &g, &b, &a ) )
            return 1;
        ttransparent = 1;
        ttranspix.red = ~a;
        ttranspix.green = b;
        ttranspix.blue = g;
        ttranspix.alpha = r;
    } else
        ttransparent = ttranspix.rgba = 0;
    if ( ( pixd = trp_lept_pix2PIX( dst ) ) == NULL )
        return 1;
    if ( ( pixs = trp_lept_pix2PIX( src ) ) == NULL ) {
        pixDestroy( &pixd );
        return 1;
    }
    (void)pixBlendColor( pixd, pixd, pixs, (l_int32)xx, (l_int32)yy, (l_float32)ffract, ttransparent, ttranspix.rgba );
    pixDestroy( &pixs );
    trp_lept_copy_PIX2pix( pixd, dst );
    pixDestroy( &pixd );
    return 0;
}

uns8b trp_lept_pix_write_ttf_text( trp_obj_t *pix, trp_obj_t *size, trp_obj_t *angle, trp_obj_t *x, trp_obj_t *y, trp_obj_t *letter_space, trp_obj_t *color, trp_obj_t *fontfile, trp_obj_t *text, trp_obj_t *brect )
{
    /*
     FIXME
     */
    return 1;
}

trp_obj_t *trp_lept_pix_count_conn_comp( trp_obj_t *pix, trp_obj_t *connectivity )
{
    uns32b c;
    l_int32 i, res;
    PIX *pixs;

    if ( trp_cast_uns32b_range( connectivity, &c, 4, 8 ) )
        return UNDEF;
    if ( ( c != 4 ) && ( c != 8 ) )
        return UNDEF;
    if ( ( pixs = trp_lept_pix2PIX_1bpp( pix ) ) == NULL )
        return UNDEF;
    i = pixCountConnComp( pixs, c, &res );
    pixDestroy( &pixs );
    if ( i )
        return UNDEF;
    return trp_sig64( res );
}

