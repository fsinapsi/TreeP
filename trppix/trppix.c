/*
    TreeP Run Time Support
    Copyright (C) 2008-2026 Frank Sinapsi

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

uns32b trp_size_internal( trp_obj_t *obj );
void trp_encode_internal( trp_obj_t *obj, uns8b **buf );
void trp_pix_conv_init();
void trp_pix_colormod_init();

#define colorval(v) (((v)+128)/257)
static uns8b trp_pix_print( trp_print_t *p, trp_pix_t *obj );
static uns8b trp_pix_close_basic( uns8b flags, trp_pix_t *obj );
static void trp_pix_finalize( void *obj, void *data );
static uns32b trp_pix_size( trp_pix_t *obj );
static void trp_pix_encode( trp_pix_t *obj, uns8b **buf );
static trp_obj_t *trp_pix_decode( uns8b **buf );
static trp_obj_t *trp_pix_equal( trp_pix_t *pix1, trp_pix_t *pix2 );
static trp_obj_t *trp_pix_width( trp_pix_t *pix );
static trp_obj_t *trp_pix_height( trp_pix_t *pix );

uns8b trp_pix_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern uns32bfun_t _trp_size_fun[];
    extern voidfun_t _trp_encode_fun[];
    extern objfun_t _trp_decode_fun[];
    extern objfun_t _trp_equal_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_PIX ] = trp_pix_print;
    _trp_close_fun[ TRP_PIX ] = trp_pix_close;
    _trp_size_fun[ TRP_PIX ] = trp_pix_size;
    _trp_encode_fun[ TRP_PIX ] = trp_pix_encode;
    _trp_decode_fun[ TRP_PIX ] = trp_pix_decode;
    _trp_equal_fun[ TRP_PIX ] = trp_pix_equal;
    _trp_width_fun[ TRP_PIX ] = trp_pix_width;
    _trp_height_fun[ TRP_PIX ] = trp_pix_height;
    trp_pix_conv_init();
    trp_pix_colormod_init();
    return 0;
}

void trp_pix_quit()
{
}

static uns8b trp_pix_print( trp_print_t *p, trp_pix_t *obj )
{
    if ( obj->sottotipo ) {
        if ( trp_print_char_star( p, "#pix" ) )
            return 1;
        if ( obj->map.p == NULL )
            if ( trp_print_char_star( p, " (closed)" ) )
                return 1;
    } else {
        uns8b buf[ 4 ];

        if ( trp_print_char_star( p, "#color r=" ) )
            return 1;
        sprintf( buf, "%03d", (int)colorval( obj->color.red ) );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( trp_print_char_star( p, ", g=" ) )
            return 1;
        sprintf( buf, "%03d", (int)colorval( obj->color.green ) );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( trp_print_char_star( p, ", b=" ) )
            return 1;
        sprintf( buf, "%03d", (int)colorval( obj->color.blue ) );
        if ( trp_print_char_star( p, buf ) )
            return 1;
        if ( trp_print_char_star( p, ", a=" ) )
            return 1;
        sprintf( buf, "%03d", (int)colorval( obj->color.alpha ) );
        if ( trp_print_char_star( p, buf ) )
            return 1;
    }
    return trp_print_char( p, '#' );
}

uns8b trp_pix_close( trp_pix_t *obj )
{
    return trp_pix_close_basic( 1, obj );
}

static uns8b trp_pix_close_basic( uns8b flags, trp_pix_t *obj )
{
    if ( obj->map.p ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( (void *)( obj->map.p ) );
        obj->map.p = NULL;
    }
    return 0;
}

static void trp_pix_finalize( void *obj, void *data )
{
    trp_pix_close_basic( 0, (trp_pix_t *)obj );
}

static uns32b trp_pix_size( trp_pix_t *obj )
{
    uns32b sz;

    if ( obj->sottotipo == 0 )
        sz = 1 + 1 + 8;
    else if ( obj->map.p )
        sz = ( ( obj->w * obj->h ) << 2 ) + 1 + 1 + 4 + 4;
    else
        sz = trp_size_internal( UNDEF );
    return sz;
}

static void trp_pix_encode( trp_pix_t *obj, uns8b **buf )
{
    if ( obj->sottotipo == 0 ) {
        uns16b *p;

        **buf = TRP_PIX;
        ++(*buf);
        **buf = 1;
        ++(*buf);
        p = (uns16b *)(*buf);
        *p = norm16( obj->color.alpha );
        (*buf) += 2;
        p = (uns16b *)(*buf);
        *p = norm16( obj->color.red );
        (*buf) += 2;
        p = (uns16b *)(*buf);
        *p = norm16( obj->color.green );
        (*buf) += 2;
        p = (uns16b *)(*buf);
        *p = norm16( obj->color.blue );
        (*buf) += 2;
    } else if ( obj->map.p ) {
        uns32b *p;

        **buf = TRP_PIX;
        ++(*buf);
        **buf = 0;
        ++(*buf);
        p = (uns32b *)(*buf);
        *p = norm32( obj->w );
        (*buf) += 4;
        p = (uns32b *)(*buf);
        *p = norm32( obj->h );
        (*buf) += 4;
        (void)memcpy( *buf, obj->map.p, ( obj->w * obj->h ) << 2 );
        (*buf) += ( ( obj->w * obj->h ) << 2 );
    } else
        trp_encode_internal( UNDEF, buf );
}

static trp_obj_t *trp_pix_decode( uns8b **buf )
{
    trp_obj_t *res;
    uns8b is_color;

    is_color = **buf;
    ++(*buf);
    if ( is_color ) {
        uns16b alpha, red, green, blue;

        alpha = norm16( *((uns16b *)(*buf)) );
        (*buf) += 2;
        red = norm16( *((uns16b *)(*buf)) );
        (*buf) += 2;
        green = norm16( *((uns16b *)(*buf)) );
        (*buf) += 2;
        blue = norm16( *((uns16b *)(*buf)) );
        (*buf) += 2;
        res = trp_pix_create_color( red, green, blue, alpha );
    } else {
        uns32b w, h;

        w = norm32( *((uns32b *)(*buf)) );
        (*buf) += 4;
        h = norm32( *((uns32b *)(*buf)) );
        (*buf) += 4;
        res = trp_pix_create_image_from_data( 1, w, h, *buf );
        (*buf) += ( ( w * h ) << 2 );
    }
    return res;
}

static trp_obj_t *trp_pix_equal( trp_pix_t *pix1, trp_pix_t *pix2 )
{
    if ( pix1->map.p && pix2->map.p ) {
        if ( ( pix1->w != pix2->w ) || ( pix1->h != pix2->h ) )
            return TRP_FALSE;
        if ( pix1->map.p == pix2->map.p )
            return TRP_TRUE;
        return ( memcmp( pix1->map.p, pix2->map.p, ( pix1->w * pix1->h ) << 2 ) == 0 ) ? TRP_TRUE : TRP_FALSE;
    }
    if ( ( pix1->sottotipo == 0 ) && ( pix2->sottotipo == 0 ) )
        return ( ( pix1->color.red == pix2->color.red ) &&
                 ( pix1->color.green == pix2->color.green ) &&
                 ( pix1->color.blue == pix2->color.blue ) &&
                 ( pix1->color.alpha == pix2->color.alpha ) )
            ? TRP_TRUE : TRP_FALSE;
    if ( ( pix1->sottotipo == 0 ) || ( pix2->sottotipo == 0 ) )
        return TRP_FALSE;
    return ( pix1->map.p == pix2->map.p ) ? TRP_TRUE : TRP_FALSE;
}

static trp_obj_t *trp_pix_width( trp_pix_t *pix )
{
    return pix->map.p ? trp_sig64( pix->w ) : UNDEF;
}

static trp_obj_t *trp_pix_height( trp_pix_t *pix )
{
    return pix->map.p ? trp_sig64( pix->h ) : UNDEF;
}

trp_obj_t *trp_pix_create_image_from_data( int must_copy, uns32b w, uns32b h, uns8b *data )
{
    trp_pix_t *pix;
    uns8b *map;

    if ( must_copy ) {
        uns32b n = ( w * h ) << 2;

        if ( ( map = malloc( n ) ) == NULL )
            return UNDEF;
        memcpy( map, data, n );
    } else {
        map = data;
    }
    pix = trp_gc_malloc_atomic_finalize( sizeof( trp_pix_t ), trp_pix_finalize );
    pix->tipo = TRP_PIX;
    pix->sottotipo = 1;
    pix->w = w;
    pix->h = h;
    pix->map.p = map;
    pix->color.red = 0xff;
    pix->color.green = 0xff;
    pix->color.blue = 0xff;
    pix->color.alpha = 0xff;
    return (trp_obj_t *)pix;
}

trp_obj_t *trp_pix_create_color( uns16b red, uns16b green, uns16b blue, uns16b alpha )
{
    trp_pix_t *pix = trp_gc_malloc_atomic( sizeof( trp_pix_t ) );

    pix->tipo = TRP_PIX;
    pix->sottotipo = 0;
    pix->w = 0;
    pix->h = 0;
    pix->map.p = NULL;
    pix->color.red = red;
    pix->color.green = green;
    pix->color.blue = blue;
    pix->color.alpha = alpha;
    return (trp_obj_t *)pix;
}

uns8b trp_pix_decode_color( trp_obj_t *obj, uns16b *red, uns16b *green, uns16b *blue, uns16b *alpha )
{
    if ( obj->tipo != TRP_PIX )
        return 1;
    if ( ((trp_pix_t *)obj)->sottotipo )
        return 1;
    *red = ((trp_pix_t *)obj)->color.red;
    *green = ((trp_pix_t *)obj)->color.green;
    *blue = ((trp_pix_t *)obj)->color.blue;
    *alpha = ((trp_pix_t *)obj)->color.alpha;
    return 0;
}

uns8b trp_pix_decode_color_uns8b( trp_obj_t *obj, trp_obj_t *pix, uns8b *red, uns8b *green, uns8b *blue, uns8b *alpha )
{
    if ( pix ) {
        if ( pix->tipo != TRP_PIX )
            return 1;
        if ( ((trp_pix_t *)pix)->map.p == NULL )
            return 1;
    }
    if ( obj ) {
        uns16b r, g, b, a;

        if ( trp_pix_decode_color( obj, &r, &g, &b, &a ) )
            return 1;
        *red = colorval( r );
        *green = colorval( g );
        *blue = colorval( b );
        *alpha = colorval( a );
    } else {
        if ( pix == NULL )
            return 1;
        *red = colorval( ((trp_pix_t *)pix)->color.red );
        *green = colorval( ((trp_pix_t *)pix)->color.green );
        *blue = colorval( ((trp_pix_t *)pix)->color.blue );
        *alpha = colorval( ((trp_pix_t *)pix)->color.alpha );
    }
    return 0;
}

trp_obj_t *trp_pix_color( trp_obj_t *red, trp_obj_t *green, trp_obj_t *blue, trp_obj_t *alpha )
{
    uns32b r, g, b, a;

    if ( trp_cast_uns32b_range( red, &r, 0, 0xff ) ||
         trp_cast_uns32b_range( green, &g, 0, 0xff ) ||
         trp_cast_uns32b_range( blue, &b, 0, 0xff ) )
        return UNDEF;
    if ( alpha ) {
        if ( trp_cast_uns32b_range( alpha, &a, 0, 0xff ) )
            return UNDEF;
    } else
        a = 0xff;
    return trp_pix_create_color( 257 * (uns16b)r, 257 * (uns16b)g,  257 * (uns16b)b, 257 * (uns16b)a );
}

trp_obj_t *trp_pix_color_red( trp_obj_t *obj )
{
    uns16b r, g, b, a;

    if ( trp_pix_decode_color( obj, &r, &g, &b, &a ) )
        return UNDEF;
    return trp_sig64( colorval( r ) );
}

trp_obj_t *trp_pix_color_green( trp_obj_t *obj )
{
    uns16b r, g, b, a;

    if ( trp_pix_decode_color( obj, &r, &g, &b, &a ) )
        return UNDEF;
    return trp_sig64( colorval( g ) );
}

trp_obj_t *trp_pix_color_blue( trp_obj_t *obj )
{
    uns16b r, g, b, a;

    if ( trp_pix_decode_color( obj, &r, &g, &b, &a ) )
        return UNDEF;
    return trp_sig64( colorval( b ) );
}

trp_obj_t *trp_pix_color_alpha( trp_obj_t *obj )
{
    uns16b r, g, b, a;

    if ( trp_pix_decode_color( obj, &r, &g, &b, &a ) )
        return UNDEF;
    return trp_sig64( colorval( a ) );
}

trp_obj_t *trp_pix_create_basic( uns32b w, uns32b h )
{
    uns32b n;
    uns8b *map;

    n = ( w * h ) << 2;
    if ( n == 0 )
        return UNDEF;
    if ( ( map = malloc( n ) ) == NULL )
        return UNDEF;
    memset( map, 0xff, n );
    return trp_pix_create_image_from_data( 0, w, h, map );
}

trp_obj_t *trp_pix_create( trp_obj_t *w, trp_obj_t *h )
{
    uns32b ww, hh;

    if ( trp_cast_uns32b_rint_range( w, &ww, 1, 0xffff ) ||
         trp_cast_uns32b_rint_range( h ? h : w, &hh, 1, 0xffff ) )
        return UNDEF;
    return trp_pix_create_basic( ww, hh );
}

uns8b trp_pix_set_color( trp_obj_t *pix, trp_obj_t *color )
{
    uns8b *p;
    uns16b r, g, b, a;

    if ( trp_pix_is_not_valid( pix ) )
        return 1;
    if ( trp_pix_decode_color( color, &r, &g, &b, &a ) )
        return 1;
    ((trp_pix_t *)pix)->color.red = r;
    ((trp_pix_t *)pix)->color.green = g;
    ((trp_pix_t *)pix)->color.blue = b;
    ((trp_pix_t *)pix)->color.alpha = a;
    return 0;
}

trp_obj_t *trp_pix_get_color( trp_obj_t *pix )
{
    if ( trp_pix_is_not_valid( pix ) )
        return UNDEF;
    return trp_pix_create_color( ((trp_pix_t *)pix)->color.red,
                                 ((trp_pix_t *)pix)->color.green,
                                 ((trp_pix_t *)pix)->color.blue,
                                 ((trp_pix_t *)pix)->color.alpha );
}

