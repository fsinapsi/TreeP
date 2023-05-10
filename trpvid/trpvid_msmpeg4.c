/*
    TreeP Run Time Support
    Copyright (C) 2008-2023 Frank Sinapsi

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

uns8b trp_vid_parse_msmpeg4( trp_vid_t *vid )
{
    uns32b typ, qscale;
    uns8b c;

    c = vid->buf[ 0 ];
    typ = ( c & 0xc0 ) >> 6;
    if ( typ > 1 ) {
        vid->error = "MS MPEG4: frame coding type > 1";
        return 1;
    }
    qscale = ( c & 0x3e ) >> 1;
    if ( qscale == 0 ) {
        vid->error = "MS MPEG4: qscale = 0";
        return 1;
    }
    if ( vid->bitstream_type == 0 ) {
        /*
         FIXME
         qui bisognerebbe fare un'analisi piu' approfondita per
         cercare di capire se si tratta effettivamente di un MS MPEG4
         */
    }
    if ( typ == 0 )
        vid->cnt_vol++;
    trp_vid_update_qscale( vid, 2, typ, qscale );
    return 0;
}

