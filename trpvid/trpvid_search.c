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

#include "./trpvid_internal.h"

static trp_obj_t *trp_vid_search_internal( int verso, trp_obj_t *obj, trp_obj_t *f_cnt,
                                           trp_obj_t *s_min, trp_obj_t *s_max,
                                           trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                           trp_obj_t *ttyp );

trp_obj_t *trp_vid_search_next( trp_obj_t *obj, trp_obj_t *f_cnt,
                                trp_obj_t *s_min, trp_obj_t *s_max,
                                trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                trp_obj_t *ttyp )
{
    return trp_vid_search_internal( 0, obj, f_cnt, s_min, s_max, a_int, q_min, q_max, ttyp );
}

trp_obj_t *trp_vid_search_prev( trp_obj_t *obj, trp_obj_t *f_cnt,
                                trp_obj_t *s_min, trp_obj_t *s_max,
                                trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                trp_obj_t *ttyp )
{
    return trp_vid_search_internal( 1, obj, f_cnt, s_min, s_max, a_int, q_min, q_max, ttyp );
}

static trp_obj_t *trp_vid_search_internal( int verso, trp_obj_t *obj, trp_obj_t *f_cnt,
                                           trp_obj_t *s_min, trp_obj_t *s_max,
                                           trp_obj_t *a_int, trp_obj_t *q_min, trp_obj_t *q_max,
                                           trp_obj_t *ttyp )
{
    trp_vid_t *vid;
    uns32b framecnt, smin, smax, avg_int, qmin, qmax, typ;
    uns32b size, qscale;
    uns32b buf_tot = 0, buf_len = 0;

    if ( trp_vid_check( obj, &vid ) ||
         trp_cast_uns32b( f_cnt, &framecnt ) ||
         trp_cast_uns32b( s_min, &smin ) ||
         trp_cast_uns32b( s_max, &smax ) ||
         trp_cast_uns32b( a_int, &avg_int ) ||
         trp_cast_uns32b( q_min, &qmin ) ||
         trp_cast_uns32b( q_max, &qmax ) )
        return UNDEF;
    if ( framecnt >= vid->cnt_vop )
        return UNDEF;
    if ( ttyp == UNDEF )
        ttyp = NULL;
    if ( ttyp ) {
        if ( trp_cast_uns32b( ttyp, &typ ) )
            return UNDEF;
    } else
        typ = 0;
    if ( avg_int ) {
        int x, w, h;

        if ( verso == 0 ) {
            w = framecnt - ( avg_int - 1 );
            h = framecnt + avg_int;
        } else {
            w = framecnt - avg_int;
            h = framecnt + ( avg_int - 1 );
        }
        if ( w < 0 )
            w = 0;
        if ( h >= vid->cnt_vop )
            h = vid->cnt_vop - 1;
        for ( x = w ; x <= h ; x++ ) {
            buf_tot += vid->qscale[ x ].size;
            buf_len++;
        }
    }
    obj = UNDEF;
    for ( ; ; ) {
        if ( verso == 0 ) {
            framecnt++;
            if ( framecnt == vid->cnt_vop )
                break;
            if ( avg_int ) {
                if ( framecnt + avg_int < vid->cnt_vop ) {
                    buf_tot += vid->qscale[ framecnt + avg_int ].size;
                    buf_len++;
                }
                size = buf_tot / buf_len;
                if ( framecnt >= avg_int ) {
                    buf_tot -= vid->qscale[ framecnt - avg_int ].size;
                    buf_len--;
                }
            } else {
                size = vid->qscale[ framecnt ].size;
            }
        } else {
            if ( framecnt == 0 )
                break;
            framecnt--;
            if ( avg_int ) {
                if ( framecnt >= avg_int ) {
                    buf_tot += vid->qscale[ framecnt - avg_int ].size;
                    buf_len++;
                }
                size = buf_tot / buf_len;
                if ( framecnt + avg_int < vid->cnt_vop ) {
                    buf_tot -= vid->qscale[ framecnt + avg_int ].size;
                    buf_len--;
                }
            } else {
                size = vid->qscale[ framecnt ].size;
            }
        }
        qscale = vid->qscale[ framecnt ].qscale;
        if ( ( ( ttyp == NULL ) || ( vid->qscale[ framecnt ].typ == typ ) ) &&
             ( size >= smin ) && ( size <= smax ) &&
             ( qscale >= qmin ) && ( qscale <= qmax ) ) {
            obj = trp_sig64( framecnt );
            break;
        }
    }
    return obj;
}

