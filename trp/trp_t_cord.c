/*
    TreeP Run Time Support
    Copyright (C) 2008-2021 Frank Sinapsi

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

/*
 FIXME
 controllare che questo sorgente vada bene anche su sistemi big endian...
 */

#include "trp.h"

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

typedef struct {
    sig32b pos;
    sig32b result;
    trp_queue_t q;
    trp_array_t a;
    uns8b ignore_case;
} trp_cord_match_t;

typedef struct {
    uns32b cnt;
    trp_queue_t q;
} trp_cord_trim_t;

extern void trp_queue_init_internal( trp_queue_t *q );

static trp_obj_t *trp_cord_any2utf8( trp_obj_t *obj, uns8b *tbl );
static uns8b trp_cord_search_internal( uns8b flags, trp_obj_t *obj, trp_obj_t *s, uns32b *pos, uns32b nth );
static int trp_cord_match_pre_cback( uns8b c, trp_cord_match_t *m );
static int trp_cord_match_cback( uns8b c, trp_cord_match_t *m );
static trp_obj_t *trp_cord_match_internal( uns8b flags, trp_obj_t **o, va_list args, uns32b *idx );
static trp_obj_t *trp_cord_match_less( trp_cons_t *i, trp_cons_t *j );
static void trp_cord_match_clear( trp_cord_match_t *m );
static int trp_cord_trim_cback( uns8b c, trp_cord_trim_t *m );
static trp_obj_t *trp_cord_trim_internal( uns8b flags, trp_obj_t *s, va_list args );
static trp_obj_t *trp_cord_max_fix_basic( uns8b flags, trp_obj_t *s1, trp_obj_t *s2 );
static sig32b trp_cord_utf8_next( uns8b *p );
static uns8b trp_cord_decode_scoring_matrix( trp_obj_t *scoring_matrix, sig32b *sm );
static sig32b trp_cord_amino_index( uns8b amino );
static trp_obj_t *trp_cord_alignment_score_low( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix );
static uns8b trp_cord_global_alignment_affine_low( uns8b flags, trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, sig32b *sm, sig32b *score, trp_obj_t **as1, trp_obj_t **as2 );
static uns8b trp_cord_local_alignment_affine_low( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, sig32b *sm, sig32b *score, trp_obj_t **as1, trp_obj_t **as2 );

static uns8b _trp_cord_iso2utf8[ 256 ] = {
    194,128,194,129,194,130,194,131,194,132,194,133,194,134,194,135,
    194,136,194,137,194,138,194,139,194,140,194,141,194,142,194,143,
    194,144,194,145,194,146,194,147,194,148,194,149,194,150,194,151,
    194,152,194,153,194,154,194,155,194,156,194,157,194,158,194,159,
    194,160,194,161,194,162,194,163,194,164,194,165,194,166,194,167,
    194,168,194,169,194,170,194,171,194,172,194,173,194,174,194,175,
    194,176,194,177,194,178,194,179,194,180,194,181,194,182,194,183,
    194,184,194,185,194,186,194,187,194,188,194,189,194,190,194,191,
    195,128,195,129,195,130,195,131,195,132,195,133,195,134,195,135,
    195,136,195,137,195,138,195,139,195,140,195,141,195,142,195,143,
    195,144,195,145,195,146,195,147,195,148,195,149,195,150,195,151,
    195,152,195,153,195,154,195,155,195,156,195,157,195,158,195,159,
    195,160,195,161,195,162,195,163,195,164,195,165,195,166,195,167,
    195,168,195,169,195,170,195,171,195,172,195,173,195,174,195,175,
    195,176,195,177,195,178,195,179,195,180,195,181,195,182,195,183,
    195,184,195,185,195,186,195,187,195,188,195,189,195,190,195,191
};

static uns8b _trp_cord_koi8_r2utf8[ 384 ] = {
    226,148,128,226,148,130,226,148,140,226,148,144,226,148,148,226,
    148,152,226,148,156,226,148,164,226,148,172,226,148,180,226,148,
    188,226,150,128,226,150,132,226,150,136,226,150,140,226,150,144,
    226,150,145,226,150,146,226,150,147,226,140,160,226,150,160,226,
    136,153,226,136,154,226,137,136,226,137,164,226,137,165,194,160,
      0,226,140,161,194,176,  0,194,178,  0,194,183,  0,195,183,  0,
    226,149,144,226,149,145,226,149,146,209,145,  0,226,149,147,226,
    149,148,226,149,149,226,149,150,226,149,151,226,149,152,226,149,
    153,226,149,154,226,149,155,226,149,156,226,149,157,226,149,158,
    226,149,159,226,149,160,226,149,161,208,129,  0,226,149,162,226,
    149,163,226,149,164,226,149,165,226,149,166,226,149,167,226,149,
    168,226,149,169,226,149,170,226,149,171,226,149,172,194,169,  0,
    209,142,  0,208,176,  0,208,177,  0,209,134,  0,208,180,  0,208,
    181,  0,209,132,  0,208,179,  0,209,133,  0,208,184,  0,208,185,
      0,208,186,  0,208,187,  0,208,188,  0,208,189,  0,208,190,  0,
    208,191,  0,209,143,  0,209,128,  0,209,129,  0,209,130,  0,209,
    131,  0,208,182,  0,208,178,  0,209,140,  0,209,139,  0,208,183,
      0,209,136,  0,209,141,  0,209,137,  0,209,135,  0,209,138,  0,
    208,174,  0,208,144,  0,208,145,  0,208,166,  0,208,148,  0,208,
    149,  0,208,164,  0,208,147,  0,208,165,  0,208,152,  0,208,153,
      0,208,154,  0,208,155,  0,208,156,  0,208,157,  0,208,158,  0,
    208,159,  0,208,175,  0,208,160,  0,208,161,  0,208,162,  0,208,
    163,  0,208,150,  0,208,146,  0,208,172,  0,208,171,  0,208,151,
      0,208,168,  0,208,173,  0,208,169,  0,208,167,  0,208,170,  0
};

static uns8b _trp_cord_greek2utf8[ 384 ] = {
    194,128,  0,194,129,  0,194,130,  0,194,131,  0,194,132,  0,194,
    133,  0,194,134,  0,194,135,  0,194,136,  0,194,137,  0,194,138,
      0,194,139,  0,194,140,  0,194,141,  0,194,142,  0,194,143,  0,
    194,144,  0,194,145,  0,194,146,  0,194,147,  0,194,148,  0,194,
    149,  0,194,150,  0,194,151,  0,194,152,  0,194,153,  0,194,154,
      0,194,155,  0,194,156,  0,194,157,  0,194,158,  0,194,159,  0,
    194,160,  0,226,128,152,226,128,153,194,163,  0,226,130,172,226,
    130,175,194,166,  0,194,167,  0,194,168,  0,194,169,  0,205,186,
      0,194,171,  0,194,172,  0,194,173,  0,  0,  0,  0,226,128,149,
    194,176,  0,194,177,  0,194,178,  0,194,179,  0,206,132,  0,206,
    133,  0,206,134,  0,194,183,  0,206,136,  0,206,137,  0,206,138,
      0,194,187,  0,206,140,  0,194,189,  0,206,142,  0,206,143,  0,
    206,144,  0,206,145,  0,206,146,  0,206,147,  0,206,148,  0,206,
    149,  0,206,150,  0,206,151,  0,206,152,  0,206,153,  0,206,154,
      0,206,155,  0,206,156,  0,206,157,  0,206,158,  0,206,159,  0,
    206,160,  0,206,161,  0,  0,  0,  0,206,163,  0,206,164,  0,206,
    165,  0,206,166,  0,206,167,  0,206,168,  0,206,169,  0,206,170,
      0,206,171,  0,206,172,  0,206,173,  0,206,174,  0,206,175,  0,
    206,176,  0,206,177,  0,206,178,  0,206,179,  0,206,180,  0,206,
    181,  0,206,182,  0,206,183,  0,206,184,  0,206,185,  0,206,186,
      0,206,187,  0,206,188,  0,206,189,  0,206,190,  0,206,191,  0,
    207,128,  0,207,129,  0,207,130,  0,207,131,  0,207,132,  0,207,
    133,  0,207,134,  0,207,135,  0,207,136,  0,207,137,  0,207,138,
      0,207,139,  0,207,140,  0,207,141,  0,207,142,  0,  0,  0,  0
};

static uns8b _trp_cord_windows12522utf8[ 384 ] = {
    226,130,172,  0,  0,  0,226,128,154,198,146,  0,226,128,158,226,
    128,166,226,128,160,226,128,161,203,134,  0,226,128,176,197,160,
      0,226,128,185,197,146,  0,  0,  0,  0,197,189,  0,  0,  0,  0,
      0,  0,  0,226,128,152,226,128,153,226,128,156,226,128,157,226,
    128,162,226,128,147,226,128,148,203,156,  0,226,132,162,197,161,
      0,226,128,186,197,147,  0,  0,  0,  0,197,190,  0,197,184,  0,
    194,160,  0,194,161,  0,194,162,  0,194,163,  0,194,164,  0,194,
    165,  0,194,166,  0,194,167,  0,194,168,  0,194,169,  0,194,170,
      0,194,171,  0,194,172,  0,194,173,  0,194,174,  0,194,175,  0,
    194,176,  0,194,177,  0,194,178,  0,194,179,  0,194,180,  0,194,
    181,  0,194,182,  0,194,183,  0,194,184,  0,194,185,  0,194,186,
      0,194,187,  0,194,188,  0,194,189,  0,194,190,  0,194,191,  0,
    195,128,  0,195,129,  0,195,130,  0,195,131,  0,195,132,  0,195,
    133,  0,195,134,  0,195,135,  0,195,136,  0,195,137,  0,195,138,
      0,195,139,  0,195,140,  0,195,141,  0,195,142,  0,195,143,  0,
    195,144,  0,195,145,  0,195,146,  0,195,147,  0,195,148,  0,195,
    149,  0,195,150,  0,195,151,  0,195,152,  0,195,153,  0,195,154,
      0,195,155,  0,195,156,  0,195,157,  0,195,158,  0,195,159,  0,
    195,160,  0,195,161,  0,195,162,  0,195,163,  0,195,164,  0,195,
    165,  0,195,166,  0,195,167,  0,195,168,  0,195,169,  0,195,170,
      0,195,171,  0,195,172,  0,195,173,  0,195,174,  0,195,175,  0,
    195,176,  0,195,177,  0,195,178,  0,195,179,  0,195,180,  0,195,
    181,  0,195,182,  0,195,183,  0,195,184,  0,195,185,  0,195,186,
      0,195,187,  0,195,188,  0,195,189,  0,195,190,  0,195,191,  0
};

static sig32b _trp_cord_blosum62[ 400 ] = {
 4,  0, -2, -1, -2,  0, -2, -1, -1, -1, -1, -2, -1, -1, -1,  1,  0,  0, -3, -2,
 0,  9, -3, -4, -2, -3, -3, -1, -3, -1, -1, -3, -3, -3, -3, -1, -1, -1, -2, -2,
-2, -3,  6,  2, -3, -1, -1, -3, -1, -4, -3,  1, -1,  0, -2,  0, -1, -3, -4, -3,
-1, -4,  2,  5, -3, -2,  0, -3,  1, -3, -2,  0, -1,  2,  0,  0, -1, -2, -3, -2,
-2, -2, -3, -3,  6, -3, -1,  0, -3,  0,  0, -3, -4, -3, -3, -2, -2, -1,  1,  3,
 0, -3, -1, -2, -3,  6, -2, -4, -2, -4, -3,  0, -2, -2, -2,  0, -2, -3, -2, -3,
-2, -3, -1,  0, -1, -2,  8, -3, -1, -3, -2,  1, -2,  0,  0, -1, -2, -3, -2,  2,
-1, -1, -3, -3,  0, -4, -3,  4, -3,  2,  1, -3, -3, -3, -3, -2, -1,  3, -3, -1,
-1, -3, -1,  1, -3, -2, -1, -3,  5, -2, -1,  0, -1,  1,  2,  0, -1, -2, -3, -2,
-1, -1, -4, -3,  0, -4, -3,  2, -2,  4,  2, -3, -3, -2, -2, -2, -1,  1, -2, -1,
-1, -1, -3, -2,  0, -3, -2,  1, -1,  2,  5, -2, -2,  0, -1, -1, -1,  1, -1, -1,
-2, -3,  1,  0, -3,  0,  1, -3,  0, -3, -2,  6, -2,  0,  0,  1,  0, -3, -4, -2,
-1, -3, -1, -1, -4, -2, -2, -3, -1, -3, -2, -2,  7, -1, -2, -1, -1, -2, -4, -3,
-1, -3,  0,  2, -3, -2,  0, -3,  1, -2,  0,  0, -1,  5,  1,  0, -1, -2, -2, -1,
-1, -3, -2,  0, -3, -2,  0, -3,  2, -2, -1,  0, -2,  1,  5, -1, -1, -3, -3, -2,
 1, -1,  0,  0, -2,  0, -1, -2,  0, -2, -1,  1, -1,  0, -1,  4,  1, -2, -3, -2,
 0, -1, -1, -1, -2, -2, -2, -1, -1, -1, -1,  0, -1, -1, -1,  1,  5,  0, -2, -2,
 0, -1, -3, -2, -1, -3, -3,  3, -2,  1,  1, -3, -2, -2, -3, -2,  0,  4, -3, -1,
-3, -2, -4, -3,  1, -2, -2, -3, -3, -2, -1, -4, -4, -2, -3, -3, -2, -3, 11,  2,
-2, -2, -3, -2,  3, -3,  2, -1, -2, -1, -1, -2, -3, -1, -2, -2, -2, -1,  2,  7
};

static sig32b _trp_cord_pam250[ 400 ] = {
 2, -2,  0,  0, -3,  1, -1, -1, -1, -2, -1,  0,  1,  0, -2,  1,  1,  0, -6, -3,
-2, 12, -5, -5, -4, -3, -3, -2, -5, -6, -5, -4, -3, -5, -4,  0, -2, -2, -8,  0,
 0, -5,  4,  3, -6,  1,  1, -2,  0, -4, -3,  2, -1,  2, -1,  0,  0, -2, -7, -4,
 0, -5,  3,  4, -5,  0,  1, -2,  0, -3, -2,  1, -1,  2, -1,  0,  0, -2, -7, -4,
-3, -4, -6, -5,  9, -5, -2,  1, -5,  2,  0, -3, -5, -5, -4, -3, -3, -1,  0,  7,
 1, -3,  1,  0, -5,  5, -2, -3, -2, -4, -3,  0,  0, -1, -3,  1,  0, -1, -7, -5,
-1, -3,  1,  1, -2, -2,  6, -2,  0, -2, -2,  2,  0,  3,  2, -1, -1, -2, -3,  0,
-1, -2, -2, -2,  1, -3, -2,  5, -2,  2,  2, -2, -2, -2, -2, -1,  0,  4, -5, -1,
-1, -5,  0,  0, -5, -2,  0, -2,  5, -3,  0,  1, -1,  1,  3,  0,  0, -2, -3, -4,
-2, -6, -4, -3,  2, -4, -2,  2, -3,  6,  4, -3, -3, -2, -3, -3, -2,  2, -2, -1,
-1, -5, -3, -2,  0, -3, -2,  2,  0,  4,  6, -2, -2, -1,  0, -2, -1,  2, -4, -2,
 0, -4,  2,  1, -3,  0,  2, -2,  1, -3, -2,  2,  0,  1,  0,  1,  0, -2, -4, -2,
 1, -3, -1, -1, -5,  0,  0, -2, -1, -3, -2,  0,  6,  0,  0,  1,  0, -1, -6, -5,
 0, -5,  2,  2, -5, -1,  3, -2,  1, -2, -1,  1,  0,  4,  1, -1, -1, -2, -5, -4,
-2, -4, -1, -1, -4, -3,  2, -2,  3, -3,  0,  0,  0,  1,  6,  0, -1, -2,  2, -4,
 1,  0,  0,  0, -3,  1, -1, -1,  0, -3, -2,  1,  1, -1,  0,  2,  1, -1, -2, -3,
 1, -2,  0,  0, -3,  0, -1,  0,  0, -2, -1,  0,  0, -1, -1,  1,  3,  0, -5, -3,
 0, -2, -2, -2, -1, -1, -2,  4, -2,  2,  2, -2, -1, -2, -2, -1,  0,  4, -6, -2,
-6, -8, -7, -7,  0, -7, -3, -5, -3, -2, -4, -4, -6, -5,  2, -2, -5, -6, 17,  0,
-3,  0, -4, -4,  7, -5,  0, -1, -4, -1, -2, -2, -5, -4, -4, -3, -3, -2,  0, 10
};

static uns8b _amino_acid[ 20 ] = {
    'A','C','D','E','F','G','H','I','K','L',
    'M','N','P','Q','R','S','T','V','W','Y'
};

static double _amino_acid_weight[ 20 ] = {
    71.03711, 103.00919, 115.02694, 129.04259, 147.06841,
    57.02146, 137.05891, 113.08406, 128.09496, 113.08406,
    131.04049, 114.04293, 97.05276, 128.05858, 156.10111,
    87.03203, 101.04768, 99.06841, 186.07931, 163.06333
};

uns8b trp_cord_print( trp_print_t *p, trp_cord_t *obj )
{
    CORD_pos i;

    CORD_FOR( i, obj->c )
        if ( trp_print_char( p, CORD_pos_fetch( i ) ) )
            return 1;
    return 0;
}

uns32b trp_cord_size( trp_cord_t *obj )
{
    return 1 + 4 + obj->len;
}

void trp_cord_encode( trp_cord_t *obj, uns8b **buf )
{
    uns32b *p;
    CORD_pos i;

    **buf = TRP_CORD;
    ++(*buf);
    p = (uns32b *)(*buf);
    *p = norm32( obj->len );
    (*buf) += 4;
    CORD_FOR( i, obj->c ) {
        **buf = CORD_pos_fetch( i );
        ++(*buf);
    }
}

trp_obj_t *trp_cord_decode( uns8b **buf )
{
    uns32b len;
    trp_obj_t *res;

    len = norm32( *((uns32b *)(*buf)) );
    (*buf) += 4;
    if ( len ) {
        uns32b i;
        CORD_ec x;

        CORD_ec_init( x );
        for( i = 0 ; i < len ; i++ ) {
            if ( **buf == 0 ) {
                register size_t count = 1;

                CORD_ec_flush_buf( x );
                for ( i++ ; i < len ; i++ ) {
                    ++(*buf);
                    if ( **buf )
                        break;
                    count++;
                }
                x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( count ) );
                if ( i == len )
                    break;
            }
            CORD_ec_append( x, **buf );
            ++(*buf);
        }
        res = trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
    } else {
        res = EMPTYCORD;
    }
    return res;
}

/*
 ho riscritto CORD_cmp perché quella originale non funziona su
 specifici casi; il motivo è che usa CORD_pos_chars_left(p)
 in modo improprio: da cord_pos.h:
 Number of characters in cache.  <= 0 ==> none
 però in CORD_cmp abbiamo i test == 0 (nelle vecchie versioni
 c'era correttamente <= 0);
 il modo in cui l'ho riscritto è un po' più inefficiente
 ma corretto
 */

static int trp_CORD_cmp( trp_cord_t *o1, trp_cord_t *o2 )
{
    CORD_pos pos1, pos2;
    CORD c1, c2;
    uns32b len1 = o1->len, len2 = o2->len, len;
    uns8b v1, v2;

    if ( len2 == 0 )
        return len1 ? 1 : 0;
    if ( len1 == 0 )
        return -1;
    c1 = o1->c;
    c2 = o2->c;
    if ( CORD_IS_STRING( c1 ) && CORD_IS_STRING( c2 ) )
        return strcmp( c1, c2 );
    CORD_set_pos( pos1, c1, 0 );
    CORD_set_pos( pos2, c2, 0 );
    len = ( len1 < len2 ) ? len1 : len2;
    for ( ; ; ) {
        v1 = (uns8b)( CORD_pos_fetch( pos1 ) );
        v2 = (uns8b)( CORD_pos_fetch( pos2 ) );
        if ( v1 != v2 )
            return ( v1 < v2 ) ? -1 : 1;
        if ( --len == 0 )
            break;
        CORD_next( pos1 );
        CORD_next( pos2 );
    }
    return ( len1 == len2 ) ? 0 : ( ( len1 < len2 ) ? -1 : 1 );
}

trp_obj_t *trp_cord_equal( trp_cord_t *o1, trp_cord_t *o2 )
{
    if ( o1->len != o2->len )
        return TRP_FALSE;
    return ( trp_CORD_cmp( o1, o2 ) == 0 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_cord_less( trp_cord_t *o1, trp_cord_t *o2 )
{
    return ( trp_CORD_cmp( o1, o2 ) < 0 ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_cord_length( trp_cord_t *obj )
{
    return trp_sig64( obj->len );
}

trp_obj_t *trp_cord_nth( uns32b n, trp_cord_t *obj )
{
    if ( n >= obj->len )
        return UNDEF;
    return trp_char( (uns8b)CORD_fetch( obj->c, n ) );
}

trp_obj_t *trp_cord_sub( uns32b start, uns32b len, trp_cord_t *obj )
{
    uns32b n;

    if ( start > obj->len )
        return UNDEF;
    n = obj->len - start;
    if ( n > len )
        n = len;
    if ( n == 0 )
        return EMPTYCORD;
    return trp_cord_cons( CORD_substr( obj->c, start, n ), n );
}

trp_obj_t *trp_cord_cat( trp_cord_t *obj, va_list args )
{
    uns32b len;
    CORD res;

    res = obj->c;
    len = obj->len;
    for ( obj = va_arg( args, trp_cord_t * ) ;
          obj ;
          obj = va_arg( args, trp_cord_t * ) ) {
        if ( obj->tipo != TRP_CORD )
            obj = (trp_cord_t *)trp_sprint( (trp_obj_t *)obj, NULL );
        res = CORD_cat( res, obj->c );
        len += obj->len;
    }
    return trp_cord_cons( res, len );
}

uns8b trp_cord_in( trp_obj_t *obj, trp_cord_t *seq, uns32b *pos, uns32b nth )
{
    uns32b i = 0;
    uns8b res = 1, d;
    CORD_pos j;

    if ( obj->tipo != TRP_CHAR )
        return trp_cord_search_internal( 0, obj, (trp_obj_t *)seq, pos, nth );
    d = ((trp_char_t *)obj)->c;
    CORD_FOR( j, seq->c ) {
        if ( CORD_pos_fetch( j ) == d ) {
            res = 0;
            *pos = i;
            if ( nth == 0 )
                break;
            nth--;
        }
        i++;
    }
    return res;
}

trp_obj_t *trp_cord_reverse( trp_cord_t * obj )
{
    uns32b len = obj->len;
    CORD_pos i;
    CORD_ec x;

    for ( CORD_set_pos( i, obj->c, len - 1 ), CORD_ec_init( x ) ;
          CORD_pos_valid( i ) ;
          CORD_prev( i ) )
        CORD_ec_append( x, CORD_pos_fetch( i ) );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_cord_cons( CORD c, uns32b len )
{
    trp_cord_t *obj;

    if ( len == 0 )
        return EMPTYCORD;
    obj = trp_gc_malloc( sizeof( trp_cord_t ) );
    obj->tipo = TRP_CORD;
    obj->len = len;
    obj->c = c;
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_cord( const uns8b *s )
{
    CORD c = CORD_from_char_star( s );
    return trp_cord_cons( c, CORD_len( c ) );
}

trp_obj_t *trp_cord_empty()
{
    static trp_cord_t *obj = NULL;

    if ( obj == NULL ) {
        obj = trp_gc_malloc( sizeof( trp_cord_t ) );
        obj->tipo = TRP_CORD;
        obj->len = 0;
        obj->c = CORD_EMPTY;
    }
    return (trp_obj_t *)obj;
}

trp_obj_t *trp_cord_explode( trp_obj_t *obj )
{
    trp_obj_t *res = NIL;
    trp_cons_t *c, *prec;
    CORD_pos i;

    if ( obj->tipo != TRP_CORD )
        return UNDEF;

    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        c = (trp_cons_t *)trp_cons( trp_char( CORD_pos_fetch( i ) ), NIL );
        if ( res == NIL )
            res = (trp_obj_t *)c;
        else
            prec->cdr = (trp_obj_t *)c;
        prec = c;
    }
    return res;
}

trp_obj_t *trp_cord_utf8_length( trp_obj_t *obj )
{
    uns32b l = 0;
    CORD_pos i;
    uns8b c, pc, n, to_ignore = 0;

    if ( obj->tipo != TRP_CORD )
        return UNDEF;
    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        c = CORD_pos_fetch( i );
        if ( to_ignore ) {
            if ( ( c ^ 0x80 ) >= 0x40 )
                break;
            if ( n == 3 ) {
                if ( to_ignore == 2 ) {
                    if ( ( ( pc < 0xe1 ) && ( c < 0xa0 ) ) ||
                         ( ( pc == 0xed ) && ( c >= 0xa0 ) ) )
                        break;
                }
            } else if ( n == 4 ) {
                if ( to_ignore == 3 ) {
                    if ( ( ( pc < 0xf1 ) && ( c < 0x90 ) ) ||
                         ( ( pc >= 0xf4 ) && ( ( pc != 0xf4 ) || ( c >= 0x90 ) ) ) )
                        break;
                }
            }
            to_ignore--;
            continue;
        }
        l++;
        if ( c < 0x80 )
            continue;
        if ( c < 0xc2 ) {
            to_ignore = 1;
            break;
        }
        if ( c < 0xe0 ) {
            n = 2;
            to_ignore = 1;
            continue;
        }
        if ( c < 0xf0 ) {
            pc = c;
            n = 3;
            to_ignore = 2;
            continue;
        }
        if ( c < 0xf8 ) {
            pc = c;
            n = 4;
            to_ignore = 3;
            continue;
        }
        to_ignore = 1;
        break;
    }
    return to_ignore ? UNDEF : trp_sig64( l );
}

trp_obj_t *trp_cord_iso2utf8( trp_obj_t *obj )
{
    uns32b len = 0;
    CORD_pos i;
    CORD_ec x;
    uns8b c;

    if ( obj->tipo != TRP_CORD )
        return UNDEF;
    CORD_ec_init( x );
    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        c = CORD_pos_fetch( i );
        if ( c & 0x80 ) {
            c <<= 1;
            CORD_ec_append( x, _trp_cord_iso2utf8[ c ] );
            CORD_ec_append( x, _trp_cord_iso2utf8[ c | 1 ] );
            len += 2;
        } else {
            if ( c ) {
                CORD_ec_append( x, c );
                ++len;
            }
        }
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_cord_utf82iso( trp_obj_t *obj )
{
    uns32b len = 0;
    CORD_pos i;
    CORD_ec x;
    uns16b *p, *q;
    uns8b c = 0, j, s[ 2 ];

    if ( obj->tipo != TRP_CORD )
        return UNDEF;
    p = (uns16b *)s;
    q = (uns16b *)_trp_cord_iso2utf8;
    CORD_ec_init( x );
    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        if ( c & 0x80 ) {
            s[ 0 ] = c;
            s[ 1 ] = CORD_pos_fetch( i );
            for ( j = 0 ; j < 128 ; j++ )
                if ( *p == q[ j ] )
                    break;
            if ( j == 128 )
                break;
            CORD_ec_append( x, j | 0x80 );
            ++len;
            c = 0;
        } else {
            c = CORD_pos_fetch( i );
            if ( c && ( c < 0x80 ) ) {
                CORD_ec_append( x, c );
                ++len;
            }
        }
    }
    if ( c & 0x80 )
        return UNDEF;
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

static trp_obj_t *trp_cord_any2utf8( trp_obj_t *obj, uns8b *tbl )
{
    uns32b len = 0;
    CORD_pos i;
    CORD_ec x;
    uns16b d;
    uns8b c;

    if ( obj->tipo != TRP_CORD )
        return UNDEF;
    CORD_ec_init( x );
    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        c = CORD_pos_fetch( i );
        if ( c & 0x80 ) {
            d = 3 * (uns16b)( c & 0x7f );
            if ( tbl[ d ] ) {
                CORD_ec_append( x, tbl[ d ] );
                CORD_ec_append( x, tbl[ d + 1 ] );
                len += 2;
                if ( tbl[ d + 2 ] ) {
                    CORD_ec_append( x, tbl[ d + 2 ] );
                    ++len;
                }
            } else {
                CORD_ec_append( x, '?' );
                ++len;
            }
        } else {
            if ( c ) {
                CORD_ec_append( x, c );
                ++len;
            }
        }
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_cord_koi8_r2utf8( trp_obj_t *obj )
{
    return trp_cord_any2utf8( obj, _trp_cord_koi8_r2utf8 );
}

trp_obj_t *trp_cord_greek2utf8( trp_obj_t *obj )
{
    return trp_cord_any2utf8( obj, _trp_cord_greek2utf8 );
}

trp_obj_t *trp_cord_windows12522utf8( trp_obj_t *obj )
{
    return trp_cord_any2utf8( obj, _trp_cord_windows12522utf8 );
}

trp_obj_t *trp_cord_str2num( trp_obj_t *obj )
{
    trp_obj_t *res, *div, *man;
    CORD_pos i;
    uns8b c, neg, first;

    if ( obj->tipo != TRP_CORD )
        return UNDEF;

    man = UNDEF;
    res = ZERO;
    div = UNDEF;
    neg = 0;
    first = 1;
    CORD_FOR( i, ((trp_cord_t *)obj)->c ) {
        c = CORD_pos_fetch( i );
        if ( ( c >= '0' ) && ( c <= '9' ) ) {
            res = trp_cat( trp_math_times( res, DIECI, NULL ), trp_sig64( c - '0' ), NULL );
            if ( div != UNDEF )
                div = trp_math_times( div, DIECI, NULL );
            first = 0;
        } else if ( ( c == '.' ) && ( div == UNDEF ) && ( man == UNDEF ) ) {
            div = UNO;
            first = 0;
        } else if ( first && ( c == '-' ) ) {
            neg = 1;
            first = 0;
        } else if ( first && ( c == '+' ) ) {
            first = 0;
        } else if ( ( ( c == 'e' ) || ( c == 'E' ) ) && ( man == UNDEF ) ) {
            if ( first )
                man = UNO;
            else {
                if ( div != UNDEF )
                    res = trp_math_ratio( res, div, NULL );
                if ( neg )
                    res = trp_math_minus( ZERO, res, NULL );
                man = res;
            }
            res = ZERO;
            div = UNDEF;
            neg = 0;
            first = 1;
        } else
            return UNDEF;
    }
    if ( div != UNDEF )
        res = trp_math_ratio( res, div, NULL );
    if ( neg )
        res = trp_math_minus( ZERO, res, NULL );
    if ( man != UNDEF )
        res = trp_math_times( man, trp_math_pow( DIECI, res ), NULL );
    return res;
}

static uns8b trp_cord_search_internal( uns8b flags, trp_obj_t *obj, trp_obj_t *s, uns32b *pos, uns32b nth )
{
    uns32b pl, pi, pe, i, x, y;
    int fl;
    uns8b *p, *q, cs, res = 1;
    CORD_pos j;

    if ( ( obj->tipo != TRP_CORD ) ||
         ( s->tipo != TRP_CORD ) )
        return 1;
    pl = ((trp_cord_t *)obj)->len;
    if ( pl == 0 ) {
        *pos = 0;
        return 0;
    }

    if (  pl > ((trp_cord_t *)s)->len )
        return 1;

    p = trp_gc_malloc_atomic( pl );
    q = trp_gc_malloc_atomic( pl );
    i = 0;
    CORD_FOR( j, ((trp_cord_t *)obj)->c ) {
        cs =  CORD_pos_fetch( j );
        if ( flags )
            cs = trp_upcase( cs );
        q[ i++ ] = cs;
    }
    i = 1;
    x = 0;
    pi = 0;
    pe = 0;
    CORD_FOR( j, ((trp_cord_t *)s)->c ) {
        cs =  CORD_pos_fetch( j );
        if ( flags )
            cs = trp_upcase( cs );
        p[ pi++ ] = cs; if ( pi == pl ) pi = 0;
        if ( cs == q[ x ] ) {
            x++;
            if ( x == pl ) {
                *pos = i - pl;
                res = 0;
                if ( nth == 0 )
                    break;
                nth--;
                fl = 1;
                x--;
                pe++; if ( pe == pl ) pe = 0;
            } else {
                fl = 0;
            }
        } else {
            x++;
            fl = 1;
        }
        if ( fl ) {
            if ( x > 1 ) {
                x--;
                pe++; if ( pe == pl ) pe = 0;
            }
            for ( ; ; ) {
                fl = 1;
                for ( y = 0 ; y < x ; y++ )
                    if ( p[ ( y + pe ) % pl ] != q[ y ] ) {
                        fl = 0;
                        break;
                    }
                if ( fl )
                    break;
                x--;
                pe++; if ( pe == pl ) pe = 0;
                if ( x == 0 )
                    break;
            }
        }
        i++;
    }
    trp_gc_free( q );
    trp_gc_free( p );
    return res;
}

trp_obj_t *trp_cord_search_func( uns8b flags, trp_obj_t *obj, trp_obj_t *s )
{
    uns32b p;

    return trp_cord_search_internal( flags, obj, s, &p, 0 ) ? TRP_FALSE : TRP_TRUE;
}

uns8b trp_cord_search_test( uns8b flags, trp_obj_t *obj, trp_obj_t *s, trp_obj_t **pos, trp_obj_t *nth )
{
    uns32b p, n;

    if ( pos && nth ) {
        if ( nth->tipo != TRP_SIG64 )
            return 1;
        if ( ((trp_sig64_t *)nth)->val < 0 )
            return 1;
        n = ( ((trp_sig64_t *)nth)->val > 0xffffffff )
            ? 0xffffffff
            : (uns32b)(((trp_sig64_t *)nth)->val);
    } else {
        n = 0;
    }
    if ( trp_cord_search_internal( flags, obj, s, &p, n ) )
        return 1;
    if ( pos )
        *pos = trp_sig64( p );
    return 0;
}

static int trp_cord_match_pre_cback( uns8b c, trp_cord_match_t *m )
{
    if ( m->ignore_case )
        c = trp_upcase( c );
    trp_queue_put( (trp_obj_t *)( ( (trp_cons_t *)( m->a.data[ m->a.len - 1 ] ) )->car ),
                   trp_char( c ) );
    return 0;
}

static int trp_cord_match_cback( uns8b c, trp_cord_match_t *m )
{
    if ( m->ignore_case )
        c = trp_upcase( c );
    trp_queue_put( (trp_obj_t *)( &( m->q ) ), trp_char( c ) );
    for ( ; ; ) {
        if ( m->q.len < ( (trp_queue_t *)( ( (trp_cons_t *)( m->a.data[ m->pos ] ) )->car ) )->len )
            return 0;
        if ( trp_queue_equal( (trp_queue_t *)( &( m->q ) ),
                              (trp_queue_t *)( ( (trp_cons_t *)( m->a.data[ m->pos ] ) )->car ) ) == TRP_TRUE ) {
            m->result = m->pos;
            /*
             con il break viene matchato il piu' corto possibile;
             senza il break viene matchato il piu' lungo possibile
             break;
             */
        }
        m->pos++;
        if ( m->pos == m->a.len )
            break;
    }
    return 1;
}

static trp_obj_t *trp_cord_match_less( trp_cons_t *i, trp_cons_t *j )
{
    return trp_less( i->car, j->car );
}

static void trp_cord_match_clear( trp_cord_match_t *m )
{
    uns32b i;

    trp_queue_close( &( m->q ) );
    for ( i = 0 ; i < m->a.len ; i++ ) {
        trp_queue_close( (trp_queue_t *)( ( (trp_cons_t *)( m->a.data[ i ] ) )->car ) );
        trp_gc_free( ( (trp_cons_t *)( ( (trp_cons_t *)( m->a.data[ i ] ) )->car) )->cdr );
        trp_gc_free( ( (trp_cons_t *)( m->a.data[ i ] ) )->car );
        trp_gc_free( m->a.data[ i ] );
    }
    trp_array_close( &( m->a ) );
}

static trp_obj_t *trp_cord_match_internal( uns8b flags, trp_obj_t **o, va_list args, uns32b *idx )
{
    static trp_obj_t *less = NULL;
    trp_obj_t *obj;
    trp_cord_t *tmp;
    trp_cord_match_t m;
    uns32b res_len;

    if ( (*o)->tipo != TRP_CORD )
        return NULL;
    m.pos = 0;
    m.result = -1;
    trp_queue_init_internal( &( m.q ) );
    m.a.tipo = TRP_ARRAY;
    m.a.incr = 20;
    m.a.len = 0;
    m.a.data = NULL;
    m.ignore_case = ( flags & 2 ) ? 1 : 0;
    *idx = 0;
    for ( obj = va_arg( args, trp_obj_t * ) ;
          obj ;
          obj = va_arg( args, trp_obj_t * ) ) {
        if ( obj->tipo != TRP_CORD )
            tmp = (trp_cord_t *)trp_sprint( obj, NULL );
        else
            tmp = (trp_cord_t *)obj;
        if ( tmp->len <= ((trp_cord_t *)(*o))->len ) {
            if ( tmp->len == 0 ) {
                trp_cord_match_clear( &m );
                return obj;
            }
            trp_array_insert( (trp_obj_t *)( &( m.a ) ),
                              NULL,
                              trp_cons( trp_queue(),
#if __WORDSIZE == 64
                                        trp_cons( obj, (trp_obj_t *)((uns64b)( *idx )) )
#else
                                        trp_cons( obj, (trp_obj_t *)( *idx ) )
#endif
                                      ),
                              NULL );
            if ( flags & 1 )
                CORD_riter( tmp->c, (CORD_iter_fn)trp_cord_match_pre_cback, &m );
            else
                CORD_iter( tmp->c, (CORD_iter_fn)trp_cord_match_pre_cback, &m );
        }
        ++(*idx);
    }
    if ( m.a.len == 0 ) {
        trp_cord_match_clear( &m );
        return NULL;
    }
    if ( less == NULL )
        less = trp_funptr( trp_cord_match_less, 2, UNDEF );
    trp_array_quicksort( (trp_obj_t *)( &( m.a ) ), less );
    if ( flags & 1 )
        CORD_riter( ((trp_cord_t *)(*o))->c, (CORD_iter_fn)trp_cord_match_cback, &m );
    else
        CORD_iter( ((trp_cord_t *)(*o))->c, (CORD_iter_fn)trp_cord_match_cback, &m );
    if ( m.result == -1 ) {
        obj = NULL;
    } else {
        obj = ( (trp_cons_t *)( ( (trp_cons_t *)( m.a.data[ m.result ] ) )->cdr) )->car;
#if __WORDSIZE == 64
        *idx = (uns32b)((uns64b)( ( (trp_cons_t *)( ( (trp_cons_t *)( m.a.data[ m.result ] ) )->cdr) )->cdr ));
#else
        *idx = (uns32b)( ( (trp_cons_t *)( ( (trp_cons_t *)( m.a.data[ m.result ] ) )->cdr) )->cdr );
#endif
        res_len = ( (trp_queue_t *)( ( (trp_cons_t *)( m.a.data[ m.result ] ) )->car ) )->len;
    }
    trp_cord_match_clear( &m );
    if ( obj )
        if ( flags & 4 ) {
            uns32b new_len = ((trp_cord_t *)(*o))->len - res_len;
            *o = trp_cord_cons( CORD_substr( ((trp_cord_t *)(*o))->c,
                                             ( flags & 1 ) ? 0 : res_len,
                                             new_len ),
                                new_len );
        }
    return obj;
}

trp_obj_t *trp_cord_lmatch_func( uns8b ignore_case, trp_obj_t *o, ... )
{
    va_list args;
    uns32b idx;

    va_start( args, o );
    o = trp_cord_match_internal( ignore_case ? 2 : 0, &o, args, &idx );
    va_end( args );
    return o ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_cord_rmatch_func( uns8b ignore_case, trp_obj_t *o, ... )
{
    va_list args;
    uns32b idx;

    va_start( args, o );
    o = trp_cord_match_internal( ignore_case ? 3 : 1, &o, args, &idx );
    va_end( args );
    return o ? TRP_TRUE : TRP_FALSE;
}

uns8b trp_cord_match_test( uns8b flags, trp_obj_t **which, trp_obj_t **which_idx, trp_obj_t **o, ... )
{
    trp_obj_t *tmp;
    va_list args;
    uns32b idx;

    va_start( args, o );
    tmp = trp_cord_match_internal( flags, o, args, &idx );
    va_end( args );
    if ( tmp == NULL )
        return 1;
    if ( which )
        *which = tmp;
    if ( which_idx )
        *which_idx = trp_sig64( idx );
    return 0;
}

static int trp_cord_trim_cback( uns8b c, trp_cord_trim_t *m )
{
    int res = 1;
    trp_queue_elem *elem;

    for ( elem = (trp_queue_elem *)( m->q.first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) )
        if ( ((trp_char_t *)elem->val)->c == c ) {
            m->cnt = m->cnt + 1;
            res = 0;
            break;
        }
    return res;
}

static trp_obj_t *trp_cord_trim_internal( uns8b flags, trp_obj_t *s, va_list args )
{
    trp_obj_t *obj;
    trp_cord_trim_t m;

    if ( s->tipo != TRP_CORD )
        return UNDEF;
    trp_queue_init_internal( &( m.q ) );
    for ( obj = va_arg( args, trp_obj_t * ) ;
          obj ;
          obj = va_arg( args, trp_obj_t * ) ) {
        if ( obj->tipo != TRP_CHAR ) {
            trp_queue_close( &( m.q ) );
            return UNDEF;
        }
        trp_queue_put( (trp_obj_t *)( &( m.q ) ), obj );
    }
    if ( m.q.len == 0 )
        trp_queue_put( (trp_obj_t *)( &( m.q ) ), trp_char( ' ' ) );
    m.cnt = 0;
    if ( flags )
        CORD_riter( ((trp_cord_t *)s)->c, (CORD_iter_fn)trp_cord_trim_cback, &m );
    else
        CORD_iter( ((trp_cord_t *)s)->c, (CORD_iter_fn)trp_cord_trim_cback, &m );
    trp_queue_close( &( m.q ) );
    if ( m.cnt ) {
        uns32b len = ((trp_cord_t *)s)->len - m.cnt;
        s = trp_cord_cons( CORD_substr( ((trp_cord_t *)s)->c, flags ? 0 : m.cnt, len ), len );
    }
    return s;
}

trp_obj_t *trp_cord_ltrim( trp_obj_t *s, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, s );
    res = trp_cord_trim_internal( 0, s, args );
    va_end( args );
    return res;
}

trp_obj_t *trp_cord_rtrim( trp_obj_t *s, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, s );
    res = trp_cord_trim_internal( 1, s, args );
    va_end( args );
    return res;
}

uns8b trp_cord_ltrim_test( trp_obj_t **s, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, s );
    res = trp_cord_trim_internal( 0, *s, args );
    va_end( args );
    if ( res == UNDEF )
        return 1;
    *s = res;
    return 0;
}

uns8b trp_cord_rtrim_test( trp_obj_t **s, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, s );
    res = trp_cord_trim_internal( 1, *s, args );
    va_end( args );
    if ( res == UNDEF )
        return 1;
    *s = res;
    return 0;
}

static trp_obj_t *trp_cord_max_fix_basic( uns8b flags, trp_obj_t *s1, trp_obj_t *s2 )
{
    uns32b l1, l2, l;
    CORD c1, c2;
    CORD_pos p1, p2;
    voidfun_t CORD_succ;
    char ch1, ch2;
    uns8b ignore_case = flags & 2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return UNDEF;
    l1 = ((trp_cord_t *)s1)->len;
    l2 = ((trp_cord_t *)s2)->len;
    l = l1; if ( l > l2 ) l = l2;
    if ( l == 0 )
        return ZERO;
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    if ( flags & 1 ) {
        CORD_succ = CORD_prev;
        l1--;
        l2--;
    } else {
        CORD_succ = CORD_next;
        l1 = l2 = 0;
    }
    CORD_set_pos( p1, c1, l1 );
    CORD_set_pos( p2, c2, l2 );
    for ( l1 = 0 ; ; ) {
        ch1 = CORD_pos_fetch( p1 );
        ch2 = CORD_pos_fetch( p2 );
        if ( ignore_case ) {
            ch1 = trp_upcase( ch1 );
            ch2 = trp_upcase( ch2 );
        }
        if ( ch1 != ch2 )
            break;
        if ( ++l1 == l )
            break;
        (CORD_succ)( p1 );
        (CORD_succ)( p2 );
    }
    return trp_sig64( l1 );
}

trp_obj_t *trp_cord_max_prefix( trp_obj_t *s1, trp_obj_t *s2 )
{
    return trp_cord_max_fix_basic( 0, s1, s2 );
}

trp_obj_t *trp_cord_max_prefix_case( trp_obj_t *s1, trp_obj_t *s2 )
{
    return trp_cord_max_fix_basic( 2, s1, s2 );
}

trp_obj_t *trp_cord_max_suffix( trp_obj_t *s1, trp_obj_t *s2 )
{
    return trp_cord_max_fix_basic( 1, s1, s2 );
}

trp_obj_t *trp_cord_max_suffix_case( trp_obj_t *s1, trp_obj_t *s2 )
{
    return trp_cord_max_fix_basic( 3, s1, s2 );
}

trp_obj_t *trp_cord_load( trp_obj_t *path )
{
    uns8b *cpath = trp_csprint( path );
    FILE *fp;
    register int c;
    uns32b len;
    CORD_ec x;

    /*
     FIXME
     se il file è troppo grande,
     trattare in modo adeguato...
     */
    fp = trp_fopen( cpath, "rb" );
    trp_csprint_free( cpath );
    if ( fp == NULL )
        return UNDEF;
    CORD_ec_init( x );
    for ( len = 0 ; ; ) {
        c = getc( fp );
        if ( c == 0 ) {
            /* Append the right number of NULs */
            /* Note that any string of NULs is represented in 4 words, */
            /* independent of its length. */
            register size_t count = 1;

            CORD_ec_flush_buf( x );
            while ( ( c = getc( fp ) ) == 0 )
                count++;
            x[ 0 ].ec_cord = CORD_cat( x[ 0 ].ec_cord, CORD_nul( count ) );
            len += count;
        }
        if ( c == EOF )
            break;
        CORD_ec_append( x, c );
        ++len;
    }
    (void)fclose( fp );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_cord_tile( trp_obj_t *len, ... )
{
    uns32b l, i;
    uns8b *c, *d;
    va_list args;
    CORD_ec x;

    if ( trp_cast_uns32b( len, &l ) )
        return UNDEF;
    if ( l == 0 )
        return EMPTYCORD;
    va_start( args, len );
    len = va_arg( args, trp_obj_t * );
    c = trp_csprint_multi( len, args );
    va_end( args );
    if ( c[ 0 ] == 0 ) {
        trp_csprint_free( c );
        return trp_cord_cons( CORD_chars( ' ', l ), l );
    }
    if ( c[ 1 ] == 0 ) {
        len = trp_cord_cons( CORD_chars( c[ 0 ], l ), l );
        trp_csprint_free( c );
        return len;
    }
    CORD_ec_init( x );
    for ( d = c, i = 0 ; i < l ; i++ ) {
        CORD_ec_append( x, *d++ );
        if ( *d == 0 )
            d = c;
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), l );
}

static sig32b trp_cord_utf8_next( uns8b *p )
{
    if ( p[ 0 ] == 0 )
        return 0;
    if ( p[ 0 ] < 0x80 )
        return 1;
    if ( ( p[ 0 ] < 0xc2 ) || ( p[ 1 ] == 0 ) )
        return -1;
    if ( p[ 0 ] < 0xe0 )
        return ( ( p[ 1 ] ^ 0x80 ) < 0x40 ) ? 2 : -1;
    if ( p[ 2 ] == 0 )
        return -1;
    if ( p[ 0 ] < 0xf0 )
        return ( ( ( p[ 1 ] ^ 0x80 ) < 0x40 ) &&
                 ( ( p[ 2 ] ^ 0x80 ) < 0x40 ) &&
                 ( ( p[ 0 ] >= 0xe1 ) || ( p[ 1 ] >= 0xa0 ) ) &&
                 ( ( p[ 0 ] != 0xed ) || ( p[ 1 ] < 0xa0 ) ) ) ? 3 : -1;
    if ( ( p[ 0 ] >= 0xf8 ) || ( p[ 3 ] == 0 ) )
        return -1;
    return ( ( ( p[ 1 ] ^ 0x80) < 0x40 ) &&
             ( ( p[ 2 ] ^ 0x80) < 0x40 ) &&
             ( ( p[ 3 ] ^ 0x80) < 0x40 ) &&
             ( ( p[ 0 ] >= 0xf1 ) || ( p[ 1 ] >= 0x90 ) ) &&
             ( ( p[ 0 ] < 0xf4 ) || ( ( p[ 0 ] == 0xf4) && ( p[ 1 ] < 0x90 ) ) ) ) ? 4 : -1;
}

trp_obj_t *trp_cord_utf8_tile( trp_obj_t *len, ... )
{
    uns32b l, i;
    sig32b n;
    uns8b *c, *d;
    va_list args;
    CORD_ec x;

    if ( trp_cast_uns32b( len, &l ) )
        return UNDEF;
    if ( l == 0 )
        return EMPTYCORD;
    va_start( args, len );
    len = va_arg( args, trp_obj_t * );
    c = trp_csprint_multi( len, args );
    va_end( args );
    if ( c[ 0 ] == 0 ) {
        trp_csprint_free( c );
        return trp_cord_cons( CORD_chars( ' ', l ), l );
    }
    if ( c[ 1 ] == 0 ) {
        if ( c[ 0 ] & 0x80 )
            return UNDEF;
        len = trp_cord_cons( CORD_chars( c[ 0 ], l ), l );
        trp_csprint_free( c );
        return len;
    }
    CORD_ec_init( x );
    for ( d = c, i = 0 ; l ; l-- ) {
        n = trp_cord_utf8_next( d );
        if ( n == -1 ) {
            trp_csprint_free( c );
            return UNDEF;
        }
        for ( ; n ; n-- ) {
            CORD_ec_append( x, *d++ );
            i++;
        }
        if ( *d == 0 )
            d = c;
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), i );
}

trp_obj_t *trp_cord_utf8_head( trp_obj_t *s, trp_obj_t *len )
{
    uns32b l, res;
    sig32b n;
    uns8b *c = trp_csprint( s );

    if ( trp_cast_uns32b( len, &l ) )
        return UNDEF;
    for ( res = 0 ; l ; res += n, --l ) {
        n = trp_cord_utf8_next( c + res );
        if ( n == 0 )
            break;
        if ( n == -1 ) {
            trp_csprint_free( c );
            return UNDEF;
        }
    }
    trp_csprint_free( c );
    return trp_sig64( res );
}

trp_obj_t *trp_cord_utf8_toupper( trp_obj_t *s, ... )
{
    uns32b i;
    sig32b n;
    uns8b *c, *d;
    uns16b *q = (uns16b *)_trp_cord_iso2utf8;
    uns16b v2;
    va_list args;
    CORD_ec x;
    uns8b j;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec_init( x );
    for ( d = c, i = 0 ; ; ) {
        n = trp_cord_utf8_next( d );
        if ( n == 0 )
            break;
        if ( n == -1 ) {
            trp_csprint_free( c );
            return UNDEF;
        }
        switch ( n ) {
        case 1:
            CORD_ec_append( x, trp_upcase( *d++ ) );
            i++;
            break;
        case 2:
            v2 = *( (uns16b *)( d ) );
            for ( j = 0 ; j < 128 ; j++ )
                if ( v2 == q[ j ] )
                    break;
            if ( j == 128 ) {
                /*
                 FIXME
                 */
                for ( ; n ; n-- ) {
                    CORD_ec_append( x, *d++ );
                    i++;
                }
            } else {
                if ( j >= 96 )
                    j -= 32;
                j <<= 1;
                CORD_ec_append( x, _trp_cord_iso2utf8[ j++ ] );
                CORD_ec_append( x, _trp_cord_iso2utf8[ j ] );
                i += 2;
                d += 2;
            }
            break;
        case 3:
        case 4:
            /*
             FIXME
             */
            for ( ; n ; n-- ) {
                CORD_ec_append( x, *d++ );
                i++;
            }
            break;
        }
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), i );
}

trp_obj_t *trp_cord_utf8_tolower( trp_obj_t *s, ... )
{
    uns32b i;
    sig32b n;
    uns8b *c, *d;
    uns16b *q = (uns16b *)_trp_cord_iso2utf8;
    uns16b v2;
    va_list args;
    CORD_ec x;
    uns8b j;

    va_start( args, s );
    c = trp_csprint_multi( s, args );
    va_end( args );
    CORD_ec_init( x );
    for ( d = c, i = 0 ; ; ) {
        n = trp_cord_utf8_next( d );
        if ( n == 0 )
            break;
        if ( n == -1 ) {
            trp_csprint_free( c );
            return UNDEF;
        }
        switch ( n ) {
        case 1:
            CORD_ec_append( x, trp_downcase( *d++ ) );
            i++;
            break;
        case 2:
            v2 = *( (uns16b *)( d ) );
            for ( j = 0 ; j < 128 ; j++ )
                if ( v2 == q[ j ] )
                    break;
            if ( j == 128 ) {
                /*
                 FIXME
                 */
                for ( ; n ; n-- ) {
                    CORD_ec_append( x, *d++ );
                    i++;
                }
            } else {
                if ( ( j >= 64 ) && ( j < 96 ) )
                    j += 32;
                j <<= 1;
                CORD_ec_append( x, _trp_cord_iso2utf8[ j++ ] );
                CORD_ec_append( x, _trp_cord_iso2utf8[ j ] );
                i += 2;
                d += 2;
            }
            break;
        case 3:
        case 4:
            /*
             FIXME
             */
            for ( ; n ; n-- ) {
                CORD_ec_append( x, *d++ );
                i++;
            }
            break;
        }
    }
    trp_csprint_free( c );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), i );
}

trp_obj_t *trp_cord_subsequencep( trp_obj_t *s1, trp_obj_t *s2 )
{
    CORD_pos i1, i2;
    uns8b cs;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return TRP_FALSE;
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 ),
          CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 ) ;
          CORD_pos_valid( i1 ) ;
          CORD_next( i1 ), CORD_next( i2 ) ) {
        cs = CORD_pos_fetch( i1 );
        for ( ; ; CORD_next( i2 ) ) {
            if ( !CORD_pos_valid( i2 ) )
                return TRP_FALSE;
            if ( CORD_pos_fetch( i2 ) == cs )
                break;
        }
    }
    return TRP_TRUE;
}

trp_obj_t *trp_cord_circular_eq( trp_obj_t *s1, trp_obj_t *s2 )
{
    uns32b len1, len2, j1, j2, off2;
    CORD c1, c2;
    CORD_pos i1, i2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return TRP_FALSE;
    len1 = ((trp_cord_t *)s1)->len;
    len2 = ((trp_cord_t *)s2)->len;
    if ( len1 != len2 )
        return TRP_FALSE;
    if ( len1 == 0 )
        return TRP_TRUE;
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    for ( off2 = 0 ; off2 < len2 ; off2++ )
        for ( CORD_set_pos( i1, c1, 0 ), CORD_set_pos( i2, c2, off2 ), j1 = 0, j2 = off2 ;
              ;
              CORD_next( i1 ), j1++ ) {
            if ( j1 == len1 )
                return TRP_TRUE;
            if ( CORD_pos_fetch( i1 ) != CORD_pos_fetch( i2 ) )
                break;
            CORD_next( i2 );
            j2++;
            if ( j2 == len2 ) {
                CORD_set_pos( i2, c2, 0 );
                j2 = 0;
            }
        }
    return TRP_FALSE;
}

trp_obj_t *trp_cord_hamming_distance( trp_obj_t *s1, trp_obj_t *s2 )
{
    uns32b cnt;
    CORD_pos i1, i2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return UNDEF;
    if ( ((trp_cord_t *)s1)->len != ((trp_cord_t *)s2)->len )
        return UNDEF;
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 ),
          CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 ), cnt = 0 ;
          CORD_pos_valid( i1 ) && CORD_pos_valid( i2 ) ;
          CORD_next( i1 ), CORD_next( i2 ) )
        if ( CORD_pos_fetch( i1 ) != CORD_pos_fetch( i2 ) )
            ++cnt;
    return trp_sig64( cnt );
}

trp_obj_t *trp_cord_edit_distance( trp_obj_t *s1, trp_obj_t *s2 )
{
#   define ed(i1,i2) edit[(i1)*n2p+(i2)]
    uns32b d, t, j1, j2, n1, n2, n1p, n2p, *edit;
    CORD_pos i1, i2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return UNDEF;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( edit = malloc( n1p * n2p * sizeof( uns32b ) ) ) == NULL )
        return UNDEF;
    for ( j1 = 0 ; j1 <= n1 ; j1++ )
        ed( j1, 0 ) = j1;
    for ( j2 = 1 ; j2 <= n2 ; j2++ )
        ed( 0, j2 ) = j2;
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 ), j1 = 1 ;
          j1 <= n1 ;
          CORD_next( i1 ), j1++ )
        for ( CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 ), j2 = 1 ;
              j2 <= n2 ;
              CORD_next( i2 ), j2++ ) {
            d = ed( j1 - 1, j2 ) + 1;
            t = ed( j1, j2 - 1 ) + 1;
            if ( t < d )
                d = t;
            t = ed( j1 - 1, j2 - 1 ) + ( ( CORD_pos_fetch( i1 ) == CORD_pos_fetch( i2 ) ) ? 0 : 1 );
            if ( t < d )
                d = t;
            ed( j1, j2 ) = d;
        }
    d = ed( n1, n2 );
    free( edit );
    return trp_sig64( d );
}

static uns8b trp_cord_decode_scoring_matrix( trp_obj_t *scoring_matrix, sig32b *sm )
{
    if ( scoring_matrix ) {
        int i, j;
        sig32b *m = sm;

        for ( i = 0 ; i < 400 ; i++ ) {
            if ( scoring_matrix->tipo != TRP_CONS )
                return 1;
            if ( trp_cast_sig32b( ((trp_cons_t *)scoring_matrix)->car, m++ ) )
                return 1;
            scoring_matrix = ((trp_cons_t *)scoring_matrix)->cdr;
        }
        if ( scoring_matrix != NIL )
            return 1;
        for ( i = 0 ; i < 20 ; i++ )
            for ( j = 0 ; j < i ; j++ )
                if ( sm[ i * 20 + j ] != sm[ i + j * 20 ] )
                    return 1;
    } else
        memcpy( sm, _trp_cord_blosum62, 400 * sizeof( uns32b ) );
    return 0;
}

static sig32b trp_cord_amino_index( uns8b amino )
{
    sig32b i;

    if ( amino <= 'L' )
        if ( amino <= 'F' )
            if ( amino <= 'D' )
                i = ( amino == 'A' ) ? 0 : ( ( amino == 'C' ) ? 1 : 2 );
            else
                i = ( amino == 'E' ) ? 3 : 4;
        else
            if ( amino <= 'I' )
                i = ( amino == 'G' ) ? 5 : ( ( amino == 'H' ) ? 6 : 7 );
            else
                i = ( amino == 'K' ) ? 8 : 9;
    else
        if ( amino <= 'R' )
            if ( amino <= 'P' )
                i = ( amino == 'M' ) ? 10 : ( ( amino == 'N' ) ? 11 : 12 );
            else
                i = ( amino == 'Q' ) ? 13 : 14;
        else
            if ( amino <= 'V' )
                i = ( amino == 'S' ) ? 15 : ( ( amino == 'T' ) ? 16 : 17 );
            else
                i = ( amino == 'W' ) ? 18 : 19;
    return i;
}

trp_obj_t *trp_cord_protein_weight( trp_obj_t *s )
{
    double w = 0.0;
    CORD_pos i;

    if ( s->tipo != TRP_CORD )
        return UNDEF;
    CORD_FOR( i, ((trp_cord_t *)s)->c )
        w += _amino_acid_weight[ trp_cord_amino_index( CORD_pos_fetch( i ) ) ];
    return trp_double( w );
}

trp_obj_t *trp_cord_weight2amino( trp_obj_t *weight )
{
    int i, best;
    double w, emax, e;

    if ( trp_cast_double( weight, &w ) )
        return UNDEF;
    emax = 0.01;
    best = -1;
    for ( i = 0 ; i < 20 ; i++ ) {
        e = _amino_acid_weight[ i ] - w;
        if ( e < 0 )
            e = -e;
        if ( emax > e ) {
            emax = e;
            best = i;
        }
    }
    if ( best < 0 )
        return UNDEF;
    return trp_char( _amino_acid[ best ] );
}

static trp_obj_t *trp_cord_alignment_score_low( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix )
{
    sig32b cnt, gap_p, gap_e;
    uns8b c1, c2, gap_paid;
    CORD_pos i1, i2;
    sig32b sm[ 400 ];

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) ||
         trp_cast_sig32b( gap_opening_penalty, &gap_p ) ||
         trp_cast_sig32b( gap_extension_penalty, &gap_e ) ||
         trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( ((trp_cord_t *)s1)->len != ((trp_cord_t *)s2)->len )
        return UNDEF;
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 ),
          CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 ), cnt = 0, gap_paid = 0 ;
          CORD_pos_valid( i1 ) && CORD_pos_valid( i2 ) ;
          CORD_next( i1 ), CORD_next( i2 ) ) {
        c1 = CORD_pos_fetch( i1 );
        c2 = CORD_pos_fetch( i2 );
        if ( c1 == '-' ) {
            cnt -= ( gap_paid == 1 ) ? gap_e : gap_p + gap_e;
            gap_paid = 1;
        } else if ( c2 == '-' ) {
            cnt -= ( gap_paid == 2 ) ? gap_e : gap_p + gap_e;
            gap_paid = 2;
        } else {
            cnt += sm[ trp_cord_amino_index( c1 ) * 20 + trp_cord_amino_index( c2 ) ];
            gap_paid = 0;
        }
    }
    return trp_sig64( cnt );
}

trp_obj_t *trp_cord_alignment_score( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
    return trp_cord_alignment_score_low( s1, s2, ZERO, gap_penalty, scoring_matrix );
}

trp_obj_t *trp_cord_alignment_score_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix )
{
    return trp_cord_alignment_score_low( s1, s2, gap_opening_penalty, gap_extension_penalty, scoring_matrix );
}

trp_obj_t *trp_cord_lcs( trp_obj_t *s1, trp_obj_t *s2 )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
#   define grm(i1,i2) gridmov[(i1)*n2p+(i2)]
    uns32b left, over, j1, j2, n1, n2, n1p, n2p, *gridlen;
    uns8b *gridmov, *lcs, mov;
    CORD_pos i1, i2;
    CORD_ec x;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return UNDEF;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridmov = malloc( n1p * n2p * ( sizeof( uns8b ) + sizeof( uns32b ) ) ) ) == NULL )
        return UNDEF;
    gridlen = (uns32b *)( gridmov + n1p * n2p );
    for ( j1 = 0 ; j1 <= n1 ; j1++ ) {
        grl( j1, 0 ) = 0;
        grm( j1, 0 ) = 'e';
    }
    for ( j2 = 1 ; j2 <= n2 ; j2++ ) {
        grl( 0, j2 ) = 0;
        grm( 0, j2 ) = 'e';
    }
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, n1 - 1 ), j1 = 1 ;
          j1 <= n1 ;
          CORD_prev( i1 ), j1++ )
        for ( CORD_set_pos( i2, ((trp_cord_t *)s2)->c, n2 - 1 ), j2 = 1 ;
              j2 <= n2 ;
              CORD_prev( i2 ), j2++ ) {
            if ( CORD_pos_fetch( i1 ) == CORD_pos_fetch( i2 ) ) {
                grl( j1, j2 ) = grl( j1 - 1, j2 - 1 ) + 1;
                grm( j1, j2 ) = '\\';
            } else {
                left = grl( j1, j2 - 1 );
                over = grl( j1 - 1, j2 );
                if ( left < over ) {
                    grl( j1, j2 ) = over;
                    grm( j1, j2 ) = '^';
                } else {
                    grl( j1, j2 ) = left;
                    grm( j1, j2 ) = '<';
                }
            }
        }
    CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 );
    CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 );
    CORD_ec_init( x );
    for ( j1 = n1, j2 = n2 ; ; ) {
        mov = grm( j1, j2 );
        if ( mov == 'e' )
            break;
        if ( mov == '\\' ) {
            CORD_ec_append( x, CORD_pos_fetch( i1 ) );
            j1--;
            j2--;
            CORD_next( i1 );
            CORD_next( i2 );
        } else if ( mov == '^' ) {
            j1--;
            CORD_next( i1 );
        } else { /* mov == '<' */
            j2--;
            CORD_next( i2 );
        }
    }
    left = grl( n1, n2 );
    free( gridmov );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), left );
}

trp_obj_t *trp_cord_lcs_length( trp_obj_t *s1, trp_obj_t *s2 )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
    uns32b left, over, j1, j2, n1, n2, n1p, n2p, *gridlen;
    CORD_pos i1, i2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) )
        return UNDEF;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridlen = malloc( n1p * n2p * sizeof( uns32b ) ) ) == NULL )
        return UNDEF;
    grl( 0, 0 ) = 0;
    for ( j2 = 1 ; j2 <= n2 ; j2++ )
        grl( 0, j2 ) = 0;
    for ( CORD_set_pos( i1, ((trp_cord_t *)s1)->c, 0 ), j1 = 1 ;
          j1 <= n1 ;
          CORD_next( i1 ), j1++ ) {
        grl( j1, 0 ) = 0;
        for ( CORD_set_pos( i2, ((trp_cord_t *)s2)->c, 0 ), j2 = 1 ;
              j2 <= n2 ;
              CORD_next( i2 ), j2++ ) {
            if ( CORD_pos_fetch( i1 ) == CORD_pos_fetch( i2 ) ) {
                grl( j1, j2 ) = grl( j1 - 1, j2 - 1 ) + 1;
            } else {
                left = grl( j1, j2 - 1 );
                over = grl( j1 - 1, j2 );
                grl( j1, j2 ) = ( left < over ) ? over : left;
            }
        }
    }
    left = grl( n1, n2 );
    free( gridlen );
    return trp_sig64( left );
}

static uns8b trp_cord_global_alignment_affine_low( uns8b flags, trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, sig32b *sm, sig32b *score, trp_obj_t **as1, trp_obj_t **as2 )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
#   define grg1(i1,i2) gridgap1[(i1)*n2p+(i2)]
#   define grg2(i1,i2) gridgap2[(i1)*n2p+(i2)]
#   define grm(i1,i2) gridmov[(i1)*n2p+(i2)]
    sig32b gap_p, gap_e, sc, bsc, k, t, *gridlen, *gridgap1, *gridgap2, *sm1;
    uns32b j1, j2, n1, n2, n1p, n2p, fit1;
    uns8b *gridmov, mov;
    CORD c1, c2;
    CORD_pos i1, i2;
    CORD_ec x1, x2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) ||
         trp_cast_sig32b( gap_opening_penalty, &gap_p ) ||
         trp_cast_sig32b( gap_extension_penalty, &gap_e ) )
        return 1;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridmov = malloc( n1p * n2p * ( sizeof( uns8b ) + sizeof( sig32b ) * 3 ) ) ) == NULL )
        return 1;
    gridlen = (sig32b *)( gridmov + n1p * n2p );
    gridgap1 = gridlen + n1p * n2p;
    gridgap2 = gridgap1 + n1p * n2p;
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    grl( 0, 0 ) = 0;
    grm( 0, 0 ) = 'e';
    for ( k = gap_e, j2 = 1 ; j2 <= n2 ; j2++, k += gap_e ) {
        grl( 0, j2 ) = -gap_p - k;
        grg1( 0, j2 ) = -2000000000;
        grm( 0, j2 ) = '<';
    }
    *score = -2000000000;
    for ( CORD_set_pos( i1, c1, n1 - 1 ), k = gap_e, j1 = 1 ; j1 <= n1 ; j1++, k += gap_e, CORD_prev( i1 ) ) {
        grl( j1, 0 ) = ( flags & 1 ) ? 0 : ( -gap_p - k );
        grg2( j1, 0 ) = -2000000000;
        grm( j1, 0 ) = '^';
        sm1 = sm + trp_cord_amino_index( CORD_pos_fetch( i1 ) ) * 20;
        for ( CORD_set_pos( i2, c2, n2 - 1 ), j2 = 1 ; j2 <= n2 ; j2++, CORD_prev( i2 ) ) {
            bsc = grl( j1 - 1, j2 - 1 ) + sm1[ trp_cord_amino_index( CORD_pos_fetch( i2 ) ) ];
            mov = '\\';
            sc = grl( j1 - 1, j2 ) - gap_p;
            t = grg1( j1 - 1, j2 );
            if ( t > sc )
                sc = t;
            sc -= gap_e;
            grg1( j1, j2 ) = sc;
            if ( sc > bsc ) {
                bsc = sc;
                mov = '^';
            }
            sc = grl( j1, j2 - 1 ) - gap_p;
            t = grg2( j1, j2 - 1 );
            if ( t > sc )
                sc = t;
            sc -= gap_e;
            grg2( j1, j2 ) = sc;
            if ( sc > bsc ) {
                bsc = sc;
                mov = '<';
            }
            grl( j1, j2 ) = bsc;
            grm( j1, j2 ) = mov;
        }
        if ( bsc > *score ) {
            *score = bsc;
            fit1 = j1;
        }
    }
    if ( ( flags & 1 ) == 0 )
        *score = grl( n1, n2 );
    if ( flags & 2 ) {
        free( gridmov );
        return 0;
    }
    CORD_ec_init( x1 );
    CORD_ec_init( x2 );
    CORD_set_pos( i1, c1, ( flags & 1 ) ? ( n1 - fit1 ) : 0 );
    CORD_set_pos( i2, c2, 0 );
    for ( j1 = ( flags & 1 ) ? fit1 : n1, j2 = n2, k = 0 ; ; k++ ) {
        mov = grm( j1, j2 );
        if ( flags & 1 ) {
            if ( j2 == 0 )
                break;
        } else {
            if ( mov == 'e' )
                break;
        }
        if ( mov == '^' ) {
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, '-' );
            CORD_next( i1 );
            j1--;
        } else if ( mov == '<' ) {
            CORD_ec_append( x1, '-' );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i2 );
            j2--;
        } else { /* mov == '\\' */
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i1 );
            CORD_next( i2 );
            j1--;
            j2--;
        }
    }
    free( gridmov );
    *as1 = trp_cord_cons( CORD_balance( CORD_ec_to_cord( x1 ) ), k );
    *as2 = trp_cord_cons( CORD_balance( CORD_ec_to_cord( x2 ) ), k );
    return 0;
}

trp_obj_t *trp_cord_edit_alignment( trp_obj_t *s1, trp_obj_t *s2 )
{
    sig32b i, sm[ 400 ];

    for ( i = 0 ; i < 400 ; i++ )
        sm[ i ] = ( i % 21 ) ? -1 : 0;
    if ( trp_cord_global_alignment_affine_low( 0, s1, s2, ZERO, UNO, sm, &i, &s1, &s2 ) )
        return UNDEF;
    return trp_cons( s1, s2 );
}

trp_obj_t *trp_cord_global_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_global_alignment_affine_low( 0, s1, s2, ZERO, gap_penalty, sm, &score, &s1, &s2 ) )
        return UNDEF;
    return trp_list( trp_sig64( score ), s1, s2, NULL );
}

trp_obj_t *trp_cord_global_alignment_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_global_alignment_affine_low( 0, s1, s2, gap_opening_penalty, gap_extension_penalty, sm, &score, &s1, &s2 ) )
        return UNDEF;
    return trp_list( trp_sig64( score ), s1, s2, NULL );
}

trp_obj_t *trp_cord_fitting_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_global_alignment_affine_low( 1, s1, s2, ZERO, gap_penalty, sm, &score, &s1, &s2 ) )
        return UNDEF;
    return trp_list( trp_sig64( score ), s1, s2, NULL );
}

trp_obj_t *trp_cord_global_alignment_score( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_global_alignment_affine_low( 2, s1, s2, ZERO, gap_penalty, sm, &score, NULL, NULL ) )
        return UNDEF;
    return trp_sig64( score );
}

static uns8b trp_cord_local_alignment_affine_low( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, sig32b *sm, sig32b *score, trp_obj_t **as1, trp_obj_t **as2 )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
#   define grg1(i1,i2) gridgap1[(i1)*n2p+(i2)]
#   define grg2(i1,i2) gridgap2[(i1)*n2p+(i2)]
#   define grm(i1,i2) gridmov[(i1)*n2p+(i2)]
    sig32b gap_p, gap_e, sc, bsc, t, *gridlen, *gridgap1, *gridgap2, *sm1;
    uns32b j1, j2, n1, n2, n1p, n2p, end1, end2;
    uns8b *gridmov, mov;
    CORD c1, c2;
    CORD_pos i1, i2;

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) ||
         trp_cast_sig32b( gap_opening_penalty, &gap_p ) ||
         trp_cast_sig32b( gap_extension_penalty, &gap_e ) )
        return 1;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    if ( ( n1 == 0 ) || ( n2 == 0 ) ) {
        *score = 0;
        *as1 = *as2 = EMPTYCORD;
        return 0;
    }
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridmov = malloc( n1p * n2p * ( sizeof( uns8b ) + sizeof( sig32b ) * 3 ) ) ) == NULL )
        return 1;
    gridlen = (sig32b *)( gridmov + n1p * n2p );
    gridgap1 = gridlen + n1p * n2p;
    gridgap2 = gridgap1 + n1p * n2p;
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    grl( 0, 0 ) = 0;
    for ( j2 = 1 ; j2 <= n2 ; j2++ ) {
        grl( 0, j2 ) = 0;
        grg1( 0, j2 ) = -2000000000;
        grm( 0, j2 ) = '<';
    }
    *score = -1;
    for ( CORD_set_pos( i1, c1, 0 ), j1 = 1 ; j1 <= n1 ; j1++, CORD_next( i1 ) ) {
        grl( j1, 0 ) = 0;
        grg2( j1, 0 ) = -2000000000;
        grm( j1, 0 ) = '^';
        sm1 = sm + trp_cord_amino_index( CORD_pos_fetch( i1 ) ) * 20;
        for ( CORD_set_pos( i2, c2, 0 ), j2 = 1 ; j2 <= n2 ; j2++, CORD_next( i2 ) ) {
            bsc = 0;
            sc = grl( j1 - 1, j2 - 1 ) + sm1[ trp_cord_amino_index( CORD_pos_fetch( i2 ) ) ];
            if ( sc > bsc ) {
                bsc = sc;
                mov = '\\';
            }
            sc = grl( j1 - 1, j2 ) - gap_p;
            t = grg1( j1 - 1, j2 );
            if ( t > sc )
                sc = t;
            sc -= gap_e;
            grg1( j1, j2 ) = sc;
            if ( sc > bsc ) {
                bsc = sc;
                mov = '^';
            }
            sc = grl( j1, j2 - 1 ) - gap_p;
            t = grg2( j1, j2 - 1 );
            if ( t > sc )
                sc = t;
            sc -= gap_e;
            grg2( j1, j2 ) = sc;
            if ( sc > bsc ) {
                bsc = sc;
                mov = '<';
            }
            grl( j1, j2 ) = bsc;
            grm( j1, j2 ) = mov;
            if ( bsc > *score ) {
                *score = bsc;
                end1 = j1;
                end2 = j2;
            }
        }
    }
    for ( j1 = end1, j2 = end2 ; grl( j1, j2 ) ; )
        switch ( grm( j1, j2 ) ) {
        case '^':
            j1--;
            break;
        case '<':
            j2--;
            break;
        default: /* '\\' */
            j1--;
            j2--;
            break;
        }
    free( gridmov );
    end1 -= j1;
    end2 -= j2;
    *as1 = trp_cord_cons( CORD_substr( c1, j1, end1 ), end1 );
    *as2 = trp_cord_cons( CORD_substr( c2, j2, end2 ), end2 );
    return 0;
}

trp_obj_t *trp_cord_local_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_local_alignment_affine_low( s1, s2, ZERO, gap_penalty, sm, &score, &s1, &s2 ) )
        return UNDEF;
    return trp_list( trp_sig64( score ), s1, s2, NULL );
}

trp_obj_t *trp_cord_local_alignment_affine( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_opening_penalty, trp_obj_t *gap_extension_penalty, trp_obj_t *scoring_matrix )
{
    sig32b score, sm[ 400 ];

    if ( trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    if ( trp_cord_local_alignment_affine_low( s1, s2, gap_opening_penalty, gap_extension_penalty, sm, &score, &s1, &s2 ) )
        return UNDEF;
    return trp_list( trp_sig64( score ), s1, s2, NULL );
}

trp_obj_t *trp_cord_semiglobal_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
#   define grg1(i1,i2) gridgap1[(i1)*n2p+(i2)]
#   define grg2(i1,i2) gridgap2[(i1)*n2p+(i2)]
#   define grm(i1,i2) gridmov[(i1)*n2p+(i2)]
    sig32b gap_p, v1, v2, v3, *gridlen, *gridgap1, *gridgap2, *sm1;
    uns32b j1, j2, n1, n2, n1p, n2p;
    uns8b *gridmov, mov;
    CORD c1, c2;
    CORD_pos i1, i2;
    CORD_ec x1, x2;
    sig32b sm[ 400 ];

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) ||
         trp_cast_sig32b( gap_penalty, &gap_p ) ||
         trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridmov = malloc( n1p * n2p * ( sizeof( uns8b ) + sizeof( sig32b ) * 3 ) ) ) == NULL )
        return UNDEF;
    gridlen = (sig32b *)( gridmov + n1p * n2p );
    gridgap1 = gridlen + n1p * n2p;
    gridgap2 = gridgap1 + n1p * n2p;
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    grl( 0, 0 ) = grg1( 0, 0 ) = grg2( 0, 0 ) = 0;
    grm( 0, 0 ) = 'e';
    for ( j2 = 1 ; j2 <= n2 ; j2++ ) {
        grl( 0, j2 ) = grg1( 0, j2 ) = grg2( 0, j2 ) = 0;
        grm( 0, j2 ) = '<';
    }
    for ( CORD_set_pos( i1, c1, n1 - 1 ), j1 = 1 ; j1 <= n1 ; j1++, CORD_prev( i1 ) ) {
        grl( j1, 0 ) = grg1( j1, 0 ) = grg2( j1, 0 ) = 0;
        grm( j1, 0 ) = '^';
        sm1 = sm + trp_cord_amino_index( CORD_pos_fetch( i1 ) ) * 20;
        for ( CORD_set_pos( i2, c2, n2 - 1 ), j2 = 1 ; j2 <= n2 ; j2++, CORD_prev( i2 ) ) {
            v1 = grl( j1 - 1, j2 - 1 ) + sm1[ trp_cord_amino_index( CORD_pos_fetch( i2 ) ) ];
            v2 = grl( j1 - 1, j2 ) - gap_p;
            v3 = grl( j1, j2 - 1 ) - gap_p;
            if ( v1 >= v2 ) {
                if ( v1 >= v3 ) {
                    grl( j1, j2 ) = v1;
                    grm( j1, j2 ) = '\\';
                } else {
                    grl( j1, j2 ) = v3;
                    grm( j1, j2 ) = '<';
                }
            } else {
                if ( v2 >= v3 ) {
                    grl( j1, j2 ) = v2;
                    grm( j1, j2 ) = '^';
                } else {
                    grl( j1, j2 ) = v3;
                    grm( j1, j2 ) = '<';
                }
            }
            v2 = grg1( j1 - 1, j2 );
            v3 = grg2( j1, j2 - 1 );
            if ( v1 >= v2 ) {
                grg1( j1, j2 ) = v1;
            } else {
                grg1( j1, j2 ) = v2;
            }
            if ( v1 >= v3 ) {
                grg2( j1, j2 ) = v1;
            } else {
                grg2( j1, j2 ) = v3;
            }
        }
    }
    CORD_ec_init( x1 );
    CORD_ec_init( x2 );
    CORD_set_pos( i1, c1, 0 );
    CORD_set_pos( i2, c2, 0 );
    v2 = 0;
    j1 = n1;
    j2 = n2;
    if ( grg1( n1, n2 ) >= grg2( n1, n2 ) ) {
        v1 = grg1( n1, n2 );
        while ( grg1( j1, j2 ) > grl( j1, j2 ) ) {
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, '-' );
            CORD_next( i1 );
            j1--;
            v2++;
        }
    } else {
        v1 = grg2( n1, n2 );
        while ( grg2( j1, j2 ) > grl( j1, j2 ) ) {
            CORD_ec_append( x1, '-' );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i2 );
            j2--;
            v2++;
        }
    }
    for ( ; ; v2++ ) {
        mov = grm( j1, j2 );
        if ( mov == 'e' )
            break;
        if ( mov == '^' ) {
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, '-' );
            CORD_next( i1 );
            j1--;
        } else if ( mov == '<' ) {
            CORD_ec_append( x1, '-' );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i2 );
            j2--;
        } else { /* mov == '\\' */
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i1 );
            CORD_next( i2 );
            j1--;
            j2--;
        }
    }
    free( gridmov );
    return trp_list( trp_sig64( v1 ),
                     trp_cord_cons( CORD_balance( CORD_ec_to_cord( x1 ) ), v2 ),
                     trp_cord_cons( CORD_balance( CORD_ec_to_cord( x2 ) ), v2 ),
                     NULL );
}

trp_obj_t *trp_cord_overlap_alignment( trp_obj_t *s1, trp_obj_t *s2, trp_obj_t *gap_penalty, trp_obj_t *scoring_matrix )
{
#   define grl(i1,i2) gridlen[(i1)*n2p+(i2)]
#   define grm(i1,i2) gridmov[(i1)*n2p+(i2)]
    sig32b gap_p, v1, v2, v3, *gridlen, *sm1;
    uns32b j1, j2, n1, n2, n1p, n2p;
    uns8b *gridmov, mov;
    CORD c1, c2;
    CORD_pos i1, i2;
    CORD_ec x1, x2;
    sig32b sm[ 400 ];

    if ( ( s1->tipo != TRP_CORD ) || ( s2->tipo != TRP_CORD ) ||
         trp_cast_sig32b( gap_penalty, &gap_p ) ||
         trp_cord_decode_scoring_matrix( scoring_matrix, sm ) )
        return UNDEF;
    n1 = ((trp_cord_t *)s1)->len;
    n2 = ((trp_cord_t *)s2)->len;
    n1p = n1 + 1;
    n2p = n2 + 1;
    if ( ( gridmov = malloc( n1p * n2p * ( sizeof( uns8b ) + sizeof( sig32b ) ) ) ) == NULL )
        return UNDEF;
    gridlen = (sig32b *)( gridmov + n1p * n2p );
    c1 = ((trp_cord_t *)s1)->c;
    c2 = ((trp_cord_t *)s2)->c;
    grl( 0, 0 ) = 0;
    grm( 0, 0 ) = 'e';
    for ( j2 = 1 ; j2 <= n2 ; j2++ ) {
        grl( 0, j2 ) = 0;
        grm( 0, j2 ) = 'e';
    }
    for ( CORD_set_pos( i1, c1, n1 - 1 ), j1 = 1 ; j1 <= n1 ; j1++, CORD_prev( i1 ) ) {
        grl( j1, 0 ) = -gap_p * j1;
        grm( j1, 0 ) = '^';
        sm1 = sm + trp_cord_amino_index( CORD_pos_fetch( i1 ) ) * 20;
        for ( CORD_set_pos( i2, c2, n2 - 1 ), j2 = 1 ; j2 <= n2 ; j2++, CORD_prev( i2 ) ) {
            v1 = grl( j1 - 1, j2 - 1 ) + sm1[ trp_cord_amino_index( CORD_pos_fetch( i2 ) ) ];
            v2 = grl( j1 - 1, j2 ) - gap_p;
            v3 = grl( j1, j2 - 1 ) - gap_p;
            if ( v1 >= v2 ) {
                if ( v1 >= v3 ) {
                    grl( j1, j2 ) = v1;
                    grm( j1, j2 ) = '\\';
                } else {
                    grl( j1, j2 ) = v3;
                    grm( j1, j2 ) = '<';
                }
            } else {
                if ( v2 >= v3 ) {
                    grl( j1, j2 ) = v2;
                    grm( j1, j2 ) = '^';
                } else {
                    grl( j1, j2 ) = v3;
                    grm( j1, j2 ) = '<';
                }
            }
        }
    }
    v1 = 0;
    j1 = 0;
    for ( j2 = 1 ; j2 <= n1 ; j2++ )
        if ( grl( j2, n2 ) > v1 ) {
            v1 = grl( j2, n2 );
            j1 = j2;
        }
    CORD_ec_init( x1 );
    CORD_ec_init( x2 );
    CORD_set_pos( i1, c1, n1 - j1 );
    CORD_set_pos( i2, c2, 0 );
    v2 = 0;
    j2 = n2;
    for ( ; ; v2++ ) {
        mov = grm( j1, j2 );
        if ( mov == 'e' )
            break;
        if ( mov == '^' ) {
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, '-' );
            CORD_next( i1 );
            j1--;
        } else if ( mov == '<' ) {
            CORD_ec_append( x1, '-' );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i2 );
            j2--;
        } else { /* mov == '\\' */
            CORD_ec_append( x1, CORD_pos_fetch( i1 ) );
            CORD_ec_append( x2, CORD_pos_fetch( i2 ) );
            CORD_next( i1 );
            CORD_next( i2 );
            j1--;
            j2--;
        }
    }
    free( gridmov );
    return trp_list( trp_sig64( v1 ),
                     trp_cord_cons( CORD_balance( CORD_ec_to_cord( x1 ) ), v2 ),
                     trp_cord_cons( CORD_balance( CORD_ec_to_cord( x2 ) ), v2 ),
                     NULL );
}

