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

static uns8b trp_vid_next_start_code( trp_vid_t *vid, uns16b *startcode, uns8b userdata );
static uns8b trp_vid_parse_vol( trp_vid_t *vid );
static uns8b trp_vid_parse_vop( trp_vid_t *vid );
static uns8b trp_vid_bs_basic( trp_vid_t *vid, int size, uns32b *val, uns8b update );
static uns8b trp_vid_bs_skip( trp_vid_t *vid, int size );
static uns8b trp_vid_bs_peek( trp_vid_t *vid, int size, uns32b *val );
static uns8b trp_vid_bs( trp_vid_t *vid, int size, uns32b *val );
static uns8b trp_vid_bs_matrix( trp_vid_t *vid, uns8b **qm );
static uns8b trp_vid_warping_code( trp_vid_t *vid, uns32b *used );
static uns32b trp_vid_bitsused( uns32b val );

static int _sprite_traj_len_code[] = {
    0x000, 0x002, 0x003, 0x004, 0x005, 0x006, 0x00E, 0x01E,
    0x03E, 0x07E, 0x0FE, 0x1FE, 0x3FE, 0x7FE, 0xFFE
};

static int _sprite_traj_len_bits[] = {
    2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

uns8b trp_vid_parse_mpeg4asp( trp_vid_t *vid )
{
    uns16b startcode;
    uns8b userdata = 0, first = 1, has_vol = 0, pos_in_frame = 1;

    for ( ; ; ) {
        if ( trp_vid_next_start_code( vid, &startcode, userdata ) )
            break;
        if ( first && ( vid->buf_pos > 4 ) )
            break;
        first = 0;
        userdata = 0;
        if ( ( startcode & 0x1f0 ) == 0x120 ) {
            if ( trp_vid_parse_vol( vid ) )
                return 1;
            has_vol = 1;
        } else if ( startcode == 0x1b6 ) {
            if ( pos_in_frame == 2 ) {
                vid->packed = 1;
                if ( vid->cnt_vol )
                    vid->cnt_packed++;
            } else if ( pos_in_frame > 2 ) {
                vid->error = "Troppi VOP in un frame";
                return 1;
            }
            pos_in_frame++;
            if ( vid->cnt_vol ) {
                if ( trp_vid_parse_vop( vid ) )
                    return 1;
            } else {
                vid->missing_vol = 1;
                trp_vid_update_qscale( vid, 1, 5, 0 );
            }
        } else if ( startcode == 0x1b2 )
            userdata = 1;
    }
    if ( first ) {
        if ( vid->bitstream_type == 0 ) {
            vid->error = "Il frame non ha un valido start-code MPEG4";
            return 1;
        }
        trp_vid_update_qscale( vid, 1, 6, 0 );
        return 0;
    }
    if ( vid->matroska ) {
        if ( !has_vol )
            return 1;
    } else {
        if ( pos_in_frame == 1 ) {
            vid->error = "Il frame e' privo di header VOP";
            return 1;
        }
    }
    return 0;
}

static uns8b trp_vid_next_start_code( trp_vid_t *vid, uns16b *startcode, uns8b userdata )
{
    uns8b res = 0;
    uns32b resto, cnt;
    uns8b *p;

    resto = vid->buf_size - vid->buf_pos;
    p = vid->buf + vid->buf_pos;
    for ( cnt = 0 ; ; cnt++ ) {
        if ( resto < 4 ) {
            res = 1;
            break;
        }
        if ( ( p[ 0 ] == 0 ) && ( p[ 1 ] == 0 ) &&( p[ 2 ] == 1 ) )
            break;
        p++;
        resto--;
    }
    if ( userdata )
        trp_vid_store_userdata( vid, vid->buf + vid->buf_pos, ( res == 0 ) ? cnt : vid->buf_size - vid->buf_pos );
    if ( res == 0 ) {
        *startcode = p[ 3 ] | 0x100;
        vid->tmp_frame_pos = vid->buf_pos + cnt;
        vid->buf_pos += cnt + 4;
    }
    return res;
}

static uns8b trp_vid_parse_vol( trp_vid_t *vid )
{
    uns32b val, ver_id, time_inc_res, estimation_method, hierarchy;

    vid->cnt_vol++;
    vid->rl = 0;
    vid->r = 0;

    trp_vid_bs_skip( vid, 9 );
    trp_vid_bs( vid, 1, &val );
    if ( val ) {
        trp_vid_bs( vid, 4, &ver_id );
        trp_vid_bs_skip( vid, 3 );
    } else {
        ver_id = 1;
    }

    trp_vid_bs( vid, 4, &( vid->par ) );
    if ( vid->par == 15 ) {
        trp_vid_bs( vid, 8, &( vid->par_w ) );
        trp_vid_bs( vid, 8, &( vid->par_h ) );
    }

    trp_vid_bs( vid, 1, &val );
    if ( val ) {
        trp_vid_bs_skip( vid, 2 );
        trp_vid_bs_skip( vid, 1 );
        trp_vid_bs( vid, 1, &val );
        if ( val ) {
            trp_vid_bs_skip( vid, 32 );
            trp_vid_bs_skip( vid, 32 );
            trp_vid_bs_skip( vid, 15 );
        }
    }

    trp_vid_bs( vid, 2, &( vid->shape ) );
    if ( ( ver_id != 1 ) && ( vid->shape == 3 ) ) {
        /*
         FIXME
         il campo che viene qui letto si chiama shape_extension
         e può essere utilizzato per sapere come leggere le
         matrici grayscale (vedi sotto)
         */
        trp_vid_bs( vid, 4, &val );
    }
    trp_vid_bs_skip( vid, 1 );
    trp_vid_bs( vid, 16, &time_inc_res );
    vid->time_inc_bits = trp_vid_bitsused( time_inc_res - 1 );
    if ( vid->time_inc_bits == 0 )
        vid->time_inc_bits = 1;
    trp_vid_bs_skip( vid, 1 );

    trp_vid_bs( vid, 1, &val );
    if ( val ) {
        trp_vid_bs_skip( vid, vid->time_inc_bits );
    }

    if ( vid->shape != 2 ) {
        if ( vid->shape == 0 ) {
            trp_vid_bs_skip( vid, 1 );
            trp_vid_bs( vid, 13, &( vid->width ) );
            trp_vid_bs_skip( vid, 1 );
            trp_vid_bs( vid, 13, &( vid->height ) );
            trp_vid_bs_skip( vid, 1 );
        }
        trp_vid_bs( vid, 1, &( vid->interlaced ) );
        trp_vid_bs_skip( vid, 1 );

        trp_vid_bs( vid, ( ver_id == 1 ) ? 1 : 2, &( vid->sprite_enable ) );
        if ( vid->sprite_enable ) {
            if ( vid->sprite_enable == 1 ) {
                trp_vid_bs_skip( vid, 32 );
                trp_vid_bs_skip( vid, 24 );
            }
            trp_vid_bs( vid, 6, &( vid->sprite_warping_points ) );
            if ( vid->sprite_warping_points > MAX_WARPING_POINTS ) {
                vid->error = "Troppi warping points";
                return 1;
            }
            trp_vid_bs_skip( vid, 3 );
            if ( vid->sprite_enable == 1 ) {
                trp_vid_bs_skip( vid, 1 );
            }
        }

        if ( ( ver_id != 1 ) && vid->shape ) {
            trp_vid_bs_skip( vid, 1 );
        }

        trp_vid_bs( vid, 1, &val );
        if ( val ) {
            trp_vid_bs( vid, 4, &( vid->quant_precision ) );
            trp_vid_bs_skip( vid, 4 );
        } else {
            vid->quant_precision = 5;
        }

        if ( vid->shape == 3 ) {
            trp_vid_bs_skip( vid, 3 );
        }

        trp_vid_bs( vid, 1, &( vid->mpeg_quant ) );
        if ( vid->mpeg_quant ) {
            trp_vid_bs( vid, 1, &val );
            if ( val ) {
                trp_vid_bs_matrix( vid, &( vid->intra_quant_matrix ) );
            }
            trp_vid_bs( vid, 1, &val );
            if ( val ) {
                trp_vid_bs_matrix( vid, &( vid->inter_quant_matrix ) );
            }
            if ( vid->shape == 3 ) {
                vid->error = "Matrici grayscale non supportate";
                return 1;
            }
        }

        if ( ver_id != 1 ) {
            trp_vid_bs( vid, 1, &( vid->qpel ) );
        }

        trp_vid_bs( vid, 1, &val );
        if ( val == 0 ) {
            trp_vid_bs( vid, 2, &estimation_method );
            if ( estimation_method <= 1 ) {
                /* shape_complexity_estimation_disable */
                trp_vid_bs( vid, 1, &val );
                if ( val ) {
                    trp_vid_bs_skip( vid, 6 );
                }

                /* texture_complexity_estimation_set_1_disable */
                trp_vid_bs( vid, 1, &val );
                if ( val ) {
                    trp_vid_bs_skip( vid, 4 );
                }
                trp_vid_bs_skip( vid, 1 );

                /* texture_complexity_estimation_set_2_disable */
                trp_vid_bs( vid, 1, &val );
                if ( val ) {
                    trp_vid_bs_skip( vid, 4 );
                }

                /* motion_compensation_complexity_disable */
                trp_vid_bs( vid, 1, &val );
                if ( val ) {
                    trp_vid_bs_skip( vid, 6 );
                }
                trp_vid_bs_skip( vid, 1 );

                if ( estimation_method == 1 ) {
                    /* ersion2_complexity_estimation_disable */
                    trp_vid_bs( vid, 1, &val );
                    if ( val ) {
                        trp_vid_bs_skip( vid, 2 );
                    }
                }
            }
        }
        trp_vid_bs_skip( vid, 1 );

        trp_vid_bs( vid, 1, &val );
        if ( val ) {
            trp_vid_bs_skip( vid, 1 );
        }

        if ( ver_id != 1 ) {
            trp_vid_bs( vid, 1, &( vid->newpred_enable ) );
            if ( vid->newpred_enable ) {
                trp_vid_bs_skip( vid, 3 );
            }
            trp_vid_bs( vid, 1, &( vid->reduced_resolution_enable ) );
        }

        trp_vid_bs( vid, 1, &val );
        if ( val ) {
            trp_vid_bs( vid, 1, &hierarchy );
            trp_vid_bs_skip( vid, 26 );
            if ( ( vid->shape == 1 ) && ( hierarchy == 0 ) ) {
                trp_vid_bs_skip( vid, 22 );
            }
        }
    } else {
        if ( ver_id != 1 ) {
            trp_vid_bs( vid, 1, &val );
            if ( val ) {
                trp_vid_bs_skip( vid, 24 );
            }
            trp_vid_bs_skip( vid, 1 );
        }
    }
    return 0;
}

static uns8b trp_vid_parse_vop( trp_vid_t *vid )
{
    uns32b val, i, modulo_time_base = 0, coding_type, typ, time_inc, is_reference, qscale = 0;

    vid->rl = 0;
    vid->r = 0;

    trp_vid_bs( vid, 2, &coding_type );
    for ( ; ; ) {
        trp_vid_bs( vid, 1, &val );
        if ( val == 0 )
            break;
        modulo_time_base++;
    }
    trp_vid_bs_skip( vid, 1 );

    trp_vid_bs( vid, vid->time_inc_bits, &time_inc );
    trp_vid_bs_skip( vid, 1 );

    trp_vid_bs( vid, 1, &( vid->coded ) );

    typ = vid->coded ? coding_type : 4;
    is_reference = ( vid->coded && ( typ != 2 ) ) ? 1 : 0;

    /*
     FIXME
     aggiungere il calcolo del display time?
     */

    if ( vid->coded ) {

        if ( typ == 2 ) {
            if ( vid->cnt_bframe && ( vid->cnt_bframe <= MAX_BFRAMES ) )
                vid->cnt_bframes[ vid->cnt_bframe ]--;
            vid->cnt_bframe++;
            if ( vid->cnt_bframe <= MAX_BFRAMES )
                vid->cnt_bframes[ vid->cnt_bframe ]++;
            if ( vid->cnt_bframe > vid->max_bframes )
                vid->max_bframes = vid->cnt_bframe;
        } else {
            vid->cnt_bframe = 0;
        }

        if ( vid->newpred_enable ) {
            i = vid->time_inc_bits + 3;
            if ( i > 15 )
                i = 15;
            trp_vid_bs_skip( vid, i );
            trp_vid_bs( vid, 1, &val );
            if ( val ) {
                trp_vid_bs_skip( vid, i );
            }
            trp_vid_bs_skip( vid, 1 );
        }

        if ( ( vid->shape != 2 ) &&
             ( ( coding_type == 1 ) ||
               ( ( coding_type == 3 ) &&
                 ( vid->sprite_enable == 2 ) ) ) ) {
            trp_vid_bs_skip( vid, 1 );
        }

        if ( vid->reduced_resolution_enable &&
             ( vid->shape == 0 ) &&
             ( coding_type <= 1 ) ) {
            trp_vid_bs_skip( vid, 1 );
        }

        if ( vid->shape ) {
            if ( !( ( vid->sprite_enable == 1 ) &&
                    ( coding_type == 0 ) ) ) {
                trp_vid_bs_skip( vid, 32 );
                trp_vid_bs_skip( vid, 24 );
            }
            trp_vid_bs_skip( vid, 1 );
            trp_vid_bs( vid, 1, &val );
            if ( val ) {
                trp_vid_bs_skip( vid, 8 );
            }
        }

        if ( vid->shape != 2 ) {
            trp_vid_bs_skip( vid, 3 );
            if ( vid->interlaced ) {
                trp_vid_bs( vid, 1, &( vid->tff ) );
                trp_vid_bs( vid, 1, &( vid->alternate_scan ) );
            }
        }

        if ( vid->sprite_enable && ( coding_type == 3 ) ) {
            uns32b warping_points_used = 0;

            for ( i = 1 ; i <= vid->sprite_warping_points ; i++ ) {
                if ( trp_vid_warping_code( vid, &val ) )
                    return 1;
                if ( val ) {
                    warping_points_used = i;
                }
                /*
                 secondo libavcodec questo skip non va
                 fatto su una particolare versione di DivX...
                 (v. h263.c)
                 */
                if ( ( vid->divx_version != 500 ) || ( vid->divx_build != 413 ) )
                    trp_vid_bs_skip( vid, 1 );
                if ( trp_vid_warping_code( vid, &val ) )
                    return 1;
                if ( val ) {
                    warping_points_used = i;
                }
                trp_vid_bs_skip( vid, 1 );
            }
            if ( warping_points_used ) {
                vid->cnt_warp_points_used[ warping_points_used ]++;
            }
        }

        if ( vid->shape != 2 ) {
            trp_vid_bs( vid, vid->quant_precision, &qscale );
            if ( qscale == 0 )
                qscale = 1;
        }
    }
    if ( ( typ == 4 ) && vid->cnt_packed ) {
        vid->cnt_packed--;
    } else {
        trp_vid_update_qscale( vid, 1, typ, qscale );
    }
    return 0;
}

static uns8b trp_vid_bs_basic( trp_vid_t *vid, int size, uns32b *val, uns8b update )
{
    int rl;
    uns8b r;

    rl = vid->rl;
    r = vid->r;
    if ( size <= rl ) {
        rl -= size;
        *val = ( r >> ( 8 - size ) );
        r <<= size;
    } else {
        uns32b resto;
        uns8b *p;

        resto = vid->buf_size - vid->buf_pos;
        p = vid->buf + vid->buf_pos;
        *val = ( ( size <= 8 ) ? ( r >> ( 8 - size ) ) : ( r << ( size - 8 ) ) );
        size -= rl;
        if ( resto == 0 )
            return 1;
        resto--;
        rl = 8;
        r = *p++;
        if ( size <= rl ) {
            rl -= size;
            *val |= ( r >> ( 8 - size ) );
            r <<= size;
        } else {
            *val |= ( ( size <= 8 ) ? ( r >> ( 8 - size ) ) : ( r << ( size - 8 ) ) );
            size -= rl;
            if ( resto == 0 )
                return 1;
            resto--;
            rl = 8;
            r = *p++;
            if ( size <= rl ) {
                rl -= size;
                *val |= ( r >> ( 8 - size ) );
                r <<= size;
            } else {
                *val |= ( ( size <= 8 ) ? ( r >> ( 8 - size ) ) : ( r << ( size - 8 ) ) );
                size -= rl;
                if ( resto == 0 )
                    return 1;
                resto--;
                rl = 8;
                r = *p++;

                if ( size <= rl ) {
                    rl -= size;
                    *val |= ( r >> ( 8 - size ) );
                    r <<= size;
                } else {
                    *val |= ( ( size <= 8 ) ? ( r >> ( 8 - size ) ) : ( r << ( size - 8 ) ) );
                    size -= rl;
                    if ( resto == 0 )
                        return 1;
                    resto--;
                    rl = 8;
                    r = *p++;

                    rl -= size;
                    *val |= ( r >> ( 8 - size ) );
                    r <<= size;
                }
            }
        }
        if ( update ) {
            vid->buf_pos = vid->buf_size - resto;
        }
    }
    if ( update ) {
        vid->rl = rl;
        vid->r = r;
    }
    return 0;
}

static uns8b trp_vid_bs_skip( trp_vid_t *vid, int size )
{
    uns32b val;

    return trp_vid_bs_basic( vid, size, &val, 1 );
}

static uns8b trp_vid_bs_peek( trp_vid_t *vid, int size, uns32b *val )
{
    return trp_vid_bs_basic( vid, size, val, 0 );
}

static uns8b trp_vid_bs( trp_vid_t *vid, int size, uns32b *val )
{
    return trp_vid_bs_basic( vid, size, val, 1 );
}

static uns8b trp_vid_bs_matrix( trp_vid_t *vid, uns8b **qm )
{
    extern uns8b _ZZ_SCAN8[];
    uns32b val, lval;
    int i;

    if ( *qm == NULL )
        *qm = trp_gc_malloc( 64 );
    for ( i = 0 ; i < 64 ; i++ ) {
        if ( trp_vid_bs( vid, 8, &val ) ) {
            vid->error = "Errore lettura matrice";
            return 1;
        }
        if ( val == 0 )
            break;
        ( *qm )[ _ZZ_SCAN8[ i ] ] = (uns8b)val;
        lval = val;
    }
    for ( ; i < 64 ; i++ ) {
        ( *qm )[ _ZZ_SCAN8[ i ] ] = (uns8b)lval;
    }
    return 0;
}

static uns8b trp_vid_warping_code( trp_vid_t *vid, uns32b *used )
{
    uns32b val;
    int i;

    for ( i = 0 ; i < 15 ; i++ ) {
        if ( trp_vid_bs_peek( vid, _sprite_traj_len_bits[ i ], &val ) ) {
            vid->error = "Errore lettura warping code (1)";
            return 1;
        }
        if ( val == _sprite_traj_len_code[ i ] ) {
            trp_vid_bs_skip( vid, _sprite_traj_len_bits[ i ] );
            if ( i )
                if ( trp_vid_bs_skip( vid, i ) ) {
                    vid->error = "Errore lettura warping code (2)";
                    return 1;
                }
            *used = i ? 1 : 0;
            return 0;
        }
    }
    vid->error = "Invalid VOP (unable to find sprite trajectory length VLC)";
    return 1;
}

static uns32b trp_vid_bitsused( uns32b val )
{
    uns32b res = 0;

    while ( val ) {
        val >>= 1;
        res++;
    }
    return res;
}

