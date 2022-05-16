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

#include "./trpgtk_internal.h"
#include "../trppix/trppix_internal.h"

static void trp_gtk_destroy_map( guchar *pixels, gpointer data );
static void trp_gtk_destroy( GtkWidget *w, gpointer pbuf );
static void trp_gtk_image_copy_swap( uns32b w, uns32b h, trp_pix_color_t *src, trp_pix_color_t *dst );
static trp_pix_color_t *trp_gtk_image_create_copy_and_swap( uns32b w, uns32b h, trp_pix_color_t *map );

static void trp_gtk_destroy_map( guchar *pixels, gpointer data )
{
    free( pixels );
}

static void trp_gtk_destroy( GtkWidget *w, gpointer pbuf )
{
    g_object_unref( pbuf );
}

static void trp_gtk_image_copy_swap( uns32b w, uns32b h, trp_pix_color_t *src, trp_pix_color_t *dst )
{
    uns32b i;

    for ( i = w * h ; i ; ) {
        i--;
        dst[ i ].red = src[ i ].red;
        dst[ i ].green = src[ i ].green;
        dst[ i ].blue = src[ i ].blue;
        dst[ i ].alpha = src[ i ].alpha;
    }
}

static trp_pix_color_t *trp_gtk_image_create_copy_and_swap( uns32b w, uns32b h, trp_pix_color_t *map )
{
    trp_pix_color_t *p = malloc( ( w * h ) << 2 );

    if ( p )
        trp_gtk_image_copy_swap( w, h, map, p );
    return p;
}

trp_obj_t *trp_gtk_image_new( trp_obj_t *width, trp_obj_t *height )
{
    GtkWidget *z;
    uns32b w, h;
    uns8b *map;
    GdkPixbuf *pbuf;
    trp_obj_t *res;

    if ( trp_cast_uns32b( width, &w ) ||
         trp_cast_uns32b( height, &h ) )
        return UNDEF;
    if ( ( w == 0 ) ||
         ( h == 0 ) )
        return UNDEF;
    if ( ( map = malloc( 4 * w * h ) ) == NULL )
        return UNDEF;
    memset( map, 0xff, 4 * w * h );
    pbuf = gdk_pixbuf_new_from_data( (guchar *)map, GDK_COLORSPACE_RGB,
                                     TRUE, 8, w, h, w << 2,
                                     (GdkPixbufDestroyNotify)trp_gtk_destroy_map,
                                     NULL );
    if ( pbuf == NULL ) {
        free( map );
        return UNDEF;
    }
    z = gtk_image_new_from_pixbuf( pbuf );
    res = trp_gtk_widget( z );
    ((trp_gtk_t *)res)->id = g_signal_connect( (gpointer)z, "destroy",
                                               (GCallback)trp_gtk_destroy,
                                               (gpointer)pbuf );
    return res;
}

trp_obj_t *trp_gtk_image_new_from_pixbuf( trp_obj_t *pix )
{
    GtkWidget *z;
    uns32b w, h;
    trp_pix_color_t *map;
    GdkPixbuf *pbuf;
    trp_obj_t *res;

    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.p == NULL )
        return UNDEF;
    w = ((trp_pix_t *)pix)->w;
    h = ((trp_pix_t *)pix)->h;
    map = trp_gtk_image_create_copy_and_swap( w, h, ((trp_pix_t *)pix)->map.c );
    if ( map == NULL )
        return UNDEF;
    pbuf = gdk_pixbuf_new_from_data( (guchar *)map, GDK_COLORSPACE_RGB,
                                     TRUE, 8, w, h, w << 2,
                                     (GdkPixbufDestroyNotify)trp_gtk_destroy_map,
                                     NULL );
    if ( pbuf == NULL ) {
        free( map );
        return UNDEF;
    }
    z = gtk_image_new_from_pixbuf( pbuf );
    res = trp_gtk_widget( z );
    ((trp_gtk_t *)res)->id = g_signal_connect( (gpointer)z, "destroy",
                                               (GCallback)trp_gtk_destroy,
                                               (gpointer)pbuf );
    return res;
}

trp_obj_t *trp_gtk_image_new_from_file( trp_obj_t *path )
{
    uns8b *p = trp_csprint( path );
    GdkPixbufAnimation *pbufanim;
    GdkPixbuf *pbuf;

    pbufanim = gdk_pixbuf_animation_new_from_file( p, NULL );
    if ( pbufanim ) {
        GtkWidget *z;
        trp_obj_t *res;

        trp_csprint_free( p );
        z = gtk_image_new_from_animation( pbufanim );
        res = trp_gtk_widget( z );
        ((trp_gtk_t *)res)->id = g_signal_connect( (gpointer)z, "destroy",
                                                   (GCallback)trp_gtk_destroy,
                                                   (gpointer)pbufanim );
        return res;
    }
    pbuf = gdk_pixbuf_new_from_file( p, NULL );
    if ( pbuf ) {
        GtkWidget *z;
        trp_obj_t *res;

        trp_csprint_free( p );
        z = gtk_image_new_from_pixbuf( pbuf );
        res = trp_gtk_widget( z );
        ((trp_gtk_t *)res)->id = g_signal_connect( (gpointer)z, "destroy",
                                                   (GCallback)trp_gtk_destroy,
                                                   (gpointer)pbuf );
        return res;
    }
    {
        /*
         l'ultima spiaggia: se finora non ha funzionato niente,
         si prova a vedere se trp_pix ha un loader che vada bene
         in questo caso...
         */
        trp_obj_t *pix = trp_pix_load_basic( p );
        if ( pix != UNDEF ) {
            trp_obj_t *res = trp_gtk_image_new_from_pixbuf( pix );
            trp_close_multi( pix, NULL );
            trp_gc_free( pix );
            if ( res != UNDEF ) {
                trp_csprint_free( p );
                return res;
            }
        }
    }
    trp_csprint_free( p );
    return UNDEF;
}

void trp_gtk_image_set_from_pixbuf( trp_obj_t *obj, trp_obj_t *pix )
{
    GtkWidget *im = trp_gtk_get_widget( obj );

    if ( im )
        if ( GTK_IS_IMAGE( im ) ) {
            uns32b w, h;
            trp_pix_color_t *map;
            GdkPixbufAnimation *pbufanim = NULL;
            GdkPixbuf *pbuf = NULL, *npbuf;

            switch ( gtk_image_get_storage_type( (GtkImage *)im ) ) {
            case GTK_IMAGE_ANIMATION:
                pbufanim = gtk_image_get_animation( (GtkImage *)im );
                break;
            case GTK_IMAGE_PIXBUF:
                pbuf = gtk_image_get_pixbuf( (GtkImage *)im );
                break;
            default:
                return;
            }
            if ( pix->tipo != TRP_PIX )
                return;
            if ( ((trp_pix_t *)pix)->map.p == NULL )
                return;
            w = ((trp_pix_t *)pix)->w;
            h = ((trp_pix_t *)pix)->h;
            map = ((trp_pix_t *)pix)->map.c;
            if ( pbuf ) {
                if ( ( gdk_pixbuf_get_width( pbuf ) == w ) &&
                     ( gdk_pixbuf_get_height( pbuf ) == h ) &&
                     ( gdk_pixbuf_get_colorspace( pbuf ) == GDK_COLORSPACE_RGB ) &&
                     ( gdk_pixbuf_get_bits_per_sample( pbuf ) == 8 ) &&
                     ( gdk_pixbuf_get_n_channels( pbuf ) == 4 ) &&
                     ( gdk_pixbuf_get_has_alpha( pbuf ) == TRUE ) &&
                     ( gdk_pixbuf_get_rowstride( pbuf ) == w << 2 ) ) {
                    trp_gtk_image_copy_swap( w, h,
                                             map,
                                             (trp_pix_color_t *)gdk_pixbuf_get_pixels( pbuf ) );
                    if ( im->window )
                        gdk_window_invalidate_rect( im->window, NULL, FALSE );
                    return;
                }
            }
            map = trp_gtk_image_create_copy_and_swap( w, h, map );
            if ( map == NULL )
                return;
            npbuf = gdk_pixbuf_new_from_data( (guchar *)map, GDK_COLORSPACE_RGB,
                                              TRUE, 8, w, h, w << 2,
                                              (GdkPixbufDestroyNotify)trp_gtk_destroy_map,
                                              NULL );
            if ( npbuf == NULL ) {
                free( map );
                return;
            }
            gtk_image_set_from_pixbuf( (GtkImage *)im, npbuf );
            g_signal_handler_disconnect( (gpointer)im, ((trp_gtk_t *)obj)->id );
            ((trp_gtk_t *)obj)->id = g_signal_connect( (gpointer)im, "destroy",
                                                       (GCallback)trp_gtk_destroy,
                                                       (gpointer)npbuf );
            if ( pbuf )
                g_object_unref( (gpointer)pbuf );
            else if ( pbufanim )
                g_object_unref( (gpointer)pbufanim );
        }
}

trp_obj_t *trp_gtk_image_get_image( trp_obj_t *obj )
{
    GtkWidget *im = trp_gtk_get_widget( obj );
    trp_obj_t *res = UNDEF;

    if ( im )
        if ( GTK_IS_IMAGE( im ) )
            if ( gtk_image_get_storage_type( (GtkImage *)im ) == GTK_IMAGE_PIXBUF ) {
                GdkPixbuf *pbuf = gtk_image_get_pixbuf( (GtkImage *)im );
                if ( ( gdk_pixbuf_get_colorspace( pbuf ) == GDK_COLORSPACE_RGB ) &&
                     ( gdk_pixbuf_get_bits_per_sample( pbuf ) == 8 ) &&
                     ( gdk_pixbuf_get_n_channels( pbuf ) == 4 ) &&
                     ( gdk_pixbuf_get_has_alpha( pbuf ) == TRUE ) &&
                     ( gdk_pixbuf_get_rowstride( pbuf ) == gdk_pixbuf_get_width( pbuf ) << 2 ) )
                    res = trp_pix_create_image_from_data( 1,
                                                          (uns32b)gdk_pixbuf_get_width( pbuf ),
                                                          (uns32b)gdk_pixbuf_get_height( pbuf ),
                                                          (uns8b *)gdk_pixbuf_get_pixels( pbuf ) );
            }
    return res;
}

