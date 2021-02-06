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
#include "./trpmgl.h"
#include <mgl2/mgl.h>

typedef struct {
    uns8b tipo;
    uns8b sottotipo;
    union {
        HMGL graph;
        HMDT data;
        HMPR parser;
    } obj;
} trp_mgl_t;

static uns8b trp_mgl_print( trp_print_t *p, trp_mgl_t *obj );
static uns8b trp_mgl_close( trp_mgl_t *obj );
static uns8b trp_mgl_close_basic( uns8b flags, trp_mgl_t *obj );
static void trp_mgl_finalize( void *obj, void *data );
static trp_obj_t *trp_mgl_width( trp_mgl_t *obj );
static trp_obj_t *trp_mgl_height( trp_mgl_t *obj );
static HMGL trp_mgl_graph( trp_obj_t *obj );
static HMDT trp_mgl_data( trp_obj_t *obj );
static HMPR trp_mgl_parser( trp_obj_t *obj );
static uns8b trp_mgl_write_basic( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr, voidfun_t fun );

uns8b trp_mgl_init()
{
    extern uns8bfun_t _trp_print_fun[];
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_print_fun[ TRP_MGL ] = trp_mgl_print;
    _trp_close_fun[ TRP_MGL ] = trp_mgl_close;
    _trp_width_fun[ TRP_MGL ] = trp_mgl_width;
    _trp_height_fun[ TRP_MGL ] = trp_mgl_height;
    return 0;
}

static uns8b trp_mgl_print( trp_print_t *p, trp_mgl_t *obj )
{
    if ( trp_print_char_star( p, ( obj->sottotipo == 0 ) ? "#mgl graph"
                              : ( ( obj->sottotipo == 1 ) ? "#mgl data"
                                  : "#mgl parser" ) ) )
        return 1;
    if ( obj->obj.graph == NULL )
        if ( trp_print_char_star( p, " (closed)" ) )
            return 1;
    return trp_print_char( p, '#' );
}

static uns8b trp_mgl_close( trp_mgl_t *obj )
{
    return trp_mgl_close_basic( 1, obj );
}

static uns8b trp_mgl_close_basic( uns8b flags, trp_mgl_t *obj )
{
    if ( obj->obj.graph ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        switch ( obj->sottotipo ) {
        case 0:
            mgl_delete_graph( obj->obj.graph );
            break;
        case 1:
            mgl_delete_data( obj->obj.data );
            break;
        case 2:
            mgl_delete_parser( obj->obj.parser );
            break;
        }
        obj->obj.graph = NULL;
    }
    return 0;
}

static void trp_mgl_finalize( void *obj, void *data )
{
    trp_mgl_close_basic( 0, (trp_mgl_t *)obj );
}

static trp_obj_t *trp_mgl_width( trp_mgl_t *obj )
{
    if ( obj->obj.graph == NULL )
        return UNDEF;
    if ( obj->sottotipo != 0 )
        return UNDEF;
    return trp_sig64( mgl_get_width( obj->obj.graph ) );
}

static trp_obj_t *trp_mgl_height( trp_mgl_t *obj )
{
    if ( obj->obj.graph == NULL )
        return UNDEF;
    if ( obj->sottotipo != 0 )
        return UNDEF;
    return trp_sig64( mgl_get_height( obj->obj.graph ) );
}

static HMGL trp_mgl_graph( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_MGL )
        return NULL;
    if ( ((trp_mgl_t *)obj)->sottotipo != 0 )
        return NULL;
    return ((trp_mgl_t *)obj)->obj.graph;
}

static HMDT trp_mgl_data( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_MGL )
        return NULL;
    if ( ((trp_mgl_t *)obj)->sottotipo != 1 )
        return NULL;
    return ((trp_mgl_t *)obj)->obj.data;
}

static HMPR trp_mgl_parser( trp_obj_t *obj )
{
    if ( obj->tipo != TRP_MGL )
        return NULL;
    if ( ((trp_mgl_t *)obj)->sottotipo != 2 )
        return NULL;
    return ((trp_mgl_t *)obj)->obj.parser;
}

trp_obj_t *trp_mgl_create_graph_zb( trp_obj_t *w, trp_obj_t *h )
{
    trp_mgl_t *obj;
    HMGL gr;
    uns32b ww, hh;

    if ( trp_cast_uns32b( w, &ww ) ||
         trp_cast_uns32b( h, &hh ) )
        return UNDEF;
    gr = mgl_create_graph_zb( ww, hh );
    if ( gr == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 0;
    obj->obj.graph = gr;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_graph_ps( trp_obj_t *w, trp_obj_t *h )
{
    trp_mgl_t *obj;
    HMGL gr;
    uns32b ww, hh;

    if ( trp_cast_uns32b( w, &ww ) ||
         trp_cast_uns32b( h, &hh ) )
        return UNDEF;
    gr = mgl_create_graph_ps( ww, hh );
    if ( gr == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 0;
    obj->obj.graph = gr;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_graph_idtf()
{
    trp_mgl_t *obj;
    HMGL gr;

    gr = mgl_create_graph_idtf();
    if ( gr == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 0;
    obj->obj.graph = gr;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_data()
{
    trp_mgl_t *obj;
    HMGL dat;

    dat = mgl_create_data();
    if ( dat == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 1;
    obj->obj.data = dat;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_data_size( trp_obj_t *nx, trp_obj_t *ny, trp_obj_t *nz )
{
    trp_mgl_t *obj;
    HMGL dat;
    uns32b x, y, z;

    if ( trp_cast_uns32b( nx, &x ) ||
         trp_cast_uns32b( ny, &y ) ||
         trp_cast_uns32b( nz, &z ) )
        return UNDEF;
    dat = mgl_create_data_size( x, y, z );
    if ( dat == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 1;
    obj->obj.data = dat;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_data_file( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    trp_mgl_t *obj;
    HMGL dat;

    dat = mgl_create_data_file( cpath );
    trp_csprint_free( cpath );
    if ( dat == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 1;
    obj->obj.data = dat;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_mgl_create_parser()
{
    trp_mgl_t *obj;
    HMPR p;

    p = mgl_create_parser();
    if ( p == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_mgl_t ), trp_mgl_finalize );
    obj->tipo = TRP_MGL;
    obj->sottotipo = 2;
    obj->obj.parser = p;
    return (trp_obj_t *)obj;
}

static uns8b trp_mgl_write_basic( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr, voidfun_t fun )
{
    HMGL gr = trp_mgl_graph( mgl );
    uns8b *cpath, *cdescr = NULL;

    if ( gr == NULL )
        return 1;
    cpath = trp_csprint( path );
    if ( descr )
        cdescr = trp_csprint( descr );
    (*fun)( gr, cpath, cdescr );
    trp_csprint_free( cpath );
    if ( descr )
        trp_csprint_free( cdescr );
    return 0;
}

uns8b trp_mgl_write_bmp( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_bmp );
}

uns8b trp_mgl_write_jpg( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_jpg );
}

uns8b trp_mgl_write_png( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_png );
}

uns8b trp_mgl_write_png_solid( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_png_solid );
}

uns8b trp_mgl_write_eps( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_eps );
}

uns8b trp_mgl_write_svg( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_svg );
}

uns8b trp_mgl_write_idtf( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_idtf );
}

uns8b trp_mgl_write_gif( trp_obj_t *mgl, trp_obj_t *path, trp_obj_t *descr )
{
    return trp_mgl_write_basic( mgl, path, descr, (voidfun_t)mgl_write_gif );
}

uns8b trp_mgl_update( trp_obj_t *mgl )
{
    HMGL gr = trp_mgl_graph( mgl );

    if ( gr == NULL )
        return 1;
    mgl_update( gr );
    return 0;
}

uns8b trp_mgl_box( trp_obj_t *mgl, trp_obj_t *ticks )
{
    HMGL gr = trp_mgl_graph( mgl );

    if ( ticks == NULL )
        ticks = TRP_TRUE;
    if ( ( gr == NULL ) ||
         ( !TRP_BOOLP( ticks ) ) )
        return 1;
    mgl_box( gr, ( ticks == TRP_TRUE ) ? 1 : 0 );
    return 0;
}

uns8b trp_mgl_set_light( trp_obj_t *mgl, trp_obj_t *enable )
{
    HMGL gr = trp_mgl_graph( mgl );

    if ( enable == NULL )
        enable = TRP_TRUE;
    if ( ( gr == NULL ) ||
         ( !TRP_BOOLP( enable ) ) )
        return 1;
    mgl_set_light( gr, ( enable == TRP_TRUE ) ? 1 : 0 );
    return 0;
}

uns8b trp_mgl_rotate( trp_obj_t *mgl, trp_obj_t *tetx, trp_obj_t *tety, trp_obj_t *tetz )
{
    HMGL gr = trp_mgl_graph( mgl );
    double x, y, z;

    if ( ( gr == NULL ) ||
         trp_cast_double( tetx, &x ) ||
         trp_cast_double( tety, &y ) ||
         trp_cast_double( tetz, &z ) )
        return 1;
    mgl_rotate( gr, x, y, z );
    return 0;
}

uns8b trp_mgl_data_modify( trp_obj_t *mgd, trp_obj_t *eq, trp_obj_t *dim )
{
    uns8b *ceq;
    HMDT dat = trp_mgl_data( mgd );
    uns32b ddim;

    if ( ( dat == NULL ) ||
         trp_cast_uns32b( dim, &ddim ) )
        return 1;
    ceq = trp_csprint( eq );
    mgl_data_modify( dat, ceq, ddim );
    trp_csprint_free( ceq );
    return 0;
}

uns8b trp_mgl_plot( trp_obj_t *mgl, trp_obj_t *mgd, trp_obj_t *pen )
{
    HMGL gr = trp_mgl_graph( mgl );
    HMDT dat = trp_mgl_data( mgd );
    uns8b *cpen = NULL;

    if ( ( gr == NULL ) ||
         ( dat == NULL ) )
        return 1;
    if ( pen )
        cpen = trp_csprint( pen );
    mgl_plot( gr, dat, cpen );
    if ( pen )
        trp_csprint_free( cpen );
    return 0;
}

uns8b trp_mgl_surf( trp_obj_t *mgl, trp_obj_t *mgd, trp_obj_t *sch )
{
    HMGL gr = trp_mgl_graph( mgl );
    HMDT dat = trp_mgl_data( mgd );
    uns8b *csch = NULL;

    if ( ( gr == NULL ) ||
         ( dat == NULL ) )
        return 1;
    if ( sch )
        csch = trp_csprint( sch );
    mgl_surf( gr, dat, csch );
    if ( sch )
        trp_csprint_free( csch );
    return 0;
}

