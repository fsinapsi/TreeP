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

#include "../trp/trp.h"
#include "./trpwn.h"
#include "wn.h"

#ifdef MINGW
#define fseeko fseeko64
#define ftello ftello64
#endif

static uns8b *_trp_wn_license={
  "\170\332\245\124\313\156\333\070\024\355\252\013\003\363\017" \
  "\027\135\153\322\240\003\314\242\073\306\242\033\241\266\044" \
  "\110\162\063\136\322\026\035\023\225\104\201\244\022\350\357" \
  "\173\256\144\307\111\047\263\232\215\055\336\367\071\367\361" \
  "\107\374\361\303\207\352\144\074\171\173\014\317\312\151\122" \
  "\135\115\265\012\152\257\274\046\050\366\332\164\217\324\073" \
  "\373\144\152\135\123\260\064\332\041\242\160\322\264\116\226" \
  "\062\055\245\214\150\077\022\055\162\147\272\203\016\266\243" \
  "\155\147\236\264\363\046\214\064\164\265\166\223\365\321\066" \
  "\215\175\346\140\215\071\350\316\353\033\242\273\221\354\076" \
  "\050\323\101\034\321\340\131\113\013\224\360\331\072\072\330" \
  "\176\144\101\370\317\372\042\256\205\324\243\323\032\126\052" \
  "\114\317\223\172\322\010\342\264\252\243\071\275\017\326\342" \
  "\233\075\237\115\323\040\160\333\067\043\276\303\211\053\003" \
  "\316\240\135\353\047\203\203\355\152\023\214\355\374\315\127" \
  "\104\141\130\320\031\357\041\142\360\003\147\345\312\042\152" \
  "\155\155\216\343\134\220\361\301\231\375\020\364\073\325\322" \
  "\342\205\117\176\232\340\251\266\207\241\325\135\120\234\011" \
  "\314\070\150\106\352\007\327\333\263\025\027\147\207\100\107" \
  "\100\203\032\170\354\250\032\020\212\350\047\355\064\030\177" \
  "\164\252\013\032\300\256\315\271\160\160\246\304\276\201\112" \
  "\213\267\155\140\024\316\074\236\002\165\066\240\045\123\136" \
  "\217\232\064\227\346\043\102\073\233\241\236\133\240\031\343" \
  "\241\121\246\325\056\232\173\064\247\143\225\127\055\274\373" \
  "\136\053\107\300\043\326\153\016\156\264\047\173\234\015\316" \
  "\174\104\364\206\012\060\363\232\210\327\031\047\162\315\141" \
  "\222\373\053\260\126\375\324\023\137\006\320\135\247\032\304" \
  "\100\113\230\042\226\276\364\001\136\067\163\373\036\254\253" \
  "\123\035\350\257\233\133\132\276\040\376\162\173\373\067\117" \
  "\355\173\063\013\107\201\051\231\014\075\071\314\207\173\322" \
  "\365\071\134\165\237\224\124\146\253\352\101\024\222\104\032" \
  "\123\054\052\161\047\112\111\120\344\105\366\043\211\145\114" \
  "\237\104\211\367\247\311\040\057\222\164\051\253\054\205\373" \
  "\066\115\176\310\242\114\252\035\155\304\167\131\122\232\121" \
  "\041\363\102\226\062\255\104\225\144\151\111\131\101\010\136" \
  "\210\264\112\144\031\221\374\207\325\223\230\026\311\046\137" \
  "\047\062\346\325\331\301\152\107\331\012\006\002\122\154\341" \
  "\335\266\102\274\012\173\271\111\346\140\321\377\113\316\321" \
  "\067\262\130\336\343\371\047\002\210\273\144\315\336\060\132" \
  "\045\125\312\125\255\360\055\322\035\345\242\250\222\345\166" \
  "\055\012\312\267\105\236\201\017\150\252\173\121\341\107\322" \
  "\026\157\132\040\034\077\316\167\043\176\341\061\272\222\010" \
  "\247\070\133\156\067\227\212\350\041\301\064\061\052\200\117" \
  "\127\100\363\115\116\011\321\210\042\236\322\162\362\012\366" \
  "\340\152\231\345\273\042\371\166\317\337\125\041\142\271\021" \
  "\305\367\063\165\031\122\027\064\153\057\315\304\150\166\074" \
  "\273\030\323\167\257\027\106\352\052\157\325\310\233\202\213" \
  "\310\127\000\213\334\361\036\324\260\015\146\272\134\260\356" \
  "\207\075\256\033\273\366\020\317\207\215\367\360\365\140\376" \
  "\276\024\327\213\167\131\016\224\127\231\320\234\067\370\062" \
  "\263\310\367\346\270\374\153\231\370\206\050\357\355\301\140" \
  "\207\353\337\216\214\077\051\014\065\326\210\377\002\326\230" \
  "\107\273\105\205\227\343\360\056\001\034\370\162\346\347\253" \
  "\342\271\250\376\274\025\323\346\063\231\277\000\255\032\016" \
  "\214"
};

uns8b trp_wn_init()
{
    uns8b res = wninit() ? 1 : 0;

    if ( res == 0 ) {
        FILE *fp = datafps[ 3 ];
        char buf[ 41 ];

        res = 1;
        if ( fseeko( fp, 795, SEEK_SET ) == 0 )
            if ( fread( buf, 1, 41, fp ) == 41 )
                if ( memcmp( buf, "WordNet 3.0 Copyright 2006 by Princeton University.", 41 ) == 0 ) {
                    res = 0;
                    fseeko( fp, 0, SEEK_SET );
                }
    }
    if ( res )
        fprintf( stderr, "Initialization of WordNet failed\n" );
    return res;
}

trp_obj_t *trp_wn_release()
{
    return trp_cord( wnrelease );
}

trp_obj_t *trp_wn_license()
{
    trp_raw_t raw;

    raw.tipo = TRP_RAW;
    raw.mode = 2;
    raw.unc_tipo = TRP_CORD;
    raw.compression_level = 9;
    raw.len = 811;
    raw.unc_len = 1609;
    raw.data = _trp_wn_license;
    return trp_uncompress( (trp_obj_t *)( &raw ) );
}



