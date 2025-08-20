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

#include "../trp/trp.h"
#include "./trpchess.h"

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

typedef struct {
    uns8b flags; /* color + castling */
    uns8b enp;
    uns16b ply;
    uns16b mov;
    uns8b board[ 65 ];
} trp_chess_board_t;

typedef struct {
    uns8b domin[ 2 ][ 64 ];
} trp_chess_domin_t;

typedef struct {
    uns8b tipo;
    sig8b check;
    union {
        uns32b pieces;
        struct {
            uns8b pw;
            uns8b pb;
            uns8b kw;
            uns8b kb;
        };
    };
    trp_chess_board_t *board;
    trp_chess_domin_t *domin;
    trp_obj_t *moves;
} trp_chess_t;

typedef struct {
    uns8b idx1;
    uns16b idx2;
} trp_chess_move_t;

typedef struct {
    trp_obj_t *val;
    void *next;
} trp_queue_elem;

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

/* define the piece type */
typedef enum {
    EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
} trp_piece_type_t;

/* bitboard types */
typedef uns64b trp_chess_bb_t;

/* move structure */
typedef union {
    struct {
        uns8b MoveType;
        uns8b From;
        uns8b To;
        uns8b Prom;
    };
    uns32b Move;
} trp_chess_move_tt;

/* structure for a capture, it needs the Eval field for MVV/LVA ordering */
typedef struct {
    trp_chess_move_tt Move;
    int Eval;
} trp_chess_move_eval_t;

/*
Board structure definition

PM,P0,P1,P2 are the 4 bitboards that contain the whole board
PM is the bitboard with the side to move pieces
P0,P1 and P2: with these bitboards you can obtain every type of pieces and every pieces combinations.
*/
typedef struct {
    trp_chess_bb_t PM;
    trp_chess_bb_t P0;
    trp_chess_bb_t P1;
    trp_chess_bb_t P2;
    uns8b CastleFlags; /* ..sl..SL  short long opponent SHORT LONG side to move */
    uns8b EnPassant; /* enpassant column, =8 if not set */
    uns8b STM; /* side to move */
    uns8b Count50; /* 50 move rule counter */
    uns16b MovCnt;
} trp_chess_board_tt;

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

#define WHITE 0
#define BLACK 8

/* define the move type, for example
   KING|CASTLE is a castle move
   PAWN|CAPTURE|EP is an enpassant move
   PAWN|PROMO|CAPTURE is a promotion with a capture */
#define CASTLE  0x40
#define PROMO   0x20
#define EP      0x10
#define CAPTURE 0x08

#define PopCount(bb) (__builtin_popcountll(bb)) /* return the number of bits sets of a bitboard */
#define RevBB(bb) (__builtin_bswap64(bb)) /* reverse a bitboard */
#define MSB(bb) (__builtin_clzll(bb)^0x3f) /* return the index of the most significant bit of the bitboard, bb must always be !=0 */
#define LSB(bb) (__builtin_ctzll(bb)) /* return the index of the least significant bit of the bitboard, bb must always be !=0 */
#define ExtractLSB(bb) ((bb)&(-(bb))) /* extract the least significant bit of the bitboard */
#define ClearLSB(bb) ((bb)&((bb)-1)) /* reset the least significant bit of bb */

/* Macro to check and reset the castle rights:
   CastleSM: short castling side to move
   CastleLM: long castling side to move
   CastleSO: short castling opponent
   CastleLO: long castling opponent */
#define CastleSM(Pos) ((Pos)->CastleFlags&0x02)
#define CastleLM(Pos) ((Pos)->CastleFlags&0x01)
#define CastleSO(Pos) ((Pos)->CastleFlags&0x20)
#define CastleLO(Pos) ((Pos)->CastleFlags&0x10)
#define ResetCastleSM(Pos) ((Pos)->CastleFlags&=0xFD)
#define ResetCastleLM(Pos) ((Pos)->CastleFlags&=0xFE)
#define ResetCastleSO(Pos) ((Pos)->CastleFlags&=0xDF)
#define ResetCastleLO(Pos) ((Pos)->CastleFlags&=0xEF)

/* these Macros are used to calculate the bitboard of a particular kind of piece

   P2 P1 P0
    0  0  0    empty
    0  0  1    pawn
    0  1  0    knight
    0  1  1    bishop
    1  0  0    rook
    1  0  1    queen
    1  1  0    king
*/
#define Occupation(Pos) ((Pos)->P0 | (Pos)->P1 | (Pos)->P2) /* board occupation */
#define Pawns(Pos) ((Pos)->P0 & ~(Pos)->P1 & ~(Pos)->P2)
#define Knights(Pos) (~(Pos)->P0 & (Pos)->P1 & ~(Pos)->P2)
#define Bishops(Pos) ((Pos)->P0 & (Pos)->P1)
#define Rooks(Pos) (~(Pos)->P0 & ~(Pos)->P1 & (Pos)->P2)
#define Queens(Pos) ((Pos)->P0 & (Pos)->P2)
#define Kings(Pos) ((Pos)->P1 & (Pos)->P2) /* a bitboard with the 2 kings */

/* get the piece type giving the square */
#define Piece(Pos,sq) ((((Pos)->P2>>(sq))&1)<<2 | (((Pos)->P1>>(sq))&1)<<1 | (((Pos)->P0>>(sq))&1))

/* calculate the square related to the opponent */
#define OppSq(sq) ((sq)^0x38)
/* Absolute Square, we need this macro to return the move in long algebric notation  */
#define AbsSq(sq,col) ((col)==WHITE ? (sq):OppSq(sq))

/*
The board is always saved with the side to move in the lower part of the bitboards to use the same generation and
make for the Black and the White side.
This needs the inversion of the 4 bitboards, roll the Castle rights and update the side to move.
*/
#define ChangeSide(Pos) \
{ \
   (Pos)->PM^=Occupation((Pos)); \
   (Pos)->PM=RevBB((Pos)->PM); \
   (Pos)->P0=RevBB((Pos)->P0); \
   (Pos)->P1=RevBB((Pos)->P1); \
   (Pos)->P2=RevBB((Pos)->P2); \
   (Pos)->CastleFlags=((Pos)->CastleFlags>>4)|((Pos)->CastleFlags<<4); \
   (Pos)->STM ^= BLACK; \
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

#define trp_chess_abs_diff(a,b) (((a)>=(b))?((a)-(b)):((b)-(a)))
#define trp_chess_color_m(st) (((trp_chess_t *)(st))->board->flags&1)
#define trp_chess_arrcw_m(st) (((trp_chess_t *)(st))->board->flags&2)
#define trp_chess_arrlw_m(st) (((trp_chess_t *)(st))->board->flags&4)
#define trp_chess_arrcb_m(st) (((trp_chess_t *)(st))->board->flags&8)
#define trp_chess_arrlb_m(st) (((trp_chess_t *)(st))->board->flags&16)
#define trp_chess_piece_pack(p) (((p)&16)?12:(((p)&8)?((p)-2):(p)))
#define trp_chess_piece_unpack(p) (((p)==12)?16:(((p)>=6)?((p)+2):(p)))
static trp_obj_t *trp_chess_nth( uns32b n, trp_chess_t *obj );
static trp_obj_t *trp_chess_equal( trp_chess_t *st1, trp_chess_t *st2 );
static uns32b trp_chess_size( trp_chess_t *obj );
static void trp_chess_encode( trp_chess_t *st, uns8b **buf );
static trp_obj_t *trp_chess_decode( uns8b **buf );
static trp_chess_domin_t *trp_chess_dominance_low( trp_chess_t *st );
static int trp_chess_dominated_low( trp_chess_t *st, uns8b idx, uns8b color );
static trp_chess_t *trp_chess_clone_low( trp_chess_t *st );
static trp_obj_t *trp_chess_clone( trp_obj_t *st );
static trp_obj_t *trp_chess_legal_moves_low( trp_chess_t *st );
static uns8b trp_chess_check_low( trp_chess_t *st );
static trp_obj_t *trp_chess_st_fen_low( trp_obj_t *st, uns8b extended );
static void trp_chess_next_low( trp_chess_t *st, uns8b idx1, uns16b idx2 );
static uns8b trp_chess_search_mate_low( trp_chess_board_tt *pos, uns32b maxmoves, uns8bfun_t f, trp_chess_move_tt *winning_move, uns32b *actmoves, sig64b *positions, uns8b *interr );
static sig64b trp_chess_perft_low( trp_chess_board_tt *pos, uns32b depth, uns8bfun_t f, uns8b *interr, sig64b *interr_cnt );
static void trp_tmp_old_to_new( trp_chess_t *st, trp_chess_board_tt *pos );
static void trp_tmp_new_to_old( trp_chess_board_tt *pos, trp_chess_t *st );

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

/* array of bitboards that contains all the knight destination for every square */
static const trp_chess_bb_t KnightDest[ 64 ]= {
    0x0000000000020400ULL,0x0000000000050800ULL,0x00000000000a1100ULL,0x0000000000142200ULL,
    0x0000000000284400ULL,0x0000000000508800ULL,0x0000000000a01000ULL,0x0000000000402000ULL,
    0x0000000002040004ULL,0x0000000005080008ULL,0x000000000a110011ULL,0x0000000014220022ULL,
    0x0000000028440044ULL,0x0000000050880088ULL,0x00000000a0100010ULL,0x0000000040200020ULL,
    0x0000000204000402ULL,0x0000000508000805ULL,0x0000000a1100110aULL,0x0000001422002214ULL,
    0x0000002844004428ULL,0x0000005088008850ULL,0x000000a0100010a0ULL,0x0000004020002040ULL,
    0x0000020400040200ULL,0x0000050800080500ULL,0x00000a1100110a00ULL,0x0000142200221400ULL,
    0x0000284400442800ULL,0x0000508800885000ULL,0x0000a0100010a000ULL,0x0000402000204000ULL,
    0x0002040004020000ULL,0x0005080008050000ULL,0x000a1100110a0000ULL,0x0014220022140000ULL,
    0x0028440044280000ULL,0x0050880088500000ULL,0x00a0100010a00000ULL,0x0040200020400000ULL,
    0x0204000402000000ULL,0x0508000805000000ULL,0x0a1100110a000000ULL,0x1422002214000000ULL,
    0x2844004428000000ULL,0x5088008850000000ULL,0xa0100010a0000000ULL,0x4020002040000000ULL,
    0x0400040200000000ULL,0x0800080500000000ULL,0x1100110a00000000ULL,0x2200221400000000ULL,
    0x4400442800000000ULL,0x8800885000000000ULL,0x100010a000000000ULL,0x2000204000000000ULL,
    0x0004020000000000ULL,0x0008050000000000ULL,0x00110a0000000000ULL,0x0022140000000000ULL,
    0x0044280000000000ULL,0x0088500000000000ULL,0x0010a00000000000ULL,0x0020400000000000ULL
};

/* The same for the king */
static const trp_chess_bb_t KingDest[ 64 ] = {
    0x0000000000000302ULL,0x0000000000000705ULL,0x0000000000000e0aULL,0x0000000000001c14ULL,
    0x0000000000003828ULL,0x0000000000007050ULL,0x000000000000e0a0ULL,0x000000000000c040ULL,
    0x0000000000030203ULL,0x0000000000070507ULL,0x00000000000e0a0eULL,0x00000000001c141cULL,
    0x0000000000382838ULL,0x0000000000705070ULL,0x0000000000e0a0e0ULL,0x0000000000c040c0ULL,
    0x0000000003020300ULL,0x0000000007050700ULL,0x000000000e0a0e00ULL,0x000000001c141c00ULL,
    0x0000000038283800ULL,0x0000000070507000ULL,0x00000000e0a0e000ULL,0x00000000c040c000ULL,
    0x0000000302030000ULL,0x0000000705070000ULL,0x0000000e0a0e0000ULL,0x0000001c141c0000ULL,
    0x0000003828380000ULL,0x0000007050700000ULL,0x000000e0a0e00000ULL,0x000000c040c00000ULL,
    0x0000030203000000ULL,0x0000070507000000ULL,0x00000e0a0e000000ULL,0x00001c141c000000ULL,
    0x0000382838000000ULL,0x0000705070000000ULL,0x0000e0a0e0000000ULL,0x0000c040c0000000ULL,
    0x0003020300000000ULL,0x0007050700000000ULL,0x000e0a0e00000000ULL,0x001c141c00000000ULL,
    0x0038283800000000ULL,0x0070507000000000ULL,0x00e0a0e000000000ULL,0x00c040c000000000ULL,
    0x0302030000000000ULL,0x0705070000000000ULL,0x0e0a0e0000000000ULL,0x1c141c0000000000ULL,
    0x3828380000000000ULL,0x7050700000000000ULL,0xe0a0e00000000000ULL,0xc040c00000000000ULL,
    0x0203000000000000ULL,0x0507000000000000ULL,0x0a0e000000000000ULL,0x141c000000000000ULL,
    0x2838000000000000ULL,0x5070000000000000ULL,0xa0e0000000000000ULL,0x40c0000000000000ULL
};

/* masks for finding the pawns that can capture with an enpassant (in move generation) */
static const trp_chess_bb_t EnPassant[ 8 ] = {
    0x0000000200000000ULL,0x0000000500000000ULL,0x0000000A00000000ULL,0x0000001400000000ULL,
    0x0000002800000000ULL,0x0000005000000000ULL,0x000000A000000000ULL,0x0000004000000000ULL
};

/* masks for finding the pawns that can capture with an enpassant (in make move) */
static const trp_chess_bb_t EnPassantM[ 8 ] = {
    0x0000000002000000ULL,0x0000000005000000ULL,0x000000000A000000ULL,0x0000000014000000ULL,
    0x0000000028000000ULL,0x0000000050000000ULL,0x00000000A0000000ULL,0x0000000040000000ULL
};

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

/* return the bitboard with the rook destinations */
static inline trp_chess_bb_t GenRook( uns64b sq, trp_chess_bb_t occupation )
{
    trp_chess_bb_t piece = 1ULL << sq;
    occupation ^= piece; /* remove the selected piece from the occupation */
    trp_chess_bb_t piecesup = ( 0x0101010101010101ULL << sq ) & ( occupation | 0xFF00000000000000ULL ); /* find the pieces up */
    trp_chess_bb_t piecesdo = ( 0x8080808080808080ULL >> ( sq ^ 0x3f ) ) & ( occupation | 0x00000000000000FFULL ); /* find the pieces down */
    trp_chess_bb_t piecesri = ( 0x00000000000000FFULL << sq ) & ( occupation | 0x8080808080808080ULL ); /* find pieces on the right */
    trp_chess_bb_t piecesle = ( 0xFF00000000000000ULL >> ( sq ^ 0x3f ) ) & ( occupation | 0x0101010101010101ULL ); /* find pieces on the left */
    return ( ( ( 0x8080808080808080ULL >> ( LSB( piecesup ) ^ 0x3f ) ) & ( 0x0101010101010101ULL << MSB( piecesdo ) ) ) |
             ( ( 0xFF00000000000000ULL >> ( LSB( piecesri ) ^ 0x3f ) ) & ( 0x00000000000000FFULL << MSB( piecesle ) ) ) ) ^ piece;
    /* From every direction find the first piece and from that piece put a mask in the opposite direction.
     Put togheter all the 4 masks and remove the moving piece */
}

/* return the bitboard with the bishops destinations */
static inline trp_chess_bb_t GenBishop( uns64b sq, trp_chess_bb_t occupation )
{
    trp_chess_bb_t piece = 1ULL << sq;
    occupation ^= piece;
    trp_chess_bb_t piecesup = ( 0x8040201008040201ULL << sq ) & ( occupation | 0xFF80808080808080ULL );
    trp_chess_bb_t piecesdo = ( 0x8040201008040201ULL >> ( sq ^ 0x3f ) ) & ( occupation | 0x01010101010101FFULL );
    trp_chess_bb_t piecesle = ( 0x8102040810204081ULL << sq ) & ( occupation | 0xFF01010101010101ULL );
    trp_chess_bb_t piecesri = ( 0x8102040810204081ULL >> ( sq ^ 0x3f ) ) & ( occupation | 0x80808080808080FFULL );
    return ( ( ( 0x8040201008040201ULL >> ( LSB( piecesup ) ^ 0x3f ) ) & ( 0x8040201008040201ULL << MSB( piecesdo ) ) ) |
             ( ( 0x8102040810204081ULL >> ( LSB( piecesle ) ^ 0x3f ) ) & ( 0x8102040810204081ULL << MSB( piecesri ) ) ) ) ^ piece;
}

/* return the bitboard with pieces of the same type */
static inline trp_chess_bb_t BBPieces( trp_chess_board_tt *pos, trp_piece_type_t piece )
{
    switch ( piece ) { // find the bb with the pieces of the same type
    case PAWN:
        return Pawns( pos );
    case KNIGHT:
        return Knights( pos );
    case BISHOP:
        return Bishops( pos );
    case ROOK:
        return Rooks( pos );
    case QUEEN:
        return Queens( pos );
    case KING:
        return Kings( pos );
    }
}

/* return the bitboard with the destinations of a piece in a square (except for pawns) */
static inline trp_chess_bb_t BBDestinations( trp_piece_type_t piece, uns64b sq, trp_chess_bb_t occupation )
{
    switch ( piece ) {
    case KNIGHT:
        return KnightDest[ sq ];
    case BISHOP:
        return GenBishop( sq, occupation );
    case ROOK:
        return GenRook( sq, occupation );
    case QUEEN:
        return GenRook( sq, occupation ) | GenBishop( sq, occupation );
    case KING:
        return KingDest[ sq ];
    }
}

/* try the move and see if the king is in check. If so return the attacking pieces, if not return 0 */
static inline trp_chess_bb_t Illegal( trp_chess_board_tt *pos, trp_chess_move_tt move )
{
    trp_chess_bb_t From = 1ULL << move.From, To = 1ULL << move.To;
    trp_chess_bb_t occupation = Occupation( pos ), opposing = pos->PM ^ occupation;
    trp_chess_bb_t newoccupation, newopposing;
    trp_chess_bb_t king;
    uns64b kingsq;

    newoccupation = ( occupation ^ From ) | To;
    newopposing = opposing & ~To;
    if ( ( move.MoveType & 0x07 ) == KING ) {
        king = To;
        kingsq = move.To;
    } else {
        king = Kings( pos ) & pos->PM;
        kingsq = LSB( king );
        if ( move.MoveType & EP ) {
            newopposing ^= To >> 8;
            newoccupation ^= To >> 8;
        }
    }
    return ( ( ( KnightDest[ kingsq ] & Knights( pos ) ) |
               ( GenRook( kingsq, newoccupation ) & ( Rooks( pos ) | Queens( pos ) ) ) |
               ( GenBishop( kingsq, newoccupation ) & ( Bishops( pos ) | Queens( pos ) ) ) |
               ( ( ( ( king << 9 ) & 0xFEFEFEFEFEFEFEFEULL ) | ( ( king << 7 ) & 0x7F7F7F7F7F7F7F7FULL ) ) & Pawns( pos ) ) |
               ( KingDest[ kingsq ] & Kings( pos ) ) ) & newopposing );
}

/* Generate all pseudo-legal quiet moves */
static inline trp_chess_move_tt *GenerateQuiets( trp_chess_board_tt *pos, trp_chess_move_tt *const quiets )
{
    trp_chess_bb_t occupation,opposing;
    occupation = Occupation( pos );
    opposing = occupation^pos->PM;
    trp_chess_bb_t pieces, destinations;
    trp_piece_type_t piece;

    trp_chess_move_tt *pquiets=quiets;
    for ( piece = KING ; piece >= KNIGHT ; piece-- ) { // generate moves from king to knight
        // generate moves for every piece of the same type of the side to move
        for ( pieces = BBPieces( pos, piece ) & pos->PM ; pieces ; pieces = ClearLSB( pieces ) ) {
            uns64b sq = LSB( pieces );
            // for every destinations on a free square generate a move
            for ( destinations = ~occupation & BBDestinations( piece, sq, occupation ) ; destinations ; destinations = ClearLSB( destinations ) ) {
                pquiets->MoveType = piece;
                pquiets->From = sq;
                pquiets->To = LSB( destinations );
                pquiets->Prom = EMPTY;
                pquiets++;
            }
        }
    }

    /* one pawns push */
    trp_chess_bb_t push1 = ( ( ( Pawns( pos ) & pos->PM ) << 8 ) & ~occupation ) & 0x00FFFFFFFFFFFFFFULL;
    for ( pieces = push1 ; pieces ; pieces = ClearLSB( pieces ) ) {
        pquiets->MoveType = PAWN;
        pquiets->From = LSB( pieces ) - 8;
        pquiets->To = LSB( pieces );
        pquiets->Prom = EMPTY;
        pquiets++;
    }

    /* double pawns pushes */
    for ( pieces = ( push1 << 8 ) & ~occupation & 0x00000000FF000000ULL ; pieces ; pieces = ClearLSB( pieces ) ) {
        pquiets->MoveType = PAWN;
        pquiets->From = LSB( pieces ) - 16;
        pquiets->To = LSB( pieces );
        pquiets->Prom = EMPTY;
        pquiets++;
    }

    /* check if long castling is possible */
    if ( CastleLM( pos ) && !( occupation & 0x0EULL ) ) {
        trp_chess_bb_t roo, bis;
        roo = ExtractLSB( 0x1010101010101000ULL & occupation ); /* column e */
        roo |= ExtractLSB( 0x0808080808080800ULL & occupation ); /*column d */
        roo |= ExtractLSB( 0x0404040404040400ULL & occupation ); /*column c */
        roo |= ExtractLSB( 0x00000000000000E0ULL & occupation );  /* row 1 */
        bis = ExtractLSB( 0x0000000102040800ULL & occupation ); /*antidiag from e1/e8 */
        bis |= ExtractLSB( 0x0000000001020400ULL & occupation ); /*antidiag from d1/d8 */
        bis |= ExtractLSB( 0x0000000000010200ULL & occupation ); /*antidiag from c1/c8 */
        bis |= ExtractLSB( 0x0000000080402000ULL & occupation ); /*diag from e1/e8 */
        bis |= ExtractLSB( 0x0000008040201000ULL & occupation ); /*diag from d1/d8 */
        bis |= ExtractLSB( 0x0000804020100800ULL & occupation ); /*diag from c1/c8 */
        if ( ! ( ( ( roo & ( Rooks( pos ) | Queens( pos ) ) ) | ( bis & ( Bishops( pos ) | Queens( pos ) ) ) | ( 0x00000000003E7700ULL & Knights( pos ) ) |
                   ( 0x0000000000003E00ULL & Pawns( pos ) ) | ( Kings( pos ) & 0x0000000000000600ULL ) ) & opposing ) ) {
            /* check if c1/c8 d1/d8 e1/e8 are not attacked */
            pquiets->MoveType = KING | CASTLE;
            pquiets->From = 4;
            pquiets->To = 2;
            pquiets->Prom = EMPTY;
            pquiets++;
        }
    }
    /* check if short castling is possible */
    if ( CastleSM( pos ) && !( occupation & 0x60ULL ) ) {
        trp_chess_bb_t roo, bis;
        roo = ExtractLSB( 0x1010101010101000ULL & occupation ); /* column e */
        roo |= ExtractLSB( 0x2020202020202000ULL & occupation ); /* column f */
        roo |= ExtractLSB( 0x4040404040404000ULL & occupation ); /* column g */
        roo |= 1ULL << MSB( 0x000000000000000FULL & ( occupation | 0x1ULL ) ); /* row 1 */
        bis = ExtractLSB( 0x0000000102040800ULL & occupation ); /* antidiag from e1/e8 */
        bis |= ExtractLSB( 0x0000010204081000ULL & occupation ); /*antidiag from f1/f8 */
        bis |= ExtractLSB( 0x0001020408102000ULL & occupation ); /*antidiag from g1/g8 */
        bis |= ExtractLSB( 0x0000000080402000ULL & occupation ); /*diag from e1/e8 */
        bis |= ExtractLSB( 0x0000000000804000ULL & occupation ); /*diag from f1/f8 */
        bis |= 0x0000000000008000ULL; /*diag from g1/g8 */
        if ( ! ( ( ( roo & ( Rooks( pos ) | Queens( pos ) ) ) | ( bis & ( Bishops( pos ) | Queens( pos ) ) ) | ( 0x0000000000F8DC00ULL & Knights( pos ) ) |
                   ( 0x000000000000F800ULL & Pawns( pos ) ) | ( Kings( pos ) & 0x0000000000004000ULL ) ) & opposing ) ) {
            /* check if e1/e8 f1/f8 g1/g8 are not attacked */
            pquiets->MoveType = KING | CASTLE;
            pquiets->From = 4;
            pquiets->To = 6;
            pquiets->Prom = EMPTY;
            pquiets++;
        }
    }
    return pquiets;
}

/* Generate all pseudo-legal capture and promotions */
static inline trp_chess_move_eval_t *GenerateCapture( trp_chess_board_tt *pos, trp_chess_move_eval_t *const capture )
{
    trp_chess_bb_t opposing,occupation;
    occupation = Occupation( pos );
    opposing = pos->PM ^ occupation;
    trp_chess_move_eval_t *pcapture = capture;
    trp_chess_bb_t pieces, destinations;
    trp_piece_type_t piece;

    for ( piece = KING ; piece >= KNIGHT ; piece-- ) { // generate moves from king to knight
        // generate moves for every piece of the same type of the side to move
        for ( pieces = BBPieces( pos, piece ) & pos->PM ; pieces ; pieces = ClearLSB( pieces ) ) {
            uns64b sq = LSB( pieces );
            // for every destinations on an opponent pieces generate a move
            for ( destinations = opposing & BBDestinations( piece, sq, occupation) ; destinations ; destinations = ClearLSB( destinations ) ) {
                pcapture->Move.MoveType = piece | CAPTURE;
                pcapture->Move.From = sq;
                pcapture->Move.To = LSB( destinations );
                pcapture->Move.Prom = EMPTY;
                pcapture->Eval = ( Piece( pos, LSB( destinations ) ) << 4 ) | ( KING - piece );
                pcapture++;
            }
        }
    }

    /* Generate pawns right captures */
    pieces = Pawns( pos ) & pos->PM;
    for ( destinations = ( pieces << 9 ) & 0x00FEFEFEFEFEFEFEULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
        pcapture->Move.MoveType = PAWN | CAPTURE;
        pcapture->Move.From = LSB( destinations ) - 9;
        pcapture->Move.To = LSB( destinations );
        pcapture->Move.Prom = EMPTY;
        pcapture->Eval = ( Piece( pos, LSB( destinations ) ) << 4 ) | ( KING - PAWN );
        pcapture++;
    }
    /* Generate pawns left captures */
    for ( destinations = ( pieces << 7 ) & 0x007F7F7F7F7F7F7FULL & opposing; destinations ; destinations = ClearLSB( destinations ) ) {
        pcapture->Move.MoveType = PAWN | CAPTURE;
        pcapture->Move.From = LSB( destinations ) - 7;
        pcapture->Move.To = LSB( destinations );
        pcapture->Move.Prom = EMPTY;
        pcapture->Eval = ( Piece( pos, LSB( destinations ) ) << 4 ) | ( KING - PAWN );
        pcapture++;
    }

    /* Generate pawns promotions */
    if ( pieces&0x00FF000000000000ULL ) {
        /* promotions with left capture */
        for ( destinations = ( pieces << 9 ) & 0xFE00000000000000ULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO | CAPTURE;
            move.From = LSB( destinations ) - 9;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move;
            pcapture->Eval = ( QUEEN << 4 ) | ( KING - PAWN );
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move;
                pcapture->Eval = piece - ROOK - 1; /* keep behind the other captures-promotions */
                pcapture++;
            }
        }
        /* promotions with right capture */
        for ( destinations = ( pieces << 7 ) & 0x7F00000000000000ULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO | CAPTURE;
            move.From = LSB( destinations ) - 7;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move;
            pcapture->Eval = ( QUEEN << 4 ) | ( KING - PAWN );
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move;
                pcapture->Eval = piece - ROOK - 1; /* keep behind the other captures-promotions */
                pcapture++;
            }
        }
        /* no capture promotions */
        for ( destinations = ( ( pieces << 8 ) & ~occupation ) & 0xFF00000000000000ULL ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO;
            move.From = LSB( destinations ) - 8;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move;
            pcapture->Eval = ( QUEEN << 4 ) | ( KING - PAWN );
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move;
                pcapture->Eval = piece - ROOK - 1; /* keep behind the other captures-promotions */
                pcapture++;
            }
        }
    }

    if ( pos->EnPassant != 8 ) {
        /* Generate EnPassant captures */
        for ( destinations = pieces & EnPassant[ pos->EnPassant ] ; destinations ; destinations = ClearLSB( destinations ) ) {
            pcapture->Move.MoveType = PAWN | EP | CAPTURE;
            pcapture->Move.From = LSB( destinations );
            pcapture->Move.To = 40 + pos->EnPassant;
            pcapture->Move.Prom = EMPTY;
            pcapture->Eval = ( PAWN << 4 ) | ( KING - PAWN );
            pcapture++;
        }
    }
    return pcapture;
}

/* Generate all pseudo-legal capture and promotions */
static inline trp_chess_move_tt *GenerateCapture2( trp_chess_board_tt *pos, trp_chess_move_tt *const capture )
{
    trp_chess_bb_t opposing,occupation;
    occupation = Occupation( pos );
    opposing = pos->PM ^ occupation;
    trp_chess_move_tt *pcapture = capture;
    trp_chess_bb_t pieces, destinations;
    trp_piece_type_t piece;

    for ( piece = KING ; piece >= KNIGHT ; piece-- ) { // generate moves from king to knight
        // generate moves for every piece of the same type of the side to move
        for ( pieces = BBPieces( pos, piece ) & pos->PM ; pieces ; pieces = ClearLSB( pieces ) ) {
            uns64b sq = LSB( pieces );
            // for every destinations on an opponent pieces generate a move
            for ( destinations = opposing & BBDestinations( piece, sq, occupation) ; destinations ; destinations = ClearLSB( destinations ) ) {
                pcapture->MoveType = piece | CAPTURE;
                pcapture->From = sq;
                pcapture->To = LSB( destinations );
                pcapture->Prom = EMPTY;
                pcapture++;
            }
        }
    }

    /* Generate pawns right captures */
    pieces = Pawns( pos ) & pos->PM;
    for ( destinations = ( pieces << 9 ) & 0x00FEFEFEFEFEFEFEULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
        pcapture->MoveType = PAWN | CAPTURE;
        pcapture->From = LSB( destinations ) - 9;
        pcapture->To = LSB( destinations );
        pcapture->Prom = EMPTY;
        pcapture++;
    }
    /* Generate pawns left captures */
    for ( destinations = ( pieces << 7 ) & 0x007F7F7F7F7F7F7FULL & opposing; destinations ; destinations = ClearLSB( destinations ) ) {
        pcapture->MoveType = PAWN | CAPTURE;
        pcapture->From = LSB( destinations ) - 7;
        pcapture->To = LSB( destinations );
        pcapture->Prom = EMPTY;
        pcapture++;
    }

    /* Generate pawns promotions */
    if ( pieces&0x00FF000000000000ULL ) {
        /* promotions with left capture */
        for ( destinations = ( pieces << 9 ) & 0xFE00000000000000ULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO | CAPTURE;
            move.From = LSB( destinations ) - 9;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move.Move;
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move.Move;
                pcapture++;
            }
        }
        /* promotions with right capture */
        for ( destinations = ( pieces << 7 ) & 0x7F00000000000000ULL & opposing ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO | CAPTURE;
            move.From = LSB( destinations ) - 7;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move.Move;
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move.Move;
                pcapture++;
            }
        }
        /* no capture promotions */
        for ( destinations = ( ( pieces << 8 ) & ~occupation ) & 0xFF00000000000000ULL ; destinations ; destinations = ClearLSB( destinations ) ) {
            trp_chess_move_tt move;
            move.MoveType = PAWN | PROMO;
            move.From = LSB( destinations ) - 8;
            move.To = LSB( destinations );
            move.Prom = QUEEN;
            pcapture->Move = move.Move;
            pcapture++;
            for ( piece = ROOK ; piece >= KNIGHT ; piece-- ) { /* generate underpromotions */
                move.Prom = piece;
                pcapture->Move = move.Move;
                pcapture++;
            }
        }
    }

    if ( pos->EnPassant != 8 ) {
        /* Generate EnPassant captures */
        for ( destinations = pieces & EnPassant[ pos->EnPassant ] ; destinations ; destinations = ClearLSB( destinations ) ) {
            pcapture->MoveType = PAWN | EP | CAPTURE;
            pcapture->From = LSB( destinations );
            pcapture->To = 40 + pos->EnPassant;
            pcapture->Prom = EMPTY;
            pcapture++;
        }
    }
    return pcapture;
}

/* Make the move */
static inline void trp_chess_make_move( trp_chess_board_tt *pos, trp_chess_move_tt move )
{
    trp_chess_bb_t part = 1ULL << move.From;
    trp_chess_bb_t dest = 1ULL << move.To;

    if ( pos->STM == BLACK )
        pos->MovCnt++;
    switch ( move.MoveType & 0x07 ) {
    case PAWN:
        if ( move.MoveType & EP ) {
            /* EnPassant */
            pos->PM ^= part | dest;
            pos->P0 ^= part | dest;
            pos->P0 ^= dest >> 8; /* delete the captured pawn */
            pos->EnPassant = 8;
        } else {
            if ( move.MoveType & CAPTURE ) {
                /* Delete the captured piece */
                pos->P0 &= ~dest;
                pos->P1 &= ~dest;
                pos->P2 &= ~dest;
            }
            if ( move.MoveType & PROMO ) {
                pos->PM ^= part | dest;
                pos->P0 ^= part;
                pos->P0 |= (trp_chess_bb_t)( move.Prom & 1 ) << move.To;
                pos->P1 |= (trp_chess_bb_t)( ( move.Prom >> 1 ) & 1 ) << move.To;
                pos->P2 |= (trp_chess_bb_t)( move.Prom >>2 ) << move.To;
                pos->EnPassant = 8; /* clear enpassant */
            } else { /* capture or push */
                pos->PM ^= part | dest;
                pos->P0 ^= part | dest;
                pos->EnPassant = 8; /* clear enpassant */
                if ( move.To == move.From + 16 && EnPassantM[ move.To & 0x07 ] & Pawns( pos ) & ( pos->PM ^ Occupation( pos ) ) )
                    pos->EnPassant = move.To & 0x07; /* save enpassant column */
            }
            if ( move.MoveType & CAPTURE ) {
                if ( CastleSO( pos ) && move.To == 63 )
                    ResetCastleSO( pos ); /* captured the opponent king side rook */
                else if (CastleLO( pos ) && move.To == 56 )
                    ResetCastleLO( pos ); /* captured the opponent quuen side rook */
            }
        }
        pos->Count50 = 0;
        ChangeSide( pos );
        break;
    case KNIGHT:
    case BISHOP:
    case ROOK:
    case QUEEN:
        if ( move.MoveType & CAPTURE ) {
            pos->P0 &= ~dest;
            pos->P1 &= ~dest;
            pos->P2 &= ~dest;
        }
        pos->PM ^= part | dest;
        pos->P0 ^= ( move.MoveType & 1 ) ? part | dest : 0;
        pos->P1 ^= ( move.MoveType & 2 ) ? part | dest : 0;
        pos->P2 ^= ( move.MoveType & 4 ) ? part | dest : 0;
        pos->EnPassant = 8;
        if ( ( move.MoveType&0x7 ) == ROOK ) { /* update the castle rights */
            if ( CastleSM( pos ) && move.From == 7 ) ResetCastleSM( pos );
            else if ( CastleLM( pos ) && move.From == 0 ) ResetCastleLM( pos );
        }
        if ( move.MoveType & CAPTURE ) { /* update the castle rights */
            if ( CastleSO( pos ) && move.To == 63 ) ResetCastleSO( pos );
            else if ( CastleLO( pos ) && move.To == 56 ) ResetCastleLO( pos );
            pos->Count50 = 0;
        }
        ChangeSide( pos );
        if ( move.MoveType & CAPTURE )
            pos->Count50 = 0;
        else
            pos->Count50++;
        break;
    case KING:
        if ( move.MoveType & CAPTURE ) {
            pos->P0 &= ~dest;
            pos->P1 &= ~dest;
            pos->P2 &= ~dest;
        }
        pos->PM ^= part | dest;
        pos->P1 ^= part | dest;
        pos->P2 ^= part | dest;
        if ( CastleSM( pos ) ) ResetCastleSM( pos ); /* update the castle rights */
        if ( CastleLM( pos ) ) ResetCastleLM( pos );
        pos->EnPassant = 8;
        if (move.MoveType & CAPTURE) {
            if ( CastleSO( pos ) && move.To == 63 ) ResetCastleSO( pos );
            else if ( CastleLO( pos ) && move.To == 56 ) ResetCastleLO( pos );
            pos->Count50 = 0;
        } else if ( move.MoveType & CASTLE ) {
            if ( move.To == 6 ) { pos->PM ^= 0x00000000000000A0ULL; pos->P2 ^= 0x00000000000000A0ULL; } /* short castling */
            else { pos->P2 ^= 0x0000000000000009ULL; pos->PM ^= 0x0000000000000009ULL; } /* long castling */
        }
        ChangeSide( pos );
        if ( move.MoveType & CAPTURE )
            pos->Count50 = 0;
        else
            pos->Count50++;
        break;
    default:
        break;
    }
}

/* If the king is in check this function return the pieces that are attacking the king. If there aren't it returns 0 */
static inline trp_chess_bb_t InCheck( trp_chess_board_tt *pos )
{
    trp_chess_bb_t occ, king, queens;
    uns64b kingsq;

    occ = Occupation( pos );
    king = Kings( pos ) & pos->PM;
    queens = Queens( pos );
    kingsq = LSB( king );
    return ( ( ( KnightDest[ kingsq ] & Knights( pos ) ) |
               ( GenRook ( kingsq, occ ) & ( Rooks( pos ) | queens ) ) |
               ( GenBishop ( kingsq, occ ) & ( Bishops( pos ) | queens ) ) |
               ( ( ( ( king << 9 ) & 0xFEFEFEFEFEFEFEFEULL ) | ( ( king << 7 ) & 0x7F7F7F7F7F7F7F7FULL ) ) & Pawns( pos ) ) |
               ( KingDest[ kingsq ] & Kings( pos ) ) )
             & ( occ ^ pos->PM ) );
}

static int trp_chess_moves_count( trp_chess_board_tt *pos )
{
    trp_chess_move_tt moves[ 256 ], *pmoves;
    int res = 0;

    for ( pmoves = GenerateQuiets( pos, GenerateCapture2( pos, moves ) ) ; pmoves > moves ; ) {
        pmoves--;
        if ( !Illegal( pos, *pmoves ) )
            res++;
    }
    return res;
}

static trp_obj_t *trp_chess_move( trp_chess_board_tt *pos, trp_chess_move_tt *move )
{
    uns16b to = move->To;
    uns8b from = move->From;

    if ( pos->STM ) {
        from = OppSq( from );
        to = OppSq( to );
    }
    if ( move->Prom != EMPTY )
        to |= ( move->Prom - PAWN ) << 6;
    return trp_cons( trp_sig64( from ), trp_sig64( to ) );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

uns8b trp_chess_init()
{
    extern objfun_t _trp_nth_fun[];
    extern objfun_t _trp_equal_fun[];
    extern uns32bfun_t _trp_size_fun[];
    extern voidfun_t _trp_encode_fun[];
    extern objfun_t _trp_decode_fun[];

    _trp_nth_fun[ TRP_CHESS ] = trp_chess_nth;
    _trp_equal_fun[ TRP_CHESS ] = trp_chess_equal;
    _trp_size_fun[ TRP_CHESS ] = trp_chess_size;
    _trp_encode_fun[ TRP_CHESS ] = trp_chess_encode;
    _trp_decode_fun[ TRP_CHESS ] = trp_chess_decode;
    return 0;
}

static trp_obj_t *trp_chess_nth( uns32b n, trp_chess_t *obj )
{
    uns8b p;

    if ( n > 63 )
        return UNDEF;
    p = obj->board->board[ n ];
    return trp_sig64( trp_chess_piece_pack( p ) );
}

static trp_obj_t *trp_chess_equal( trp_chess_t *st1, trp_chess_t *st2 )
{
    return memcmp( st1->board, st2->board, sizeof( trp_chess_board_t ) ) ? TRP_FALSE : TRP_TRUE;
}

static uns32b trp_chess_size( trp_chess_t *st )
{
    return 1 + 38;
}

static void trp_chess_encode( trp_chess_t *st, uns8b **buf )
{
    uns16b *p;
    uns8b n, p1, p2;

    **buf = TRP_CHESS;
    ++(*buf);
    for ( n = 0 ; n < 64 ; ) {
        p1 = st->board->board[ n++ ];
        p2 = st->board->board[ n++ ];
        **buf = ( ( trp_chess_piece_pack( p1 ) << 4 ) | trp_chess_piece_pack( p2 ) );
        ++(*buf);
    }
    **buf = st->board->flags;
    ++(*buf);
    **buf = st->board->enp;
    ++(*buf);
    p = (uns16b *)(*buf);
    *p = norm16( st->board->ply );
    (*buf) += 2;
    p = (uns16b *)(*buf);
    *p = norm16( st->board->mov );
    (*buf) += 2;
}

static trp_obj_t *trp_chess_decode( uns8b **buf )
{
    trp_chess_board_t *board;
    trp_chess_t *st;
    uns8b n, p, q, pn, qn;

    board = trp_gc_malloc_atomic( sizeof( trp_chess_board_t ) );
    board->board[ 64 ] = 32;
    st = trp_gc_malloc( sizeof( trp_chess_t ) );
    st->tipo = TRP_CHESS;
    st->check = -1;
    st->board = board;
    st->domin = NULL;
    st->moves = NULL;
    st->pw = 0;
    st->pb = 0;
    for ( n = 0 ; n < 64 ; ) {
        p = **buf;
        ++(*buf);
        q = ( p & 15 );
        p >>= 4;
        pn = n;
        board->board[ n++ ] = trp_chess_piece_unpack( p );
        qn = n;
        board->board[ n++ ] = trp_chess_piece_unpack( q );
        if ( p < 12 )
            if ( p < 6 )
                if ( p == 5 )
                    st->kw = pn;
                else
                    st->pw++;
            else
                if ( p == 11 )
                    st->kb = pn;
                else
                    st->pb++;
        if ( q < 12 )
            if ( q < 6 )
                if ( q == 5 )
                    st->kw = qn;
                else
                    st->pw++;
            else
                if ( q == 11 )
                    st->kb = qn;
                else
                    st->pb++;
    }
    board->flags = **buf;
    ++(*buf);
    board->enp = **buf;
    ++(*buf);
    board->ply = norm16( *((uns16b *)(*buf)) );
    (*buf) += 2;
    board->mov = norm16( *((uns16b *)(*buf)) );
    (*buf) += 2;
    return (trp_obj_t *)st;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_eq( trp_obj_t *st1, trp_obj_t *st2 )
{
    if ( ( st1->tipo != TRP_CHESS ) || ( st2->tipo != TRP_CHESS ) )
        return TRP_FALSE;
    if ( ((trp_chess_t *)st1)->board->flags != ((trp_chess_t *)st2)->board->flags )
        return TRP_FALSE;
    if ( ((trp_chess_t *)st1)->board->enp != ((trp_chess_t *)st2)->board->enp )
        return TRP_FALSE;
    if ( memcmp( ((trp_chess_t *)st1)->board->board, ((trp_chess_t *)st2)->board->board, 64 ) )
        return TRP_FALSE;
    return TRP_TRUE;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static trp_chess_domin_t *trp_chess_dominance_low( trp_chess_t *st )
{
    if ( st->domin == NULL ) {
        trp_chess_board_tt pos;
        trp_chess_bb_t occ, pieces, dst;
        trp_piece_type_t piece;
        uns8b i, color, oppsq;

        trp_tmp_old_to_new( st, &pos );
        st->domin = trp_gc_malloc_atomic( sizeof( trp_chess_domin_t ) );
        memset( st->domin, 0, sizeof( trp_chess_domin_t ) );
        for ( i = 0 ; ; i++ ) {
            color = pos.STM >> 3;
            oppsq = color ? 0x38 : 0;
            occ = Occupation( &pos );
            for ( piece = KING ; piece >= KNIGHT ; piece-- )
                for ( pieces = BBPieces( &pos, piece ) & pos.PM ; pieces ; pieces = ClearLSB( pieces ) )
                    for ( dst = BBDestinations( piece, LSB( pieces ), occ ) ; dst ; dst = ClearLSB( dst ) )
                        st->domin->domin[ color ][ LSB( dst ) ^ oppsq ]++;
            pieces = Pawns( &pos ) & pos.PM;
            for ( dst = ( pieces << 9 ) & 0xFEFEFEFEFEFEFEFEULL ; dst ; dst = ClearLSB( dst ) )
                st->domin->domin[ color ][ LSB( dst ) ^ oppsq ]++;
            for ( dst = ( pieces << 7 ) & 0x7F7F7F7F7F7F7F7FULL ; dst ; dst = ClearLSB( dst ) )
                st->domin->domin[ color ][ LSB( dst ) ^ oppsq ]++;
            if ( i == 1 )
                break;
            ChangeSide( &pos );
        }
    }
    return st->domin;
}

trp_obj_t *trp_chess_dominance( trp_obj_t *st, trp_obj_t *idx, trp_obj_t *color )
{
    uns32b iidx, ccolor;

    if ( ( st->tipo != TRP_CHESS ) ||
         trp_cast_uns32b_range( idx, &iidx, 0, 63 ) ||
         trp_cast_uns32b_range( color, &ccolor, 0, 1 ) )
        return UNDEF;
    return trp_sig64( trp_chess_dominance_low( (trp_chess_t *)st )->domin[ ccolor ][ iidx ] );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static int trp_chess_dominated_low( trp_chess_t *st, uns8b idx, uns8b color )
{
    trp_chess_board_tt pos;
    trp_chess_bb_t piece, occ, queens;

    if ( st->domin )
        return st->domin->domin[ color ][ idx ] ? 1 : 0;
    trp_tmp_old_to_new( st, &pos );
    if ( pos.STM )
        idx = OppSq( idx );
    if ( color == trp_chess_color_m( st ) ) {
        ChangeSide( &pos );
        idx = OppSq( idx );
    }
    piece = 1ULL << idx;
    occ = Occupation( &pos ) | piece;
    queens = Queens( &pos );
    return ( ( ( KnightDest[ idx ] & Knights( &pos ) ) |
               ( GenRook ( idx, occ ) & ( Rooks( &pos ) | queens ) ) |
               ( GenBishop ( idx, occ ) & ( Bishops( &pos ) | queens ) ) |
               ( ( ( ( piece << 9 ) & 0xFEFEFEFEFEFEFEFEULL ) | ( ( piece << 7 ) & 0x7F7F7F7F7F7F7F7FULL ) ) & Pawns( &pos ) ) |
               ( KingDest[ idx ] & Kings( &pos ) ) )
             & ( occ ^ pos.PM ) ) ? 1 : 0;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static int trp_chess_dominated_not_p_low( trp_chess_t *st, uns8b idx, uns8b color )
{
    trp_chess_board_tt pos;
    trp_chess_bb_t occ, queens;

    trp_tmp_old_to_new( st, &pos );
    if ( pos.STM )
        idx = OppSq( idx );
    if ( color == trp_chess_color_m( st ) ) {
        ChangeSide( &pos );
        idx = OppSq( idx );
    }
    occ = Occupation( &pos ) | ( 1ULL << idx );
    queens = Queens( &pos );
    return ( ( ( KnightDest[ idx ] & Knights( &pos ) ) |
               ( GenRook ( idx, occ ) & ( Rooks( &pos ) | queens ) ) |
               ( GenBishop ( idx, occ ) & ( Bishops( &pos ) | queens ) ) |
               ( KingDest[ idx ] & Kings( &pos ) ) )
             & ( occ ^ pos.PM ) ) ? 1 : 0;
}

trp_obj_t *trp_chess_dominated_not_p( trp_obj_t *st, trp_obj_t *idx, trp_obj_t *color )
{
    uns32b iidx, ccolor;

    if ( ( st->tipo != TRP_CHESS ) ||
         trp_cast_uns32b_range( idx, &iidx, 0, 63 ) ||
         trp_cast_uns32b_range( color, &ccolor, 0, 1 ) )
        return UNDEF;
    return trp_chess_dominated_not_p_low( (trp_chess_t *)st, (uns8b)iidx, (uns8b)ccolor ) ? TRP_TRUE : TRP_FALSE;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static trp_chess_t *trp_chess_clone_low( trp_chess_t *st )
{
    trp_chess_t *newst;

    newst = trp_gc_malloc( sizeof( trp_chess_t ) );
    newst->tipo = TRP_CHESS;
    newst->check = st->check;
    newst->pieces = st->pieces;
    newst->board = trp_gc_malloc_atomic( sizeof( trp_chess_board_t ) );
    newst->domin = st->domin;
    newst->moves = st->moves;
    memcpy( newst->board, st->board, sizeof( trp_chess_board_t ) );
    return newst;
}

static trp_obj_t *trp_chess_clone( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    (void)trp_chess_dominance_low( (trp_chess_t *)st );
    (void)trp_chess_check_low( (trp_chess_t *)st );
    (void)trp_chess_legal_moves_low( (trp_chess_t *)st );
    return (trp_obj_t *)trp_chess_clone_low( (trp_chess_t *)st );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static trp_obj_t *trp_chess_legal_moves_low( trp_chess_t *st )
{
    if ( st->moves == NULL ) {
        trp_obj_t *movel = NIL;
        trp_chess_board_tt pos;
        trp_chess_move_tt moves[ 256 ], *pmoves;
        uns16b to;
        uns8b from;

        trp_tmp_old_to_new( st, &pos );
        for ( pmoves = GenerateQuiets( &pos, GenerateCapture2( &pos, moves ) ) ; pmoves > moves ; ) {
            pmoves--;
            if ( !Illegal( &pos, *pmoves ) )
                movel = trp_cons( trp_chess_move( &pos, pmoves ), movel );
        }
        st->moves = movel;
    }
    return st->moves;
}

trp_obj_t *trp_chess_legal_moves( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_legal_moves_low( (trp_chess_t *)st );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static uns8b trp_chess_check_low( trp_chess_t *st )
{
    if ( st->check < 0 ) {
        trp_chess_board_tt pos;

        trp_tmp_old_to_new( st, &pos );
        st->check = InCheck( &pos ) ? 1 : 0;
    }
    return (uns8b)( st->check );
}

trp_obj_t *trp_chess_check( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_check_low( (trp_chess_t *)st ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_checkmate( trp_obj_t *st )
{
    trp_obj_t *o;

    if ( ( o = trp_chess_check( st ) ) != TRP_TRUE )
        return o;
    return ( trp_chess_legal_moves_low( (trp_chess_t *)st ) == NIL ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_stalemate( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    if ( trp_chess_check_low( (trp_chess_t *)st ) )
        return TRP_FALSE;
    return ( trp_chess_legal_moves_low( (trp_chess_t *)st ) == NIL ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_double_check( trp_obj_t *st )
{
    trp_chess_board_tt pos;

    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    trp_tmp_old_to_new( (trp_chess_t *)st, &pos );
    return ( PopCount( InCheck( &pos ) ) < 2 ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_chess_draw_by_insufficient_material( trp_obj_t *st )
{
    uns8b pw, pb;

    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    pw = ((trp_chess_t *)st)->pw + 1;
    pb = ((trp_chess_t *)st)->pb + 1;
    if ( ( pw > 1 ) && ( pb > 1 ) ) {
        if ( ( pw == 2 ) && ( pb == 2 ) ) {
            uns8b i, s = 2, t;

            for ( i = 0 ; i < 64 ; i++ ) {
                t = ((trp_chess_t *)st)->board->board[ i ];
                if ( ( t == 2 ) || ( t == 10 ) ) {
                    t = ( ( i & 7 ) + ( i >> 3 ) ) & 1;
                    if ( t == s ) /* solo due alfieri dello stesso tipo */
                        return TRP_TRUE;
                    if ( s != 2 )
                        return TRP_FALSE;
                    s = t;
                }
            }
        }
        return TRP_FALSE;
    }
    /* ( pw == 1 ) || ( pb == 1 ) */
    if ( ( pw == 1 ) && ( pb == 1 ) )
        return TRP_TRUE;
    if ( ( pw == 2 ) || ( pb == 2 ) ) {
        uns8b i, t;

        for ( i = 0 ; i < 64 ; i++ ) {
            t = ((trp_chess_t *)st)->board->board[ i ];
            if ( ( t == 1 ) || ( t == 2 ) || ( t == 9 ) || ( t == 10 ) )
                return TRP_TRUE;
        }
    }
    return TRP_FALSE;
}

/*
 con minaccia di matto intendo una posizione in cui,
 se potesse muovere il giocatore che ha appena mosso,
 potrebbe mattare
 */

trp_obj_t *trp_chess_threat_of_checkmate( trp_obj_t *st )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt winning_move;
    sig64b positions;
    uns32b actmoves;
    uns8b interr;

    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    /*
     matto o stallo non sono minacce di matto
     */
    if ( trp_chess_legal_moves_low( (trp_chess_t *)st ) == NIL )
        return TRP_FALSE;
    /*
     uno scacco doppio non è minaccia di matto
     */
    interr = trp_chess_color_m( st );
    if ( trp_chess_dominance_low( (trp_chess_t *)st )->domin[ 1 - interr ][ interr ? ((trp_chess_t *)st)->kb : ((trp_chess_t *)st)->kw ] >= 2 )
        return TRP_FALSE;
    trp_tmp_old_to_new( (trp_chess_t *)st, &pos );
    ChangeSide( &pos );
    pos.EnPassant = 8;
    positions = 0;
    interr = 0;
    return trp_chess_search_mate_low( &pos, 1, NULL, &winning_move, &actmoves, &positions, &interr ) ? TRP_FALSE : TRP_TRUE;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static trp_obj_t *trp_chess_st_fen_low( trp_obj_t *st, uns8b extended )
{
    uns32b len, i, j;
    CORD_ec x;
    uns8b k;

    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    CORD_ec_init( x );
    len = 0;
    for ( i = 56 ; ;  i -= 8) {
        for ( j = 0 ; j < 8 ; ) {
            k = ((trp_chess_t *)st)->board->board[ i + j ];
            k = trp_chess_piece_pack( k );
            if ( k < 12 ) {
                k = "PNBRQKpnbrqk"[ k ];
                j++;
            } else {
                for ( k = '1' ; ; k++ ) {
                    if ( ++j == 8 )
                        break;
                    if ( ((trp_chess_t *)st)->board->board[ i + j ] != 16 )
                        break;
                }
            }
            CORD_ec_append( x, k );
            len++;
        }
        if ( i == 0 )
            break;
        CORD_ec_append( x, '/' );
        len++;
    }
    CORD_ec_append( x, ' ' );
    CORD_ec_append( x, "wb"[ trp_chess_color_m(st) ] );
    CORD_ec_append( x, ' ' );
    len += 3;
    if ( ((trp_chess_t *)st)->board->flags & 30 ) {
        if ( trp_chess_arrcw_m( st ) ) {
            CORD_ec_append( x, 'K' );
            len++;
        }
        if ( trp_chess_arrlw_m( st ) ) {
            CORD_ec_append( x, 'Q' );
            len++;
        }
        if ( trp_chess_arrcb_m( st ) ) {
            CORD_ec_append( x, 'k' );
            len++;
        }
        if ( trp_chess_arrlb_m( st ) ) {
            CORD_ec_append( x, 'q' );
            len++;
        }
    } else {
        CORD_ec_append( x, '-' );
        len++;
    }
    CORD_ec_append( x, ' ' );
    len++;
    if ( ((trp_chess_t *)st)->board->enp == 64 ) {
        CORD_ec_append( x, '-' );
        len++;
    } else {
        k = ((trp_chess_t *)st)->board->enp;
        CORD_ec_append( x, 'a' + ( k & 7 ) );
        CORD_ec_append( x, '1' + ( k >> 3 ) );
        len += 2;
    }
    if ( extended ) {
        CORD_ec_append( x, ' ' );
        i = ((trp_chess_t *)st)->board->ply;
        for ( j = 10, k = 1 ; ( i / j ) > 0 ; j *= 10, k++ );
        len += k;
        for ( ; k ; k-- ) {
            j /= 10;
            CORD_ec_append( x, '0' + ( i / j ) % 10 );
        }
        CORD_ec_append( x, ' ' );
        i = ((trp_chess_t *)st)->board->mov;
        for ( j = 10, k = 1 ; ( i / j ) > 0 ; j *= 10, k++ );
        len += k;
        for ( ; k ; k-- ) {
            j /= 10;
            CORD_ec_append( x, '0' + ( i / j ) % 10 );
        }
        len += 2;
    }
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

trp_obj_t *trp_chess_st_fen( trp_obj_t *st )
{
    return trp_chess_st_fen_low( st, 1 );
}

trp_obj_t *trp_chess_st_fen_short( trp_obj_t *st )
{
    return trp_chess_st_fen_low( st, 0 );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_fen_st( trp_obj_t *fen )
{
    trp_chess_board_t *board;
    trp_chess_t *st;
    CORD_pos i;
    sig16b ply, mov, acc;
    sig8b p, color, kw, kb, arrcw, arrlw, arrcb, arrlb;
    uns8b enp1, enp2, c, err, x, pw, pb;

    if ( fen->tipo != TRP_CORD )
        return UNDEF;
    board = trp_gc_malloc_atomic( sizeof( trp_chess_board_t ) );
    board->board[ 64 ] = 32;
    p = 56;
    color = -1;
    kw = -1;
    kb = -1;
    arrcw = -1;
    arrlw = -1;
    arrcb = -1;
    arrlb = -1;
    enp1 = 64;
    enp2 = 64;
    err = 0;
    x = 0;
    pw = 0;
    pb = 0;
    ply = -1;
    mov = -1;
    acc = -1;
    CORD_FOR( i, ((trp_cord_t *)fen)->c ) {
        if ( err )
            break;
        c = CORD_pos_fetch( i );
        if ( ( p == 56 ) && ( x == 0 ) && ( ( c == ' ' ) || ( c == '\t' ) || ( c == '[' ) ) )
            continue;
        if ( p >= 0 ) {
            if ( x == 8 ) {
                p -= 8;
                x = 0;
                if ( ( ( p < 0 ) && ( c != ' ' ) && ( c != '\t' ) ) ||
                     ( ( p >= 0 ) && ( c != '/' ) ) ) {
                    err = 1;
                    break;
                }
                continue;
            }
            if ( ( c >= '1' ) && ( c <= '8' ) ) {
                c -= '0';
                if ( x + c > 8 ) {
                    err = 1;
                    break;
                }
                do {
                    board->board[ p + x ] = 16;
                    x++;
                } while ( --c );
                continue;
            }
            switch ( c ) {
            case 'P':
                board->board[ p + x ] = 0;
                x++;
                pw++;
                break;
            case 'N':
                board->board[ p + x ] = 1;
                x++;
                pw++;
                break;
            case 'B':
                board->board[ p + x ] = 2;
                x++;
                pw++;
                break;
            case 'R':
                board->board[ p + x ] = 3;
                x++;
                pw++;
                break;
            case 'Q':
                board->board[ p + x ] = 4;
                x++;
                pw++;
                break;
            case 'K':
                if ( kw >= 0 ) {
                    err = 1;
                    break;
                }
                kw = p + x;
                board->board[ kw ] = 5;
                x++;
                break;
            case 'p':
                board->board[ p + x ] = 8;
                x++;
                pb++;
                break;
            case 'n':
                board->board[ p + x ] = 9;
                x++;
                pb++;
                break;
            case 'b':
                board->board[ p + x ] = 10;
                x++;
                pb++;
                break;
            case 'r':
                board->board[ p + x ] = 11;
                x++;
                pb++;
                break;
            case 'q':
                board->board[ p + x ] = 12;
                x++;
                pb++;
                break;
            case 'k':
                if ( kb >= 0 ) {
                    err = 1;
                    break;
                }
                kb = p + x;
                board->board[ kb ] = 13;
                x++;
                break;
            default:
                err = 1;
                break;
            }
            continue;
        }
        if ( color < 0 ) {
            switch ( c ) {
            case ' ':
            case '\t':
            case ']':
                break;
            case 'w':
                color = 0;
                break;
            case 'b':
                color = 1;
                break;
            default:
                err = 1;
                break;
            }
            continue;
        }
        if ( ( arrcw < 0 ) || ( arrlw < 0 ) || ( arrcb < 0 ) || ( arrlb < 0 ) ) {
            switch ( c ) {
            case ' ':
            case '\t':
            case ']':
                if ( ( arrcw >= 0 ) || ( arrlw >= 0 ) || ( arrcb >= 0 ) || ( arrlb >= 0 ) ) {
                    if ( arrcw < 0 )
                        arrcw = 0;
                    if ( arrlw < 0 )
                        arrlw = 0;
                    if ( arrcb < 0 )
                        arrcb = 0;
                    if ( arrlb < 0 )
                        arrlb = 0;
                }
                break;
            case '-':
                if ( ( arrcw < 0 ) && ( arrlw < 0 ) && ( arrcb < 0 ) && ( arrlb < 0 ) ) {
                    arrcw = 0;
                    arrlw = 0;
                    arrcb = 0;
                    arrlb = 0;
                } else
                    err = 1;
                break;
            case 'K':
                arrcw = 2;
                break;
            case 'Q':
                arrlw = 4;
                break;
            case 'k':
                arrcb = 8;
                break;
            case 'q':
                arrlb = 16;
                break;
            default:
                err = 1;
                break;
            }
            continue;
        }
        if ( ( enp1 == 64 ) || ( enp2 == 64 ) ) {
            switch ( c ) {
            case ' ':
            case '\t':
            case ']':
                if ( ( enp1 < 64 ) || ( enp2 < 64 ) )
                    err = 1;
                break;
            case '-':
                if ( ( enp1 == 64 ) && ( enp2 == 64 ) ) {
                    enp1 = 8;
                    enp2 = 8;
                } else
                    err = 1;
                break;
            default:
                if ( enp1 == 64 )
                    if ( ( c >= 'a' ) && ( c <= 'h' ) )
                        enp1 = c - 'a';
                    else
                        err = 1;
                else
                    if ( ( c >= '1' ) && ( c <= '8' ) )
                        enp2 = c - '1';
                    else
                        err = 1;
                break;
            }
            continue;
        }
        if ( ply < 0 ) {
            if ( ( c == ' ' ) || ( c == '\t' ) || ( c == ']' ) ) {
                if ( acc >= 0 ) {
                    ply = acc;
                    acc = -1;
                }
            } else if ( ( c >= '0' ) && ( c <= '9' ) ) {
                if ( acc < 0 )
                    acc = 0;
                else
                    acc = 10 * acc;
                acc += ( c - '0' );
            } else
                err = 1;
            continue;
        }
        if ( mov < 0 ) {
            if ( ( c == ' ' ) || ( c == '\t' ) || ( c == ']' ) ) {
                if ( acc >= 0 ) {
                    mov = acc;
                    acc = -1;
                }
            } else if ( ( c >= '0' ) && ( c <= '9' ) ) {
                if ( acc < 0 )
                    acc = 0;
                else
                    acc = 10 * acc;
                acc += ( c - '0' );
            } else
                err = 1;
            continue;
        }
        if ( ( c != ' ' ) && ( c != '\t' ) && ( c != ']' ) )
            err = 1;
    }
    if ( acc >= 0 )
        mov = acc;
    if ( ply < 0 )
        ply = 0;
    if ( mov < 0 )
        mov = 1;
    if ( ( arrcw < 0 ) || ( arrlw < 0 ) || ( arrcb < 0 ) || ( arrlb < 0 ) ) {
        if ( ( arrcw < 0 ) && ( arrlw < 0 ) && ( arrcb < 0 ) && ( arrlb < 0 ) ) {
            arrcw = 0;
            arrlw = 0;
            arrcb = 0;
            arrlb = 0;
        } else
            err = 1;
    }
    if ( err || ( kw < 0 ) || ( kb < 0 ) || ( color < 0 ) || ( ( enp1 < 64 ) && ( enp2 == 64 ) ) || ( mov < 1 ) ) {
        trp_gc_free( board );
        return UNDEF;
    }
    board->flags = ( ((uns8b)color) | ((uns8b)arrcw) | ((uns8b)arrlw) | ((uns8b)arrcb) | ((uns8b)arrlb) );
    board->enp = ( ( enp1 == 8 ) || ( enp1 == 64 ) ) ? 64 : enp1 + 8 * enp2;
    board->ply = ply;
    board->mov = mov;
    st = trp_gc_malloc( sizeof( trp_chess_t ) );
    st->tipo = TRP_CHESS;
    st->pw = pw;
    st->pb = pb;
    st->kw = kw;
    st->kb = kb;
    st->check = -1;
    st->board = board;
    st->domin = NULL;
    st->moves = NULL;
    if ( trp_chess_dominated_low( st, color ? kw : kb, color ) ) {
        /*
         il re del colore che non muove non può essere sotto scacco
         */
        trp_gc_free( st );
        trp_gc_free( board );
        return UNDEF;
    }
    return (trp_obj_t *)st;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_st_raw_short( trp_obj_t *st )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    trp_obj_t *raw;
    uns8b *d, n, p, q;

    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    if ( ( raw = trp_raw_internal( 34, 0 ) ) == UNDEF )
        return UNDEF;
    for ( n = 0, d = ((trp_raw_t *)raw)->data ; n < 64 ; ) {
        p = ((trp_chess_t *)st)->board->board[ n++ ];
        q = ((trp_chess_t *)st)->board->board[ n++ ];
        *d++ = ( trp_chess_piece_pack( p ) << 4 ) | trp_chess_piece_pack( q );
    }
    *d++ = ((trp_chess_t *)st)->board->flags;
    *d = ((trp_chess_t *)st)->board->enp;
    return raw;
}

trp_obj_t *trp_chess_raw_short_st( trp_obj_t *raw )
{
    trp_chess_board_t *board;
    trp_chess_t *st;
    uns8b *d, n, p, q, pn, qn;

    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    if ( ((trp_raw_t *)raw)->len != 34 )
        return UNDEF;
    board = trp_gc_malloc_atomic( sizeof( trp_chess_board_t ) );
    board->board[ 64 ] = 32;
    st = trp_gc_malloc( sizeof( trp_chess_t ) );
    st->tipo = TRP_CHESS;
    st->check = -1;
    st->board = board;
    st->domin = NULL;
    st->moves = NULL;
    st->pw = 0;
    st->pb = 0;
    for ( n = 0, d = ((trp_raw_t *)raw)->data ; n < 64 ; ) {
        p = *d++;
        q = ( p & 15 );
        p >>= 4;
        pn = n;
        board->board[ n++ ] = trp_chess_piece_unpack( p );
        qn = n;
        board->board[ n++ ] = trp_chess_piece_unpack( q );
        if ( p < 12 )
            if ( p < 6 )
                if ( p == 5 )
                    st->kw = pn;
                else
                    st->pw++;
            else
                if ( p == 11 )
                    st->kb = pn;
                else
                    st->pb++;
        if ( q < 12 )
            if ( q < 6 )
                if ( q == 5 )
                    st->kw = qn;
                else
                    st->pw++;
            else
                if ( q == 11 )
                    st->kb = qn;
                else
                    st->pb++;
    }
    board->flags = *d++;
    board->enp = *d;
    board->ply = 0;
    board->mov = 1;
    return (trp_obj_t *)st;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_start()
{
    static trp_obj_t *st_start = NULL;

    if ( st_start == NULL )
        st_start = trp_chess_fen_st( trp_cord( "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" ) );
    return trp_chess_clone( st_start );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_color( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( trp_chess_color_m( st ) );
}

trp_obj_t *trp_chess_arrcw( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_arrcw_m( st ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_arrlw( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_arrlw_m( st ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_arrcb( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_arrcb_m( st ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_arrlb( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_chess_arrlb_m( st ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_enp( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return ( ((trp_chess_t *)st)->board->enp == 64 ) ? UNDEF : trp_sig64( ((trp_chess_t *)st)->board->enp );
}

trp_obj_t *trp_chess_ply( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( ((trp_chess_t *)st)->board->ply );
}

trp_obj_t *trp_chess_mov( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( ((trp_chess_t *)st)->board->mov );
}

trp_obj_t *trp_chess_white_pieces( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( ((trp_chess_t *)st)->pw + 1 );
}

trp_obj_t *trp_chess_black_pieces( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( ((trp_chess_t *)st)->pb + 1 );
}

trp_obj_t *trp_chess_pieces( trp_obj_t *st )
{
    if ( st->tipo != TRP_CHESS )
        return UNDEF;
    return trp_sig64( ((trp_chess_t *)st)->pw + ((trp_chess_t *)st)->pb + 2 );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_zero_moves( trp_obj_t *st )
{
    if ( ( st = trp_chess_legal_moves( st ) ) == UNDEF )
        return UNDEF;
    return ( st == NIL ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_one_move_only( trp_obj_t *st )
{
    if ( ( st = trp_chess_legal_moves( st ) ) == UNDEF )
        return UNDEF;
    if ( st == NIL )
        return TRP_FALSE;
    return ( trp_cdr( st ) == NIL ) ? TRP_TRUE : TRP_FALSE;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static void trp_chess_next_low( trp_chess_t *st, uns8b idx1, uns16b idx2 )
{
    uns8b color, p1, p2, promozione, cattura, pedone = 0, enp = 64, d;

    color = trp_chess_color_m( st );
    promozione = idx2 >> 6;
    idx2 = idx2 & 63;
    p1 = st->board->board[ idx1 ];
    p2 = st->board->board[ idx2 ];
    cattura = ( p2 < 16 ) ? idx2 : 64;
    st->board->board[ idx1 ] = 16;
    st->board->board[ idx2 ] = p1;
    if ( color ) {
        switch ( p1 ) {
        case 8:
            d = idx1 - idx2;
            pedone = 1;
            if ( d == 16 ) {
                uns8b col = idx1 & 7;
                /*
                 settiamo enp solo se c'è un pedone adiacente che effettivamente
                 può catturare questo alla prossima mossa; in tal modo si ottengono
                 più posizioni equivalenti...
                 */
                if ( col > 0 )
                    if ( st->board->board[ idx2 - 1 ] == 0 )
                        enp = ( idx1 + idx2 ) >> 1;
                if ( col < 7 )
                    if ( st->board->board[ idx2 + 1 ] == 0 )
                        enp = ( idx1 + idx2 ) >> 1;
            } else if ( ( d != 8 ) && ( cattura == 64 ) ) {
                /* è una presa al varco */
                cattura = ( idx1 & 248 ) | ( idx2 & 7 );
                st->board->board[ cattura ] = 16;
            } else
                st->board->board[ idx2 ] += promozione;
            break;
        case 11:
            if ( idx1 == 56 )
                st->board->flags &= 239;
            else if ( idx1 == 63 )
                st->board->flags &= 247;
            break;
        case 13:
            st->kb = idx2;
            st->board->flags &= 231;
            if ( idx1 == 60 )
                if ( idx2 == 62 ) {
                    st->board->board[ 61 ] = 11;
                    st->board->board[ 63 ] = 16;
                } else if ( idx2 == 58 ) {
                    st->board->board[ 59 ] = 11;
                    st->board->board[ 56 ] = 16;
                }
            break;
        default:
            break;
        }
        st->board->flags &= 254;
        if ( cattura < 64 )
            st->pw--;
        st->board->mov++;
    } else {
        switch ( p1 ) {
        case 0:
            d = idx2 - idx1;
            pedone = 1;
            if ( d == 16 ) {
                uns8b col = idx1 & 7;
                /*
                 settiamo enp solo se c'è un pedone adiacente che effettivamente
                 può catturare questo alla prossima mossa; in tal modo si ottengono
                 più posizioni equivalenti...
                 */
                if ( col > 0 )
                    if ( st->board->board[ idx2 - 1 ] == 8 )
                        enp = ( idx1 + idx2 ) >> 1;
                if ( col < 7 )
                    if ( st->board->board[ idx2 + 1 ] == 8 )
                        enp = ( idx1 + idx2 ) >> 1;
            } else if ( ( d != 8 ) && ( cattura == 64 ) ) {
                /* è una presa al varco */
                cattura = ( idx1 & 248 ) | ( idx2 & 7 );
                st->board->board[ cattura ] = 16;
            } else
                st->board->board[ idx2 ] += promozione;
            break;
        case 3:
            if ( idx1 == 0 )
                st->board->flags &= 251;
            else if ( idx1 == 7 )
                st->board->flags &= 253;
            break;
        case 5:
            st->kw = idx2;
            st->board->flags &= 249;
            if ( idx1 == 4 )
                if ( idx2 == 6 ) {
                    st->board->board[ 5 ] = 3;
                    st->board->board[ 7 ] = 16;
                } else if ( idx2 == 2 ) {
                    st->board->board[ 3 ] = 3;
                    st->board->board[ 0 ] = 16;
                }
            break;
        default:
            break;
        }
        st->board->flags |= 1;
        if ( cattura < 64 )
            st->pb--;
    }
    st->board->enp = enp;
    if ( ( cattura < 64 ) || pedone ) {
        st->board->ply = 0;
        if ( p2 == 3 ) {
            if ( idx2 == 0 )
                st->board->flags &= 251;
            else if ( idx2 == 7 )
                st->board->flags &= 253;
        } else if ( p2 == 11 ) {
            if ( idx2 == 56 )
                st->board->flags &= 239;
            else if ( idx2 == 63 )
                st->board->flags &= 247;
        }
    } else {
        st->board->ply++;
    }
    st->check = -1;
    st->domin = NULL;
    st->moves = NULL;
}

uns8b trp_chess_next( trp_obj_t *st, trp_obj_t *move )
{
    if ( st->tipo != TRP_CHESS )
        return 1;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return 1;
    trp_chess_next_low( (trp_chess_t *)st, ((trp_sig64_t *)trp_car( move ))->val, ((trp_sig64_t *)trp_cdr( move ))->val );
    return 0;
}

trp_obj_t *trp_chess_next_fun( trp_obj_t *st, trp_obj_t *move )
{
    trp_obj_t *newst;

    if ( ( newst = trp_chess_clone( st ) ) == UNDEF )
        return UNDEF;
    if ( move ) {
        if ( trp_chess_next( newst, move ) ) {
            trp_gc_free( ((trp_chess_t *)newst)->board );
            trp_gc_free( newst );
            return UNDEF;
        }
    } else {
        ((trp_chess_t *)newst)->check = -1;
        ((trp_chess_t *)newst)->domin = NULL;
        ((trp_chess_t *)newst)->moves = NULL;
        ((trp_chess_t *)newst)->board->enp = 64;
        if ( trp_chess_color_m( newst ) )
            ((trp_chess_t *)newst)->board->flags &= 254;
        else
            ((trp_chess_t *)newst)->board->flags |= 1;
    }
    return newst;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_diff( trp_obj_t *st1, trp_obj_t *st2 )
{
    trp_chess_move_tt moves[ 256 ], *pmoves;
    trp_chess_board_tt pos1, pos2, posc;
    uns16b to;
    uns8b from;

    if ( ( st1->tipo != TRP_CHESS ) || ( st2->tipo != TRP_CHESS ) )
        return UNDEF;
    trp_tmp_old_to_new( (trp_chess_t *)st1, &pos1 );
    trp_tmp_old_to_new( (trp_chess_t *)st2, &pos2 );
    for ( pmoves = GenerateQuiets( &pos1, GenerateCapture2( &pos1, moves ) ) ; pmoves > moves ; ) {
        pmoves--;
        if ( !Illegal( &pos1, *pmoves ) ) {
            posc = pos1;
            trp_chess_make_move( &posc, *pmoves );
            if ( memcmp( &posc, &pos2, sizeof( trp_chess_board_tt ) ) == 0 )
                return trp_chess_move( &pos1, pmoves );
        }
    }
    return UNDEF;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static uns8b trp_chess_move_validate( trp_obj_t *st, trp_obj_t *move, trp_chess_board_tt *pos, trp_chess_move_tt *mov )
{
    trp_chess_move_tt moves[ 256 ], *pmoves;
    uns32b from, to;
    uns16b tto;
    uns8b ffrom;

    if ( ( st->tipo != TRP_CHESS ) || ( move->tipo != TRP_CONS ) )
        return 1;
    if ( trp_cast_uns32b_range( ((trp_cons_t *)move)->car, &from, 0, 0xff ) ||
         trp_cast_uns32b_range( ((trp_cons_t *)move)->cdr, &to, 0, 0xffff ) )
        return 1;
    trp_tmp_old_to_new( (trp_chess_t *)st, pos );
    for ( pmoves = GenerateQuiets( pos, GenerateCapture2( pos, moves ) ) ; pmoves > moves ; ) {
        pmoves--;
        if ( !Illegal( pos, *pmoves ) ) {
            ffrom = pmoves->From;
            tto = pmoves->To;
            if ( pos->STM ) {
                ffrom = OppSq( ffrom );
                tto = OppSq( tto );
            }
            if ( pmoves->Prom != EMPTY )
                tto |= ( pmoves->Prom - PAWN ) << 6;
            if ( ( ffrom == from ) && ( tto == to ) ) {
                *mov = *pmoves;
                return 0;
            }
        }
    }
    return 1;
}

trp_obj_t *trp_chess_move_is_capture( trp_obj_t *st, trp_obj_t *move )
{
    uns8b idx;

    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return TRP_FALSE;
    idx = ((trp_sig64_t *)trp_cdr( move ))->val & 63;
    if ( ((trp_chess_t *)st)->board->board[ idx ] != 16 )
        return TRP_TRUE;
    if ( idx == ((trp_chess_t *)st)->board->enp )
        if ( ( ((trp_chess_t *)st)->board->board[ ((trp_sig64_t *)trp_car( move ))->val ] & 7 ) == 0 )
            return TRP_TRUE; /* è una presa al varco */
    return TRP_FALSE;
}

trp_obj_t *trp_chess_move_is_capture_ep( trp_obj_t *st, trp_obj_t *move )
{
    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return TRP_FALSE;
    if ( ((trp_sig64_t *)trp_cdr( move ))->val == ((trp_chess_t *)st)->board->enp )
        if ( ( ((trp_chess_t *)st)->board->board[ ((trp_sig64_t *)trp_car( move ))->val ] & 7 ) == 0 )
            return TRP_TRUE;
    return TRP_FALSE;
}

trp_obj_t *trp_chess_move_is_castle( trp_obj_t *st, trp_obj_t *move )
{
    uns8b idx1, idx2;

    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return TRP_FALSE;
    idx1 = ((trp_sig64_t *)trp_car( move ))->val;
    idx2 = ((trp_sig64_t *)trp_cdr( move ))->val & 63;
    return ( ( ( idx2 == idx1 + 2 ) || ( idx1 == idx2 + 2 ) ) && ( ( ((trp_chess_t *)st)->board->board[ idx1 ] & 7 ) == 5 ) ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_move_is_promotion( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt mov;

    if ( trp_chess_move_validate( st, move, &pos, &mov ) )
        return TRP_FALSE;
    return ( mov.Prom == EMPTY ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_chess_move_is_check( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt mov;

    if ( trp_chess_move_validate( st, move, &pos, &mov ) )
        return TRP_FALSE;
    trp_chess_make_move( &pos, mov );
    return InCheck( &pos ) ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_move_is_checkmate( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt mov;

    if ( trp_chess_move_validate( st, move, &pos, &mov ) )
        return TRP_FALSE;
    trp_chess_make_move( &pos, mov );
    if ( !InCheck( &pos ) )
        return TRP_FALSE;
    return trp_chess_moves_count( &pos ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_chess_move_is_stalemate( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt mov;

    if ( trp_chess_move_validate( st, move, &pos, &mov ) )
        return TRP_FALSE;
    trp_chess_make_move( &pos, mov );
    if ( InCheck( &pos ) )
        return TRP_FALSE;
    return trp_chess_moves_count( &pos ) ? TRP_FALSE : TRP_TRUE;
}

trp_obj_t *trp_chess_move_is_discovery_check( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_t *newst;
    uns16b idx2;
    uns8b idx1, color, res;

    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return TRP_FALSE;
    newst = trp_chess_clone_low( (trp_chess_t *)st );
    idx1 = ((trp_sig64_t *)trp_car( move ))->val;
    idx2 = ((trp_sig64_t *)trp_cdr( move ))->val;
    trp_chess_next_low( newst, idx1, idx2 );
    color = trp_chess_color_m( newst );
    res = trp_chess_dominance_low( newst )->domin[ 1 - color ][ color ? newst->kb : newst->kw ];
    if ( res ) {
        newst->board->board[ idx2 & 63 ] = ( trp_chess_color_m( newst ) << 3 );
        trp_gc_free( newst->domin );
        newst->domin = NULL;
        res = ( trp_chess_dominance_low( newst )->domin[ 1 - color ][ color ? newst->kb : newst->kw ] == res ) ? 1 : 0;
        /*
         FIXME
         il caso particolare di una presa e.p. che provoca uno scacco indirettamente
         va considerato "discovery check" oppure no?
         vedi /home/frank/wd/a/scacchi/curiosità/is-a-discovery-check-or-not.pgn
         */
    }
    trp_gc_free( newst->board );
    trp_gc_free( newst->domin );
    trp_gc_free( newst );
    return res ? TRP_TRUE : TRP_FALSE;
}

trp_obj_t *trp_chess_move_is_double_check( trp_obj_t *st, trp_obj_t *move )
{
    trp_chess_board_tt pos;
    trp_chess_t *newst;
    uns8b color, res;

    if ( st->tipo != TRP_CHESS )
        return TRP_FALSE;
    if ( trp_in_func( move, trp_chess_legal_moves_low( (trp_chess_t *)st ), NULL ) != TRP_TRUE )
        return TRP_FALSE;
    newst = trp_chess_clone_low( (trp_chess_t *)st );
    trp_chess_next_low( newst, ((trp_sig64_t *)trp_car( move ))->val, ((trp_sig64_t *)trp_cdr( move ))->val );
    trp_tmp_old_to_new( newst, &pos );
    trp_gc_free( newst->board );
    trp_gc_free( newst->domin );
    trp_gc_free( newst );
    return ( PopCount( InCheck( &pos ) ) < 2 ) ? TRP_FALSE : TRP_TRUE;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_move_alg( trp_obj_t *st, trp_obj_t *move )
{
    trp_obj_t *newst;
    uns32b len;
    CORD_ec x;
    sig16b idx2;
    sig8b idx1, d;
    uns8b p;

    if ( ( newst = trp_chess_next_fun( st, move ) ) == UNDEF )
        return UNDEF;
    CORD_ec_init( x );
    len = 0;
    idx1 = ((trp_sig64_t *)trp_car( move ))->val;
    idx2 = ((trp_sig64_t *)trp_cdr( move ))->val;
    p = ((trp_chess_t *)st)->board->board[ idx1 ];
    d = ( idx2 & 7 ) - ( idx1 & 7 );
    if ( ( ( p & 7 ) == 5 ) && ( ( d == 2 ) || ( d == -2 ) ) ) {
        CORD_ec_append( x, 'O' );
        CORD_ec_append( x, '-' );
        CORD_ec_append( x, 'O' );
        len += 3;
        if ( d < 0 ) {
            CORD_ec_append( x, '-' );
            CORD_ec_append( x, 'O' );
            len += 2;
        }
    } else {
        uns8b capture =
            ((trp_chess_t *)newst)->pw +
            ((trp_chess_t *)newst)->pb <
            ((trp_chess_t *)st)->pw +
            ((trp_chess_t *)st)->pb ? 1 : 0;
        if ( ( p & 7 ) == 0 ) {
            if ( capture ) {
                CORD_ec_append( x, 'a' + ( idx1 & 7 ) );
                len++;
            }
        } else {
            trp_obj_t *l, *move;
            uns8b xx = 1, yy = 1, amb = 0;

            CORD_ec_append( x, "NBRQK"[ ( p & 7 ) - 1 ] );
            len++;
            for ( l = trp_chess_legal_moves_low( (trp_chess_t *)st ) ; l != NIL ; l = trp_cdr( l ) ) {
                move = trp_car( l );
                if ( ((trp_sig64_t *)trp_cdr( move ))->val == idx2 )
                    if ( ( d = ((trp_sig64_t *)trp_car( move ))->val ) != idx1 )
                        if ( ((trp_chess_t *)st)->board->board[ d ] == p ) {
                            amb = 1;
                            if ( ( d & 7 ) == ( idx1 & 7 ) )
                                xx = 0;
                            if ( ( d >> 3 ) == ( idx1 >> 3 ) )
                                yy = 0;
                        }
            }
            if ( amb ) {
                if ( xx ) {
                    CORD_ec_append( x, 'a' + ( idx1 & 7 ) );
                    len++;
                } else if ( yy ) {
                    CORD_ec_append( x, '1' + ( idx1 >> 3 ) );
                    len++;
                } else {
                    CORD_ec_append( x, 'a' + ( idx1 & 7 ) );
                    CORD_ec_append( x, '1' + ( idx1 >> 3 ) );
                    len += 2;
                }
            }
        }
        if ( capture ) {
            CORD_ec_append( x, 'x' );
            len++;
        }
        capture = idx2 & 63;
        CORD_ec_append( x, 'a' + ( capture & 7 ) );
        CORD_ec_append( x, '1' + ( capture >> 3 ) );
        len += 2;
        idx2 >>= 6;
        if ( idx2 ) {
            CORD_ec_append( x, '=' );
            CORD_ec_append( x, "NBRQ"[ idx2 - 1 ] );
            len += 2;
        }
    }
    if ( trp_chess_check_low( (trp_chess_t *)newst ) ) {
        CORD_ec_append( x, trp_chess_legal_moves_low( (trp_chess_t *)newst ) == NIL ? '#' : '+' );
        len++;
    }
    trp_gc_free( ((trp_chess_t *)newst)->board );
    trp_gc_free( newst );
    return trp_cord_cons( CORD_balance( CORD_ec_to_cord( x ) ), len );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static uns8b trp_chess_search_mate_low( trp_chess_board_tt *pos, uns32b maxmoves, uns8bfun_t f, trp_chess_move_tt *winning_move, uns32b *actmoves, sig64b *positions, uns8b *interr )
{
    trp_chess_move_tt moves[ 512 ], move, *pmoves1, *pmoves2;
    trp_chess_board_tt nextpos;
    int zeromoves;
    uns8b nontrovato = 1;

    for ( pmoves1 = GenerateCapture2( pos, GenerateQuiets( pos, moves ) ) ; pmoves1 > moves ; ) {
        pmoves1--;
        move = *pmoves1;
        if ( Illegal( pos, move ) )
            continue;
        ++(*positions);
        if ( f )
            if ( ( *positions & 0x1ffff ) == 0 )
                if ( (f)() )
                    *interr = 1;
        nextpos = *pos;
        trp_chess_make_move( &nextpos, move );
        zeromoves = 1;
        for ( pmoves2 = GenerateCapture2( &nextpos, GenerateQuiets( &nextpos, pmoves1 ) ) ; pmoves2 > pmoves1 ; ) {
            pmoves2--;
            if ( !Illegal( &nextpos, *pmoves2 ) ) {
                zeromoves = 0;
                break;
            }
        }
        if ( zeromoves ) {
            if ( InCheck( &nextpos ) ) {
                *winning_move = move;
                *actmoves = 1;
                return 0;
            }
        } else if ( maxmoves > 1 ) {
            uns32b m = nontrovato ? maxmoves - 1 : *actmoves - 2;

            if ( m ) {
                trp_chess_board_tt nextnextpos;
                trp_chess_move_tt next_winning_move;
                uns32b k = 0, resactmoves;

                for ( ; ; pmoves2-- ) {
                    if ( !Illegal( &nextpos, *pmoves2 ) ) {
                        ++(*positions);
                        if ( f )
                            if ( ( *positions & 0x1ffff ) == 0 )
                                if ( (f)() )
                                    *interr = 1;
                        if ( *interr )
                            return nontrovato;
                        nextnextpos = nextpos;
                        trp_chess_make_move( &nextnextpos, *pmoves2 );
                        if ( trp_chess_search_mate_low( &nextnextpos, m, f, &next_winning_move, &resactmoves, positions, interr ) ) {
                            k = 0;
                            break;
                        }
                        if ( resactmoves > k )
                            k = resactmoves;
                    }
                    if ( pmoves2 == pmoves1 )
                        break;
                }
                if ( k ) {
                    *winning_move = move;
                    *actmoves = k + 1;
                    nontrovato = 0;
                }
            }
        }
    }
    return nontrovato;
}

trp_obj_t *trp_chess_search_mate( trp_obj_t *st, trp_obj_t *maxmoves, trp_obj_t *net )
{
    trp_chess_board_tt pos;
    trp_chess_move_tt winning_move;
    trp_obj_t *move, *actmoves;
    sig64b positions;
    uns32b mmaxmoves, aactmoves;
    uns8bfun_t f;
    uns8b interr;

    if ( ( st->tipo != TRP_CHESS ) || trp_cast_uns32b_range( maxmoves, &mmaxmoves, 1, 200 ) )
        return UNDEF;
    if ( net ) {
        if ( net->tipo != TRP_NETPTR )
            return UNDEF;
        if ( ((trp_netptr_t *)net)->nargs > 0 )
            return UNDEF;
        f = ((trp_netptr_t *)net)->f;
    } else
        f = NULL;
    positions = 0;
    interr = 0;
    trp_tmp_old_to_new( (trp_chess_t *)st, &pos );
    if ( trp_chess_search_mate_low( &pos, mmaxmoves, f, &winning_move, &aactmoves, &positions, &interr ) ) {
        move = UNDEF;
        actmoves = UNDEF;
    } else {
        move = trp_chess_move( &pos, &winning_move );
        actmoves = trp_sig64( aactmoves );
    }
    return trp_list( move, actmoves, trp_sig64( positions), NULL );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static sig64b trp_chess_perft_low( trp_chess_board_tt *pos, uns32b depth, uns8bfun_t f, uns8b *interr, sig64b *interr_cnt )
{
    trp_chess_move_tt moves[ 256 ], *pmoves;
    trp_chess_board_tt nextpos;
    sig64b tot = 0;

    depth--;
    for ( pmoves = GenerateQuiets( pos, moves ) ; pmoves > moves ; ) {
        pmoves--;
        if ( Illegal( pos, *pmoves ) )
            continue;
        if ( depth ) {
            nextpos = *pos;
            trp_chess_make_move( &nextpos, *pmoves );
            tot += trp_chess_perft_low( &nextpos, depth, f, interr, interr_cnt );
            if ( *interr )
                break;
        } else
            tot++;
    }
    for ( pmoves = GenerateCapture2( pos, moves ) ; pmoves > moves ; ) {
        pmoves--;
        if ( Illegal( pos, *pmoves ) )
            continue;
        if ( depth ) {
            nextpos = *pos;
            trp_chess_make_move( &nextpos, *pmoves );
            tot += trp_chess_perft_low( &nextpos, depth, f, interr, interr_cnt );
            if ( *interr )
                break;
        } else
            tot++;
    }
    if ( f ) {
        ++(*interr_cnt);
        if ( ( *interr_cnt & 0x7ffff ) == 0 )
            if ( (f)() )
                *interr = 1;
    }
    return tot;
}

trp_obj_t *trp_chess_perft( trp_obj_t *st, trp_obj_t *depth, trp_obj_t *net )
{
    trp_chess_board_tt pos;
    sig64b interr_cnt;
    uns32b ddepth;
    uns8bfun_t f;
    uns8b interr;

    if ( ( st->tipo != TRP_CHESS ) || trp_cast_uns32b_range( depth, &ddepth, 0, 200 ) )
        return UNDEF;
    if ( net ) {
        if ( net->tipo != TRP_NETPTR )
            return UNDEF;
        if ( ((trp_netptr_t *)net)->nargs > 0 )
            return UNDEF;
        f = ((trp_netptr_t *)net)->f;
    } else
        f = NULL;
    if ( ddepth == 0 )
        return UNO;
    interr = 0;
    interr_cnt = 0;
    trp_tmp_old_to_new( (trp_chess_t *)st, &pos );
    return trp_sig64( trp_chess_perft_low( &pos, ddepth, f, &interr, &interr_cnt ) );
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

trp_obj_t *trp_chess_qmoves_raw( trp_obj_t *qmoves )
{
    extern trp_obj_t *trp_raw_internal( uns32b sz, uns8b use_malloc );
    trp_obj_t *raw;
    trp_queue_elem *elem;
    uns16b *d;
    uns32b idx1, idx2;

    if ( qmoves->tipo != TRP_QUEUE )
        return UNDEF;
    if ( ( raw = trp_raw_internal( ((trp_queue_t *)qmoves)->len << 1, 0 ) ) == UNDEF )
        return UNDEF;
    d = (uns16b *)( ((trp_raw_t *)raw)->data );
    for ( elem = (trp_queue_elem *)( ((trp_queue_t *)qmoves)->first ) ;
          elem ;
          elem = (trp_queue_elem *)( elem->next ) ) {
        if ( elem->val->tipo != TRP_CONS ) {
            trp_gc_free( ((trp_raw_t *)raw)->data );
            trp_gc_free( raw );
            return UNDEF;
        }
        if ( trp_cast_uns32b_range( trp_car( elem->val ), &idx1, 0, 63 ) ||
             trp_cast_uns32b_range( trp_cdr( elem->val ), &idx2, 0, 511 ) ) {
            trp_gc_free( ((trp_raw_t *)raw)->data );
            trp_gc_free( raw );
            return UNDEF;
        }
        *d++ = ( idx1 | ( idx2 << 6 ) );
    }
    return raw;
}

trp_obj_t *trp_chess_raw_qmoves( trp_obj_t *raw )
{
    trp_obj_t *queue;
    uns16b *d;
    uns32b len;

    if ( raw->tipo != TRP_RAW )
        return UNDEF;
    len = ((trp_raw_t *)raw)->len;
    if ( len & 1 )
        return UNDEF;
    for ( d = (uns16b *)( ((trp_raw_t *)raw)->data ), queue = trp_queue(), len >>= 1 ; len ; len--, d++ )
        trp_queue_put( queue, trp_cons( trp_sig64( (*d) & 63 ), trp_sig64( (*d) >> 6 ) ) );
    return queue;
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

static void trp_tmp_old_to_new( trp_chess_t *st, trp_chess_board_tt *pos )
{
    uns8b n, piece, pieceside;

    memset( pos, 0, sizeof( trp_chess_board_tt ) );
    pos->P0 = pos->P1 = pos->P2 = pos->PM = 0;
    for ( n = 0 ; n < 64 ; n++ ) {
        piece = st->board->board[ n ];
        if ( piece < 14 ) {
            pieceside = ( piece & 8 ) ? BLACK : WHITE;
            piece = ( piece & 7 ) + 1;
            pos->P0 |= ( (uns64b)piece & 1 ) << n;
            pos->P1 |= ( (uns64b)( piece >> 1 ) & 1 ) << n;
            pos->P2 |= ( (uns64b)piece >> 2 ) << n;
            if ( pieceside == WHITE )
                pos->PM |= 1ULL << n;
        }
    }
    pos->CastleFlags = 0;
    if ( trp_chess_arrcw_m( st ) )
        pos->CastleFlags |= 0x02;
    if ( trp_chess_arrlw_m( st ) )
        pos->CastleFlags |= 0x01;
    if ( trp_chess_arrcb_m( st ) )
        pos->CastleFlags |= 0x20;
    if ( trp_chess_arrlb_m( st ) )
        pos->CastleFlags |= 0x10;
    pos->STM = WHITE;
    pos->EnPassant = ( st->board->enp == 64 ) ? 8 : st->board->enp & 7;
    pos->Count50 = (uns8b)(st->board->ply);
    pos->MovCnt = st->board->mov;
    if ( trp_chess_color_m( st ) )
        ChangeSide( pos );
}

static void trp_tmp_new_to_old( trp_chess_board_tt *pos, trp_chess_t *st )
{
    uns8b sq, oppsq;

    st->tipo = TRP_CHESS;
    st->check = -1;

    for ( sq = 0 ; sq < 64 ; sq++ ) {
    }
    /*
     FIXME
     da fare
     */
}

/* ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ***                                                                                                                                                                      ***
   ****************************************************************************************************************************************************************************
   ****************************************************************************************************************************************************************************
 */

