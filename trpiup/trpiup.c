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
#include "./trpiup.h"
#include "../trppix/trppix_internal.h"

typedef struct {
    uns8b tipo;
    Ihandle *h;
} trp_iup_t;

typedef Ihandle * (*ihandle_t)();

static uns8b trp_iup_close( trp_iup_t *obj );
static trp_obj_t *trp_iup_equal( trp_iup_t *obj1, trp_iup_t *obj2 );
static trp_obj_t *trp_iup_length( trp_iup_t *obj );
static trp_obj_t *trp_iup_nth( uns32b n, trp_iup_t *obj );
static trp_obj_t *trp_iup_width( trp_iup_t *obj );
static trp_obj_t *trp_iup_height( trp_iup_t *obj );
static trp_obj_t *trp_iup_handle( Ihandle *h );
static Ihandle *trp_iup_check( trp_obj_t *ih );
static trp_obj_t *trp_iup_container_low( ihandle_t f, trp_obj_t *child, va_list args );
static trp_obj_t *trp_iup_container_low_low( uns8b flags, ihandle_t f, trp_obj_t *child, va_list args );

uns8b trp_iup_init( int *argc, char ***argv )
{
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_equal_fun[];
    extern objfun_t _trp_length_fun[];
    extern objfun_t _trp_nth_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    if ( IupOpen( argc, argv ) ) {
        fprintf( stderr, "Initialization of IUP failed\n" );
        return 1;
    }
//    IupImageLibOpen();
    IupSetGlobal( "UTF8MODE", "YES" );
    _trp_close_fun[ TRP_IUP ] = trp_iup_close;
    _trp_equal_fun[ TRP_IUP ] = trp_iup_equal;
    _trp_length_fun[ TRP_IUP ] = trp_iup_length;
    _trp_nth_fun[ TRP_IUP ] = trp_iup_nth;
    _trp_width_fun[ TRP_IUP ] = trp_iup_width;
    _trp_height_fun[ TRP_IUP ] = trp_iup_height;
    return 0;
}

void trp_iup_quit()
{
    IupClose();
}

static uns8b trp_iup_close( trp_iup_t *obj )
{
    if ( obj->h ) {
        IupDestroy( obj->h );
        obj->h = NULL;
    }
    return 0;
}

static trp_obj_t *trp_iup_equal( trp_iup_t *obj1, trp_iup_t *obj2 )
{
    return ( ( obj1->h ) && ( obj1->h == obj2->h ) ) ? TRP_TRUE : TRP_FALSE;
}

static trp_obj_t *trp_iup_length( trp_iup_t *obj )
{
    if ( obj->h == NULL )
        return UNDEF;
    return trp_sig64( IupGetInt( obj->h, "COUNT" ) );
}

static trp_obj_t *trp_iup_nth( uns32b n, trp_iup_t *obj )
{
    if ( obj->h == NULL )
        return UNDEF;
    return trp_iup_handle( IupGetChild( obj->h, n ) );
}

static trp_obj_t *trp_iup_width( trp_iup_t *obj )
{
    if ( obj->h == NULL )
        return UNDEF;
    return UNDEF;
}

static trp_obj_t *trp_iup_height( trp_iup_t *obj )
{
    if ( obj->h == NULL )
        return UNDEF;
    return UNDEF;
}

static trp_obj_t *trp_iup_handle( Ihandle *h )
{
    trp_iup_t *res;

    if ( h == NULL )
        return UNDEF;
    res = trp_gc_malloc( sizeof( trp_iup_t ) );
    res->tipo = TRP_IUP;
    res->h = h;
    return (trp_obj_t *)res;
}

static Ihandle *trp_iup_check( trp_obj_t *ih )
{
    if ( ih->tipo != TRP_IUP )
        return NULL;
    return ((trp_iup_t *)ih)->h;
}

trp_obj_t *trp_iup_version()
{
    return trp_cord( IupVersion() );
}

trp_obj_t *trp_iup_version_date()
{
    return trp_cord( IupVersionDate() );
}

trp_obj_t *trp_iup_version_number()
{
    return trp_sig64( IupVersionNumber() );
}

uns8b trp_iup_main_loop()
{
    return IupMainLoop() ? 1 : 0;
}

uns8b trp_iup_loop_step()
{
    return IupLoopStep() ? 1 : 0;
}

uns8b trp_iup_loop_step_wait()
{
    return IupLoopStepWait() ? 1 : 0;
}

uns8b trp_iup_main_loop_level()
{
    return IupMainLoopLevel() ? 1 : 0;
}

uns8b trp_iup_flush()
{
    IupFlush();
    return 0;
}

uns8b trp_iup_exit_loop()
{
    IupExitLoop();
    return 0;
}

uns8b trp_iup_set_str_global( trp_obj_t *name, trp_obj_t *value, ... )
{
    uns8b *nn, *vv;
    va_list args;

    nn = trp_csprint( name );
    if ( value == NULL ) {
        IupSetGlobal( nn, NULL );
        trp_csprint_free( nn );
        return 0;
    }
    va_start( args, value );
    vv = trp_csprint_multi( value, args );
    va_end( args );
    IupSetStrGlobal( nn, vv );
    trp_csprint_free( nn );
    trp_csprint_free( vv );
    return 0;
}

uns8b trp_iup_set_attribute( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *ihvalue )
{
    uns8b *nn;
    Ihandle *h, *hv;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         ( ( hv = trp_iup_check( ihvalue ) ) == NULL ) )
        return 1;
    nn = trp_csprint( name );
    IupSetAttribute( h, nn, (char *)hv );
    trp_csprint_free( nn );
    return 0;
}

uns8b trp_iup_set_str_attribute( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value, ... )
{
    uns8b *nn, *vv;
    Ihandle *h;
    va_list args;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    nn = trp_csprint( name );
    if ( value == NULL ) {
        IupSetAttribute( h, nn, NULL );
        trp_csprint_free( nn );
        return 0;
    }
    va_start( args, value );
    vv = trp_csprint_multi( value, args );
    va_end( args );
    IupSetStrAttribute( h, nn, vv );
    trp_csprint_free( nn );
    trp_csprint_free( vv );
    return 0;
}

uns8b trp_iup_set_int( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value )
{
    uns8b *nn;
    Ihandle *h;
    sig32b v;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_sig32b( value, &v ) )
        return 1;
    nn = trp_csprint( name );
    IupSetInt( h, nn, v );
    trp_csprint_free( nn );
    return 0;
}

uns8b trp_iup_set_double( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *value )
{
    uns8b *nn;
    Ihandle *h;
    double v;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_double( value, &v ) )
        return 1;
    nn = trp_csprint( name );
    IupSetDouble( h, nn, v );
    trp_csprint_free( nn );
    return 0;
}

uns8b trp_iup_set_attribute_handle( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *ih_named )
{
    uns8b *nn;
    Ihandle *h, *hn;

    if ( ih == UNDEF )
        h = NULL;
    else  if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    if ( ( hn = trp_iup_check( ih_named ) ) == NULL )
        return 1;
    nn = trp_csprint( name );
    IupSetAttributeHandle( h, nn, hn );
    trp_csprint_free( nn );
    return 0;
}

static int trp_iup_cback_action( Ihandle *self )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_ACTION" ))( trp_iup_handle( self ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_action_cb( Ihandle *self )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_ACTION_CB" ))( trp_iup_handle( self ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_close_cb( Ihandle *self )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_CLOSE_CB" ))( trp_iup_handle( self ) ) ? IUP_CLOSE : IUP_IGNORE;
}

static int trp_iup_cback_destroy_cb( Ihandle *self )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_DESTROY_CB" ))( trp_iup_handle( self ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_k_any( Ihandle *self, int c )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_K_ANY" ))( trp_iup_handle( self ),
                                                               trp_sig64( c ) ) ? IUP_CONTINUE : IUP_IGNORE;
}

static int trp_iup_cback_caret_cb( Ihandle *self, int lin, int col )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_CARET_CB" ))( trp_iup_handle( self ),
                                                                  trp_sig64( lin ),
                                                                  trp_sig64( col ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_tabchangepos_cb( Ihandle *self, int newpos, int oldpos )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_TABCHANGEPOS_CB" ))( trp_iup_handle( self ),
                                                                         trp_sig64( newpos ),
                                                                         trp_sig64( oldpos ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_dropfiles_cb( Ihandle *self, char *path, int num, int x, int y )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_DROPFILES_CB" ))( trp_iup_handle( self ),
                                                                      trp_cord( path ),
                                                                      trp_sig64( num ),
                                                                      trp_sig64( x ),
                                                                      trp_sig64( y ) ) ? IUP_IGNORE : IUP_DEFAULT;
}

static int trp_iup_cback_button_cb( Ihandle *self, int button, int pressed, int x, int y, char *status )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_BUTTON_CB" ))( trp_iup_handle( self ),
                                                                   trp_sig64( button ),
                                                                   trp_sig64( pressed ),
                                                                   trp_sig64( x ),
                                                                   trp_sig64( y ),
                                                                   UNDEF ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_resize_cb( Ihandle *self, int width, int height )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_RESIZE_CB" ))( trp_iup_handle( self ),
                                                                   trp_sig64( width ),
                                                                   trp_sig64( height ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int trp_iup_cback_valuechanged_cb( Ihandle *self )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_VALUECHANGED_CB" ))( trp_iup_handle( self ) ) ? IUP_CLOSE : IUP_DEFAULT;
}

static int testDragDataSize_cb( Ihandle *self, char* type )
{
    return 1;  // return the size of the data to be dragged
}

static int testDragData_cb( Ihandle *self, char* type, void *data, int len )
{
    sprintf( data, "" );
    return IUP_DEFAULT;
}

static int trp_iup_cback_dragbegin_cb( Ihandle *self, int x, int y )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_DRAGBEGIN_CB" ))( trp_iup_handle( self ),
                                                                      trp_sig64( x ),
                                                                      trp_sig64( y ) ) ? IUP_IGNORE : IUP_DEFAULT;
}

static int trp_iup_cback_dropdata_cb( Ihandle *self, char *type, void *data, int size, int x, int y )
{
    return ((uns8bfun_t)IupGetAttribute( self, "TRP_DROPDATA_CB" ))( trp_iup_handle( self ),
                                                                     trp_sig64( x ),
                                                                     trp_sig64( y ) ) ? IUP_DEFAULT : IUP_DEFAULT;
}

struct cback {
    uns8b *name;
    uns8b nargs;
    Icallback cb;
};

#define TRP_IUP_CALLBACKS 13

static struct cback _trp_iup_callbacks[ TRP_IUP_CALLBACKS ] = {
    { "ACTION", 1, (Icallback)trp_iup_cback_action },
    { "ACTION_CB", 1, (Icallback)trp_iup_cback_action_cb },
    { "CLOSE_CB", 1, (Icallback)trp_iup_cback_close_cb },
    { "DESTROY_CB", 1, (Icallback)trp_iup_cback_destroy_cb },
    { "K_ANY", 2, (Icallback)trp_iup_cback_k_any },
    { "CARET_CB", 3, (Icallback)trp_iup_cback_caret_cb },
    { "TABCHANGEPOS_CB", 3, (Icallback)trp_iup_cback_tabchangepos_cb },
    { "DROPFILES_CB", 5, (Icallback)trp_iup_cback_dropfiles_cb },
    { "BUTTON_CB", 6, (Icallback)trp_iup_cback_button_cb },
    { "RESIZE_CB", 3, (Icallback)trp_iup_cback_resize_cb },
    { "VALUECHANGED_CB", 1, (Icallback)trp_iup_cback_valuechanged_cb },
    { "DRAGBEGIN_CB", 3, (Icallback)trp_iup_cback_dragbegin_cb },
    { "DROPDATA_CB", 3, (Icallback)trp_iup_cback_dropdata_cb }
}; 

uns8b trp_iup_set_callback( trp_obj_t *ih, trp_obj_t *name, trp_obj_t *cback )
{
    int i;
    uns8b *nn;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    if ( cback == NULL ) {
        nn = trp_csprint( name );
        (void)IupSetCallback( h, nn, NULL );
        trp_csprint_free( nn );
        return 0;
    }
    if ( cback->tipo != TRP_NETPTR )
        return 1;
    nn = trp_csprint( name );
    for ( i = 0 ; i < TRP_IUP_CALLBACKS ; i++ )
        if ( strcmp( nn, _trp_iup_callbacks[ i ].name  ) == 0 ) {
            uns8b *aa;

            if ( ((trp_netptr_t *)cback)->nargs != _trp_iup_callbacks[ i ].nargs ) {
                trp_csprint_free( nn );
                return 1;
            }
            aa = trp_gc_malloc_atomic( strlen( nn ) + 5 );
            sprintf( aa, "TRP_%s", nn );
            IupSetAttribute( h, aa, (char *)(((trp_netptr_t *)cback)->f) );
            trp_gc_free( aa );
            (void)IupSetCallback( h, nn, _trp_iup_callbacks[ i ].cb );
            if ( strcmp( nn, "DRAGBEGIN_CB" ) == 0 ) {
                IupSetAttribute( h, "DRAGSOURCE", "YES" );
                IupSetAttribute( h, "DRAGTYPES", "TEXT" );
                IupSetCallback( h, "DRAGDATASIZE_CB",  (Icallback)testDragDataSize_cb );
                IupSetCallback( h, "DRAGDATA_CB",  (Icallback)testDragData_cb );
            } else if ( strcmp( nn, "DROPDATA_CB" ) == 0 ) {
                IupSetAttribute( h, "DROPTARGET", "YES" );
                IupSetAttribute( h, "DROPTYPES", "TEXT" );
            }
            trp_csprint_free( nn );
            return 0;
        }
    trp_csprint_free( nn );
    return 1;
}

uns8b trp_iup_popup( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y )
{
    Ihandle *h;
    uns32b xx, yy;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_uns32b_range( x, &xx, 0, 100000 ) ||
         trp_cast_uns32b_range( y, &yy, 0, 100000 ) )
        return 1;
    return IupPopup( h, xx, yy ) ? 1 : 0;
}

uns8b trp_iup_show( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    return IupShow( h ) ? 1 : 0;
}

uns8b trp_iup_show_xy( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y )
{
    Ihandle *h;
    uns32b xx, yy;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_uns32b_range( x, &xx, 0, 100000 ) ||
         trp_cast_uns32b_range( y, &yy, 0, 100000 ) )
        return 1;
    return IupShowXY( h, xx, yy ) ? 1 : 0;
}

uns8b trp_iup_hide( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    return IupHide( h ) ? 1 : 0;
}

uns8b trp_iup_map( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    return IupMap( h ) ? 1 : 0;
}

uns8b trp_iup_unmap( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupUnmap( h );
    return 0;
}

uns8b trp_iup_detach( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupDetach( h );
    return 0;
}

uns8b trp_iup_refresh( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupRefresh( h );
    return 0;
}

uns8b trp_iup_refresh_children( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupRefreshChildren( h );
    return 0;
}

uns8b trp_iup_update( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupUpdate( h );
    return 0;
}

uns8b trp_iup_update_children( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupUpdateChildren( h );
    return 0;
}

uns8b trp_iup_redraw( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupRedraw( h, 0 );
    return 0;
}

uns8b trp_iup_redraw_children( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    IupRedraw( h, 1 );
    return 0;
}

uns8b trp_iup_set_focus( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return 1;
    (void)IupSetFocus( h );
    return 0;
}

uns8b trp_iup_append( trp_obj_t *ih, trp_obj_t *new_child )
{
    Ihandle *h, *nc;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         ( ( nc = trp_iup_check( new_child ) ) == NULL ) )
        return 1;
    return IupAppend( h, nc ) ? 0 : 1;
}

uns8b trp_iup_insert( trp_obj_t *ih, trp_obj_t *ref_child, trp_obj_t *new_child )
{
    Ihandle *h, *rc, *nc;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         ( ( rc = trp_iup_check( ref_child ) ) == NULL ) ||
         ( ( nc = trp_iup_check( new_child ) ) == NULL ) )
        return 1;
    return IupInsert( h, rc, nc ) ? 0 : 1;
}

uns8b trp_iup_message( trp_obj_t *title, trp_obj_t *text, ... )
{
    uns8b *ti = trp_csprint( title ), *te;
    va_list args;

    va_start( args, text );
    te = trp_csprint_multi( text, args );
    va_end( args );
    IupMessage( ti, te );
    trp_csprint_free( ti );
    trp_csprint_free( te );
    return 0;
}

trp_obj_t *trp_iup_get_global( trp_obj_t *name )
{
    uns8b *nn = trp_csprint( name ), *vv;

    vv = IupGetGlobal( nn );
    trp_csprint_free( nn );
    return vv ? trp_cord( vv ) : UNDEF;
}

trp_obj_t *trp_iup_get_attribute( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h, *aa;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    aa = (Ihandle *)IupGetAttribute( h, nn );
    trp_csprint_free( nn );
    if ( aa )
        res = trp_iup_handle( aa );
    else
        res = UNDEF;
    return res;
}

trp_obj_t *trp_iup_get_str_attribute( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn, *aa;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    aa = IupGetAttribute( h, nn );
    trp_csprint_free( nn );
    if ( aa )
        res = trp_cord( aa );
    else
        res = UNDEF;
    return res;
}

trp_obj_t *trp_iup_get_int( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    res = trp_sig64( IupGetInt( h, nn ) );
    trp_csprint_free( nn );
    return res;
}

trp_obj_t *trp_iup_get_int2( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    res = trp_sig64( IupGetInt2( h, nn ) );
    trp_csprint_free( nn );
    return res;
}

trp_obj_t *trp_iup_get_double( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    res = trp_double( IupGetDouble( h, nn ) );
    trp_csprint_free( nn );
    return res;
}

trp_obj_t *trp_iup_get_attribute_handle( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h;

    if ( ih == UNDEF )
        h = NULL;
    else  if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    res = trp_iup_handle( IupGetAttributeHandle( h, nn ) );
    trp_csprint_free( nn );
    return res;
}

trp_obj_t *trp_iup_get_child_count( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    return trp_sig64( IupGetChildCount( h ) );
}

trp_obj_t *trp_iup_get_focus()
{
    return trp_iup_handle( IupGetFocus() );
}

trp_obj_t *trp_iup_timer()
{
    return trp_iup_handle( IupTimer() );
}

trp_obj_t *trp_iup_fill()
{
    return trp_iup_handle( IupFill() );
}

trp_obj_t *trp_iup_dialog( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupDialog( h ) );
}

static trp_obj_t *trp_iup_container_low( ihandle_t f, trp_obj_t *child, va_list args )
{
    return trp_iup_container_low_low( 0, f, child, args );
}

static trp_obj_t *trp_iup_container_low_low( uns8b flags, ihandle_t f, trp_obj_t *child, va_list args )
{
    int n = 0;
    trp_obj_t *l = NIL;
    Ihandle **h = NULL;

    for ( ; child ; child = va_arg( args, trp_obj_t * ) ) {
        if ( ( flags & 1 ) && ( child == UNDEF ) )
            continue;
        l = trp_cons( child, l );
        ++n;
    }
    if ( n == 1 ) {
        child = trp_car( l );
        if ( child->tipo == TRP_ARRAY ) {
            trp_gc_free( l );
            if ( ( h = malloc( ( ((trp_array_t *)child)->len + 1 ) * sizeof( Ihandle * ) ) ) == NULL )
                return UNDEF;
            for ( n = 0 ; n < ((trp_array_t *)child)->len ; n++ ) {
                l = ((trp_array_t *)child)->data[ n ];
                if ( trp_iup_check( l ) == NULL ) {
                    free( h );
                    return UNDEF;
                }
                h[ n ] = ((trp_iup_t *)l)->h;
            }
            h[ n ] = NULL;
        } /* else if ... FIXME aggiungere liste o altro  */
    }
    if ( h == NULL ) {
        if ( ( h = malloc( ( n + 1 ) * sizeof( Ihandle * ) ) ) == NULL ) {
            trp_free_list( l );
            return UNDEF;
        }
        h[ n ] = NULL;
        while ( l != NIL ) {
            child = trp_car( l );
            if ( trp_iup_check( child ) == NULL ) {
                free( h );
                trp_free_list( l );
                return UNDEF;
            }
            h[ --n ] = ((trp_iup_t *)child)->h;
            child = trp_cdr( l );
            trp_gc_free( l );
            l = child;
        }
    }
    l = trp_iup_handle( (f)( h ) );
    free( h );
    return l;
}

trp_obj_t *trp_iup_vbox( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupVboxv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_zbox( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupZboxv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_hbox( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupHboxv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_cbox( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupCboxv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_normalizer( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupNormalizerv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_tabs( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low( IupTabsv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_sbox( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupSbox( h ) );
}

trp_obj_t *trp_iup_scroll_box( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupScrollBox( h ) );
}

trp_obj_t *trp_iup_frame( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupFrame( h ) );
}

trp_obj_t *trp_iup_radio( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupRadio( h ) );
}

trp_obj_t *trp_iup_expander( trp_obj_t *child )
{
    Ihandle *h;

    if ( child == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( child ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupExpander( h ) );
}

trp_obj_t *trp_iup_split( trp_obj_t *child1, trp_obj_t *child2 )
{
    Ihandle *h1, *h2;

    if ( ( ( h1 = trp_iup_check( child1 ) ) == NULL ) ||
         ( ( h2 = trp_iup_check( child2 ) ) == NULL ) )
        return UNDEF;
    return trp_iup_handle( IupSplit( h1, h2 ) );
}

trp_obj_t *trp_iup_label( trp_obj_t *title, ... )
{
    uns8b *s;
    va_list args;

    if ( title ) {
        va_start( args, title );
        s = trp_csprint_multi( title, args );
        va_end( args );
    } else
        s = NULL;
    title = trp_iup_handle( IupLabel( s ) );
    if ( s )
        trp_csprint_free( s );
    return title;
}

trp_obj_t *trp_iup_animated_label( trp_obj_t *animation )
{
    Ihandle *h;

    if ( animation == NULL )
        h = NULL;
    else if ( ( h = trp_iup_check( animation ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupAnimatedLabel( h ) );
}

trp_obj_t *trp_iup_button( trp_obj_t *title )
{
    uns8b *tt;

    if ( title )
        tt = trp_csprint( title );
    else
        tt = NULL;
    title = trp_iup_handle( IupButton( tt, NULL ) );
    if ( tt )
        trp_csprint_free( tt );
    return title;
}

trp_obj_t *trp_iup_toggle( trp_obj_t *title )
{
    uns8b *tt;

    if ( title )
        tt = trp_csprint( title );
    else
        tt = NULL;
    title = trp_iup_handle( IupToggle( tt, NULL ) );
    if ( tt )
        trp_csprint_free( tt );
    return title;
}

trp_obj_t *trp_iup_list()
{
    return trp_iup_handle( IupList( NULL ) );
}

trp_obj_t *trp_iup_flatlist()
{
    return trp_iup_handle( IupFlatList() );
}

trp_obj_t *trp_iup_text()
{
    return trp_iup_handle( IupText( NULL ) );
}

trp_obj_t *trp_iup_user()
{
    return trp_iup_handle( IupUser() );
}

trp_obj_t *trp_iup_clipboard()
{
    return trp_iup_handle( IupClipboard() );
}

trp_obj_t *trp_iup_progress_bar()
{
    return trp_iup_handle( IupProgressBar() );
}

trp_obj_t *trp_iup_item( trp_obj_t *title )
{
    uns8b *ti = trp_csprint( title );

    title = trp_iup_handle( IupItem( ti, NULL ) );
    trp_csprint_free( ti );
    return title;
}

trp_obj_t *trp_iup_submenu( trp_obj_t *title, trp_obj_t *child )
{
    Ihandle *h;
    uns8b *ti;
    trp_obj_t *res;

    if ( child ) {
        if ( ( h = trp_iup_check( child ) ) == NULL )
            return UNDEF;
    } else
        h = NULL;
    ti = trp_csprint( title );
    res = trp_iup_handle( IupSubmenu( ti, h ) );
    trp_csprint_free( ti );
    return res;
}

trp_obj_t *trp_iup_separator()
{
    return trp_iup_handle( IupSeparator() );
}

trp_obj_t *trp_iup_menu( trp_obj_t *child, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, child );
    res = trp_iup_container_low_low( 1, IupMenuv, child, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_iup_date_pick()
{
    return trp_iup_handle( IupDatePick() );
}

trp_obj_t *trp_iup_val()
{
    return trp_iup_handle( IupVal( NULL ) );
}

trp_obj_t *trp_iup_calendar()
{
    return trp_iup_handle( IupCalendar() );
}

trp_obj_t *trp_iup_image_rgba( trp_obj_t *pix )
{
    uns8b *p;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ( p = ((trp_pix_t *)pix)->map.p ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupImageRGBA( ((trp_pix_t *)pix)->w, ((trp_pix_t *)pix)->h, p ) );
}

trp_obj_t *trp_iup_file_dlg()
{
    return trp_iup_handle( IupFileDlg() );
}

trp_obj_t *trp_iup_message_dlg()
{
    return trp_iup_handle( IupMessageDlg() );
}

trp_obj_t *trp_iup_color_dlg()
{
    return trp_iup_handle( IupColorDlg() );
}

trp_obj_t *trp_iup_font_dlg()
{
    return trp_iup_handle( IupFontDlg() );
}

trp_obj_t *trp_iup_progress_dlg()
{
    return trp_iup_handle( IupProgressDlg() );
}

trp_obj_t *trp_iup_get_brother( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupGetBrother( h ) );
}

trp_obj_t *trp_iup_get_parent( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupGetParent( h ) );
}

trp_obj_t *trp_iup_get_dialog( trp_obj_t *ih )
{
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    return trp_iup_handle( IupGetDialog( h ) );
}

trp_obj_t *trp_iup_get_dialog_child( trp_obj_t *ih, trp_obj_t *name )
{
    trp_obj_t *res;
    uns8b *nn;
    Ihandle *h;

    if ( ( h = trp_iup_check( ih ) ) == NULL )
        return UNDEF;
    nn = trp_csprint( name );
    res = trp_iup_handle( IupGetDialogChild( h, nn ) );
    trp_csprint_free( nn );
    return res;
}

trp_obj_t *trp_iup_text_convert_lin_col_to_pos( trp_obj_t *ih, trp_obj_t *lin, trp_obj_t *col )
{
    int pos;
    uns32b ll, cc;
    Ihandle *h;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_uns32b( lin, &ll ) ||
         trp_cast_uns32b( col, &cc ) )
        return UNDEF;
    IupTextConvertLinColToPos( h, ll, cc, &pos );
    return trp_sig64( pos );
}

trp_obj_t *trp_iup_text_convert_pos_to_lin_col( trp_obj_t *ih, trp_obj_t *pos )
{
    int lin, col;
    uns32b pp;
    Ihandle *h;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_uns32b( pos, &pp ) )
        return UNDEF;
    IupTextConvertPosToLinCol( h, pp, &lin, &col );
    return trp_cons( trp_sig64( lin ), trp_sig64( col ) );
}

trp_obj_t *trp_iup_convert_xy_to_pos( trp_obj_t *ih, trp_obj_t *x, trp_obj_t *y )
{
    uns32b xx, yy;
    Ihandle *h;

    if ( ( ( h = trp_iup_check( ih ) ) == NULL ) ||
         trp_cast_uns32b( x, &xx ) ||
         trp_cast_uns32b( y, &yy ) )
        return UNDEF;
    return trp_sig64( IupConvertXYToPos( h, xx, yy ) );
}



