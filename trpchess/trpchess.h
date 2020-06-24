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

#ifndef __trpchess__h
#define __trpchess__h

uns8b trp_chess_init();
trp_obj_t *trp_chess_eq( trp_obj_t *st1, trp_obj_t *st2 );
trp_obj_t *trp_chess_dominance( trp_obj_t *st, trp_obj_t *idx, trp_obj_t *color );
trp_obj_t *trp_chess_dominated_not_p( trp_obj_t *st, trp_obj_t *idx, trp_obj_t *color );
trp_obj_t *trp_chess_legal_moves( trp_obj_t *st );
trp_obj_t *trp_chess_check( trp_obj_t *st );
trp_obj_t *trp_chess_checkmate( trp_obj_t *st );
trp_obj_t *trp_chess_stalemate( trp_obj_t *st );
trp_obj_t *trp_chess_double_check( trp_obj_t *st );
trp_obj_t *trp_chess_draw_by_insufficient_material( trp_obj_t *st );
trp_obj_t *trp_chess_threat_of_checkmate( trp_obj_t *st );
trp_obj_t *trp_chess_st_fen( trp_obj_t *st );
trp_obj_t *trp_chess_st_fen_short( trp_obj_t *st );
trp_obj_t *trp_chess_fen_st( trp_obj_t *fen );
trp_obj_t *trp_chess_st_raw_short( trp_obj_t *st );
trp_obj_t *trp_chess_raw_short_st( trp_obj_t *raw );
trp_obj_t *trp_chess_start();
trp_obj_t *trp_chess_color( trp_obj_t *st );
trp_obj_t *trp_chess_arrcw( trp_obj_t *st );
trp_obj_t *trp_chess_arrlw( trp_obj_t *st );
trp_obj_t *trp_chess_arrcb( trp_obj_t *st );
trp_obj_t *trp_chess_arrlb( trp_obj_t *st );
trp_obj_t *trp_chess_enp( trp_obj_t *st );
trp_obj_t *trp_chess_ply( trp_obj_t *st );
trp_obj_t *trp_chess_mov( trp_obj_t *st );
trp_obj_t *trp_chess_white_pieces( trp_obj_t *st );
trp_obj_t *trp_chess_black_pieces( trp_obj_t *st );
trp_obj_t *trp_chess_pieces( trp_obj_t *st );
trp_obj_t *trp_chess_zero_moves( trp_obj_t *st );
trp_obj_t *trp_chess_one_move_only( trp_obj_t *st );
uns8b trp_chess_next( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_next_fun( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_diff( trp_obj_t *st1, trp_obj_t *st2 );
trp_obj_t *trp_chess_move_is_capture( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_capture_ep( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_castle( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_promotion( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_check( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_checkmate( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_stalemate( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_discovery_check( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_is_double_check( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_move_alg( trp_obj_t *st, trp_obj_t *move );
trp_obj_t *trp_chess_search_mate( trp_obj_t *st, trp_obj_t *maxmoves, trp_obj_t *net );
trp_obj_t *trp_chess_perft( trp_obj_t *st, trp_obj_t *depth, trp_obj_t *net );
trp_obj_t *trp_chess_qmoves_raw( trp_obj_t *qmoves );
trp_obj_t *trp_chess_raw_qmoves( trp_obj_t *raw );

#endif /* !__trpchess__h */
