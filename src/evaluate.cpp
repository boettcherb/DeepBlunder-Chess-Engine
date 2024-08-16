#include "defs.h"
#include "board.h"


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
         0,  0,  5, 20, 20,  5,  0,  0,
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
         0,  0,  5, 20, 20,  5,  0,  0,
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


/*
 *
 * Pawn evaluations. Determine if a pawn is isolated, doubled, protected,
 * passed, or backwards. Isolated, doubled, and backwards pawns get a penalty
 * to their evaluation, while protected and passed pawns get a bonus.
 *
 */
static bool pawnIsIsolated(int square, uint64 friendlyPawns) {
    return !(friendlyPawns & sideFiles[square & 0x7]);
}
static bool pawnIsDoubled(int square, uint64 friendlyPawns) {
    return countBits(sameFile[square & 0x7] & friendlyPawns) > 1;
}
static bool whitePawnIsProtected(int square, uint64 friendlyPawns) {
    uint64 protectionSquares =
        ((1ULL << (square - 7)) & 0xFEFEFEFEFEFEFEFE) |
        ((1ULL << (square - 9)) & 0x7F7F7F7F7F7F7F7F);
    return friendlyPawns & protectionSquares;
}
static bool blackPawnIsProtected(int square, uint64 friendlyPawns) {
    uint64 protectionSquares =
        (1ULL << (square + 7)) & 0x7F7F7F7F7F7F7F7F |
        (1ULL << (square + 9)) & 0xFEFEFEFEFEFEFEFE;
    return friendlyPawns & protectionSquares;
}
static bool whitePawnIsPassed(int square, uint64 enemyPawns) {
    int file = square & 0x7, rank = square >> 3;
    return !(enemyPawns & (adjFiles[file] << ((rank + 1) << 3)));
}
static bool blackPawnIsPassed(int square, uint64 enemyPawns) {
    int file = square & 0x7, rank = square >> 3;
    return !(enemyPawns & (adjFiles[file] >> ((8 - rank) << 3)));
}
static bool whitePawnIsBackwards(int square, uint64 friendlyPawns,
                                 uint64 enemyPawns) {
    int file = square & 0x7, rank = square >> 3;
    uint64 behind = sideFiles[file] >> ((7 - rank) << 3);
    uint64 blockers = (1ULL << (square + 15)) | (1ULL << (square + 17));
    return !(behind & friendlyPawns) && (blockers & enemyPawns);
}
static bool blackPawnIsBackwards(int square, uint64 friendlyPawns,
                                 uint64 enemyPawns) {
    int file = square & 0x7, rank = square >> 3;
    uint64 behind = sideFiles[file] << (rank << 3);
    uint64 blockers = (1ULL << (square - 15)) | (1ULL << (square - 17));
    return !(behind & friendlyPawns) && (blockers & enemyPawns);
}


/*
 *
 * Return an evaluation of the current position. The evaluation is an integer
 * that is positive if the engine thinks that white is winning, and negative if
 * the engine thinks that black is winning. The evaluation is based on 100ths
 * of a pawn. So if white is winning by 1 pawn, the evaluation would be 100.
 *
 * Currently, the evaluation is based on:
 * 1) The material of each side.
 * 2) The value of a piece on a certain square, given by the pieceValue array.
 * 3) Pawn structure:
 *      - Bonus is given for passed and protected pawns
 *      - Penalty is given for backwards, doubled, isolated, and blocked pawns
 * 
 * TODO: 
 * 4) King Safety:
 *      - Penalty for too many open files/diagonals around the king
 *      - Penalty for not enough pawns in front of the king
 *      - Penalty for enemy pawns too close to king
 *      - Penalty for enemy pieces attacking squares around the king
 *      - Penalty for not being castled
 * 5) Pieces
 *      - Rooks get a bonus for being connected and for being on the same rank
 *        / file as the enemy king
 *      - Bishops get a bonus if both are still on board (bishop pair), for
 *        being on the same diagonal as the king, and for having more enemy
 *        pawns on the same color and friendly pawns on the opposite color.
 *      - Knights get a bonus for every blocked pawn on the board and for there
 *        being more pawns on the board
 *      - Queens get a bonus for proximity to enemy king and for being on same
 *      - rank/file/diagonal as enemy king.
 *      - all pieces get a large penalty if they are very immobile (< 2 moves
 *        available). Encorporate enemy pawn attacks into this.
 *      - Each side gets a mobility score: the side with the most move choices
 *        gets a bonus
 */
int Board::evaluatePosition() const {
    assert(boardIsValid());
    int eval = material[WHITE] - material[BLACK];
    uint64 whitePieces = colorBitboards[WHITE];
    while (whitePieces) {
        int square = getLSB(whitePieces);
        eval += pieceValue[pieces[square]][square];
        whitePieces &= whitePieces - 1;
    }
    uint64 blackPieces = colorBitboards[BLACK];
    while (blackPieces) {
        int square = getLSB(blackPieces);
        eval -= pieceValue[pieces[square]][square];
        blackPieces &= blackPieces - 1;
    }
    uint64 whitePawns = pieceBitboards[WHITE_PAWN];
    uint64 friendlyPawns = pieceBitboards[WHITE_PAWN];
    uint64 enemyPawns = pieceBitboards[BLACK_PAWN];
    while (whitePawns) {
        int pawn = getLSB(whitePawns);
        if (pawnIsIsolated(pawn, friendlyPawns))       eval -= 15;
        if (pawnIsDoubled(pawn, friendlyPawns))        eval -= 5;
        if (whitePawnIsProtected(pawn, friendlyPawns)) eval += 5;
        if (whitePawnIsPassed(pawn, enemyPawns))       eval += 20;
        if (pieces[pawn + 8] != INVALID)               eval -= 3;
        else if (whitePawnIsBackwards(pawn, friendlyPawns, enemyPawns))
            eval -= 10;
        whitePawns &= whitePawns - 1;
    }
    uint64 blackPawns = pieceBitboards[BLACK_PAWN];
    friendlyPawns = pieceBitboards[BLACK_PAWN];
    enemyPawns = pieceBitboards[WHITE_PAWN];
    while (blackPawns) {
        int pawn = getLSB(blackPawns);
        if (pawnIsIsolated(pawn, friendlyPawns))       eval += 15;
        if (pawnIsDoubled(pawn, friendlyPawns))        eval += 5;
        if (blackPawnIsProtected(pawn, friendlyPawns)) eval -= 5;
        if (blackPawnIsPassed(pawn, enemyPawns))       eval -= 20;
        if (pieces[pawn - 8] != INVALID)               eval += 3;
        else if (blackPawnIsBackwards(pawn, friendlyPawns, enemyPawns))
            eval += 10;
        blackPawns &= blackPawns - 1;
    }
    return sideToMove == WHITE ? eval : -eval;
}
