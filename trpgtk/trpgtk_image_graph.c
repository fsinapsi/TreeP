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

#include "./trpgtk_internal.h"
#include "../trpvid/trpvid_internal.h"

static void trp_gtk_graph_calculate_max_avg_frame_size( trp_vid_t *vid );
static void trp_gtk_graph_internal( int tipo, trp_vid_t *vid, trp_obj_t *im, trp_obj_t *framecnt,
                                    trp_obj_t *mag, trp_obj_t *avrg_int,
                                    trp_obj_t *vbv_size, trp_obj_t *vbv_init, trp_obj_t *vbv_rate, trp_obj_t *fps );

void trp_gtk_graph_qscale( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt )
{
    trp_gtk_graph_internal( 0, (trp_vid_t *)obj, im, framecnt, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF );
}

void trp_gtk_graph_size( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt, trp_obj_t *mag, trp_obj_t *avg_int )
{
    trp_gtk_graph_internal( 1, (trp_vid_t *)obj, im, framecnt, mag, avg_int, UNDEF, UNDEF, UNDEF, UNDEF );
}

void trp_gtk_graph_vbvfill( trp_obj_t *obj, trp_obj_t *im, trp_obj_t *framecnt, trp_obj_t *vbv_size, trp_obj_t *vbv_init, trp_obj_t *vbv_rate, trp_obj_t *fps )
{
    trp_gtk_graph_internal( 2, (trp_vid_t *)obj, im, framecnt, UNDEF, UNDEF, vbv_size, vbv_init, vbv_rate, fps );
}

static void trp_gtk_graph_calculate_max_avg_frame_size( trp_vid_t *vid )
{
    if ( vid->max_frame_size == -1 ) {
        uns32b i, cnt = 0;
        sig32b size;
        uns64b tot = 0;

        for ( i = 0 ; i < vid->cnt_vop ; i++ ) {
            if ( size = vid->qscale[ i ].size ) {
                cnt++;
                tot += size;
            }
            if ( vid->max_frame_size < size )
                vid->max_frame_size = size;
        }
        vid->avg_frame_size = ( tot + cnt / 2 ) / cnt;
    }
}

static void trp_gtk_graph_internal( int tipo, trp_vid_t *vid, trp_obj_t *im, trp_obj_t *framecnt,
                                    trp_obj_t *mag, trp_obj_t *avrg_int,
                                    trp_obj_t *v_size, trp_obj_t *v_init, trp_obj_t *v_rate, trp_obj_t *fps )
{
    GtkWidget *wim = trp_gtk_get_widget( im );
    GdkPixbuf *pbuf;
    double dmag;
    uns32b fcnt, avg_int, vbv_size, vbv_init, vbv_rate, w, h, typ, val, x, y;
    uns32b level0, level1, level2, level_max;
    uns32b buf_tot = 0, buf_len = 0, rr0;
    uns8b *map, *p, *p0, *p1, *p2, r0, g0, b0, r, g, b, i_frame_type;

    if ( ( wim == NULL ) ||
         ( vid->tipo != TRP_VID ) ||
         trp_cast_uns32b( framecnt, &fcnt ) )
        return;
    if ( ( !GTK_IS_IMAGE( wim ) ) ||
         ( vid->fp == NULL ) )
        return;
    if ( gtk_image_get_storage_type( (GtkImage *)wim ) != GTK_IMAGE_PIXBUF )
        return;
    pbuf = gtk_image_get_pixbuf( (GtkImage *)wim );
    if ( ( gdk_pixbuf_get_colorspace( pbuf ) != GDK_COLORSPACE_RGB ) ||
         ( gdk_pixbuf_get_bits_per_sample( pbuf ) != 8 ) ||
         ( gdk_pixbuf_get_n_channels( pbuf ) != 4 ) ||
         ( gdk_pixbuf_get_has_alpha( pbuf ) != TRUE ) ||
         ( gdk_pixbuf_get_rowstride( pbuf ) != gdk_pixbuf_get_width( pbuf ) << 2 ) )
        return;
    switch ( tipo ) {
    case 0:
        if ( vid->bitstream_type == 3 ) {
            level0 = 10;
            level1 = 20;
            level2 = 40;
            level_max = 45;
        } else {
            level0 = 4;
            level1 = 8;
            level2 = 16;
            level_max = 18;
        }
        r0 = 0x00;
        g0 = 0xbb;
        b0 = 0x00;
        break;
    case 1:
        trp_gtk_graph_calculate_max_avg_frame_size( vid );
        if ( trp_cast_double( mag, &dmag ) ||
             trp_cast_uns32b( avrg_int, &avg_int ) ||
             ( vid->max_frame_size == -1 ) )
            return;
        if ( dmag < 1.0 )
            return;
        level0 = level1 = vid->avg_frame_size;
        level2 = vid->max_frame_size;
        level_max = ( 11 * vid->max_frame_size + 5 ) / 10;
        level_max = (uns32b) ( ( (double)level_max ) / dmag + 0.5 );
        if ( avg_int ) {
            w = ( fcnt > avg_int ) ? fcnt - avg_int : 0;
            h = fcnt + avg_int - 1;
            if ( h >= vid->cnt_vop )
                h = vid->cnt_vop - 1;
            for ( x = w ; x <= h ; x++ ) {
                buf_tot += vid->qscale[ x ].size;
                buf_len++;
            }
        }
        r0 = 0x00;
        g0 = 0xbb;
        b0 = 0x00;
        break;
    case 2:
        if ( trp_cast_uns32b( v_size, &vbv_size ) ||
             trp_cast_uns32b( v_init, &vbv_init ) ||
             trp_cast_uns32b( v_rate, &vbv_rate ) ||
             trp_cast_double( fps, &dmag ) ||
             ( vid->cnt_vop == 0 ) )
            return;
        if ( dmag <= 0.0 )
            return;
        vbv_size = vbv_size >> 3;
        vbv_init = vbv_init >> 3;
        rr0 = (uns32b)( ( ( ( (double)vbv_rate ) / dmag ) / 8.0 ) + 0.5 );
        if ( fcnt > vid->cnt_vop )
            fcnt = vid->cnt_vop;
        buf_tot = vbv_init;
        for ( x = 0 ; x < fcnt ; x++ ) {
            buf_tot += rr0;
            val = vid->qscale[ x ].size;
            if ( buf_tot >= val ) {
                buf_tot -= val;
                if ( buf_tot > vbv_size )
                    buf_tot = vbv_size;
            } else {
                buf_tot = vbv_init;
            }
        }
        level0 = level1 = level2 = vbv_size;
        level_max = ( 11 * vbv_size + 5 ) / 10;
        r0 = 0x18;
        g0 = 0xc2;
        b0 = 0xde;
        break;
    }
    w = gdk_pixbuf_get_width( pbuf );
    h = gdk_pixbuf_get_height( pbuf );
    map = (uns8b *)gdk_pixbuf_get_pixels( pbuf );
    i_frame_type = ( vid->bitstream_type == 3 ) ? 2 : 0;
    if ( level0 < level_max )
        p0 = map + 4 * ( h - 1 - ( level0 * h ) / level_max ) * w;
    if ( level1 < level_max )
        p1 = map + 4 * ( h - 1 - ( level1 * h ) / level_max ) * w;
    if ( level2 < level_max  )
        p2 = map + 4 * ( h - 1 - ( level2 * h ) / level_max ) * w;
    memset( map, 0xff, 4 * w * h );
    for ( x = 0 ; x < w ; x++ ) {
        if ( x % 3 == 0 ) {
            if ( level2 < level_max ) {
                /* linea tratteggiata rossa */
                p2[ 0 ] = 0xcc;
                p2[ 1 ] = 0x00;
                p2[ 2 ] = 0x00;
                p2 += 12;
            }
            if ( level1 < level_max ) {
                /* linea tratteggiata gialla */
                p1[ 0 ] = 0xcc;
                p1[ 1 ] = 0xbb;
                p1[ 2 ] = 0x00;
                p1 += 12;
            }
            if ( level0 < level_max ) {
                /* linea tratteggiata verde */
                p0[ 0 ] = r0;
                p0[ 1 ] = g0;
                p0[ 2 ] = b0;
                p0 += 12;
            }
        }
        if ( fcnt < vid->cnt_vop ) {
            typ = vid->qscale[ fcnt ].typ;
            switch ( tipo ) {
            case 0:
                val = vid->qscale[ fcnt ].qscale;
                break;
            case 1:
                if ( avg_int ) {
                    if ( fcnt + avg_int < vid->cnt_vop ) {
                        buf_tot += vid->qscale[ fcnt + avg_int ].size;
                        buf_len++;
                    }
                    val = buf_tot / buf_len;
                    if ( fcnt >= avg_int ) {
                        buf_tot -= vid->qscale[ fcnt - avg_int ].size;
                        buf_len--;
                    }
                } else {
                    val = vid->qscale[ fcnt ].size;
                }
                break;
            case 2:
                buf_tot += rr0;
                val = vid->qscale[ fcnt ].size;
                if ( buf_tot >= val ) {
                    buf_tot -= val;
                    if ( buf_tot > vbv_size )
                        buf_tot = vbv_size;
                    val = buf_tot;
                } else {
                    buf_tot = vbv_init;
                    val = 2 * vbv_size;
                }
            }
            if ( ( val > level2 ) || ( val >= level_max ) ) {
                y = ( val >= level_max ) ? 0 : h - 1 - ( val * h ) / level_max;
                p = map + 4 * ( y * w + x );
                if ( typ == i_frame_type ) {
                    r = 0x00;
                    g = 0x00;
                    b = 0x00;
                } else {
                    if ( tipo == 0 ) {
                        r = 0x00;
                        g = 0x00;
                        b = 0xcc;
                    } else {
                        if ( val > level1 ) {
                            r = 0xcc;
                            g = 0x00;
                            b = 0x00;
                        } else {
                            r = r0;
                            g = g0;
                            b = b0;
                        }
                    }
                }
                for ( ; y < h ; y++, p += 4 * w ) {
                    p[ 0 ] = r;
                    p[ 1 ] = g;
                    p[ 2 ] = b;
                }
            } else if ( val > level1 ) {
                y = h - 1 - ( val * h ) / level_max;
                p = map + 4 * ( y * w + x );
                if ( typ == i_frame_type ) {
                    r = 0x00;
                    g = 0x00;
                    b = 0x00;
                } else {
                    r = 0xcc;
                    g = 0x00;
                    b = 0x00;
                }
                for ( ; y < h ; y++, p += 4 * w ) {
                    p[ 0 ] = r;
                    p[ 1 ] = g;
                    p[ 2 ] = b;
                }
            } else if ( val > level0 ) {
                y = h - 1 - ( val * h ) / level_max;
                p = map + 4 * ( y * w + x );
                if ( typ == i_frame_type ) {
                    r = 0x00;
                    g = 0x00;
                    b = 0x00;
                } else {
                    r = 0xcc;
                    g = 0xbb;
                    b = 0x00;
                }
                for ( ; y < h ; y++, p += 4 * w ) {
                    p[ 0 ] = r;
                    p[ 1 ] = g;
                    p[ 2 ] = b;
                }
            } else if ( val ) {
                y = h - 1 - ( val * h ) / level_max;
                p = map + 4 * ( y * w + x );
                if ( typ == i_frame_type ) {
                    r = 0x00;
                    g = 0x00;
                    b = 0x00;
                } else {
                    r = r0;
                    g = g0;
                    b = b0;
                }
                for ( ; y < h ; y++, p += 4 * w ) {
                    p[ 0 ] = r;
                    p[ 1 ] = g;
                    p[ 2 ] = b;
                }
            }
            fcnt++;
        }
    }
    if ( wim->window )
        gdk_window_invalidate_rect( wim->window, NULL, FALSE );
}

