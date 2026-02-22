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

#include "../trp/trp.h"
#include "./trpdbf.h"
#include "libdbf/libdbf.h"
#include "libdbf/dbf.h"

typedef struct {
    uns8b tipo;
    P_DBF *db;
} trp_dbf_t;

static uns8b trp_dbf_close( trp_dbf_t *obj );
static uns8b trp_dbf_close_basic( uns8b flags, trp_dbf_t *obj );
static void trp_dbf_finalize( void *obj, void *data );
static P_DBF *trp_dbf_dbf( trp_obj_t *db );
static trp_obj_t *trp_dbf_width( trp_obj_t *db );
static trp_obj_t *trp_dbf_height( trp_obj_t *db );

uns8b trp_dbf_init()
{
    extern uns8bfun_t _trp_close_fun[];
    extern objfun_t _trp_width_fun[];
    extern objfun_t _trp_height_fun[];

    _trp_close_fun[ TRP_DBF ] = trp_dbf_close;
    _trp_width_fun[ TRP_DBF ] = trp_dbf_width;
    _trp_height_fun[ TRP_DBF ] = trp_dbf_height;
    return 0;
}

static uns8b trp_dbf_close( trp_dbf_t *obj )
{
    return trp_dbf_close_basic( 1, obj );
}

static uns8b trp_dbf_close_basic( uns8b flags, trp_dbf_t *obj )
{
    if ( obj->db ) {
        if ( flags & 1 )
            trp_gc_remove_finalizer( (trp_obj_t *)obj );
        dbf_Close( obj->db );
        obj->db = NULL;
    }
    return 0;
}

static void trp_dbf_finalize( void *obj, void *data )
{
    trp_dbf_close_basic( 0, (trp_dbf_t *)obj );
}

static P_DBF *trp_dbf_dbf( trp_obj_t *db )
{
    if ( db->tipo != TRP_DBF )
        return NULL;
    return ((trp_dbf_t *)db)->db;
}

static trp_obj_t *trp_dbf_width( trp_obj_t *db )
{
    P_DBF *dbf = trp_dbf_dbf( db );

    if ( dbf == NULL )
        return UNDEF;
    return trp_sig64( dbf->columns );
}

static trp_obj_t *trp_dbf_height( trp_obj_t *db )
{
    P_DBF *dbf = trp_dbf_dbf( db );

    if ( dbf == NULL )
        return UNDEF;
    return trp_sig64( dbf->header->records );
}

trp_obj_t *trp_dbf_open( trp_obj_t *path )
{
    trp_dbf_t *obj;
    uns8b *cpath = trp_csprint( path );
    P_DBF *db;

    db = dbf_Open( cpath );
    trp_csprint_free( cpath );
    if ( db == NULL )
        return UNDEF;
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_dbf_t ), trp_dbf_finalize );
    obj->tipo = TRP_DBF;
    obj->db = db;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_dbf_get_string_version( trp_obj_t *db )
{
    P_DBF *dbf = trp_dbf_dbf( db );

    if ( dbf == NULL )
        return UNDEF;
    return trp_cord( dbf_GetStringVersion( dbf ) );
}

trp_obj_t *trp_dbf_create_from_dbf( trp_obj_t *path, trp_obj_t *db )
{
    trp_dbf_t *obj;
    P_DBF *dbf = trp_dbf_dbf( db );
    uns8b *cpath;
    DB_FIELD *new_fields;

    if ( dbf == NULL )
        return UNDEF;
    new_fields = malloc( sizeof( DB_FIELD ) * dbf->columns );
    if ( new_fields == NULL )
        return UNDEF;
    memcpy( new_fields, dbf->fields, sizeof( DB_FIELD ) * dbf->columns );
    cpath = trp_csprint( path );
    dbf = dbf_Create( cpath, new_fields, dbf->columns );
    trp_csprint_free( cpath );
    if ( dbf == NULL ) {
        free( new_fields );
        return UNDEF;
    }
    obj = trp_gc_malloc_atomic_finalize( sizeof( trp_dbf_t ), trp_dbf_finalize );
    obj->tipo = TRP_DBF;
    obj->db = dbf;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_dbf_column_name( trp_obj_t *db, trp_obj_t *idx )
{
    P_DBF *dbf = trp_dbf_dbf( db );
    uns32b i;

    if ( dbf == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, dbf->columns - 1 ) )
        return UNDEF;
    return trp_cord( dbf_ColumnName( dbf, i ) );
}

trp_obj_t *trp_dbf_column_size( trp_obj_t *db, trp_obj_t *idx )
{
    P_DBF *dbf = trp_dbf_dbf( db );
    uns32b i;

    if ( dbf == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, dbf->columns - 1 ) )
        return UNDEF;
    return trp_sig64( dbf_ColumnSize( dbf, i ) );
}

trp_obj_t *trp_dbf_column_type( trp_obj_t *db, trp_obj_t *idx )
{
    P_DBF *dbf = trp_dbf_dbf( db );
    uns32b i;

    if ( dbf == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, dbf->columns - 1 ) )
        return UNDEF;
    return trp_char( dbf_ColumnType( dbf, i ) );
}

trp_obj_t *trp_dbf_column_decimals( trp_obj_t *db, trp_obj_t *idx )
{
    P_DBF *dbf = trp_dbf_dbf( db );
    uns32b i;

    if ( dbf == NULL )
        return UNDEF;
    if ( trp_cast_uns32b_range( idx, &i, 0, dbf->columns - 1 ) )
        return UNDEF;
    return trp_sig64( dbf_ColumnDecimals( dbf, i ) );
}

trp_obj_t *trp_dbf_get_date( trp_obj_t *db )
{
    P_DBF *dbf = trp_dbf_dbf( db );
    char date[ 11 ];

    if ( dbf == NULL )
        return UNDEF;
    if ( dbf_GetDate( dbf, date ) < 0 )
        return UNDEF;
    return trp_cord( date );
}

uns8b trp_dbf_copy_record( trp_obj_t *db_dst, trp_obj_t *db_src )
{
    P_DBF *dbf_dst = trp_dbf_dbf( db_dst );
    P_DBF *dbf_src = trp_dbf_dbf( db_src );
    char *record;
    int len;

    if ( ( dbf_dst == NULL ) || ( dbf_src == NULL ) )
        return 1;
    len = dbf_RecordLength( dbf_src );
    record = malloc( len );
    if ( record == NULL )
        return 1;
    if ( dbf_ReadRecord( dbf_src, record, len ) < 0 ) {
        free( record );
        return 1;
    }
    if ( dbf_WriteRecord( dbf_dst, record + 1, len - 1 ) < 0 ) {
        free( record );
        return 1;
    }
    free( record );
    return 0;
}

