/*
    TreeP Run Time Support
    Copyright (C) 2008-2025 Frank Sinapsi

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

uns8b trp_pix_scale_test( trp_obj_t *pix_i, trp_obj_t *pix_o )
{
    uns8b *mapi, *mapo;

    if ( ( pix_i->tipo != TRP_PIX ) || ( pix_o->tipo != TRP_PIX ) )
        return 1;
    mapi = ((trp_pix_t *)pix_i)->map.p;
    mapo = ((trp_pix_t *)pix_o)->map.p;
    if ( ( mapi == NULL ) || ( mapo == NULL ) )
        return 1;
    return trp_pix_scale_low( ((trp_pix_t *)pix_i)->w, ((trp_pix_t *)pix_i)->h, mapi,
                              ((trp_pix_t *)pix_o)->w, ((trp_pix_t *)pix_o)->h, &mapo );
}

trp_obj_t *trp_pix_scale( trp_obj_t *pix, trp_obj_t *w, trp_obj_t *h )
{
    trp_obj_t *res;

    if ( h )
        res = trp_pix_create( w, h );
    else {
        uns32b wi, hi, ww, hh;

        if ( ( pix->tipo != TRP_PIX ) || trp_cast_uns32b_rint_range( w, &ww, 1, 0xffff ) )
            return UNDEF;
        if ( ((trp_pix_t *)pix)->map.p == NULL )
            return UNDEF;
        wi = ((trp_pix_t *)pix)->w;
        hi = ((trp_pix_t *)pix)->h;
        if ( wi >= hi ) {
            hh = ( ww * hi + ( wi >> 1 ) ) / wi;
        } else {
            hh = ww;
            ww = ( hh * wi + ( hi >> 1 ) ) / hi;
        }
        res = trp_pix_create_basic( ww, hh );
    }
    if ( res != UNDEF )
        if ( trp_pix_scale_test( pix, res ) ) {
            (void)trp_pix_close( (trp_pix_t *)res );
            trp_gc_free( res );
            res = UNDEF;
        }
    return res;
}

uns8b trp_pix_scale_low( uns32b wi, uns32b hi, uns8b *idata, uns32b wo, uns32b ho, uns8b **odata )
{
    static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    static uns32b *minj = NULL;
    static uns32b prev_wi = 0, prev_hi = 0, prev_wo = 0, prev_ho = 0;
    uns64b tcol[ 4 ];
    uns8b *q, *r, *pre_r, *pre_pre_r;
    uns32b *maxj, *mini, *maxi, *pesominj, *pesomaxj, *pesomini, *pesomaxi;
    uns32b btpp = 4, btppwi, area, aream, peso, pesoy,
        pesointermedio, pesomassimo, x, y, i, j;

    if ( *odata == NULL )
        if ( ( *odata = malloc( ( wo * ho ) << 2 ) ) == NULL )
            return 1;
    if ( ( wo == wi ) && ( ho == hi ) ) {
        memcpy( *odata, idata, ( wo * ho ) << 2 );
        return 0;
    }
    pthread_mutex_lock( &mut );
    if ( ( wo != prev_wo ) || ( ho != prev_ho ) ) {
        if ( minj )
            minj = trp_gc_realloc( minj, ( sizeof( uns32b ) * ( wo + ho ) ) << 2 );
        else
            minj = trp_gc_malloc_atomic( ( sizeof( uns32b ) * ( wo + ho ) ) << 2 );
    }
    maxj = minj + ho;
    mini = maxj + ho;
    maxi = mini + wo;
    pesominj = maxi + wo;
    pesomaxj = pesominj + ho;
    pesomini = pesomaxj + ho;
    pesomaxi = pesomini + wo;
    area = wi * hi;
    aream = area >> 1;
    if ( ( wo != prev_wo ) || ( ho != prev_ho ) ||
         ( wi != prev_wi ) || ( hi != prev_hi ) ) {
        for ( y = 0 ; y < ho ; y++ ) {
            minj[ y ] = ( y * hi ) / ho;
            maxj[ y ] = ( ( y + 1 ) * hi + ho - 1 ) / ho - 1;
            pesominj[ y ] = ho + ho * minj[ y ] - y * hi;
            if ( minj[ y ] < maxj[ y ] )
                pesomaxj[ y ] = ( y + 1 ) * hi - ho * maxj[ y ];
            else
                pesomaxj[ y ] = pesominj[ y ] + ( y + 1 ) * hi - ho * maxj[ y ] - ho;
        }
        for ( x = 0 ; x < wo ; x++ ) {
            mini[ x ] = ( x * wi ) / wo;
            maxi[ x ] = ( ( x + 1 ) * wi + wo - 1 ) / wo - 1;
            pesomini[ x ] = wo + wo * mini[ x ] - x * wi;
            if ( mini[ x ] < maxi[ x ] )
                pesomaxi[ x ] = ( x + 1 ) * wi - wo * maxi[ x ];
            else
                pesomaxi[ x ] = pesomini[ x ] + ( x + 1 ) * wi - wo * maxi[ x ] - wo;
        }
        prev_wo = wo;
        prev_ho = ho;
        prev_wi = wi;
        prev_hi = hi;
    }
    pesomassimo = wo * ho;
    btppwi = btpp * wi;
    q = *odata;
    for ( y = 0 ; y < ho ; y++ ) {
        pre_pre_r = idata + btpp * minj[ y ] * wi;
        for ( x = 0 ; x < wo ; x++ ) {
            tcol[ 0 ] = tcol[ 1 ] = tcol[ 2 ] = tcol[ 3 ] = aream;
            pesoy = pesominj[ y ];
            pesointermedio = wo * pesoy;
            pre_r = pre_pre_r + btpp * mini[ x ];
            for ( j = minj[ y ] ; ; j++ ) {
                if ( j == maxj[ y ] ) {
                    pesoy = pesomaxj[ y ];
                    pesointermedio = wo * pesoy;
                }
                peso = pesomini[ x ] * pesoy;
                r = pre_r;
                for ( i = mini[ x ] ; ; i++ ) {
                    if ( i == maxi[ x ] )
                        peso = pesomaxi[ x ] * pesoy;
                    tcol[ 0 ] += peso * (uns32b)( *r++ );
                    tcol[ 1 ] += peso * (uns32b)( *r++ );
                    tcol[ 2 ] += peso * (uns32b)( *r++ );
                    tcol[ 3 ] += peso * (uns32b)( *r++ );
                    if ( i == maxi[ x ] )
                        break;
                    peso = pesointermedio;
                }
                if ( j == maxj[ y ] )
                    break;
                pesoy = ho;
                pesointermedio = pesomassimo;
                pre_r += btppwi;
            }
            *q++ = tcol[ 0 ] / area;
            *q++ = tcol[ 1 ] / area;
            *q++ = tcol[ 2 ] / area;
            *q++ = tcol[ 3 ] / area;
        }
    }
    pthread_mutex_unlock( &mut );
    return 0;
}

