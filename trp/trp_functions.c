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

#include "trp.h"

static trp_obj_t *trp_default_obj( trp_obj_t *obj );
static uns8b trp_default_print( trp_print_t *p, trp_obj_t *obj );
static uns8b trp_default_close( trp_obj_t *obj );
static trp_obj_t *trp_default_nth( uns32b n, trp_obj_t *obj );
static trp_obj_t *trp_default_sub( uns32b start, uns32b len, trp_obj_t *obj );
static trp_obj_t *trp_default_cat( trp_obj_t *obj, va_list args );
static uns8b trp_in_interv( trp_obj_t *obj, trp_obj_t *from, trp_obj_t *to );
static uns8b trp_default_in( trp_obj_t *obj, trp_obj_t *seq, uns32b *pos, uns32b nth );
static void trp_default_encode( trp_obj_t *obj, uns8b **buf );
static trp_obj_t *trp_default_relation( trp_obj_t *o1, trp_obj_t *o2 );

uns8b *_trp_tipo_descr[ TRP_MAX ] = {
    "TRP_SPECIAL",
    "TRP_RAW",
    "TRP_CHAR",
    "TRP_DATE",
    "TRP_FILE",
    "TRP_SIG64",
    "TRP_MPI",
    "TRP_RATIO",
    "TRP_COMPLEX",
    "TRP_CONS",
    "TRP_ARRAY",
    "TRP_QUEUE",
    "TRP_STACK",
    "TRP_CORD",
    "TRP_TREE",
    "TRP_MATRIX",
    "TRP_FUNPTR",
    "TRP_NETPTR",
    "TRP_THREAD",
    "TRP_CURL",
    "TRP_PIX",
    "TRP_ASSOC",
    "TRP_MYSQL",
    "TRP_SQLITE3",
    "TRP_AUD",
    "TRP_VID",
    "TRP_GTK",
    "TRP_AVI",
    "TRP_AVCODEC",
    "TRP_OPENCV",
    "TRP_VLFEAT",
    "TRP_SUF",
    "TRP_MGL",
    "TRP_IUP",
    "TRP_REGEX",
    "TRP_FIBO",
    "TRP_CHESS"
};

uns8bfun_t _trp_print_fun[ TRP_MAX ] = {
    trp_special_print,
    trp_raw_print,
    trp_char_print,
    trp_date_print,
    trp_file_print,
    trp_sig64_print,
    trp_mpi_print,
    trp_ratio_print,
    trp_complex_print,
    trp_list_print,
    trp_array_print,
    trp_queue_print,
    trp_stack_print,
    trp_cord_print,
    trp_tree_print,
    trp_default_print, /* matrix */
    trp_funptr_print,
    trp_netptr_print,
    trp_default_print, /* thread */
    trp_default_print, /* curl */
    trp_default_print, /* pix */
    trp_assoc_print,
    trp_default_print, /* mysql */
    trp_default_print, /* sqlite3 */
    trp_default_print, /* aud */
    trp_default_print, /* vid */
    trp_default_print, /* gtk */
    trp_default_print, /* avi */
    trp_default_print, /* avcodec */
    trp_default_print, /* opencv */
    trp_default_print, /* vlfeat */
    trp_default_print, /* suf */
    trp_default_print, /* mgl */
    trp_default_print, /* iup */
    trp_default_print, /* regex */
    trp_fibo_print,
    trp_default_print  /* chess */
};

uns32bfun_t _trp_size_fun[ TRP_MAX ] = {
    trp_special_size,
    trp_raw_size,
    trp_char_size,
    trp_date_size,
    trp_special_size, /* file */
    trp_sig64_size,
    trp_mpi_size,
    trp_ratio_size,
    trp_complex_size,
    trp_list_size,
    trp_array_size,
    trp_queue_size,
    trp_stack_size,
    trp_cord_size,
    trp_tree_size,
    trp_special_size, /* matrix */
    trp_special_size, /* funptr */
    trp_special_size, /* netptr */
    trp_special_size, /* thread */
    trp_special_size, /* curl */
    trp_special_size, /* pix */
    trp_assoc_size,
    trp_special_size, /* mysql */
    trp_special_size, /* sqlite3 */
    trp_special_size, /* aud */
    trp_special_size, /* vid */
    trp_special_size, /* gtk */
    trp_special_size, /* avi */
    trp_special_size, /* avcodec */
    trp_special_size, /* opencv */
    trp_special_size, /* vlfeat */
    trp_special_size, /* suf */
    trp_special_size, /* mgl */
    trp_special_size, /* iup */
    trp_special_size, /* regex */
    trp_special_size, /* fibo */
    trp_special_size  /* chess */
};

voidfun_t _trp_encode_fun[ TRP_MAX ] = {
    trp_special_encode,
    trp_raw_encode,
    trp_char_encode,
    trp_date_encode,
    trp_default_encode, /* file */
    trp_sig64_encode,
    trp_mpi_encode,
    trp_ratio_encode,
    trp_complex_encode,
    trp_list_encode,
    trp_array_encode,
    trp_queue_encode,
    trp_stack_encode,
    trp_cord_encode,
    trp_tree_encode,
    trp_default_encode, /* matrix */
    trp_default_encode, /* funptr */
    trp_default_encode, /* netptr */
    trp_default_encode, /* thread */
    trp_default_encode, /* curl */
    trp_default_encode, /* pix */
    trp_assoc_encode,
    trp_default_encode, /* mysql */
    trp_default_encode, /* sqlite3 */
    trp_default_encode, /* aud */
    trp_default_encode, /* vid */
    trp_default_encode, /* gtk */
    trp_default_encode, /* avi */
    trp_default_encode, /* avcodec */
    trp_default_encode, /* opencv */
    trp_default_encode, /* vlfeat */
    trp_default_encode, /* suf */
    trp_default_encode, /* mgl */
    trp_default_encode, /* iup */
    trp_default_encode, /* regex */
    trp_default_encode, /* fibo */
    trp_default_encode  /* chess */
};

objfun_t _trp_decode_fun[ TRP_MAX ] = {
    trp_special_decode,
    trp_raw_decode,
    trp_char_decode,
    trp_date_decode,
    trp_special_decode, /* file */
    trp_sig64_decode,
    trp_mpi_decode,
    trp_ratio_decode,
    trp_complex_decode,
    trp_list_decode,
    trp_array_decode,
    trp_queue_decode,
    trp_stack_decode,
    trp_cord_decode,
    trp_tree_decode,
    trp_special_decode, /* matrix */
    trp_special_decode, /* funptr */
    trp_special_decode, /* netptr */
    trp_special_decode, /* thread */
    trp_special_decode, /* curl */
    trp_special_decode, /* pix */
    trp_assoc_decode,
    trp_special_decode, /* mysql */
    trp_special_decode, /* sqlite3 */
    trp_special_decode, /* aud */
    trp_special_decode, /* vid */
    trp_special_decode, /* gtk */
    trp_special_decode, /* avi */
    trp_special_decode, /* avcodec */
    trp_special_decode, /* opencv */
    trp_special_decode, /* vlfeat */
    trp_special_decode, /* suf */
    trp_special_decode, /* mgl */
    trp_special_decode, /* iup */
    trp_special_decode, /* regex */
    trp_special_decode, /* fibo */
    trp_special_decode  /* chess */
};

objfun_t _trp_equal_fun[ TRP_MAX ] = {
    trp_special_equal,
    trp_raw_equal,
    trp_char_equal,
    trp_date_equal,
    trp_file_equal,
    trp_sig64_equal,
    trp_mpi_equal,
    trp_ratio_equal,
    trp_complex_equal,
    trp_list_equal,
    trp_array_equal,
    trp_queue_equal,
    trp_stack_equal,
    trp_cord_equal,
    trp_tree_equal,
    trp_default_relation, /* matrix */
    trp_funptr_equal,
    trp_netptr_equal,
    trp_default_relation, /* thread */
    trp_default_relation, /* curl */
    trp_default_relation, /* pix */
    trp_assoc_equal,
    trp_default_relation, /* mysql */
    trp_default_relation, /* sqlite3 */
    trp_default_relation, /* aud */
    trp_default_relation, /* vid */
    trp_default_relation, /* gtk */
    trp_default_relation, /* avi */
    trp_default_relation, /* avcodec */
    trp_default_relation, /* opencv */
    trp_default_relation, /* vlfeat */
    trp_default_relation, /* suf */
    trp_default_relation, /* mgl */
    trp_default_relation, /* iup */
    trp_default_relation, /* regex */
    trp_default_relation, /* fibo */
    trp_default_relation  /* chess */
};

objfun_t _trp_less_fun[ TRP_MAX ] = {
    trp_special_less,
    trp_default_relation, /* raw */
    trp_char_less,
    trp_date_less,
    trp_default_relation, /* file */
    trp_sig64_less,
    trp_mpi_less,
    trp_ratio_less,
    trp_complex_less,
    trp_list_less,
    trp_array_less,
    trp_queue_less,
    trp_default_relation, /* stack */
    trp_cord_less,
    trp_tree_less,
    trp_default_relation, /* matrix */
    trp_funptr_less,
    trp_netptr_less,
    trp_default_relation, /* thread */
    trp_default_relation, /* curl */
    trp_default_relation, /* pix */
    trp_default_relation, /* assoc */
    trp_default_relation, /* mysql */
    trp_default_relation, /* sqlite3 */
    trp_default_relation, /* aud */
    trp_default_relation, /* vid */
    trp_default_relation, /* gtk */
    trp_default_relation, /* avi */
    trp_default_relation, /* avcodec */
    trp_default_relation, /* opencv */
    trp_default_relation, /* vlfeat */
    trp_default_relation, /* suf */
    trp_default_relation, /* mgl */
    trp_default_relation, /* iup */
    trp_default_relation, /* regex */
    trp_default_relation, /* fibo */
    trp_default_relation  /* chess */
};

uns8bfun_t _trp_close_fun[ TRP_MAX ] = {
    trp_default_close, /* special */
    trp_raw_close,
    trp_default_close, /* char */
    trp_default_close, /* date */
    trp_file_close,
    trp_default_close, /* sig64 */
    trp_default_close, /* mpi */
    trp_default_close, /* ratio */
    trp_default_close, /* complex */
    trp_default_close, /* list */
    trp_array_close,
    trp_queue_close,
    trp_default_close, /* stack */
    trp_default_close, /* cord */
    trp_default_close, /* tree */
    trp_default_close, /* matrix */
    trp_default_close, /* funptr */
    trp_default_close, /* netptr */
    trp_default_close, /* thread */
    trp_default_close, /* curl */
    trp_default_close, /* pix */
    trp_default_close, /* assoc */
    trp_default_close, /* mysql */
    trp_default_close, /* sqlite3 */
    trp_default_close, /* aud */
    trp_default_close, /* vid */
    trp_default_close, /* gtk */
    trp_default_close, /* avi */
    trp_default_close, /* avcodec */
    trp_default_close, /* opencv */
    trp_default_close, /* vlfeat */
    trp_default_close, /* suf */
    trp_default_close, /* mgl */
    trp_default_close, /* iup */
    trp_regex_close,
    trp_default_close, /* fibo */
    trp_default_close  /* chess */
};

objfun_t _trp_length_fun[ TRP_MAX ] = {
    trp_special_length,
    trp_raw_length,
    trp_char_length,
    trp_date_length,
    trp_file_length,
    trp_sig64_length,
    trp_mpi_length,
    trp_ratio_length,
    trp_complex_length,
    trp_list_length,
    trp_array_length,
    trp_queue_length,
    trp_stack_length,
    trp_cord_length,
    trp_tree_length,
    trp_default_obj, /* matrix */
    trp_funptr_length,
    trp_netptr_length,
    trp_default_obj, /* thread */
    trp_default_obj, /* curl */
    trp_default_obj, /* pix */
    trp_assoc_length,
    trp_default_obj, /* mysql */
    trp_default_obj, /* sqlite3 */
    trp_default_obj, /* aud */
    trp_default_obj, /* vid */
    trp_default_obj, /* gtk */
    trp_default_obj, /* avi */
    trp_default_obj, /* avcodec */
    trp_default_obj, /* opencv */
    trp_default_obj, /* vlfeat */
    trp_default_obj, /* suf */
    trp_default_obj, /* mgl */
    trp_default_obj, /* iup */
    trp_regex_length,
    trp_fibo_length,
    trp_default_obj  /* chess */
};

objfun_t _trp_width_fun[ TRP_MAX ] = {
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj, /* matrix */
    trp_default_obj,
    trp_default_obj,
    trp_default_obj, /* thread */
    trp_default_obj, /* curl */
    trp_default_obj, /* pix */
    trp_default_obj, /* assoc */
    trp_default_obj, /* mysql */
    trp_default_obj, /* sqlite3 */
    trp_default_obj, /* aud */
    trp_default_obj, /* vid */
    trp_default_obj, /* gtk */
    trp_default_obj, /* avi */
    trp_default_obj, /* avcodec */
    trp_default_obj, /* opencv */
    trp_default_obj, /* vlfeat */
    trp_default_obj, /* suf */
    trp_default_obj, /* mgl */
    trp_default_obj, /* iup */
    trp_default_obj, /* regex */
    trp_fibo_width,
    trp_default_obj  /* chess */
};

objfun_t _trp_height_fun[ TRP_MAX ] = {
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj,
    trp_default_obj, /* matrix */
    trp_default_obj,
    trp_default_obj,
    trp_default_obj, /* thread */
    trp_default_obj, /* curl */
    trp_default_obj, /* pix */
    trp_assoc_height,
    trp_default_obj, /* mysql */
    trp_default_obj, /* sqlite3 */
    trp_default_obj, /* aud */
    trp_default_obj, /* vid */
    trp_default_obj, /* gtk */
    trp_default_obj, /* avi */
    trp_default_obj, /* avcodec */
    trp_default_obj, /* opencv */
    trp_default_obj, /* vlfeat */
    trp_default_obj, /* suf */
    trp_default_obj, /* mgl */
    trp_default_obj, /* iup */
    trp_default_obj, /* regex */
    trp_default_obj, /* fibo */
    trp_default_obj  /* chess */
};

objfun_t _trp_nth_fun[ TRP_MAX ] = {
    trp_default_nth, /* special */
    trp_raw_nth,
    trp_default_nth, /* char */
    trp_default_nth, /* date */
    trp_default_nth, /* file */
    trp_default_nth, /* sig64 */
    trp_default_nth, /* mpi */
    trp_default_nth, /* ratio */
    trp_default_nth, /* complex */
    trp_list_nth,
    trp_array_nth,
    trp_queue_nth,
    trp_stack_nth,
    trp_cord_nth,
    trp_tree_nth,
    trp_default_nth, /* matrix */
    trp_default_nth, /* funptr */
    trp_default_nth, /* netptr */
    trp_default_nth, /* thread */
    trp_default_nth, /* curl */
    trp_default_nth, /* pix */
    trp_default_nth, /* assoc --- implementata nella funzione stessa */
    trp_default_nth, /* mysql */
    trp_default_nth, /* sqlite3 */
    trp_default_nth, /* aud */
    trp_default_nth, /* vid */
    trp_default_nth, /* gtk */
    trp_default_nth, /* avi */
    trp_default_nth, /* avcodec */
    trp_default_nth, /* opencv */
    trp_default_nth, /* vlfeat */
    trp_default_nth, /* suf */
    trp_default_nth, /* mgl */
    trp_default_nth, /* iup */
    trp_default_nth, /* regex */
    trp_default_nth, /* fibo */
    trp_default_nth  /* chess */
};

objfun_t _trp_sub_fun[ TRP_MAX ] = {
    trp_special_sub,
    trp_default_sub, /* raw */
    trp_default_sub, /* char */
    trp_default_sub, /* date */
    trp_default_sub, /* file */
    trp_default_sub, /* sig64 */
    trp_default_sub, /* mpi */
    trp_default_sub, /* ratio */
    trp_default_sub, /* complex */
    trp_list_sub,
    trp_array_sub,
    trp_default_sub, /* queue */
    trp_default_sub, /* stack */
    trp_cord_sub,
    trp_default_sub, /* tree */
    trp_default_sub, /* matrix */
    trp_default_sub, /* funptr */
    trp_default_sub, /* netptr */
    trp_default_sub, /* thread */
    trp_default_sub, /* curl */
    trp_default_sub, /* pix */
    trp_default_sub, /* assoc */
    trp_default_sub, /* mysql */
    trp_default_sub, /* sqlite3 */
    trp_default_sub, /* aud */
    trp_default_sub, /* vid */
    trp_default_sub, /* gtk */
    trp_default_sub, /* avi */
    trp_default_sub, /* avcodec */
    trp_default_sub, /* opencv */
    trp_default_sub, /* vlfeat */
    trp_default_sub, /* suf */
    trp_default_sub, /* mgl */
    trp_default_sub, /* iup */
    trp_default_sub, /* regex */
    trp_default_sub, /* fibo */
    trp_default_sub  /* chess */
};

objfun_t _trp_cat_fun[ TRP_MAX ] = {
    trp_list_cat,
    trp_raw_cat, /* raw */
    trp_char_cat, /* char */
    trp_date_cat, /* date */
    trp_default_cat, /* file */
    trp_math_cat, /* sig64 */
    trp_math_cat, /* mpi */
    trp_math_cat, /* ratio */
    trp_math_cat, /* complex */
    trp_list_cat,
    trp_default_cat, /* array */
    trp_default_cat, /* queue */
    trp_default_cat, /* stack */
    trp_cord_cat,
    trp_default_cat, /* tree */
    trp_default_cat, /* matrix */
    trp_default_cat, /* funptr */
    trp_default_cat, /* netptr */
    trp_default_cat, /* thread */
    trp_default_cat, /* curl */
    trp_default_cat, /* pix */
    trp_default_cat, /* assoc */
    trp_default_cat, /* mysql */
    trp_default_cat, /* sqlite3 */
    trp_default_cat, /* aud */
    trp_default_cat, /* vid */
    trp_default_cat, /* gtk */
    trp_default_cat, /* avi */
    trp_default_cat, /* avcodec */
    trp_default_cat, /* opencv */
    trp_default_cat, /* vlfeat */
    trp_default_cat, /* suf */
    trp_default_cat, /* mgl */
    trp_default_cat, /* iup */
    trp_default_cat, /* regex */
    trp_default_cat, /* fibo */
    trp_default_cat  /* chess */
};

uns8bfun_t _trp_in_fun[ TRP_MAX ] = {
    trp_default_in, /* special */
    trp_default_in, /* raw */
    trp_default_in, /* char */
    trp_default_in, /* date */
    trp_default_in, /* file */
    trp_default_in, /* sig64 */
    trp_default_in, /* mpi */
    trp_default_in, /* ratio */
    trp_default_in, /* complex */
    trp_list_in,
    trp_array_in,
    trp_queue_in,
    trp_stack_in,
    trp_cord_in,
    trp_default_in, /* tree */
    trp_default_in, /* matrix */
    trp_default_in, /* funptr */
    trp_default_in, /* netptr */
    trp_default_in, /* thread */
    trp_default_in, /* curl */
    trp_default_in, /* pix */
    trp_assoc_in, /* assoc */
    trp_default_in, /* mysql */
    trp_default_in, /* sqlite3 */
    trp_default_in, /* aud */
    trp_default_in, /* vid */
    trp_default_in, /* gtk */
    trp_default_in, /* avi */
    trp_default_in, /* avcodec */
    trp_default_in, /* opencv */
    trp_default_in, /* vlfeat */
    trp_default_in, /* suf */
    trp_default_in, /* mgl */
    trp_default_in, /* iup */
    trp_default_in, /* regex */
    trp_default_in, /* fibo */
    trp_default_in  /* chess */
};

static trp_obj_t *trp_default_obj( trp_obj_t *obj )
{
    return UNDEF;
}

static uns8b trp_default_print( trp_print_t *p, trp_obj_t *obj )
{
    uns8b *msg;

    msg = _trp_tipo_descr[ obj->tipo ];
    return trp_print_char_star( p, msg );
}

uns8b trp_close( trp_obj_t *obj )
{
    return (_trp_close_fun[ obj->tipo ])( obj );
}

uns8b trp_close_multi( trp_obj_t *obj, ... )
{
    va_list args;

    va_start( args, obj );
    for ( ; obj ; obj = va_arg( args, trp_obj_t * ) )
        (_trp_close_fun[ obj->tipo ])( obj );
    va_end( args );
    return 0;
}

static uns8b trp_default_close( trp_obj_t *obj )
{
    return 0;
}

trp_obj_t *trp_length( trp_obj_t *obj )
{
    return (_trp_length_fun[ obj->tipo ])( obj );
}

trp_obj_t *trp_width( trp_obj_t *obj )
{
    return (_trp_width_fun[ obj->tipo ])( obj );
}

trp_obj_t *trp_height( trp_obj_t *obj )
{
    return (_trp_height_fun[ obj->tipo ])( obj );
}

trp_obj_t *trp_nth( trp_obj_t *n, trp_obj_t *obj )
{
    uns32b nn;

    if ( obj->tipo == TRP_ASSOC )
        return trp_assoc_get( obj, n );
    if ( trp_cast_uns32b( n, &nn ) ) {
        sig32b mm;

        if ( trp_cast_sig32b( n, &mm ) )
            return UNDEF;
        n = (_trp_length_fun[ obj->tipo ])( obj );
        if ( n->tipo != TRP_SIG64 )
            return UNDEF;
        mm += (sig32b)( ((trp_sig64_t *)n)->val );
        if ( mm < 0 )
            return UNDEF;
        nn = (uns32b)mm;
    }
    return (_trp_nth_fun[ obj->tipo ])( nn, obj );
}

static trp_obj_t *trp_default_nth( uns32b n, trp_obj_t *obj )
{
    return UNDEF;
}

trp_obj_t *trp_sub( trp_obj_t *start, trp_obj_t *len, trp_obj_t *obj )
{
    uns32b s, l;

    if ( ( start->tipo != TRP_SIG64 ) || ( len->tipo != TRP_SIG64 ) )
        return UNDEF;
    if ( ( ((trp_sig64_t *)start)->val < 0 ) ||
         ( ((trp_sig64_t *)start)->val > 0xffffffff ) ||
         ( ((trp_sig64_t *)len)->val < 0 ) )
        return UNDEF;
    s = (uns32b)( ((trp_sig64_t *)start)->val );
    l = ( ((trp_sig64_t *)len)->val > 0xffffffff )
        ? 0xffffffff
        : (uns32b)( ((trp_sig64_t *)len)->val );
    return (_trp_sub_fun[ obj->tipo ])( s, l, obj );
}

static trp_obj_t *trp_default_sub( uns32b start, uns32b len, trp_obj_t *obj )
{
    return UNDEF;
}

trp_obj_t *trp_cat( trp_obj_t *obj, ... )
{
    trp_obj_t *res;
    va_list args;

    va_start( args, obj );
    res = (_trp_cat_fun[ obj->tipo ])( obj, args );
    va_end( args );
    return res;
}

static trp_obj_t *trp_default_cat( trp_obj_t *obj, va_list args )
{
    return UNDEF;
}

trp_obj_t *trp_in_func( trp_obj_t *obj, trp_obj_t *seq, trp_obj_t *interv )
{
    uns32b p;
    trp_obj_t *res;

    if ( interv )
        res = trp_in_interv( obj, seq, interv ) ? TRP_FALSE : TRP_TRUE;
    else
        res = (_trp_in_fun[ seq->tipo ])( obj, seq, &p, 0 ) ? TRP_FALSE : TRP_TRUE;
    return res;
}

uns8b trp_in_test( trp_obj_t *obj, trp_obj_t *seq, trp_obj_t *interv, trp_obj_t **pos, trp_obj_t *nth )
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
    if ( interv ) {
        if ( trp_in_interv( obj, seq, interv ) )
            return 1;
    } else {
        if ( (_trp_in_fun[ seq->tipo ])( obj, seq, &p, n ) )
            return 1;
    }
    if ( pos )
        *pos = trp_sig64( p );
    return 0;
}

static uns8b trp_in_interv( trp_obj_t *obj, trp_obj_t *from, trp_obj_t *to )
{
    if ( ( ( obj->tipo == TRP_SIG64 ) || ( obj->tipo == TRP_MPI ) ) &&
         ( ( from->tipo == TRP_SIG64 ) || ( from->tipo == TRP_MPI ) ) &&
         ( ( to->tipo == TRP_SIG64 ) || ( to->tipo == TRP_MPI ) ) ) {
        if ( trp_less( obj, from ) == TRP_TRUE )
            return 1;
        if ( trp_less( to, obj ) == TRP_TRUE )
            return 1;
        return 0;
    }
    if ( ( obj->tipo == TRP_CHAR ) &&
         ( from->tipo == TRP_CHAR ) &&
         ( to->tipo == TRP_CHAR ) ) {
        if ( ((trp_char_t *)obj)->c < ((trp_char_t *)from)->c )
            return 1;
        if ( ((trp_char_t *)to)->c < ((trp_char_t *)obj)->c )
            return 1;
        return 0;
    }
    return 1;
}

static uns8b trp_default_in( trp_obj_t *obj, trp_obj_t *seq, uns32b *pos, uns32b nth )
{
    return 1;
}

static void trp_default_encode( trp_obj_t *obj, uns8b **buf )
{
    **buf = TRP_SPECIAL;
    ++(*buf);
    **buf = 0; /* UNDEF */
    ++(*buf);
}

static trp_obj_t *trp_default_relation( trp_obj_t *o1, trp_obj_t *o2 )
{
    return TRP_FALSE;
}

trp_obj_t *trp_reverse( trp_obj_t *obj )
{
    switch ( obj->tipo ) {
    case TRP_CONS:
        obj = trp_list_reverse( obj );
        break;
    case TRP_CORD:
        obj = trp_cord_reverse( (trp_cord_t *)obj );
        break;
    default:
        obj = UNDEF;
        break;
    }
    return obj;
}

trp_obj_t *trp_typeof( trp_obj_t *obj )
{
    return trp_cord( _trp_tipo_descr[ obj->tipo ] );
}

trp_obj_t *trp_typev( trp_obj_t *obj )
{
    uns32b n;

    if ( trp_cast_uns32b( obj, &n ) )
        return UNDEF;
    if ( n >= TRP_MAX )
        return UNDEF;
    return trp_cord( _trp_tipo_descr[ n ] );
}

trp_obj_t *trp_integerp( trp_obj_t *obj )
{
    return ( ( obj->tipo == TRP_SIG64 ) ||
             ( obj->tipo == TRP_MPI ) ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_rationalp( trp_obj_t *obj )
{
    return ( ( obj->tipo == TRP_SIG64 ) ||
             ( obj->tipo == TRP_MPI ) ||
             ( obj->tipo == TRP_RATIO ) ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_complexp( trp_obj_t *obj )
{
    return ( ( obj->tipo == TRP_SIG64 ) ||
             ( obj->tipo == TRP_MPI ) ||
             ( obj->tipo == TRP_RATIO ) ||
             ( obj->tipo == TRP_COMPLEX ) ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_listp( trp_obj_t *obj )
{
    return ( ( obj->tipo == TRP_CONS ) ||
             ( obj == NIL ) ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_booleanp( trp_obj_t *obj )
{
    return ( ( obj == TRP_TRUE ) ||
             ( obj == TRP_FALSE ) ) ? TRP_TRUE : TRP_FALSE;
}

