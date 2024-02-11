/*
    TreeP Run Time Support
    Copyright (C) 2008-2024 Frank Sinapsi

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
#include "./trpcairo.h"
#include "../trppix/trppix_internal.h"
#include <librsvg/rsvg.h>

typedef struct {
    cairo_font_face_t *ft_font_face;
    FT_Face ft_face;
    uns8b *ft_aux_data;
} trp_cairo_destroy_ft_t;

typedef struct {
    uns8b tipo;
    cairo_surface_t *surface;
    cairo_t *cr;
    flt64b w;
    flt64b h;
    trp_cairo_destroy_ft_t *d;
    uns8b *data;
    uns32b size;
    uns32b pos;
} trp_cairo_t;

static FT_Library _ft_library;
static pthread_mutex_t _trp_cairo_mut = PTHREAD_MUTEX_INITIALIZER;
#define TRP_CAIRO_LOCK pthread_mutex_lock( &_trp_cairo_mut )
#define TRP_CAIRO_UNLOCK pthread_mutex_unlock( &_trp_cairo_mut )
#define colorval(v) (((v)+128)/257)
static uns8b trp_cairo_print( trp_print_t *p, trp_cairo_t *obj );
static void trp_cairo_destroy_ft( trp_cairo_t *c );
static void trp_cairo_destroy( trp_cairo_t *c );
static uns8b trp_cairo_close( trp_cairo_t *obj );
static uns8b trp_cairo_close_basic( uns8b flags, trp_cairo_t *obj );
static void trp_cairo_finalize( void *obj, void *data );
static trp_obj_t *trp_cairo_width( trp_cairo_t *c );
static trp_obj_t *trp_cairo_height( trp_cairo_t *c );
static trp_cairo_t *trp_cairo_get( trp_obj_t *obj );
static cairo_status_t trp_cairo_cback( trp_cairo_t *obj, const unsigned char *data, unsigned int length );
static trp_obj_t *trp_cairo_flush_and_close_low( uns8b type, trp_obj_t *obj );
static uns8b trp_cairo_op0( voidfun_t op, trp_obj_t *obj );
static uns8b trp_cairo_op1( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1 );
static uns8b trp_cairo_op2( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2 );
static uns8b trp_cairo_op3( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3 );
static uns8b trp_cairo_op4( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4 );
static uns8b trp_cairo_op5( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4, trp_obj_t *x5 );
static uns8b trp_cairo_op6( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4, trp_obj_t *x5, trp_obj_t *x6 );
static trp_obj_t *trp_cairo_ucs4_to_utf8( uns32b unicode );
static trp_obj_t *trp_cairo_get_ft_name_low( uns8b which, trp_obj_t *obj );

uns8b trp_cairo_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_CAIRO ] = trp_cairo_print;
    _trp_close_fun[ TRP_CAIRO ] = trp_cairo_close;
    _trp_width_fun[ TRP_CAIRO ] = trp_cairo_width;
    _trp_height_fun[ TRP_CAIRO ] = trp_cairo_height;
    if ( FT_Init_FreeType( &_ft_library ) )
        return 1;
    return 0;
}

void trp_cairo_quit()
{
    FT_Done_FreeType( _ft_library );
}

static uns8b trp_cairo_print( trp_print_t *p, trp_cairo_t *obj )
{
    if ( trp_print_char_star( p, "#cairo " ) )
        return 1;
    if ( trp_print_obj( p, trp_double( obj->w ) ) )
        return 1;
    if ( trp_print_char_star( p, " x " ) )
        return 1;
    if ( trp_print_obj( p, trp_double( obj->h ) ) )
        return 1;
    if ( obj->surface == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static void trp_cairo_destroy_ft( trp_cairo_t *c )
{
    if ( c->d ) {
        cairo_font_face_destroy( c->d->ft_font_face );
        c->d = NULL;
    }
}

static void trp_cairo_destroy( trp_cairo_t *c )
{
    cairo_destroy( c->cr );
    cairo_surface_destroy( c->surface );
    c->surface = NULL;
    c->cr = NULL;
    trp_cairo_destroy_ft( c );
}

static uns8b trp_cairo_close( trp_cairo_t *obj )
{
    return trp_cairo_close_basic( 1, obj );
}

static uns8b trp_cairo_close_basic( uns8b flags, trp_cairo_t *obj )
{
    if ( obj->surface ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        free( obj->data );
        obj->data = NULL;
        trp_cairo_destroy( obj );
    }
    return 0;
}

static void trp_cairo_finalize( void *obj, void *data )
{
    trp_cairo_close_basic( 0, (trp_cairo_t *)obj );
}

static trp_obj_t *trp_cairo_width( trp_cairo_t *c )
{
    return trp_double( c->w );
}

static trp_obj_t *trp_cairo_height( trp_cairo_t *c )
{
    return trp_double( c->h );
}

static trp_cairo_t *trp_cairo_get( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_CAIRO )
        return NULL;
    if ( ((trp_cairo_t *)obj)->surface == NULL )
        return NULL;
    return (trp_cairo_t *)obj;
}

static cairo_status_t trp_cairo_cback( trp_cairo_t *obj, const unsigned char *data, unsigned int length )
{
    if ( obj->pos + length > obj->size ) {
        uns8b *new_data;
        uns32b new_size;

        new_size = ( obj->size << 1 );
        if ( obj->pos + length > new_size )
            new_size = obj->pos + length;
        if ( ( new_data = realloc( obj->data, new_size ) ) == NULL )
            return CAIRO_STATUS_WRITE_ERROR;
        obj->data = new_data;
        obj->size = new_size;
    }
    if ( length ) {
        memcpy( obj->data + obj->pos, data, length );
        obj->pos += length;
    }
    return CAIRO_STATUS_SUCCESS;
}

trp_obj_t *trp_cairo_svg_surface_create_for_stream( trp_obj_t *w, trp_obj_t *h )
{
    trp_cairo_t *res;
    flt64b ww, hh;

    if ( trp_cast_flt64b( w, &ww ) || trp_cast_flt64b( h, &hh ) )
        return UNDEF;
    if ( ( ww <= 0.0 ) || ( hh <= 0.0 ) )
        return UNDEF;
    res = trp_gc_malloc_atomic_finalize( sizeof( trp_cairo_t ), trp_cairo_finalize );
    res->tipo = TRP_CAIRO;
    res->w = ww;
    res->h = hh;
    res->d = NULL;
    res->data = NULL;
    res->size = 0;
    res->pos = 0;
    res->surface = cairo_svg_surface_create_for_stream( (cairo_write_func_t)trp_cairo_cback, res, ww, hh );
    if ( cairo_surface_status( res->surface ) != CAIRO_STATUS_SUCCESS ) {
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        return UNDEF;
    }
    res->cr = cairo_create( res->surface );
    if ( cairo_status( res->cr ) != CAIRO_STATUS_SUCCESS ) {
        cairo_destroy( res->cr );
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        return UNDEF;
    }
    return (trp_obj_t *)res;
}

trp_obj_t *trp_cairo_svg_surface_create_from_svg( trp_obj_t *src )
{
    trp_cairo_t *res;
    RsvgHandle *handle;
    gdouble ww, hh;
    RsvgRectangle viewport;

    switch ( src->tipo ) {
        case TRP_CORD:
            {
                uns8b *cpath = trp_csprint( src );
                handle = rsvg_handle_new_from_file( cpath, NULL );
                trp_csprint_free( cpath );
            }
            break;
        case TRP_RAW:
            handle = rsvg_handle_new_from_data( ((trp_raw_t *)src)->data, ((trp_raw_t *)src)->len, NULL );
            break;
        default:
            return UNDEF;
    }
    if ( handle == NULL )
        return UNDEF;
    if ( !rsvg_handle_get_intrinsic_size_in_pixels( handle, &ww, &hh ) ) {
        ww = 720.0;
        hh = 460.0;
    }
    res = trp_gc_malloc_atomic_finalize( sizeof( trp_cairo_t ), trp_cairo_finalize );
    res->tipo = TRP_CAIRO;
    res->w = ww;
    res->h = hh;
    res->d = NULL;
    res->data = NULL;
    res->size = 0;
    res->pos = 0;
    res->surface = cairo_svg_surface_create_for_stream( (cairo_write_func_t)trp_cairo_cback, res, ww, hh );
    if ( cairo_surface_status( res->surface ) != CAIRO_STATUS_SUCCESS ) {
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        g_object_unref( handle );
        return UNDEF;
    }
    res->cr = cairo_create( res->surface );
    if ( cairo_status( res->cr ) != CAIRO_STATUS_SUCCESS ) {
        cairo_destroy( res->cr );
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        g_object_unref( handle );
        return UNDEF;
    }
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = ww;
    viewport.height = hh;
    if ( !rsvg_handle_render_document( handle, res->cr, &viewport, NULL ) ) {
        free( res->data );
        cairo_destroy( res->cr );
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        g_object_unref( handle );
        return UNDEF;
    }
    cairo_surface_flush( res->surface );
    g_object_unref( handle );
    return (trp_obj_t *)res;
}

trp_obj_t *trp_cairo_pdf_surface_create( trp_obj_t *path, trp_obj_t *w, trp_obj_t *h )
{
    trp_cairo_t *res;
    uns8b *cpath;
    flt64b ww, hh;

    if ( trp_cast_flt64b( w, &ww ) || trp_cast_flt64b( h, &hh ) )
        return UNDEF;
    if ( ( ww <= 0.0 ) || ( hh <= 0.0 ) )
        return UNDEF;
    res = trp_gc_malloc_atomic_finalize( sizeof( trp_cairo_t ), trp_cairo_finalize );
    res->tipo = TRP_CAIRO;
    res->w = ww;
    res->h = hh;
    res->d = NULL;
    res->data = NULL;
    res->size = 0;
    res->pos = 0;
    cpath = trp_csprint( path );
    res->surface = cairo_pdf_surface_create( cpath, ww, hh );
    trp_csprint_free( cpath );
    if ( cairo_surface_status( res->surface ) != CAIRO_STATUS_SUCCESS ) {
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        return UNDEF;
    }
    res->cr = cairo_create( res->surface );
    if ( cairo_status( res->cr ) != CAIRO_STATUS_SUCCESS ) {
        cairo_destroy( res->cr );
        cairo_surface_destroy( res->surface );
        trp_gc_remove_finalizer( (trp_obj_t *)res );
        trp_gc_free( res );
        return UNDEF;
    }
    return (trp_obj_t *)res;
}

static trp_obj_t *trp_cairo_flush_and_close_low( uns8b type, trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );

    if ( c == NULL )
        return UNDEF;
    trp_gc_remove_finalizer( obj );
    cairo_surface_flush( c->surface );
    trp_cairo_destroy( c );
    switch ( type ) {
        case 0:
            {
                extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );

                obj = trp_raw_internal( c->pos, 0 );
                memcpy( ((trp_raw_t *)obj)->data, c->data, c->pos );
            }
            break;
        case 1:
            if ( c->pos == 0 )
                return trp_cord_empty();
            {
                uns8b z = 0;

                if ( trp_cairo_cback( c, &z, 1 ) != CAIRO_STATUS_SUCCESS ) {
                    free( c->data );
                    c->data = NULL;
                    return UNDEF;
                }
            }
            obj = trp_cord( c->data );
            break;
    }
    free( c->data );
    c->data = NULL;
    return obj;
}

trp_obj_t *trp_cairo_flush_and_close_raw( trp_obj_t *obj )
{
    return trp_cairo_flush_and_close_low( 0, obj );
}

trp_obj_t *trp_cairo_flush_and_close_string( trp_obj_t *obj )
{
    return trp_cairo_flush_and_close_low( 1, obj );
}

static uns8b trp_cairo_op0( voidfun_t op, trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );

    if ( c == NULL )
        return 1;
    ( op )( c->cr );
    return 0;
}

static uns8b trp_cairo_op1( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) )
        return 1;
    ( op )( c->cr, xx1 );
    return 0;
}

static uns8b trp_cairo_op2( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1, xx2;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) || trp_cast_flt64b( x2, &xx2 ) )
        return 1;
    ( op )( c->cr, xx1, xx2 );
    return 0;
}

static uns8b trp_cairo_op3( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1, xx2, xx3;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) || trp_cast_flt64b( x2, &xx2 ) ||
                          trp_cast_flt64b( x3, &xx3 ) )
        return 1;
    ( op )( c->cr, xx1, xx2, xx3 );
    return 0;
}

static uns8b trp_cairo_op4( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1, xx2, xx3, xx4;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) || trp_cast_flt64b( x2, &xx2 ) ||
                          trp_cast_flt64b( x3, &xx3 ) || trp_cast_flt64b( x4, &xx4 ) )
        return 1;
    ( op )( c->cr, xx1, xx2, xx3, xx4 );
    return 0;
}

static uns8b trp_cairo_op5( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4, trp_obj_t *x5 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1, xx2, xx3, xx4, xx5;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) || trp_cast_flt64b( x2, &xx2 ) ||
                          trp_cast_flt64b( x3, &xx3 ) || trp_cast_flt64b( x4, &xx4 ) ||
                          trp_cast_flt64b( x5, &xx5 ) )
        return 1;
    ( op )( c->cr, xx1, xx2, xx3, xx4, xx5 );
    return 0;
}

static uns8b trp_cairo_op6( voidfun_t op, trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *x2, trp_obj_t *x3, trp_obj_t *x4, trp_obj_t *x5, trp_obj_t *x6 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx1, xx2, xx3, xx4, xx5, xx6;

    if ( ( c == NULL ) || trp_cast_flt64b( x1, &xx1 ) || trp_cast_flt64b( x2, &xx2 ) ||
                          trp_cast_flt64b( x3, &xx3 ) || trp_cast_flt64b( x4, &xx4 ) ||
                          trp_cast_flt64b( x5, &xx5 ) || trp_cast_flt64b( x6, &xx6 ) )
        return 1;
    ( op )( c->cr, xx1, xx2, xx3, xx4, xx5, xx6 );
    return 0;
}

trp_obj_t *trp_cairo_get_matrix( trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    cairo_matrix_t m;

    if ( c == NULL )
        return UNDEF;
    cairo_get_matrix( c->cr, &m );
    return trp_list( trp_list( trp_double( m.xx ), trp_double( m.xy ), trp_double( m.x0 ), NULL ),
                     trp_list( trp_double( m.yx ), trp_double( m.yy ), trp_double( m.y0 ), NULL ),
                     NULL );
}

trp_obj_t *trp_cairo_user_to_device( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx, yy;

    if ( ( c == NULL ) || trp_cast_flt64b( x, &xx ) || trp_cast_flt64b( y, &yy ) )
        return UNDEF;
    cairo_user_to_device( c->cr, &xx, &yy );
    return trp_cons( trp_double( xx ), trp_double( yy ) );
}

trp_obj_t *trp_cairo_user_to_device_distance( trp_obj_t *obj, trp_obj_t *dx, trp_obj_t *dy )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx, yy;

    if ( ( c == NULL ) || trp_cast_flt64b( dx, &xx ) || trp_cast_flt64b( dy, &yy ) )
        return UNDEF;
    cairo_user_to_device_distance( c->cr, &xx, &yy );
    return trp_cons( trp_double( xx ), trp_double( yy ) );
}

trp_obj_t *trp_cairo_device_to_user( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx, yy;

    if ( ( c == NULL ) || trp_cast_flt64b( x, &xx ) || trp_cast_flt64b( y, &yy ) )
        return UNDEF;
    cairo_device_to_user( c->cr, &xx, &yy );
    return trp_cons( trp_double( xx ), trp_double( yy ) );
}

trp_obj_t *trp_cairo_device_to_user_distance( trp_obj_t *obj, trp_obj_t *dx, trp_obj_t *dy )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    flt64b xx, yy;

    if ( ( c == NULL ) || trp_cast_flt64b( dx, &xx ) || trp_cast_flt64b( dy, &yy ) )
        return UNDEF;
    cairo_device_to_user_distance( c->cr, &xx, &yy );
    return trp_cons( trp_double( xx ), trp_double( yy ) );
}

static trp_obj_t *trp_cairo_ucs4_to_utf8( uns32b unicode )
{
    uns8b utf8[ 5 ];

    if ( unicode < 0x80 ) {
        utf8[ 0 ] = unicode;
        utf8[ 1 ] = 0;
    } else {
        uns8b *p;
        int bytes;

        if ( unicode < 0x800 )
            bytes = 2;
        else if ( unicode < 0x10000 )
            bytes = 3;
        else if ( unicode < 0x200000 )
            bytes = 4;
        else
            return UNDEF;
        for ( p = utf8 + bytes ; p > utf8 ; ) {
            *--p = 0x80 | ( unicode & 0x3f );
            unicode >>= 6;
        }
        *p |= 0xf0 << ( 4 - bytes );
        utf8[ bytes ] = 0;
    }
    return trp_cord( utf8 );
}

static trp_obj_t *trp_cairo_get_ft_name_low( uns8b which, trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    FT_Face ft_face = NULL;
    uns8b *cpath = NULL;
    uns8b tofree;

    if ( c ) {
        tofree = 0;
        if ( c->d )
            ft_face = c->d->ft_face;
    } else {
        tofree = 1;
        TRP_CAIRO_LOCK;
        if ( obj->tipo == TRP_RAW ) {
            if ( FT_New_Memory_Face( _ft_library, ((trp_raw_t *)obj)->data, ((trp_raw_t *)obj)->len, 0, &ft_face ) )
                ft_face = NULL;
        } else {
            cpath = trp_csprint( obj );
            if ( FT_New_Face( _ft_library, cpath, 0, &ft_face ) )
                ft_face = NULL;
        }
        TRP_CAIRO_UNLOCK;
    }
    if ( ft_face == NULL ) {
        if ( cpath )
            trp_csprint_free( cpath );
        return UNDEF;
    }
    obj = UNDEF;
    switch ( which ) {
        case 0:
            if ( ft_face->family_name )
                obj = trp_cord( ft_face->family_name );
            break;
        case 1:
            if ( ft_face->style_name )
                obj = trp_cord( ft_face->style_name );
            break;
        case 2:
            {
                const uns8b *name;

                if ( name = FT_Get_Postscript_Name( ft_face ) )
                    obj = trp_cord( name );
            }
            break;
        case 3:
            {
                FT_UInt index;
                FT_ULong ch;

                if ( FT_Select_Charmap( ft_face, FT_ENCODING_UNICODE ) == 0 ) {
                    obj = trp_queue();
                    for ( ch = FT_Get_First_Char( ft_face, &index ) ;
                          index ;
                          ch = FT_Get_Next_Char( ft_face, ch, &index ) )
                         trp_queue_put( obj, trp_cairo_ucs4_to_utf8( ch ) );
                }
            }
            break;
    }
    if ( tofree ) {
        TRP_CAIRO_LOCK;
        FT_Done_Face( ft_face );
        TRP_CAIRO_UNLOCK;
    }
    if ( cpath )
        trp_csprint_free( cpath );
    return obj;
}

trp_obj_t *trp_cairo_get_ft_family_name( trp_obj_t *obj )
{
    return trp_cairo_get_ft_name_low( 0, obj );
}

trp_obj_t *trp_cairo_get_ft_style_name( trp_obj_t *obj )
{
    return trp_cairo_get_ft_name_low( 1, obj );
}

trp_obj_t *trp_cairo_get_ft_postscript_name( trp_obj_t *obj )
{
    return trp_cairo_get_ft_name_low( 2, obj );
}

trp_obj_t *trp_cairo_get_ft_available_chars( trp_obj_t *obj )
{
    return trp_cairo_get_ft_name_low( 3, obj );
}

trp_obj_t *trp_cairo_toy_font_face_get_family( trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    const uns8b *name;

    if ( c == NULL )
        return UNDEF;
    if ( ( name = cairo_toy_font_face_get_family( cairo_get_font_face( c->cr ) ) ) == NULL )
        return UNDEF;
    return trp_cord( name );
}

trp_obj_t *trp_cairo_font_extents( trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    cairo_font_extents_t extents;

    if ( c == NULL )
        return UNDEF;
    cairo_font_extents( c->cr, &extents );
    return trp_list( trp_double( extents.ascent ),
                     trp_double( extents.descent ),
                     trp_double( extents.height ),
                     trp_double( extents.max_x_advance ),
                     trp_double( extents.max_y_advance ),
                     NULL );
}

trp_obj_t *trp_cairo_text_extents( trp_obj_t *obj, trp_obj_t *s, ...  )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns8b *txt;
    va_list args;
    cairo_text_extents_t extents;

    if ( c == NULL )
        return UNDEF;
    va_start( args, s );
    txt = trp_csprint_multi( s, args );
    va_end( args );
    cairo_text_extents( c->cr, txt, &extents );
    trp_csprint_free( txt );
    return trp_list( trp_double( extents.x_bearing ),
                     trp_double( extents.y_bearing ),
                     trp_double( extents.width ),
                     trp_double( extents.height ),
                     trp_double( extents.x_advance ),
                     trp_double( extents.y_advance ),
                     NULL );
}

uns8b trp_cairo_surface_flush( trp_obj_t *obj )
{
    trp_cairo_t *c = trp_cairo_get( obj );

    if ( c == NULL )
        return 1;
    cairo_surface_flush( c->surface );
    return 0;
}

uns8b trp_cairo_save( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_save, obj );
}

uns8b trp_cairo_restore( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_restore, obj );
}

uns8b trp_cairo_copy_page( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_copy_page, obj );
}

uns8b trp_cairo_show_page( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_show_page, obj );
}

uns8b trp_cairo_translate( trp_obj_t *obj, trp_obj_t *tx, trp_obj_t *ty )
{
    return trp_cairo_op2( cairo_translate, obj, tx, ty );
}

uns8b trp_cairo_scale( trp_obj_t *obj, trp_obj_t *sx, trp_obj_t *sy )
{
    return trp_cairo_op2( cairo_scale, obj, sx, sy );
}

uns8b trp_cairo_rotate( trp_obj_t *obj, trp_obj_t *angle )
{
    return trp_cairo_op1( cairo_rotate, obj, angle );
}

uns8b trp_cairo_transform( trp_obj_t *obj, trp_obj_t *xx, trp_obj_t *xy, trp_obj_t *x0, trp_obj_t *yx, trp_obj_t *yy, trp_obj_t *y0 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    cairo_matrix_t m;

    if ( ( c == NULL ) || trp_cast_flt64b( xx, &(m.xx) ) || trp_cast_flt64b( xy, &(m.xy) ) || trp_cast_flt64b( x0, &(m.x0) ) ||
                          trp_cast_flt64b( yx, &(m.yx) ) || trp_cast_flt64b( yy, &(m.yy) ) || trp_cast_flt64b( y0, &(m.y0) ) )
        return 1;
    cairo_transform( c->cr, &m );
    return 0;
}

uns8b trp_cairo_set_matrix( trp_obj_t *obj, trp_obj_t *xx, trp_obj_t *xy, trp_obj_t *x0, trp_obj_t *yx, trp_obj_t *yy, trp_obj_t *y0 )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    cairo_matrix_t m;

    if ( ( c == NULL ) || trp_cast_flt64b( xx, &(m.xx) ) || trp_cast_flt64b( xy, &(m.xy) ) || trp_cast_flt64b( x0, &(m.x0) ) ||
                          trp_cast_flt64b( yx, &(m.yx) ) || trp_cast_flt64b( yy, &(m.yy) ) || trp_cast_flt64b( y0, &(m.y0) ) )
        return 1;
    cairo_set_matrix( c->cr, &m );
    return 0;
}

uns8b trp_cairo_identity_matrix( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_identity_matrix, obj );
}

uns8b trp_cairo_set_antialias( trp_obj_t *obj, trp_obj_t *antialias )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns32b ant;

    if ( ( c == NULL ) || trp_cast_uns32b( antialias, &ant ) )
        return 1;
    cairo_set_antialias( c->cr, ant );
    return 0;
}

uns8b trp_cairo_select_font_face( trp_obj_t *obj, trp_obj_t *family, trp_obj_t *slant, trp_obj_t *weight )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns32b sla, wei;
    uns8b *fam;

    if ( ( c == NULL ) || trp_cast_uns32b( slant, &sla ) || trp_cast_uns32b( weight, &wei ) )
        return 1;
    fam = trp_csprint( family );
    cairo_select_font_face( c->cr, fam, sla, wei );
    trp_csprint_free( fam );
    trp_cairo_destroy_ft( c );
    return 0;
}

static void trp_cairo_set_font_face_ft_destroy( void *p )
{
    trp_cairo_destroy_ft_t *d = (trp_cairo_destroy_ft_t *)p;

    /*
     * ho il sospetto di memory leaks:
     * in trp_cairo_destroy_ft() il refcount di ft_font_face
     * può essere 1 (e questa funzione viene chiamata)
     * oppure maggiore di 1 (questa funzione non viene
     * chiamata);
     * è maggiore di 1, ad esempio, quando viene chiamata
     * cairo_show_text(), che aumenta il refcount
     */
//    fprintf( stderr, "### FT_Done_Face( %p )\n", d->ft_face );
    TRP_CAIRO_LOCK;
    FT_Done_Face( d->ft_face );
    TRP_CAIRO_UNLOCK;
    free( d->ft_aux_data );
    free( d );
}

uns8b trp_cairo_set_font_face_ft( trp_obj_t *obj, trp_obj_t *path )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    trp_cairo_destroy_ft_t *d;

    if ( c == NULL )
        return 1;
    if ( ( d = malloc( sizeof( trp_cairo_destroy_ft_t ) ) ) == NULL )
        return 1;
    if ( path->tipo == TRP_RAW ) {
        uns32b len = ((trp_raw_t *)path)->len;

        if ( ( d->ft_aux_data = malloc( len ) ) == NULL ) {
            free( d );
            return 1;
        }
        memcpy( d->ft_aux_data, ((trp_raw_t *)path)->data, len );
        TRP_CAIRO_LOCK;
        if ( FT_New_Memory_Face( _ft_library, d->ft_aux_data, len, 0, &( d->ft_face ) ) ) {
            TRP_CAIRO_UNLOCK;
            free( d->ft_aux_data );
            free( d );
            return 1;
        }
    } else {
        uns32b len;
        uns8b *cpath = trp_csprint( path );

        len = strlen( cpath ) + 1;
        if ( ( d->ft_aux_data = malloc( len ) ) == NULL ) {
            free( d );
            return 1;
        }
        memcpy( d->ft_aux_data, cpath, len );
        trp_csprint_free( cpath );
        TRP_CAIRO_LOCK;
        if ( FT_New_Face( _ft_library, d->ft_aux_data, 0, &( d->ft_face ) ) ) {
            TRP_CAIRO_UNLOCK;
            free( d->ft_aux_data );
            free( d );
            return 1;
        }
    }
    TRP_CAIRO_UNLOCK;
//    fprintf( stderr, "### FT_New_Face( %p )\n", d->ft_face );
    d->ft_font_face = cairo_ft_font_face_create_for_ft_face( d->ft_face, 0 );
    if ( cairo_font_face_set_user_data( d->ft_font_face, NULL, d, (cairo_destroy_func_t)trp_cairo_set_font_face_ft_destroy ) ) {
        cairo_font_face_destroy( d->ft_font_face );
        TRP_CAIRO_LOCK;
        FT_Done_Face( d->ft_face );
        TRP_CAIRO_UNLOCK;
        free( d->ft_aux_data );
        free( d );
        return 1;
    }
    cairo_set_font_face( c->cr, d->ft_font_face );
    trp_cairo_destroy_ft( c );
    c->d = d;
    return 0;
}

uns8b trp_cairo_set_font_size( trp_obj_t *obj, trp_obj_t *size )
{
    return trp_cairo_op1( cairo_set_font_size, obj, size );
}

uns8b trp_cairo_set_line_width( trp_obj_t *obj, trp_obj_t *width )
{
    return trp_cairo_op1( cairo_set_line_width, obj, width );
}

uns8b trp_cairo_set_line_cap( trp_obj_t *obj, trp_obj_t *line_cap )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns32b lcap;

    if ( ( c == NULL ) || trp_cast_uns32b( line_cap, &lcap ) )
        return 1;
    cairo_set_line_cap( c->cr, lcap );
    return 0;
}

uns8b trp_cairo_set_line_join( trp_obj_t *obj, trp_obj_t *line_join )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns32b ljoin;

    if ( ( c == NULL ) || trp_cast_uns32b( line_join, &ljoin ) )
        return 1;
    cairo_set_line_join( c->cr, ljoin );
    return 0;
}

uns8b trp_cairo_set_fill_rule( trp_obj_t *obj, trp_obj_t *fill_rule )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns32b frule;

    if ( ( c == NULL ) || trp_cast_uns32b( fill_rule, &frule ) )
        return 1;
    cairo_set_fill_rule( c->cr, frule );
    return 0;
}

uns8b trp_cairo_set_miter_limit( trp_obj_t *obj, trp_obj_t *limit )
{
    return trp_cairo_op1( cairo_set_miter_limit, obj, limit );
}

uns8b trp_cairo_set_source_rgba( trp_obj_t *obj, trp_obj_t *color )
{
    trp_cairo_t *c = trp_cairo_get( obj );

    if ( ( c == NULL ) || ( color->tipo != TRP_PIX ) )
        return 1;
    if ( ((trp_pix_t *)color)->sottotipo )
        return 1;
    cairo_set_source_rgba( c->cr,
                           ((flt64b)colorval(((trp_pix_t *)color)->color.red ))/255.0,
                           ((flt64b)colorval(((trp_pix_t *)color)->color.green ))/255.0,
                           ((flt64b)colorval(((trp_pix_t *)color)->color.blue ))/255.0,
                           ((flt64b)colorval(((trp_pix_t *)color)->color.alpha ))/255.0 );
    return 0;
}

uns8b trp_cairo_set_source_surface( trp_obj_t *obj, trp_obj_t *src, trp_obj_t *x, trp_obj_t *y, trp_obj_t *alpha )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    cairo_surface_t *surface;
    flt64b xx, yy, aalpha;
    uns8b res = 1;

    if ( ( c == NULL ) || trp_cast_flt64b( x, &xx ) || trp_cast_flt64b( y, &yy ) )
        return 1;
    if ( alpha ) {
        if ( trp_cast_flt64b_range( alpha, &aalpha, 0.0, 1.0 ) )
            return 1;
    } else
        aalpha = 1.0;
    switch ( src->tipo ) {
        case TRP_PIX:
            {
                trp_pix_color_t *s, *d;
                flt64b m;
                uns32b w, h, i;

                s = ((trp_pix_t *)src)->map.c;
                w = ((trp_pix_t *)src)->w;
                h = ((trp_pix_t *)src)->h;
                if ( ( d = malloc( ( w * h ) << 2 ) ) == NULL )
                    return 1;
                for ( i = w * h ; i ; ) {
                    i--;
                    m = 255.0 / ((flt64b)( s[ i ].alpha ));
                    d[ i ].red = (uns8b)(((flt64b)( s[ i ].blue )) / m);
                    d[ i ].green = (uns8b)(((flt64b)( s[ i ].green )) / m);
                    d[ i ].blue = (uns8b)(((flt64b)( s[ i ].red )) / m);
                    d[ i ].alpha = s[ i ].alpha;
                }
                surface = cairo_image_surface_create_for_data( (uns8b *)d, CAIRO_FORMAT_ARGB32, w, h, w << 2 );
                if ( cairo_surface_set_user_data( surface, NULL, d, (cairo_destroy_func_t)free ) ) {
                    cairo_surface_destroy( surface );
                    free( d );
                    return 1;
                }
            }
            cairo_set_source_surface( c->cr, surface, xx, yy );
            cairo_paint_with_alpha( c->cr, aalpha );
            cairo_surface_flush( c->surface );
            cairo_surface_destroy( surface );
            res = 0;
            break;
        case TRP_CORD:
        case TRP_RAW:
            {
                RsvgHandle *handle;
                gdouble ww, hh;
                RsvgRectangle viewport;

                if ( src->tipo == TRP_CORD ) {
                    uns8b *cpath = trp_csprint( src );
                    handle = rsvg_handle_new_from_file( cpath, NULL );
                    trp_csprint_free( cpath );
                } else
                    handle = rsvg_handle_new_from_data( ((trp_raw_t *)src)->data, ((trp_raw_t *)src)->len, NULL );
                if ( handle == NULL )
                    return 1;
                if ( !rsvg_handle_get_intrinsic_size_in_pixels( handle, &ww, &hh ) ) {
                    ww = 720.0;
                    hh = 460.0;
                }
                surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, ww, hh );
                cairo_set_source_surface( c->cr, surface, 0, 0 );
                viewport.x = xx;
                viewport.y = yy;
                viewport.width = ww;
                viewport.height = hh;
                if ( rsvg_handle_render_document( handle, c->cr, &viewport, NULL ) )
                    res = 0;
                cairo_surface_flush( surface );
                cairo_paint_with_alpha( c->cr, aalpha );
                cairo_surface_flush( c->surface );
                cairo_surface_destroy( surface );
                g_object_unref( handle );
            }
            break;
        case TRP_CAIRO:
            {
                trp_cairo_t *s = trp_cairo_get( src );

                if ( s ) {
                    cairo_surface_flush( s->surface );
                    cairo_set_source_surface( c->cr, s->surface, xx, yy );
                    cairo_paint_with_alpha( c->cr, aalpha );
                    cairo_surface_flush( c->surface );
                    res = 0;
                }
            }
            break;
    }
    return res;
}

uns8b trp_cairo_new_path( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_new_path, obj );
}

uns8b trp_cairo_close_path( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_close_path, obj );
}

uns8b trp_cairo_stroke( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_stroke, obj );
}

uns8b trp_cairo_stroke_preserve( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_stroke_preserve, obj );
}

uns8b trp_cairo_fill( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_fill, obj );
}

uns8b trp_cairo_fill_preserve( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_fill_preserve, obj );
}

uns8b trp_cairo_clip( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_clip, obj );
}

uns8b trp_cairo_clip_preserve( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_clip_preserve, obj );
}

uns8b trp_cairo_reset_clip( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_reset_clip, obj );
}

uns8b trp_cairo_paint( trp_obj_t *obj )
{
    return trp_cairo_op0( cairo_paint, obj );
}

uns8b trp_cairo_paint_with_alpha( trp_obj_t *obj, trp_obj_t *alpha )
{
    return trp_cairo_op1( cairo_paint_with_alpha, obj, alpha );
}

uns8b trp_cairo_move_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    return trp_cairo_op2( cairo_move_to, obj, x, y );
}

uns8b trp_cairo_rel_move_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    return trp_cairo_op2( cairo_rel_move_to, obj, x, y );
}

uns8b trp_cairo_line_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    return trp_cairo_op2( cairo_line_to, obj, x, y );
}

uns8b trp_cairo_rel_line_to( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y )
{
    return trp_cairo_op2( cairo_rel_line_to, obj, x, y );
}

uns8b trp_cairo_curve_to( trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *x3, trp_obj_t *y3 )
{
    return trp_cairo_op6( cairo_curve_to, obj, x1, y1, x2, y2, x3, y3 );
}

uns8b trp_cairo_rel_curve_to( trp_obj_t *obj, trp_obj_t *x1, trp_obj_t *y1, trp_obj_t *x2, trp_obj_t *y2, trp_obj_t *x3, trp_obj_t *y3 )
{
    return trp_cairo_op6( cairo_rel_curve_to, obj, x1, y1, x2, y2, x3, y3 );
}

uns8b trp_cairo_rectangle( trp_obj_t *obj, trp_obj_t *x, trp_obj_t *y, trp_obj_t *w, trp_obj_t *h )
{
    return trp_cairo_op4( cairo_rectangle, obj, x, y, w, h );
}

uns8b trp_cairo_arc( trp_obj_t *obj, trp_obj_t *xc, trp_obj_t *yc, trp_obj_t *radius, trp_obj_t *angle1, trp_obj_t *angle2 )
{
    return trp_cairo_op5( cairo_arc, obj, xc, yc, radius, angle1, angle2 );
}

uns8b trp_cairo_arc_negative( trp_obj_t *obj, trp_obj_t *xc, trp_obj_t *yc, trp_obj_t *radius, trp_obj_t *angle1, trp_obj_t *angle2 )
{
    return trp_cairo_op5( cairo_arc_negative, obj, xc, yc, radius, angle1, angle2 );
}

uns8b trp_cairo_show_text( trp_obj_t *obj, trp_obj_t *s, ... )
{
    trp_cairo_t *c = trp_cairo_get( obj );
    uns8b *txt;
    va_list args;

    if ( c == NULL )
        return 1;
    va_start( args, s );
    txt = trp_csprint_multi( s, args );
    va_end( args );
//    fprintf( stderr, "### refcount (before show text) = %d\n", cairo_font_face_get_reference_count( c->d->ft_font_face ) );
    cairo_show_text( c->cr, txt );
//    fprintf( stderr, "### refcount (after show text) = %d\n", cairo_font_face_get_reference_count( c->d->ft_font_face ) );
    trp_csprint_free( txt );
    return 0;
}

