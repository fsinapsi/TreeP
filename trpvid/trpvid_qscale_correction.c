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

#include "./trpvid_internal.h"

trp_obj_t *trp_vid_qscale_correction_a( trp_obj_t *obj )
{
    trp_vid_t *vid;
    uns32b res;
    uns32b sum, cnt, i, j, k;
    double avg, avg_first_500;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( ( vid->bitstream_type < 1 ) ||
         ( vid->bitstream_type > 3 ) )
        return UNDEF;
    sum = ( vid->cnt_qscale_cnt[ 0 ] +
            vid->cnt_qscale_cnt[ 1 ] +
            vid->cnt_qscale_cnt[ 2 ] +
            vid->cnt_qscale_cnt[ 3 ] +
            ( ( vid->bitstream_type == 3 ) ? vid->cnt_qscale_cnt[ 4 ] : 0 ) );

    /*
     prima regola:
     se i frame utili sono meno di 4500, si ritorna 0
     */
    if ( sum < 4500 )
        return ZERO;

    /*
     si calcola la media totale
     */
    avg = (double)( vid->cnt_qscale_avg[ 0 ] +
                    vid->cnt_qscale_avg[ 1 ] +
                    vid->cnt_qscale_avg[ 2 ] +
                    vid->cnt_qscale_avg[ 3 ] +
                    ( ( vid->bitstream_type == 3 ) ?
                      vid->cnt_qscale_avg[ 4 ] : 0 ) ) / (double)sum;

    /*
     si calcola la media dei primi 500 frames "effettivi"
     (non si conteggiano quelli a qscale=0)
     */
    for ( cnt = 0, sum = 0, i = 0 ; ; ) {
        if ( cnt >= vid->cnt_vop - 100 )
            return ZERO;
        j = vid->qscale[ cnt++ ].qscale;
        if ( j > 0 ) {
            sum += j;
            if ( ++i == 500 )
                break;
        }
    }
    avg_first_500 = (double)sum / (double)500.0;
    /*
     seconda regola:
     se la media dei primi 500 non supera la media totale di almeno 3,
     si ritorna 0
     */
    if ( avg_first_500 < avg + (double)3.0 )
        return ZERO;

    /*
     questo aumenta la precisione...
     */
    if ( avg_first_500 > avg + (double)5.0 )
        avg_first_500 = avg + (double)5.0;

    /*
     terza regola: si cerca la prima "finestra" di 50 frames
     la cui media sia minore della media dei primi 500 frames di
     almeno 3; se non si trova una tale finestra nel primo ottavo
     del filmato, si ritorna 0
     */
    for ( i = cnt, sum = 0 ; i < cnt + 50 ; i++ )
        sum += vid->qscale[ i ].qscale;
    for ( i = cnt ; ; ) {
        avg = (double)sum / (double)50.0;
        if ( avg + (double)3.0 <= avg_first_500 ) {
            res = i + 50;
            break;
        }
        if ( i * 8 >= vid->cnt_vop ) {
            res = 0;
            break;
        }
        sum = sum - vid->qscale[ i ].qscale + vid->qscale[ i + 50 ].qscale;
        i++;
    }
    return trp_sig64( res );
}

trp_obj_t *trp_vid_qscale_correction_b( trp_obj_t *obj )
{
    trp_vid_t *vid;
    uns32b res;
    uns32b sum, cnt, i, j, k;
    double avg, avg_last_500;

    if ( trp_vid_check( obj, &vid ) )
        return UNDEF;
    if ( ( vid->bitstream_type < 1 ) ||
         ( vid->bitstream_type > 3 ) )
        return UNDEF;
    sum = ( vid->cnt_qscale_cnt[ 0 ] +
            vid->cnt_qscale_cnt[ 1 ] +
            vid->cnt_qscale_cnt[ 2 ] +
            vid->cnt_qscale_cnt[ 3 ] +
            ( ( vid->bitstream_type == 3 ) ? vid->cnt_qscale_cnt[ 4 ] : 0 ) );

    /*
     prima regola:
     se i frame utili sono meno di 4500, si ritorna 0
     */
    if ( sum < 4500 )
        return ZERO;

    /*
     si calcola la media totale
     */
    avg = (double)( vid->cnt_qscale_avg[ 0 ] +
                    vid->cnt_qscale_avg[ 1 ] +
                    vid->cnt_qscale_avg[ 2 ] +
                    vid->cnt_qscale_avg[ 3 ] +
                    ( ( vid->bitstream_type == 3 ) ?
                      vid->cnt_qscale_avg[ 4 ] : 0 ) ) / (double)sum;

    /*
     si calcola la media degli ultimi 500 frames "effettivi"
     (non si conteggiano quelli a qscale=0)
     */
    for ( cnt = vid->cnt_vop, sum = 0, i = 0 ; ; ) {
        if ( cnt <= 100 )
            return ZERO;
        j = vid->qscale[ --cnt ].qscale;
        if ( j > 0 ) {
            sum += j;
            if ( ++i == 500 )
                break;
        }
    }
    avg_last_500 = (double)sum / (double)500.0;

    /*
     seconda regola:
     se la media degli ultimi 500 non supera la media totale di almeno 3,
     si ritorna 0
     */
    if ( avg_last_500 < avg + (double)3.0 )
        return ZERO;

    /*
     questo aumenta la precisione...
     */
    if ( avg_last_500 > avg + (double)5.0 )
        avg_last_500 = avg + (double)5.0;

    /*
     terza regola: si cerca (dal fondo) la prima "finestra" di 50 frames
     la cui media sia minore della media degli ultimi 500 frames di
     almeno 3; se non si trova una tale finestra nell'ultimo ottavo
     del filmato, si ritorna 0
     */
    for ( i = cnt - 50, sum = 0 ; i < cnt ; i++ )
        sum += vid->qscale[ i ].qscale;
    for ( i = cnt - 50 ; ; ) {
        avg = (double)sum / (double)50.0;
        if ( avg + (double)3.0 <= avg_last_500 ) {
            res = vid->cnt_vop - i;
            break;
        }
        if ( i * 8 <= vid->cnt_vop * 7 ) {
            res = 0;
            break;
        }
        i--;
        sum = sum + vid->qscale[ i ].qscale - vid->qscale[ i + 50 ].qscale;
    }
    return trp_sig64( res );
}

