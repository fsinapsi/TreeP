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
#include "./trpid3tag.h"
#include <id3tag.h>

static trp_obj_t *trp_id3tag_ucs4_to_cord_internal( const id3_ucs4_t *u, const uns8b *v );
#define trp_id3tag_ucs4_to_cord(u) trp_id3tag_ucs4_to_cord_internal((u),NULL)
#define trp_id3tag_uns8b_to_cord(v) trp_id3tag_ucs4_to_cord_internal(NULL,(v))

static trp_obj_t *trp_id3tag_ucs4_to_cord_internal( const id3_ucs4_t *u, const uns8b *v )
{
    trp_obj_t *res = NULL;

    if ( u || v ) {
        uns8b *s;

        if ( u )
            s = id3_ucs4_utf8duplicate( u );
        else
            s = (uns8b *)strdup( v );
        if ( s ) {
            int i;

            for ( i = strlen( s ) ; i ; ) {
                i--;
                if ( ( s[ i ] != ' ' ) &&
                     ( s[ i ] != '\r' ) &&
                     ( s[ i ] != '\n' ) &&
                     ( s[ i ] != '\t' ) ) {
                    i++;
                    break;
                }
            }
            if ( i ) {
                s[ i ] = 0;
                res = trp_cord( s );
            }
            free( s );
        }
    }
    return res;
}

trp_obj_t *trp_id3tag_file( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    FILE *fp;
    const uns8b *s;
    trp_obj_t *res = NIL, *val, *val1, *val2;
    struct id3_file *f;
    struct id3_tag *tag;
    struct id3_frame *frame;
    union id3_field *field;
    const id3_ucs4_t *u;
    uns32b i, j;

    fp = trp_fopen( cpath, "rb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return UNDEF;
    f = id3_file_fdopen( fileno( fp ), ID3_FILE_MODE_READONLY );
    if ( f == NULL ) {
        fclose( fp );
        return UNDEF;
    }
    tag = id3_file_tag( f );
    for ( i = tag->nframes ; i ; ) {
        frame = tag->frames[ --i ];
        val = NIL;
        if ( ( frame->id[ 0 ] == 'T' ) &&
             ( frame->nfields == 2 ) ) {
            int genre = ( strcmp( frame->id, "TCON" ) == 0 );
            field = &( frame->fields[ 1 ] );
            for ( j = id3_field_getnstrings( field ) ; j ; ) {
                if ( u = id3_field_getstrings( field, --j ) ) {
                    if ( genre ) {
                        int gn = id3_genre_number( u );
                        if ( gn != -1 ) {
                            const id3_ucs4_t *newu = id3_genre_index( gn );
                            if ( newu )
                                u = newu;
                        }
                    }
                    if ( val1 = trp_id3tag_ucs4_to_cord( u ) )
                        val = trp_cons( val1, val );
                }
            }
        } else if ( ( strcmp( frame->id, "COMM" ) == 0 ) &&
                    ( frame->nfields == 4 ) ) {
            /*
             FIXME
             considerare anche il linguaggio (in frame->fields[ 1 ])
             */
            val1 = trp_id3tag_ucs4_to_cord( id3_field_getstring( &( frame->fields[ 2 ] ) ) );
            val2 = trp_id3tag_ucs4_to_cord( id3_field_getfullstring( &( frame->fields[ 3 ] ) ) );
            if ( val1 || val2 )
                val = trp_cons( val1 ? val1 : EMPTYCORD, val2 ? val2 : EMPTYCORD );
        } else if ( ( strcmp( frame->id, "WXXX" ) == 0 ) &&
                    ( frame->nfields == 3 ) ) {
            val1 = trp_id3tag_ucs4_to_cord( id3_field_getstring( &( frame->fields[ 1 ] ) ) );
            val2 = trp_id3tag_uns8b_to_cord( id3_field_getlatin1( &( frame->fields[ 2 ] ) ) );
            if ( val2 )
                val = trp_cons( val2, val );
            if ( val1 )
                val = trp_cons( val1, val );
        } else if ( ( ( strcmp( frame->id, "UFID" ) == 0 ) ||
                      ( strcmp( frame->id, "PRIV" ) == 0 ) ) &&
                    ( frame->nfields == 2 ) ) {
            val1 = trp_id3tag_uns8b_to_cord( id3_field_getlatin1( &( frame->fields[ 0 ] ) ) );
            if ( val1 )
                val = trp_cons( val1, NIL );
        } else if ( ( ( strcmp( frame->id, "ZOBS" ) == 0 ) ) &&
                    ( frame->nfields == 2 ) ) {
            if ( s = id3_field_getframeid( &( frame->fields[ 0 ] ) ) )
                if ( s[ 0 ] )
                    val = trp_cons( trp_cord( s ), NIL );
        } else {
            val = trp_cons( trp_cord( "value not displayed" ), NIL );
        }
        if ( val != NIL )
            res = trp_cons( trp_list( trp_cord( frame->id ),
                                      trp_cord( frame->description ),
                                      val,
                                      NULL ),
                            res );
    }
    id3_file_close( f );
    fclose( fp );
    return res;
}

