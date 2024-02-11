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

#include "./trppix_internal.h"

uns8bfun_t _trp_pix_load_webp = NULL;
uns8bfun_t _trp_pix_load_qoi = NULL;
uns8bfun_t _trp_pix_load_openjp2 = NULL;
uns8bfun_t _trp_pix_load_jbig2 = NULL;
uns8bfun_t _trp_pix_load_rsvg = NULL;
uns8bfun_t _trp_pix_load_lept = NULL;
uns8bfun_t _trp_pix_load_heif = NULL;
uns8bfun_t _trp_pix_load_sail = NULL;
uns8bfun_t _trp_pix_load_cv = NULL;
uns8bfun_t _trp_pix_load_webp_memory = NULL;
uns8bfun_t _trp_pix_load_qoi_memory = NULL;
uns8bfun_t _trp_pix_load_openjp2_memory = NULL;
uns8bfun_t _trp_pix_load_jbig2_memory = NULL;
uns8bfun_t _trp_pix_load_rsvg_memory = NULL;
uns8bfun_t _trp_pix_load_lept_memory = NULL;
uns8bfun_t _trp_pix_load_heif_memory = NULL;
uns8bfun_t _trp_pix_load_sail_memory = NULL;
uns8bfun_t _trp_pix_load_cv_memory = NULL;

enum {
    TRP_PIX_PNG = 0,
    TRP_PIX_JPG,
    TRP_PIX_PNM,
    TRP_PIX_GIF,
    TRP_PIX_TGA,
    TRP_PIX_XPM,
    TRP_PIX_PTG,
    TRP_PIX_WEBP,
    TRP_PIX_QOI,
    TRP_PIX_OPENJP2,
    TRP_PIX_JBIG2,
    TRP_PIX_RSVG,
    TRP_PIX_LEPT,
    TRP_PIX_HEIF,
    TRP_PIX_SAIL,
    TRP_PIX_CV,
    TRP_PIX_MAX /* lasciarlo sempre per ultimo */
};

static uns8b *_trp_pix_decoder[ TRP_PIX_MAX ] = {
    "png",
    "jpg",
    "pnm",
    "gif",
    "tga",
    "xpm",
    "ptg",
    "webp",
    "qoi",
    "openjp2",
    "jbig2",
    "svg",
    "lept",
    "heif",
    "sail",
    "cv"
};

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

static trp_obj_t *trp_pix_load_thumbnail_memory_low( trp_obj_t *raw, uns32b sw, uns32b sh, trp_obj_t *w, trp_obj_t *h );

trp_obj_t *trp_pix_loader( trp_obj_t *pix )
{
    if ( pix->tipo != TRP_PIX )
        return UNDEF;
    if ( ((trp_pix_t *)pix)->map.c == NULL )
        return UNDEF;
    return trp_cord( _trp_pix_decoder[ ((trp_pix_t *)pix)->sottotipo - 1 ] );
}

trp_obj_t *trp_pix_load( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    trp_obj_t *res;

    res = trp_pix_load_low( cpath );
    trp_csprint_free( cpath );
    return res;
}

trp_obj_t *trp_pix_load_multiple( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    trp_obj_t *res = UNDEF;
    uns8b sottotipo;

    if ( res == UNDEF ) {
        res = trp_pix_load_gif_multiple( cpath );
        sottotipo = TRP_PIX_GIF + 1;
    }
    trp_csprint_free( cpath );
    if ( res != UNDEF ) {
        trp_queue_elem *elem;

        for ( elem = (trp_queue_elem *)( (trp_queue_t *)( ((trp_queue_t *)res)->first ) ) ;
              elem ;
              elem = (trp_queue_elem *)( elem->next ) )
            ((trp_pix_t *)( ((trp_cons_t *)( elem->val ))->car ))->sottotipo = sottotipo;
    }
    return res;
}

trp_obj_t *trp_pix_load_memory( trp_obj_t *raw, trp_obj_t *cnt )
{
    uns32b isize;

    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( cnt ) {
        if ( trp_cast_uns32b( cnt, &isize ) )
            return UNDEF;
        if ( isize > ((trp_raw_t *)raw)->len )
            isize = ((trp_raw_t *)raw)->len;
    } else
        isize = ((trp_raw_t *)raw)->len;
    return trp_pix_load_memory_low( ((trp_raw_t *)raw)->data, isize );
}

trp_obj_t *trp_pix_load_memory_ext( trp_obj_t *raw, trp_obj_t *w, trp_obj_t *h, trp_obj_t *cnt )
{
    trp_obj_t *res;
    uns32b ww, hh, isize;

    if ( ( raw->tipo != TRP_RAW ) ||
         trp_cast_uns32b( w, &ww ) ||
         trp_cast_uns32b( h, &hh ) )
        return UNDEF;
    if ( cnt ) {
        if ( trp_cast_uns32b( cnt, &isize ) )
            return UNDEF;
        if ( isize > ((trp_raw_t *)raw)->len )
            isize = ((trp_raw_t *)raw)->len;
    } else
        isize = ((trp_raw_t *)raw)->len;
    res = trp_pix_load_memory_low( ((trp_raw_t *)raw)->data, isize );
    if ( res == UNDEF )
        if ( isize && ( ww * hh * 3 == isize ) ) {
            uns8b *data, *p, *s;

            if ( data = malloc( ( ww * hh ) << 2 ) ) {
                for ( p = data, s = ((trp_raw_t *)raw)->data, isize = ww * hh ; isize ; isize-- ) {
                    *p++ = *s++;
                    *p++ = *s++;
                    *p++ = *s++;
                    *p++ = 0xff;
                }
                res = trp_pix_create_image_from_data( 0, ww, hh, data );
            }
        }
    return res;
}

trp_obj_t *trp_pix_load_low( uns8b *cpath )
{
    trp_obj_t *res;
    uns8b *data;
    uns32b w, h;
    uns8b sottotipo, notdone = 1;

    if ( notdone )
        if ( trp_pix_load_png( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PNG + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_jpg( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_JPG + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_pnm( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PNM + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_gif( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_GIF + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_tga( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_TGA + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_xpm( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_XPM + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_ptg( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PTG + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_webp )
        if ( ( _trp_pix_load_webp )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_WEBP + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_qoi )
        if ( ( _trp_pix_load_qoi )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_QOI + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_openjp2 )
        if ( ( _trp_pix_load_openjp2 )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_OPENJP2 + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_jbig2 )
        if ( ( _trp_pix_load_jbig2 )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_JBIG2 + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_rsvg )
        if ( ( _trp_pix_load_rsvg )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_RSVG + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_lept )
        if ( ( _trp_pix_load_lept )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_LEPT + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_heif )
        if ( ( _trp_pix_load_heif )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_HEIF + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_sail )
        if ( ( _trp_pix_load_sail )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_SAIL + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_cv )
        if ( ( _trp_pix_load_cv )( cpath, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_CV + 1;
            notdone = 0;
        }
    if ( notdone )
        return UNDEF;
    res = trp_pix_create_image_from_data( 0, w, h, data );
    ((trp_pix_t *)res)->sottotipo = sottotipo;
    return res;
}

trp_obj_t *trp_pix_load_memory_low( uns8b *idata, uns32b isize )
{
    trp_obj_t *res;
    uns8b *data;
    uns32b w, h;
    uns8b sottotipo, notdone = 1;

    if ( notdone )
        if ( trp_pix_load_png_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PNG + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_jpg_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_JPG + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_pnm_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PNM + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_gif_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_GIF + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_tga_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_TGA + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_xpm_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_XPM + 1;
            notdone = 0;
        }
    if ( notdone )
        if ( trp_pix_load_ptg_memory( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_PTG + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_webp_memory )
        if ( ( _trp_pix_load_webp_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_WEBP + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_qoi_memory  )
        if ( ( _trp_pix_load_qoi_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_QOI + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_openjp2_memory  )
        if ( ( _trp_pix_load_openjp2_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_OPENJP2 + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_jbig2_memory  )
        if ( ( _trp_pix_load_jbig2_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_JBIG2 + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_rsvg_memory  )
        if ( ( _trp_pix_load_rsvg_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_RSVG + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_lept_memory  )
        if ( ( _trp_pix_load_lept_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_LEPT + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_heif_memory  )
        if ( ( _trp_pix_load_heif_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_HEIF + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_sail_memory  )
        if ( ( _trp_pix_load_sail_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_SAIL + 1;
            notdone = 0;
        }
    if ( notdone && _trp_pix_load_cv_memory  )
        if ( ( _trp_pix_load_cv_memory )( idata, isize, &w, &h, &data ) == 0 ) {
            sottotipo = TRP_PIX_CV + 1;
            notdone = 0;
        }
    if ( notdone )
        return UNDEF;
    res = trp_pix_create_image_from_data( 0, w, h, data );
    ((trp_pix_t *)res)->sottotipo = sottotipo;
    return res;
}

trp_obj_t *trp_pix_load_thumbnail( trp_obj_t *path, trp_obj_t *w, trp_obj_t *h )
{
    trp_obj_t *res, *nres;
    uns8b *cpath;
    Epeg_Image *ep;
    uns32b ww, hh;

    if ( trp_cast_uns32b_rint_range( w, &ww, 1, 0xffff ) )
        return UNDEF;
    if ( h )
        if ( trp_cast_uns32b_rint_range( h, &hh, 1, 0xffff ) )
            return UNDEF;
    cpath = trp_csprint( path );
    if ( ep = epeg_file_open( cpath ) ) {
        const void *p;
        int ws, hs;
        uns32b www, hhh, ww0;

        epeg_size_get( ep, &ws, &hs );
        ww0 = ww;
        if ( h == NULL ) {
            if ( ws >= hs ) {
                hh = ( hs * ww0 + ( ws >> 1 ) ) / ws;
            } else {
                hh = ww0;
                ww0 = ( ws * hh + ( hs >> 1 ) ) / hs;
            }
        }
        if ( ( ww0 <= ws ) &&
             ( hh <= hs ) ) {
            www = ww0;
            hhh = hh;
        } else {
            www = ws;
            hhh = hs;
        }
        epeg_decode_size_set( ep, www, hhh );
        epeg_decode_colorspace_set( ep, EPEG_RGBA8 );
        if ( p = epeg_pixels_get( ep, 0, 0, www, hhh ) ) {
            epeg_close( ep );
            res = trp_pix_create_image_from_data( 0, www, hhh, (uns8b *)p );
            if ( ( www == ww0 ) && ( hhh == hh ) ) {
                trp_csprint_free( cpath );
                return res;
            }
            if ( ( nres = trp_pix_create_basic( ww0, hh ) ) != UNDEF ) {
                if ( trp_pix_scale_test( res, nres ) == 0 ) {
                    (void)trp_pix_close( (trp_pix_t *)res );
                    trp_gc_free( res );
                    trp_csprint_free( cpath );
                    return nres;
                }
                (void)trp_pix_close( (trp_pix_t *)nres );
                trp_gc_free( nres );
            }
            (void)trp_pix_close( (trp_pix_t *)res );
            trp_gc_free( res );
        } else
            epeg_close( ep );
    }
    res = trp_pix_load_low( cpath );
    trp_csprint_free( cpath );
    if ( res != UNDEF ) {
        uns32b ws = ((trp_pix_t *)res)->w, hs = ((trp_pix_t *)res)->h;

        if ( h == NULL ) {
            if ( ws >= hs ) {
                hh = ( hs * ww + ( ws >> 1 ) ) / ws;
            } else {
                hh = ww;
                ww = ( ws * hh + ( hs >> 1 ) ) / hs;
            }
        }
        if ( ( ws == ww ) && ( hs == hh ) )
            return res;
        if ( ( nres = trp_pix_create_basic( ww, hh ) ) != UNDEF ) {
            if ( trp_pix_scale_test( res, nres ) == 0 ) {
                (void)trp_pix_close( (trp_pix_t *)res );
                trp_gc_free( res );
                return nres;
            }
            (void)trp_pix_close( (trp_pix_t *)nres );
            trp_gc_free( nres );
        }
        (void)trp_pix_close( (trp_pix_t *)res );
        trp_gc_free( res );
    }
    return UNDEF;
}

static trp_obj_t *trp_pix_load_thumbnail_memory_low( trp_obj_t *raw, uns32b sw, uns32b sh, trp_obj_t *w, trp_obj_t *h )
{
    uns8b *s = ((trp_raw_t *)raw)->data;
    trp_obj_t *res, *nres;
    const void *p = NULL;
    Epeg_Image *ep;
    int ws, hs;
    uns32b c, ww, hh, www, hhh, ww0;;

    if ( trp_cast_uns32b_rint_range( w, &ww, 1, 0xffff ) ||
         ( raw->tipo != TRP_RAW ) )
        return UNDEF;
    if ( h )
        if ( trp_cast_uns32b_rint_range( h, &hh, 1, 0xffff ) )
            return UNDEF;
    c = ((trp_raw_t *)raw)->len;
    if ( c == 0 )
        return UNDEF;
    if ( ( s[ 0 ] == 0xff ) &&
         ( s[ 1 ] == 0xd8 ) &&
         ( s[ 2 ] == 0xff ) // &&
//         ( s[ 3 ] == 0xfe )
       )
        if ( ep = epeg_memory_open( s, c ) ) {
            epeg_size_get( ep, &ws, &hs );
            ww0 = ww;
            if ( h == NULL ) {
                if ( ws >= hs ) {
                    hh = ( hs * ww0 + ( ws >> 1 ) ) / ws;
                } else {
                    hh = ww0;
                    ww0 = ( ws * hh + ( hs >> 1 ) ) / hs;
                }
            }
            if ( ( ww0 <= ws ) &&
                 ( hh <= hs ) ) {
                www = ww0;
                hhh = hh;
            } else {
                www = ws;
                hhh = hs;
            }
            epeg_decode_size_set( ep, www, hhh );
            epeg_decode_colorspace_set( ep, EPEG_RGBA8 );
            p = epeg_pixels_get( ep, 0, 0, www, hhh );
            epeg_close( ep );
        }
    if ( p == NULL )
        if ( trp_pix_load_png_memory( ((trp_raw_t *)raw)->data, ((trp_raw_t *)raw)->len, &www, &hhh, (uns8b **)( &p ) ) )
            p = NULL;
        else {
            ws = www;
            hs = hhh;
            ww0 = ww;
            if ( h == NULL ) {
                if ( ws >= hs ) {
                    hh = ( hs * ww0 + ( ws >> 1 ) ) / ws;
                } else {
                    hh = ww0;
                    ww0 = ( ws * hh + ( hs >> 1 ) ) / hs;
                }
            }
        }
    if ( p == NULL )
        if ( sw * sh * 3 == c ) {
            uns8b *d;

            www = sw;
            hhh = sh;
            ww0 = ww;
            if ( h == NULL ) {
                if ( sw >= sh ) {
                    hh = ( sh * ww0 + ( sw >> 1 ) ) / sw;
                } else {
                    hh = ww0;
                    ww0 = ( sw * hh + ( sh >> 1 ) ) / sh;
                }
            }
            if ( d = malloc( ( sw * sh ) << 2 ) )
                for ( p = (void *)d, c = sw * sh ; c ; c-- ) {
                    *d++ = *s++;
                    *d++ = *s++;
                    *d++ = *s++;
                    *d++ = 0xff;
                }
        }
    if ( p == NULL )
        return UNDEF;
    res = trp_pix_create_image_from_data( 0, www, hhh, (uns8b *)p );
    if ( ( www == ww0 ) && ( hhh == hh ) )
        return res;
    if ( ( nres = trp_pix_create_basic( ww0, hh ) ) != UNDEF ) {
        if ( trp_pix_scale_test( res, nres ) == 0 ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            trp_gc_free( res );
            return nres;
        }
        (void)trp_pix_close( (trp_pix_t *)nres );
        trp_gc_free( nres );
    }
    (void)trp_pix_close( (trp_pix_t *)res );
    trp_gc_free( res );
    return UNDEF;
}

trp_obj_t *trp_pix_load_thumbnail_memory( trp_obj_t *raw, trp_obj_t *w, trp_obj_t *h )
{
    return trp_pix_load_thumbnail_memory_low( raw, 0, 0, w, h );
}

trp_obj_t *trp_pix_load_thumbnail_memory_ext( trp_obj_t *raw, trp_obj_t *sw, trp_obj_t *sh, trp_obj_t *w, trp_obj_t *h )
{
    uns32b ssw, ssh;

    if ( trp_cast_uns32b( sw, &ssw ) ||
         trp_cast_uns32b( sh, &ssh ) )
        return UNDEF;
    return trp_pix_load_thumbnail_memory_low( raw, ssw, ssh, w, h );
}

void trp_pix_load_set_loader_svg( trp_obj_t *pix )
{
    if ( trp_pix_get_mapp( pix ) )
        ((trp_pix_t *)pix)->sottotipo = TRP_PIX_RSVG + 1;
}

