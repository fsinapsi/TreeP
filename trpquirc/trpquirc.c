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

#include "../trp/trp.h"
#include "./trpquirc.h"
#include "../trppix/trppix_internal.h"
#include "./quirc/lib/quirc.h"
#include <qrencode.h>

trp_obj_t *trp_quirc_decode( trp_obj_t *pix )
{
    trp_obj_t *res = NIL;
    trp_pix_color_t *c;
    struct quirc *qr;
    uns8b *image;
    int w, h, i;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( c = ((trp_pix_t *)pix)->map.c ) == NULL )
        return UNDEF;
    if ( ( qr = quirc_new() ) == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    if ( quirc_resize( qr, w + 2, h + 2 ) < 0 ) {
        quirc_destroy( qr );
        return UNDEF;
    }
    image = quirc_begin( qr, NULL, NULL );
    memset( image, 0xff, ( w + 2 ) * ( h + 2 ) );
    image += ( h + 2 );
    for ( ; w ; w-- ) {
        image++;
        for ( i = 0 ; i < h ; i++, c++ )
            *image++ = ( (uns32b)( c->red ) * 299 +
                         (uns32b)( c->green ) * 587 +
                         (uns32b)( c->blue ) * 114 + 500 ) / 1000;
        image++;
    }
    quirc_end( qr );
    for ( i = 0; i < quirc_count( qr ) ; i++ ) {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract( qr, i, &code );
        if ( !quirc_decode( &code, &data ) )
            res = trp_cons( trp_cord( data.payload ), res );
    }
    quirc_destroy( qr );
    return res;
}

trp_obj_t *trp_quirc_encode( trp_obj_t *s, trp_obj_t *level )
{
    trp_pix_color_t *map;
    uns8b *c;
    QRcode *q;
    uns32b l, i;

    if ( level ) {
        if ( trp_cast_uns32b_range( level, &l, 0, 3 ) )
            return UNDEF;
    } else
        l = 0;
    c = trp_csprint( s );
    q = QRcode_encodeString( c, 0, (QRecLevel)l, QR_MODE_8, 1 );
    trp_csprint_free( c );
    if ( q == NULL )
        return UNDEF;
    l = q->width;
    c = q->data;
    if ( ( map = malloc( ( l * l ) << 2 ) ) == NULL ) {
        QRcode_free( q );
        return UNDEF;
    }
    s = trp_pix_create_image_from_data( 0, l, l, (uns8b *)map );
    for ( i = l * l ; i ; i--, map++, c++ ) {
        map->red = map->green = map->blue = ( *c & 1 ) ? 0 : 0xff;
        map->alpha = 0xff;
    }
    QRcode_free( q );
    return s;
}

