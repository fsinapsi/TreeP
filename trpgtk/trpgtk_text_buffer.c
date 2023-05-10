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

#include "./trpgtk_internal.h"
#include "../trppix/trppix_internal.h"

static void trp_gtk_text_buffer_insert_internal( uns8b set, trp_obj_t *obj, trp_obj_t *color, va_list args );

static void trp_gtk_text_buffer_insert_internal( uns8b set, trp_obj_t *obj, trp_obj_t *color, va_list args )
{
    GtkTextBuffer *o = (GtkTextBuffer *)trp_gtk_get_widget( obj );
    if ( o ) {
        if ( GTK_IS_TEXT_VIEW( (GtkWidget *)o ) )
            o = gtk_text_view_get_buffer( (GtkTextView *)o );
        if ( GTK_IS_TEXT_BUFFER( o ) ) {
            GtkTextTag *tag;
            uns8b *p;
            GtkTextIter end_iter;

            if ( color && ( color != UNDEF ) ) {
                uns8b tagname[ 14 ];
                uns16b r, g, b, a;

                if ( trp_pix_decode_color( color, &r, &g, &b, &a ) )
                    return;
                sprintf( tagname, "c%04x%04x%04x", (int)r, (int)g, (int)b );
                tag = gtk_text_tag_table_lookup( gtk_text_buffer_get_tag_table( o ), tagname );
                if ( tag == NULL ) {
                    GdkColor c;

                    c.pixel = 0;
                    c.red = r;
                    c.green = g;
                    c.blue = b;
                    tag = gtk_text_buffer_create_tag( o, tagname, "foreground-gdk", &c, NULL );
                }
            } else
                tag = NULL;
            if ( set )
                gtk_text_buffer_set_text( o, "", 0 );
            for ( obj = va_arg( args, trp_obj_t * ) ;
                  obj ;
                  obj = va_arg( args, trp_obj_t * ) ) {
                gtk_text_buffer_get_end_iter( o, &end_iter );
                switch ( obj->tipo ) {
                case TRP_CORD:
                    if ( tag )
                        gtk_text_buffer_insert_with_tags( o, &end_iter, CORD_to_const_char_star( ((trp_cord_t *)obj)->c ), -1, tag, NULL );
                    else
                        gtk_text_buffer_insert( o, &end_iter, CORD_to_const_char_star( ((trp_cord_t *)obj)->c ), -1 );
                    break;
                default:
                    p = trp_csprint( obj );
                    if ( tag )
                        gtk_text_buffer_insert_with_tags( o, &end_iter, p, -1, tag, NULL );
                    else
                        gtk_text_buffer_insert( o, &end_iter, p, -1 );
                    trp_csprint_free( p );
                    break;
                }
            }
        }
    }
}

void trp_gtk_text_buffer_append( trp_obj_t *obj, ... )
{
    va_list args;

    va_start( args, obj );
    trp_gtk_text_buffer_insert_internal( 0, obj, NULL, args );
    va_end( args );
}

void trp_gtk_text_buffer_append_color( trp_obj_t *obj, trp_obj_t *color, ... )
{
    va_list args;

    va_start( args, color );
    trp_gtk_text_buffer_insert_internal( 0, obj, color, args );
    va_end( args );
}

trp_obj_t *trp_gtk_text_buffer_get_text( trp_obj_t *obj, trp_obj_t *include_hidden_chars )
{
    GtkTextBuffer *o = (GtkTextBuffer *)trp_gtk_get_widget( obj );
    uns8b *p;
    gboolean i;
    GtkTextIter start, end;

    if ( o == NULL )
        return UNDEF;
    if ( GTK_IS_TEXT_VIEW( (GtkWidget *)o ) )
        o = gtk_text_view_get_buffer( (GtkTextView *)o );
    if ( !GTK_IS_TEXT_BUFFER( o ) )
        return UNDEF;
    if ( include_hidden_chars ) {
        if ( !TRP_BOOLP( include_hidden_chars ) )
            return UNDEF;
        i = BOOLVAL( include_hidden_chars );
    } else
        i = TRUE;
    gtk_text_buffer_get_bounds( o, &start, &end );
    p = (uns8b *)gtk_text_buffer_get_text( o, &start, &end, i );
    obj = trp_cord( p );
    free( p );
    return obj;
}

void trp_gtk_text_buffer_set_text( trp_obj_t *obj, ... )
{
    va_list args;

    va_start( args, obj );
    trp_gtk_text_buffer_insert_internal( 1, obj, NULL, args );
    va_end( args );
}

void trp_gtk_text_buffer_set_text_color( trp_obj_t *obj, trp_obj_t *color, ... )
{
    va_list args;

    va_start( args, color );
    trp_gtk_text_buffer_insert_internal( 1, obj, color, args );
    va_end( args );
}

void trp_gtk_text_buffer_select_all( trp_obj_t *obj )
{
    GtkTextBuffer *o = (GtkTextBuffer *)trp_gtk_get_widget( obj );
    if ( o ) {
        if ( GTK_IS_TEXT_VIEW( (GtkWidget *)o ) )
            o = gtk_text_view_get_buffer( (GtkTextView *)o );
        if ( GTK_IS_TEXT_BUFFER( o ) ) {
            GtkTextIter start, end;

            gtk_text_buffer_get_bounds( o, &start, &end );
            gtk_text_buffer_select_range( o, &start, &end );
        }
    }
}

