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

#include "./trppix_internal.h"
#include <gif_lib.h>

#define TRPPIX_GIF_MAX 20000 // massima risoluzione: 20000x20000

uns8b trp_pix_info_gif( uns8b *cpath, uns32b *w, uns32b *h )
{
    GifFileType *g = DGifOpenFileName( cpath, NULL );

    if ( g == NULL )
        return 1;
    if ( DGifSlurp( g ) != GIF_OK ) {
        (void)DGifCloseFile( g, NULL );
        return 1;
    }
    *w = g->SWidth;
    *h = g->SHeight;
    (void)DGifCloseFile( g, NULL );
    return 0;
}

uns8b trp_pix_load_gif( uns8b *cpath, uns32b *w, uns32b *h, uns8b **data )
{
    uns8b *p, *q;
    GifFileType *g = DGifOpenFileName( cpath, NULL );
    SavedImage *gim;
    ColorMapObject *cmap;
    GifColorType *pal;
    GraphicsControlBlock gcb;
    int transparent;
    uns32b n, w1, h1, l, t, x, y, incp;

    if ( g == NULL )
        return 1;
    if ( DGifSlurp( g ) != GIF_OK ) {
        (void)DGifCloseFile( g, NULL );
        return 1;
    }
    *w = g->SWidth;
    *h = g->SHeight;
    if ( ( g->ImageCount < 1 ) ||
         ( *w == 0 ) || ( *w > TRPPIX_GIF_MAX ) ||
         ( *h == 0 ) || ( *h > TRPPIX_GIF_MAX ) ) {
        (void)DGifCloseFile( g, NULL );
        return 1;
    }
    n = ( *w * *h ) << 2;
    gim = &( g->SavedImages[ 0 ] );
    l = (uns32b)( gim->ImageDesc.Left );
    t = (uns32b)( gim->ImageDesc.Top );
    w1 = (uns32b)( gim->ImageDesc.Width );
    h1 = (uns32b)( gim->ImageDesc.Height );
    if ( ( cmap = gim->ImageDesc.ColorMap ) == NULL )
        cmap = g->SColorMap;
    if ( ( l + w1 > *w ) || ( t + h1 > *h ) || ( cmap == NULL ) ) {
        (void)DGifCloseFile( g, NULL );
        return 1;
    }
    (void)DGifSavedExtensionToGCB( g, 0, &gcb );
    transparent = gcb.TransparentColor;
    pal = cmap->Colors;
    if ( ( *data = malloc( n ) ) == NULL ) {
        (void)DGifCloseFile( g, NULL );
        return 1;
    }
    memset( *data, 0xff, n );
    p = *data + ( ( t * *w + l ) << 2 );
    incp = ( *w - w1 ) << 2;
    for ( y = 0, q = gim->RasterBits ; y < h1 ; y++, p += incp )
        for ( x = 0 ; x < w1 ; x++, q++ ) {
            *p++ = pal[ *q ].Red;
            *p++ = pal[ *q ].Green;
            *p++ = pal[ *q ].Blue;
            *p++ = ( ( (int)( *q ) ) == transparent ) ? 0 : 0xff;
        }
    (void)DGifCloseFile( g, NULL );
    return 0;
}

trp_obj_t *trp_pix_load_gif_multiple( uns8b *cpath )
{
    trp_obj_t *res;
    uns8b *p, *q, *prev = NULL;
    GifFileType *g = DGifOpenFileName( cpath, NULL );
    SavedImage *gim;
    ColorMapObject *cmap;
    GifColorType *pal;
    GraphicsControlBlock gcb;
    int transparent, e = 0;
    uns32b n, ww, hh, w, h, l, t, x, y, incp, j;

    if ( g == NULL )
        return UNDEF;
    if ( DGifSlurp( g ) != GIF_OK ) {
        (void)DGifCloseFile( g, NULL );
        return UNDEF;
    }
    ww = g->SWidth;
    hh = g->SHeight;
    if ( ( g->ImageCount < 1 ) ||
         ( ww == 0 ) || ( ww > TRPPIX_GIF_MAX ) ||
         ( hh == 0 ) || ( hh > TRPPIX_GIF_MAX ) ) {
        (void)DGifCloseFile( g, NULL );
        return UNDEF;
    }
    n = ( ww * hh ) << 2;
    for ( res = trp_queue(), j = 0 ; j < g->ImageCount ; j++ ) {
        gim = &( g->SavedImages[ j ] );
        l = (uns32b)( gim->ImageDesc.Left );
        t = (uns32b)( gim->ImageDesc.Top );
        w = (uns32b)( gim->ImageDesc.Width );
        h = (uns32b)( gim->ImageDesc.Height );
        if ( ( cmap = gim->ImageDesc.ColorMap ) == NULL )
            cmap = g->SColorMap;
        if ( ( l + w > ww ) || ( t + h > hh ) || ( cmap == NULL ) ) {
            e = 1;
            break;
        }
        (void)DGifSavedExtensionToGCB( g, j, &gcb );
        transparent = gcb.TransparentColor;
        pal = cmap->Colors;
        if ( ( p = malloc( n ) ) == NULL ) {
            e = 1;
            break;
        }
        if ( prev )
            memcpy( p, prev, n );
        else
            memset( p, 0, n );
        prev = p;
        (void)trp_queue_put( res, trp_cons( trp_pix_create_image_from_data( 0, ww, hh, p ),
                                            trp_math_ratio( trp_double( (double)( gcb.DelayTime ) ),
                                                            trp_sig64( 100 ),
                                                            NULL ) ) );
        p += ( t * ww + l ) << 2;
        incp = ( ww - w ) << 2;
        for ( y = 0, q = gim->RasterBits ; y < h ; y++, p += incp )
            for ( x = 0 ; x < w ; x++, q++ )
                if ( ( ( (int)( *q ) ) == transparent ) ) {
                    p += 3;
                    *p++ = ( ww == w ) && ( hh == h ) ? 0 : 0xff;
                } else {
                    *p++ = pal[ *q ].Red;
                    *p++ = pal[ *q ].Green;
                    *p++ = pal[ *q ].Blue;
                    *p++ = 0xff;
                }
    }
    (void)DGifCloseFile( g, NULL );
    if ( e ) {
        trp_obj_t *o;

        while ( ((trp_queue_t *)res)->len ) {
            o = trp_queue_get( res );
            (void)trp_close( ((trp_cons_t *)o)->car );
            trp_gc_free( o );
        }
        trp_gc_free( res );
        return UNDEF;
    }
    return res;
}

uns8b trp_pix_save_gif( trp_obj_t *pix, trp_obj_t *path, trp_obj_t *transp, trp_obj_t *delay )
{
    /*
     FIXME
     */
    return 1;
}

