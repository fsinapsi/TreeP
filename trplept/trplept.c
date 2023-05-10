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
#include "./trplept.h"
#include "../trppix/trppix_internal.h"

static PIX *trp_lept_pix2PIX( trp_obj_t *pix );
static PIX *trp_lept_pix2PIX_8bpp( trp_obj_t *pix );
static PIX *trp_lept_pix2PIX_1bpp( trp_obj_t *pix );
static trp_obj_t *trp_lept_PIX2pix( PIX *pixs );
static uns8b trp_lept_copy_PIX2pix( PIX *pixs, trp_obj_t *pix );
static uns8b trp_lept_copy_PIX2pix_low( PIX *pixs, uns32b w, uns32b h, uns8b *data );
static uns8b trp_pix_load_lept_low( PIX *pixs, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_lept( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data );
static uns8b trp_pix_load_lept_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data );

#define TRP_PI 3.1415926535897932384626433832795029L

uns8b trp_lept_init()
{
    extern uns8bfun_t _trp_pix_load_lept;
    extern uns8bfun_t _trp_pix_load_lept_memory;

    _trp_pix_load_lept = trp_pix_load_lept;
    _trp_pix_load_lept_memory = trp_pix_load_lept_memory;
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
    d = (trp_pix_color_t *)( pixGetData( pixs ) );
    for ( i = w * h ; ; ) {
        i--;
        d[ i ].red = c[ i ].alpha;
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
        d = pixGetData( pixs ) + pixGetWpl( pixs ) * i;
        for ( j = 0, buf = 0, cnt = 32 ; j < w ; j++, c++ ) {
            if ( cnt == 0 ) {
                *d++ = buf;
                buf = 0;
                cnt = 32;
            }
            cnt -= 8;
            buf |= ( ( ( (uns32b)( c->red ) * TRP_PIX_WEIGHT_RED +
                         (uns32b)( c->green ) * TRP_PIX_WEIGHT_GREEN +
                         (uns32b)( c->blue ) * TRP_PIX_WEIGHT_BLUE + 500 ) / 1000 ) << cnt );
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
        d = pixGetData( pixs ) + pixGetWpl( pixs ) * i;
        for ( j = 0, buf = cnt = 0 ; j < w ; j++, c++, ++cnt ) {
            if ( cnt == 32 ) {
                *d++ = buf;
                buf = cnt = 0;
            }
            if ( ( ( (uns32b)( c->red ) * TRP_PIX_WEIGHT_RED +
                     (uns32b)( c->green ) * TRP_PIX_WEIGHT_GREEN +
                     (uns32b)( c->blue ) * TRP_PIX_WEIGHT_BLUE + 500 ) / 1000 ) < 128 )
                buf |= ( 0x80000000 >> cnt );
        }
        *d = buf;
    }
    return pixs;
}

static trp_obj_t *trp_lept_PIX2pix( PIX *pixs )
{
    trp_obj_t *res = trp_pix_create_basic( pixGetWidth( pixs ), pixGetHeight( pixs ) );

    if ( res != UNDEF )
        if ( trp_lept_copy_PIX2pix( pixs, res ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            res = UNDEF;
        }
    return res;
}

static uns8b trp_lept_copy_PIX2pix( PIX *pixs, trp_obj_t *pix )
{
    return trp_lept_copy_PIX2pix_low( pixs, ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h, ((trp_pix_t *)pix)->map.p );
}

static uns8b trp_lept_copy_PIX2pix_low( PIX *pixs, uns32b w, uns32b h, uns8b *data )
{
    trp_pix_color_t *c = (trp_pix_color_t *)data, *d;
    uns32b i;
    uns8b tofree = 0, m;

    if ( pixGetDepth( pixs ) != 32 ) {
        PIX *pix32 = pixConvertTo32( pixs );
        if ( pix32 == NULL )
            return 1;
        pixs = pix32;
        tofree = 1;
    }
    d = (trp_pix_color_t *)( pixGetData( pixs ) );
    m = ( pixGetSpp( pixs ) == 4 ) ? 0 : 0xff;
    for ( i = w * h ; ; ) {
        i--;
        c[ i ].red = d[ i ].alpha;
        c[ i ].green = d[ i ].blue;
        c[ i ].blue = d[ i ].green;
        c[ i ].alpha = d[ i ].red | m;
        if ( i == 0 )
            break;
    }
    if ( tofree )
        pixDestroy( &pixs );
    return 0;
}

static uns8b trp_pix_load_lept_low( PIX *pixs, uns32b *w, uns32b *h, uns8b **data )
{
    uns32b d;

    if ( pixs == NULL )
        return 1;
    pixGetDimensions( pixs, w, h, &d );
    if ( d != 32 ) {
        PIX *pix32 = pixConvertTo32( pixs );

        if ( pix32 ) {
            pixDestroy( &pixs );
            pixs = pix32;
        }
    }
    if ( ( *data = malloc( ( *w * *h ) << 2 ) ) == NULL ) {
        pixDestroy( &pixs );
        return 1;
    }
    if ( trp_lept_copy_PIX2pix_low( pixs, *w, *h, *data ) ) {
        pixDestroy( &pixs );
        free( *data );
        return 1;
    }
    pixDestroy( &pixs );
    return 0;
}

static uns8b trp_pix_load_lept( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_lept_low( pixRead( cpath ), w, h, data );
}

static uns8b trp_pix_load_lept_memory( uns8b *idata, uns32b isize, uns32b *w, uns32b *h, uns8b **data )
{
    return trp_pix_load_lept_low( pixReadMem( idata, isize ), w, h, data );
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

