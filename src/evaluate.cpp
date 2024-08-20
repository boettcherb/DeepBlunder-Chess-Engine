#include "defs.h"
#include "board.h"
#include "attack.h"
#include <tuple>


/*
 * 
 * Return a bitboard with exactly one bit set corresponding to the given square
 * index (0 to 63). If 'square' is outside of this range, return 0 to prevent
 * undefined behavior.
 * 
 */
static inline constexpr uint64 BB(int square) {
    if (square < 0 || square >= 64) {
        return 0;
    }
    return 1ULL << square;
}


/*
 *
 * Perform a shift by the specified amount. If shift is outisde the range
 * [0, 63], then return 0 to avoid undefined behavior.
 *
 */
static inline constexpr uint64 RSHIFT(uint64 bb, int shift) {
    if (shift < 0 || shift >= 64) {
        return 0;
    }
    return bb >> shift;
}


/*
 *
 * pieceValue[piece][sq] gives an estimate as to how valuable a piece will be
 * when placed on the square sq. This is used for the evaluatePosition()
 * function to favor moves that are likely to be improving moves. For example,
 * a white pawn on E7 has a higher value than a pawn on E5, and a knight in the
 * center has a higher value than a knight on the edge. This will encourage the
 * engine to make improving moves such as bringing the knights to the center
 * and pushing pawns. Each value is in 100ths of a pawn, so a value of 100 is
 * worth 1 pawn.
 *
 */
static inline constexpr char pieceValue[NUM_PIECE_TYPES][64] = {
    { // white pawn
         0,  0,  0,   0,   0,  0,  0,  0,
        10, 10,  0, -10, -10,  0, 10, 10,
         5,  0,  0,   5,   5,  0,  0,  5,
         0,  0, 10,  20,  20, 10,  0,  0,
        10, 10, 20,  30,  30, 20, 10, 10,
        30, 30, 30,  40,  40, 30, 30, 30,
        70, 70, 70,  70,  70, 70, 70, 70,
         0,  0,  0,   0,   0,  0,  0,  0,
    },
    { // white knight
        -10, -10,  0,  0,  0,  0, -10, -10,
          0,   0,  0,  5,  5,  0,   0,   0,
          0,   0, 10, 10, 10, 10,   0,   0,
          0,   5, 10, 20, 20, 10,   5,   0,
          5,  10, 15, 20, 20, 15,  10,   5,
          5,  10, 10, 20, 20, 10,  10,   5,
          0,   0,  5, 10, 10,  5,   0,   0,
        -10,   0,  0,  0,  0,  0,   0, -10,
    },
    { // white bishop
        -20,  0, -10,  0,  0, -10,  0, -20,
          0, 10,   0, 10, 10,   0, 10,   0,
          0,  0,  10, 15, 15,  10,  0,   0,
          0, 10,  15, 20, 20,  15, 10,   0,
          0, 10,  15, 20, 20,  15, 10,   0,
          0,  0,  10, 15, 15,  10,  0,   0,
          0,  0,   0, 10, 10,   0,  0,   0,
        -20,  0,   0,  0,  0,   0,  0, -20,
    },
    { // white rook
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
        20, 20, 20, 20, 20, 20, 20, 20,
         0,  0,  5,  5,  5,  5,  0,  0,
    },
    { // white queen
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
    },
    { // white king
        0, 0, 15, 0, -10, 0, 20, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
    },
    { // black pawn
         0,  0,  0,   0,   0,  0,  0,  0,
        70, 70, 70,  70,  70, 70, 70, 70,
        30, 30, 30,  40,  40, 30, 30, 30,
        10, 10, 20,  30,  30, 20, 10, 10,
         0,  0, 10,  20,  20, 10,  0,  0,
         5,  0,  0,   5,   5,  0,  0,  5,
        10, 10,  0, -10, -10,  0, 10, 10,
         0,  0,  0,   0,   0,  0,  0,  0,
    },
    { // black knight
        0,   0,  0,  0,  0,  0,   0, 0,
        0,   0,  5, 10, 10,  5,   0, 0,
        5,  10, 10, 20, 20, 10,  10, 5,
        5,  10, 15, 20, 20, 15,  10, 5,
        0,   5, 10, 20, 20, 10,   5, 0,
        0,   0, 10, 10, 10, 10,   0, 0,
        0,   0,  0,  5,  5,  0,   0, 0,
        0, -10,  0,  0,  0,  0, -10, 0,
    },
    { // black bishop
        -20,  0,   0,  0,  0,   0,  0, -20,
          0,  0,   0, 10, 10,   0,  0,   0,
          0,  0,  10, 15, 15,  10,  0,   0,
          0, 10,  15, 20, 20,  15, 10,   0,
          0, 10,  15, 20, 20,  15, 10,   0,
          0,  0,  10, 15, 15,  10,  0,   0,
          0, 10,   0, 10, 10,   0, 10,   0,
        -20,  0, -10,  0,  0, -10,  0, -20,
    },
    { // black rook
         0,  0,  5,  5,  5,  5,  0,  0,
        20, 20, 20, 20, 20, 20, 20, 20,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
         0,  0,  5,  5,  5,  5,  0,  0,
    },
    { // black queen
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0, 10, 10, 10, 10, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
        0, 0,  0,  0,  0,  0, 0, 0,
    },
    { // black king
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0,  0, 0,   0, 0,  0, 0,
        0, 0, 15, 0, -10, 0, 20, 0,
    },
};

// endgame pieceSquare table for king
static inline constexpr char eg_kingValue[64] = {
    -20, -20, -20, -20, -20, -20, -20, -20,
    -20, -10, -10, -10, -10, -10, -10, -20,
    -20,   0,   0,   0,   0,   0,   0, -20,
    -20,   0,  10,  10,  10,  10,   0, -20,
    -20,   0,  10,  10,  10,  10,   0, -20,
    -20,   0,   0,   0,   0,   0,   0, -20,
    -20, -10, -10, -10, -10, -10, -10, -20,
    -20, -20, -20, -20, -20, -20, -20, -20,
};


/*
 * 
 * Bitboards representing rays going outward from a given square in the
 * directions Northeast, Southeast, Northwest, and Southwest. These are the
 * same as in attack.cpp.
 * 
 */
static inline constexpr uint64 rayNorthWest[64] = {
    0x0000000000000000, 0x0000000000000100, 0x0000000000010200, 0x0000000001020400,
    0x0000000102040800, 0x0000010204081000, 0x0001020408102000, 0x0102040810204000,
    0x0000000000000000, 0x0000000000010000, 0x0000000001020000, 0x0000000102040000,
    0x0000010204080000, 0x0001020408100000, 0x0102040810200000, 0x0204081020400000,
    0x0000000000000000, 0x0000000001000000, 0x0000000102000000, 0x0000010204000000,
    0x0001020408000000, 0x0102040810000000, 0x0204081020000000, 0x0408102040000000,
    0x0000000000000000, 0x0000000100000000, 0x0000010200000000, 0x0001020400000000,
    0x0102040800000000, 0x0204081000000000, 0x0408102000000000, 0x0810204000000000,
    0x0000000000000000, 0x0000010000000000, 0x0001020000000000, 0x0102040000000000,
    0x0204080000000000, 0x0408100000000000, 0x0810200000000000, 0x1020400000000000,
    0x0000000000000000, 0x0001000000000000, 0x0102000000000000, 0x0204000000000000,
    0x0408000000000000, 0x0810000000000000, 0x1020000000000000, 0x2040000000000000,
    0x0000000000000000, 0x0100000000000000, 0x0200000000000000, 0x0400000000000000,
    0x0800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
};
static inline constexpr uint64 rayNorthEast[64] = {
    0x8040201008040200, 0x0080402010080400, 0x0000804020100800, 0x0000008040201000,
    0x0000000080402000, 0x0000000000804000, 0x0000000000008000, 0x0000000000000000,
    0x4020100804020000, 0x8040201008040000, 0x0080402010080000, 0x0000804020100000,
    0x0000008040200000, 0x0000000080400000, 0x0000000000800000, 0x0000000000000000,
    0x2010080402000000, 0x4020100804000000, 0x8040201008000000, 0x0080402010000000,
    0x0000804020000000, 0x0000008040000000, 0x0000000080000000, 0x0000000000000000,
    0x1008040200000000, 0x2010080400000000, 0x4020100800000000, 0x8040201000000000,
    0x0080402000000000, 0x0000804000000000, 0x0000008000000000, 0x0000000000000000,
    0x0804020000000000, 0x1008040000000000, 0x2010080000000000, 0x4020100000000000,
    0x8040200000000000, 0x0080400000000000, 0x0000800000000000, 0x0000000000000000,
    0x0402000000000000, 0x0804000000000000, 0x1008000000000000, 0x2010000000000000,
    0x4020000000000000, 0x8040000000000000, 0x0080000000000000, 0x0000000000000000,
    0x0200000000000000, 0x0400000000000000, 0x0800000000000000, 0x1000000000000000,
    0x2000000000000000, 0x4000000000000000, 0x8000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
};
static inline constexpr uint64 raySouthWest[64] = {
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000001, 0x0000000000000002, 0x0000000000000004,
    0x0000000000000008, 0x0000000000000010, 0x0000000000000020, 0x0000000000000040,
    0x0000000000000000, 0x0000000000000100, 0x0000000000000201, 0x0000000000000402,
    0x0000000000000804, 0x0000000000001008, 0x0000000000002010, 0x0000000000004020,
    0x0000000000000000, 0x0000000000010000, 0x0000000000020100, 0x0000000000040201,
    0x0000000000080402, 0x0000000000100804, 0x0000000000201008, 0x0000000000402010,
    0x0000000000000000, 0x0000000001000000, 0x0000000002010000, 0x0000000004020100,
    0x0000000008040201, 0x0000000010080402, 0x0000000020100804, 0x0000000040201008,
    0x0000000000000000, 0x0000000100000000, 0x0000000201000000, 0x0000000402010000,
    0x0000000804020100, 0x0000001008040201, 0x0000002010080402, 0x0000004020100804,
    0x0000000000000000, 0x0000010000000000, 0x0000020100000000, 0x0000040201000000,
    0x0000080402010000, 0x0000100804020100, 0x0000201008040201, 0x0000402010080402,
    0x0000000000000000, 0x0001000000000000, 0x0002010000000000, 0x0004020100000000,
    0x0008040201000000, 0x0010080402010000, 0x0020100804020100, 0x0040201008040201,
};
static inline constexpr uint64 raySouthEast[64] = {
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    0x0000000000000002, 0x0000000000000004, 0x0000000000000008, 0x0000000000000010,
    0x0000000000000020, 0x0000000000000040, 0x0000000000000080, 0x0000000000000000,
    0x0000000000000204, 0x0000000000000408, 0x0000000000000810, 0x0000000000001020,
    0x0000000000002040, 0x0000000000004080, 0x0000000000008000, 0x0000000000000000,
    0x0000000000020408, 0x0000000000040810, 0x0000000000081020, 0x0000000000102040,
    0x0000000000204080, 0x0000000000408000, 0x0000000000800000, 0x0000000000000000,
    0x0000000002040810, 0x0000000004081020, 0x0000000008102040, 0x0000000010204080,
    0x0000000020408000, 0x0000000040800000, 0x0000000080000000, 0x0000000000000000,
    0x0000000204081020, 0x0000000408102040, 0x0000000810204080, 0x0000001020408000,
    0x0000002040800000, 0x0000004080000000, 0x0000008000000000, 0x0000000000000000,
    0x0000020408102040, 0x0000040810204080, 0x0000081020408000, 0x0000102040800000,
    0x0000204080000000, 0x0000408000000000, 0x0000800000000000, 0x0000000000000000,
    0x0002040810204080, 0x0004081020408000, 0x0008102040800000, 0x0010204080000000,
    0x0020408000000000, 0x0040800000000000, 0x0080000000000000, 0x0000000000000000,
};


/*
 * 
 * Bitboards representing files: sameFile[i] is the i'th file, sideFiles[i] is
 * the two files to the left and right of the i'th file, and adjFiles[i] is the
 * i'th file and the two side files.
 * 
 *  Ex: sameFile[3] =     |     Ex: sideFiles[3] =   |    Ex: adjFiles[3] =
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0 
 *   0 0 0 1 0 0 0 0      |      0 0 1 0 1 0 0 0     |     0 0 1 1 1 0 0 0
 * 
 */
static inline constexpr uint64 sameFile[8] = {
    0x0101010101010101, 0x0202020202020202,
    0x0404040404040404, 0x0808080808080808,
    0x1010101010101010, 0x2020202020202020,
    0x4040404040404040, 0x8080808080808080,
};
static inline constexpr uint64 sideFiles[8] = {
    0x0202020202020202, 0x0505050505050505,
    0x0A0A0A0A0A0A0A0A, 0x1414141414141414,
    0x2828282828282828, 0x5050505050505050,
    0xA0A0A0A0A0A0A0A0, 0x4040404040404040,
};
static inline constexpr uint64 adjFiles[8] = {
    0x0303030303030303, 0x0707070707070707,
    0x0E0E0E0E0E0E0E0E, 0x1C1C1C1C1C1C1C1C,
    0x3838383838383838, 0x7070707070707070,
    0xE0E0E0E0E0E0E0E0, 0xC0C0C0C0C0C0C0C0,
};


static inline constexpr uint64 LIGHT_SQUARES = 0x55AA55AA55AA55AA;
static inline constexpr uint64 DARK_SQUARES = 0xAA55AA55AA55AA55;
static inline constexpr uint64 CENTER = 0x00003C3C3C3C0000;


/*
 * 
 * Evalulation penalty based on how far a pawn is from a king on a certain file
 * or diagonal. This should hopefully prevent the engine from mindlessly
 * pushing pawns in front of the king.
 * 
 */
static inline constexpr char pawnDistPenalty[8] = {
    0, 0, 8, 12, 24, 34, 40,
};


/*
 * 
 * Penalty applied for having limited mobility. For example, if a piece has 0
 * possible moves it gets a penalty of 30, if it has 1 move it gets a
 * penalty of 20, etc.
 * 
 */
static inline constexpr char mobilityPenalty[32] = { 30, 20, 4, 1, };


/*
 * 
 * Evaluation bonus for proximity to a target. For example, if a rook is on the
 * same file as the enemy king, that rook will get a bonus of distBonus[0] (the
 * file distance is 0).
 * 
 */
static inline constexpr char distBonus[8] = { 4, 2, 1, 0, 0, 0, 0, 0 };


/*
 *
 * Pawn evaluations. Determine if a pawn is isolated, doubled, protected,
 * passed, or backwards. Isolated, doubled, and backwards pawns get a penalty
 * to their evaluation, while protected and passed pawns get a bonus.
 *
 */
static inline bool pawnIsIsolated(int square, uint64 friendlyPawns) {
    assert(square >= 8 && square < 56);
    return !(friendlyPawns & sideFiles[square & 0x7]);
}
static inline bool pawnIsDoubled(int square, uint64 friendlyPawns) {
    assert(square >= 8 && square < 56);
    return countBits(sameFile[square & 0x7] & friendlyPawns) > 1;
}
static inline bool whitePawnIsProtected(int square, uint64 friendlyPawns) {
    assert(square >= 8 && square < 56);
    uint64 protectionSquares = (BB(square - 7) & 0xFEFEFEFEFEFEFEFE)
        | (BB(square - 9) & 0x7F7F7F7F7F7F7F7F);
    return friendlyPawns & protectionSquares;
}
static inline bool blackPawnIsProtected(int square, uint64 friendlyPawns) {
    assert(square >= 8 && square < 56);
    uint64 protectionSquares = (BB(square + 7) & 0x7F7F7F7F7F7F7F7F)
        | (BB(square + 9) & 0xFEFEFEFEFEFEFEFE);
    return friendlyPawns & protectionSquares;
}
static inline bool whitePawnIsPassed(int square, uint64 enemyPawns) {
    assert(square >= 8 && square < 56);
    int file = square & 0x7, rank = square >> 3;
    assert(rank > 0 && rank < 7);
    return !(enemyPawns & (adjFiles[file] << ((rank + 1) << 3)));
}
static inline bool blackPawnIsPassed(int square, uint64 enemyPawns) {
    assert(square >= 8 && square < 56);
    int file = square & 0x7, rank = square >> 3;
    assert(rank > 0 && rank < 7);
    return !(enemyPawns & (adjFiles[file] >> ((8 - rank) << 3)));
}
static inline bool whitePawnIsBackwards(int square, uint64 friendlyPawns,
                                 uint64 enemyPawns) {
    assert(square >= 8 && square < 56);
    int file = square & 0x7, rank = square >> 3;
    assert(rank > 0 && rank < 7);
    uint64 behind = sideFiles[file] >> ((7 - rank) << 3);
    uint64 blockers = BB(square + 15) | BB(square + 17);
    return !(behind & friendlyPawns) && (blockers & enemyPawns);
}
static inline bool blackPawnIsBackwards(int square, uint64 friendlyPawns,
                                 uint64 enemyPawns) {
    assert(square >= 8 && square < 56);
    int file = square & 0x7, rank = square >> 3;
    assert(rank > 0 && rank < 7);
    uint64 behind = sideFiles[file] << (rank << 3);
    uint64 blockers = BB(square - 15) | BB(square - 17);
    return !(behind & friendlyPawns) && (blockers & enemyPawns);
}


/*
 *
 * Return an evaluation of the current position. The evaluation is an integer
 * that is positive if the engine thinks that the current side to move is
 * winning, and negative if the engine thinks the current side to move is
 * losing. The evaluation is based on 100ths of a pawn. So if the side to move
 * is winning by 1 pawn, the evaluation would be 100.
 *
 * The evaluation is based on:
 * 1) The material of each side.
 * 2) The value of a piece on a certain square, given by the pieceValue array.
 * 3) Pawn structure:
 *      - Bonus for passed and protected pawns.
 *      - Penalty for backwards, doubled, isolated, and blocked pawns.
 * 4) Knights:
 *      - Bonus for proximity to enemy king
 *      - Bonus for every friendly blocked pawn (favor closed positions)
 * 5) Bishops:
 *      - Bonus for having both bishops (bishop pair).
 *      - Bonus for being on same diagonal as enemy king/queen/rooks.
 *      - Bonus for friendly pawns on opposite color
 *      - Penalty for friendly pawns on same color
 *      - Penalty for being blocked by a friendly pawn.
 * 6) Rooks:
 *      - Bonus for being connected to another rook.
 *      - Bonus for being on open file.
 *      - Bonus for being on same rank/file as enemy king/queen.
 * 7) Queens:
 *      - Bonus for being on same file/rank/diagonal as enemy king/rooks.
 *      - Bonus for proximity to enemy king.
 * 8) King Safety:
 *      - Penalty for enemy pieces attacking squares around the king.
 *      - Penalty for king being on an open file.
 *      - Penalty for pushing pawns in front of the king.
 *      - Penalty for open diagonals around the king (length of diagonal matters).
 *      - Penalty for not being castled.
 *      - King safety concerns lesson as remaining enemy material decreases.
 * 9) Control / Mobility:
 *      - Bonus for controlling central squares.
 *      - Penalty for pieces being very immobile (<2 moves available).
 * 
 */
int Board::evaluatePosition() const {
    assert(boardIsValid());
    std::tuple<int, int, int> targets[12];
    const uint64 allPieces = colorBitboards[BOTH_COLORS];
    uint64 wpal = attack::getWhitePawnAttacksLeft(pieceBitboards[WHITE_PAWN]);
    uint64 wpar = attack::getWhitePawnAttacksRight(pieceBitboards[WHITE_PAWN]);
    uint64 bpal = attack::getBlackPawnAttacksLeft(pieceBitboards[BLACK_PAWN]);
    uint64 bpar = attack::getBlackPawnAttacksRight(pieceBitboards[BLACK_PAWN]);
    int eval = material[WHITE] - material[BLACK];

    // ------------------------------------------------------------------------
    // ---------------------------- WHITE -------------------------------------
    // ------------------------------------------------------------------------

    uint64 friendlyPawns = pieceBitboards[WHITE_PAWN];
    uint64 enemyPawns = pieceBitboards[BLACK_PAWN];
    uint64 friendlyPieces = colorBitboards[WHITE];
    uint64 control = 0;
    int centerControlScore = 0;
    int blockedPawns = 0;
    int numTargets = 1;
    {
        int enemyKing = getLSB(pieceBitboards[BLACK_KING]);
        targets[0] = { enemyKing & 0x7, enemyKing >> 3, 4 };
        uint64 enemyQueens = pieceBitboards[BLACK_QUEEN];
        uint64 enemyRooks = pieceBitboards[BLACK_ROOK];
        while (enemyQueens) {
            int queen = getLSB(enemyQueens);
            targets[numTargets++] = { queen & 0x7, queen >> 3, 3 };
            enemyQueens &= enemyQueens - 1;
        }
        while (enemyRooks) {
            int rook = getLSB(enemyRooks);
            targets[numTargets++] = { rook & 0x7, rook >> 3, 1 };
            enemyRooks &= enemyRooks - 1;
        }
    }
    // ------------------------------------------------------------------------
    uint64 whitePawns = pieceBitboards[WHITE_PAWN];
    control |= wpal | wpar;
    centerControlScore += countBits(wpal & CENTER) * 3;
    centerControlScore += countBits(wpar & CENTER) * 3;
    while (whitePawns) {
        int pawn = getLSB(whitePawns);
        eval += pieceValue[WHITE_PAWN][pawn];
        if (pawnIsIsolated(pawn, friendlyPawns))                   eval -= 15;
        if (pawnIsDoubled(pawn, friendlyPawns))                    eval -= 5;
        if (whitePawnIsProtected(pawn, friendlyPawns))             eval += 5;
        if (whitePawnIsPassed(pawn, enemyPawns))                   eval += 20;
        if (whitePawnIsBackwards(pawn, friendlyPawns, enemyPawns)) eval -= 10;
        if (pieces[pawn + 8] != NO_PIECE) {
            eval -= 3;
            ++blockedPawns;
        }
        whitePawns &= whitePawns - 1;
    }
    // ------------------------------------------------------------------------
    uint64 whiteKnights = pieceBitboards[WHITE_KNIGHT];
    while (whiteKnights) {
        int knight = getLSB(whiteKnights);
        eval += pieceValue[WHITE_KNIGHT][knight];
        uint64 attacks = attack::getKnightAttacks(knight);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = knight & 0x7, rank = knight >> 3;
        int distToKing = std::abs(file - std::get<0>(targets[0]))
            + std::abs(rank - std::get<1>(targets[0]));
        eval += 2 * (10 - distToKing);
        eval += blockedPawns * 3;

        attacks &= ~friendlyPieces;
        attacks &= ~(bpal | bpar);
        eval -= mobilityPenalty[countBits(attacks)];
        whiteKnights &= whiteKnights - 1;
    }
    // ------------------------------------------------------------------------
    uint64 whiteBishops = pieceBitboards[WHITE_BISHOP];
    bool hasLightBishop = false, hasDarkBishop = false;
    while (whiteBishops) {
        int bishop = getLSB(whiteBishops);
        eval += pieceValue[WHITE_BISHOP][bishop];
        uint64 attacks = attack::getBishopAttacks(bishop, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = bishop & 0x7, rank = bishop >> 3;
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int dist = std::abs(std::abs(file - targetFile) -
                                std::abs(rank - targetRank));
            eval += distBonus[dist] * mult;
        }
        if ((file & 0x1) ^ (rank & 0x1)) {
            hasLightBishop = true;
            eval -= countBits(LIGHT_SQUARES & friendlyPawns) * 2;
            eval += countBits(DARK_SQUARES & friendlyPawns) * 2;
        } else {
            hasDarkBishop = true;
            eval += countBits(LIGHT_SQUARES & friendlyPawns) * 2;
            eval -= countBits(DARK_SQUARES & friendlyPawns) * 2;
        }
        uint64 blockers = BB(bishop + 7) | BB(bishop + 9);
        if (blockers & friendlyPawns) {
            eval -= 10;
        }
        attacks &= ~friendlyPieces;
        attacks &= ~(bpal | bpar);
        eval -= mobilityPenalty[countBits(attacks)];
        whiteBishops &= whiteBishops - 1;
    }
    if (hasLightBishop && hasDarkBishop) {
        eval += 16;
    }
    // ------------------------------------------------------------------------
    uint64 whiteRooks = pieceBitboards[WHITE_ROOK];
    while (whiteRooks) {
        int rook = getLSB(whiteRooks);
        eval += pieceValue[WHITE_ROOK][rook];
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        if (attacks & pieceBitboards[WHITE_ROOK]) {
            eval += 7;
        }
        int file = rook & 0x7, rank = rook >> 3;
        if (!(sameFile[file] & friendlyPawns)) {
            eval += 20;
        }
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int minDist = std::min(std::abs(file - targetFile),
                                   std::abs(rank - targetRank));
            eval += distBonus[minDist] * mult;
        }
        attacks &= ~friendlyPieces;
        attacks &= ~(bpal | bpar);
        eval -= mobilityPenalty[countBits(attacks)];
        whiteRooks &= whiteRooks - 1;
    }
    // ------------------------------------------------------------------------
    uint64 whiteQueens = pieceBitboards[WHITE_QUEEN];
    while (whiteQueens) {
        int queen = getLSB(whiteQueens);
        eval += pieceValue[WHITE_QUEEN][queen];
        uint64 attacks = attack::getQueenAttacks(queen, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = queen & 0x7, rank = queen >> 3;
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int minDist = std::min(std::abs(file - targetFile),
                                   std::abs(rank - targetRank));
            eval += distBonus[minDist] * mult;
            minDist = std::abs(std::abs(file - targetFile) -
                               std::abs(rank - targetRank));
            eval += distBonus[minDist] * mult;
        }
        int distToKing = std::abs(file - std::get<0>(targets[0]))
            + std::abs(rank - std::get<1>(targets[0]));
        eval += 2 * (10 - distToKing);
        attacks &= ~friendlyPieces;
        attacks &= ~(bpal | bpar);
        eval -= mobilityPenalty[countBits(attacks)];
        whiteQueens &= whiteQueens - 1;
    }

    int whiteKing = getLSB(pieceBitboards[WHITE_KING]);
    uint64 kingAttacks = attack::getKingAttacks(pieceBitboards[WHITE_KING]);
    control |= kingAttacks;
    centerControlScore += countBits(kingAttacks & CENTER);
    eval += centerControlScore * 2;
    uint64 aroundKing = attack::getKingAttacks(pieceBitboards[BLACK_KING]);
    eval += countBits(aroundKing & control) * 7;

    int pawnMaterial = pieceMaterial[BLACK_PAWN]
        * countBits(pieceBitboards[BLACK_PAWN]);
    int materialCount = material[BLACK] - pawnMaterial
        + pieceMaterial[BLACK_QUEEN] * countBits(pieceBitboards[BLACK_QUEEN]);
    double materialFactor = materialCount
        / static_cast<double>(startingMaterial - pawnMaterial);
    assert(materialFactor >= 0.0);

    
    if (materialFactor < 0.25) {
        eval += eg_kingValue[whiteKing];
    } else {
        eval += pieceValue[WHITE_KING][whiteKing];
    }

    uint64 filePawns = (sameFile[whiteKing & 0x7] <<
                        (whiteKing / 8 * 8)) & friendlyPawns;
    if (!filePawns) {
        eval -= static_cast<int>(50 * materialFactor);
    } else {
        int dist = (getLSB(filePawns) - whiteKing) / 8;
        assert(dist > 0 && dist < 8);
        eval -= static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }

    uint64 rayNW = rayNorthWest[whiteKing], rayNE = rayNorthEast[whiteKing];
    uint64 pawnsNW = rayNW & friendlyPawns, pawnsNE = rayNE & friendlyPawns;
    if (!pawnsNW) {
        eval -= static_cast<int>(5 * countBits(rayNW) * materialFactor);
    } else {
        int dist = (getLSB(pawnsNW) - whiteKing) / 7;
        assert(dist > 0 && dist < 8);
        eval -= static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }
    if (!pawnsNE) {
        eval -= static_cast<int>(5 * countBits(rayNE) * materialFactor);
    } else {
        int dist = (getLSB(pawnsNE) - whiteKing) / 9;
        assert(dist > 0 && dist < 8);
        eval -= static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }

    eval -= static_cast<int>(50 * !hasCastled[WHITE] * materialFactor);

    // ------------------------------------------------------------------------
    // ---------------------------- BLACK -------------------------------------
    // ------------------------------------------------------------------------

    friendlyPawns = pieceBitboards[BLACK_PAWN];
    enemyPawns = pieceBitboards[WHITE_PAWN];
    friendlyPieces = colorBitboards[BLACK];
    control = 0;
    centerControlScore = 0;
    blockedPawns = 0;
    numTargets = 1;
    {
        int enemyKing = getLSB(pieceBitboards[WHITE_KING]);
        targets[0] = { enemyKing & 0x7, enemyKing >> 3, 4 };
        uint64 enemyQueens = pieceBitboards[WHITE_QUEEN];
        uint64 enemyRooks = pieceBitboards[WHITE_ROOK];
        while (enemyQueens) {
            int queen = getLSB(enemyQueens);
            targets[numTargets++] = { queen & 0x7, queen >> 3, 3 };
            enemyQueens &= enemyQueens - 1;
        }
        while (enemyRooks) {
            int rook = getLSB(enemyRooks);
            targets[numTargets++] = { rook & 0x7, rook >> 3, 1 };
            enemyRooks &= enemyRooks - 1;
        }
    }
    // ------------------------------------------------------------------------
    uint64 blackPawns = pieceBitboards[BLACK_PAWN];
    control |= bpal | bpar;
    centerControlScore += countBits(bpal & CENTER) * 3;
    centerControlScore += countBits(bpar & CENTER) * 3;
    while (blackPawns) {
        int pawn = getLSB(blackPawns);
        eval -= pieceValue[BLACK_PAWN][pawn];
        if (pawnIsIsolated(pawn, friendlyPawns))                   eval += 15;
        if (pawnIsDoubled(pawn, friendlyPawns))                    eval += 5;
        if (blackPawnIsProtected(pawn, friendlyPawns))             eval -= 5;
        if (blackPawnIsPassed(pawn, enemyPawns))                   eval -= 20;
        if (blackPawnIsBackwards(pawn, friendlyPawns, enemyPawns)) eval += 10;
        if (pieces[pawn - 8] != NO_PIECE) {
            eval += 3;
            ++blockedPawns;
        }
        blackPawns &= blackPawns - 1;
    }
    // ------------------------------------------------------------------------
    uint64 blackKnights = pieceBitboards[BLACK_KNIGHT];
    while (blackKnights) {
        int knight = getLSB(blackKnights);
        eval -= pieceValue[BLACK_KNIGHT][knight];
        uint64 attacks = attack::getKnightAttacks(knight);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = knight & 0x7, rank = knight >> 3;
        int distToKing = std::abs(file - std::get<0>(targets[0]))
            + std::abs(rank - std::get<1>(targets[0]));
        eval -= 2 * (10 - distToKing);
        eval -= blockedPawns * 3;
        attacks &= ~friendlyPieces;
        attacks &= ~(wpal | wpar);
        eval += mobilityPenalty[countBits(attacks)];
        blackKnights &= blackKnights - 1;
    }
    // ------------------------------------------------------------------------
    uint64 blackBishops = pieceBitboards[BLACK_BISHOP];
    hasLightBishop = false, hasDarkBishop = false;
    while (blackBishops) {
        int bishop = getLSB(blackBishops);
        eval -= pieceValue[BLACK_BISHOP][bishop];
        uint64 attacks = attack::getBishopAttacks(bishop, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = bishop & 0x7, rank = bishop >> 3;
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int dist = std::abs(std::abs(file - targetFile) -
                                std::abs(rank - targetRank));
            eval -= distBonus[dist] * mult;
        }
        if ((file & 0x1) ^ (rank & 0x1)) {
            hasLightBishop = true;
            eval += countBits(LIGHT_SQUARES & friendlyPawns) * 2;
            eval -= countBits(DARK_SQUARES & friendlyPawns) * 2;
        } else {
            hasDarkBishop = true;
            eval -= countBits(LIGHT_SQUARES & friendlyPawns) * 2;
            eval += countBits(DARK_SQUARES & friendlyPawns) * 2;
        }
        uint64 blockers = BB(bishop - 7) | BB(bishop - 9);
        if (blockers & friendlyPawns) {
            eval += 10;
        }
        attacks &= ~friendlyPieces;
        attacks &= ~(wpal | wpar);
        eval += mobilityPenalty[countBits(attacks)];
        blackBishops &= blackBishops - 1;
    }
    if (hasLightBishop && hasDarkBishop) {
        eval -= 16;
    }
    // ------------------------------------------------------------------------
    uint64 blackRooks = pieceBitboards[BLACK_ROOK];
    while (blackRooks) {
        int rook = getLSB(blackRooks);
        eval -= pieceValue[BLACK_ROOK][rook];
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        assert(!(BB(rook) & attacks));
        if (attacks & pieceBitboards[BLACK_ROOK]) {
            eval -= 7;
        }
        int file = rook & 0x7, rank = rook >> 3;
        if (!(sameFile[file] & friendlyPawns)) {
            eval -= 20;
        }
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int minDist = std::min(std::abs(file - targetFile),
                                   std::abs(rank - targetRank));
            eval -= distBonus[minDist] * mult;
        }
        attacks &= ~friendlyPieces;
        attacks &= ~(wpal | wpar);
        eval += mobilityPenalty[countBits(attacks)];
        blackRooks &= blackRooks - 1;
    }
    // ------------------------------------------------------------------------
    uint64 blackQueens = pieceBitboards[BLACK_QUEEN];
    while (blackQueens) {
        int queen = getLSB(blackQueens);
        eval -= pieceValue[BLACK_QUEEN][queen];
        uint64 attacks = attack::getQueenAttacks(queen, allPieces);
        control |= attacks;
        centerControlScore += countBits(attacks & CENTER);
        int file = queen & 0x7, rank = queen >> 3;
        for (int target = 0; target < numTargets; ++target) {
            const auto& [targetFile, targetRank, mult] = targets[target];
            int minDist = std::min(std::abs(file - targetFile),
                                   std::abs(rank - targetRank));
            eval -= distBonus[minDist] * mult;
            minDist = std::abs(std::abs(file - targetFile) -
                               std::abs(rank - targetRank));
            eval -= distBonus[minDist] * mult;
        }
        int distToKing = std::abs(file - std::get<0>(targets[0]))
                       + std::abs(rank - std::get<1>(targets[0]));
        eval -= 2 * (10 - distToKing);
        attacks &= ~friendlyPieces;
        attacks &= ~(wpal | wpar);
        eval += mobilityPenalty[countBits(attacks)];
        blackQueens &= blackQueens - 1;
    }
    
    int blackKing = getLSB(pieceBitboards[BLACK_KING]);
    kingAttacks = attack::getKingAttacks(pieceBitboards[BLACK_KING]);
    control |= kingAttacks;
    centerControlScore += countBits(kingAttacks & CENTER);
    eval -= centerControlScore * 2;
    aroundKing = attack::getKingAttacks(pieceBitboards[WHITE_KING]);
    eval -= countBits(aroundKing & control) * 7;

    pawnMaterial = pieceMaterial[WHITE_PAWN]
        * countBits(pieceBitboards[WHITE_PAWN]);
    materialCount = material[WHITE] - pawnMaterial
        + pieceMaterial[WHITE_QUEEN] * countBits(pieceBitboards[WHITE_QUEEN]);
    materialFactor = materialCount / double(startingMaterial - pawnMaterial);
    assert(materialFactor >= 0.0);

    if (materialFactor < 0.25) {
        eval -= eg_kingValue[blackKing];
    } else {
        eval -= pieceValue[BLACK_KING][blackKing];
    }

    filePawns = RSHIFT(sameFile[blackKing & 0x7],
                       64 - blackKing / 8 * 8) & friendlyPawns;
    if (!filePawns) {
        eval += static_cast<int>(50 * materialFactor);
    } else {
        int dist = ((blackKing - getMSB(filePawns)) / 8);
        assert(dist > 0 && dist < 8);
        eval += static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }

    uint64 raySW = raySouthWest[blackKing], raySE = raySouthEast[blackKing];
    uint64 pawnsSW = raySW & friendlyPawns, pawnsSE = raySE & friendlyPawns;
    if (!pawnsSW) {
        eval += static_cast<int>(5 * countBits(raySW) * materialFactor);
    } else {
        int dist = (blackKing - getMSB(pawnsSW)) / 9;
        assert(dist > 0 && dist < 8);
        eval += static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }
    if (!pawnsSE) {
        eval += static_cast<int>(5 * countBits(raySE) * materialFactor);
    } else {
        int dist = (blackKing - getMSB(pawnsSE)) / 7;
        assert(dist > 0 && dist < 8);
        eval += static_cast<int>(pawnDistPenalty[dist] * materialFactor);
    }

    eval += static_cast<int>(50 * !hasCastled[BLACK] * materialFactor);

    return sideToMove == WHITE ? eval : -eval;
}
