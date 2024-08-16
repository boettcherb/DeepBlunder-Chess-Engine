#include "defs.h"
#include "board.h"
#include "attack.h"
#include <tuple>


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
 * Evaluation bonus for proximity to a target. For example, if a rook is on the
 * same file as the enemy king, that rook will get a bonus of distBonus[0] (the
 * file distance is 0).
 * 
 */
static inline constexpr char distBonus[8] = { 4, 2, 1, 0, 0, 0, 0, 0 };


/*
 * 
 * Store the locations (file and rank) of the enemey targets (kings, queens,
 * rooks and queen(s), as well as a multiplier value. These locations are
 * compared against the locations of friendly pieces. Pieces on the same
 * rank, file, or diagonal as the enemy king/queen get an evaluation bonus,
 * times the multiplier value
 *
 */
static std::tuple<int, int, int> targets[12];


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
 *      - Bonus for every friendly blocked pawn (favor closed positions)
 *      - Bonus for proximity to enemy king
 * 6) Rooks:
 *      - Bonus for being connected to another rook.
 *      - Bonus for being on open file.
 *      - Bonus for being on same rank/file as enemy king/queen.
 * 7) Queens:
 *      - Bonus for being on same file/rank/diagonal as enemy king/rooks.
 *      - Bonus for proximity to enemy king.
 * 
 * 
 * TODO:
 * 
 * 5) Bishops:
 *      - Bonus for having both bishops (bishop pair).
 *      - Bonus for being on same diagonal as enemy king/queen/rooks.
 *      - Bonus for friendly pawns on opposite color
 *      - Penalty for friendly pawns on same color
 *      - Penalty for being blocked by a friendly pawn.
 * 8) King Safety:
 *      - Penalty for too many open files/diagonals around the king
 *        (length of diagonal matters)
 *      - Penalty for not enough pawns in front of the king
 *      - Penalty for enemy pawns too close to king
 *      - Penalty for enemy pieces attacking squares around the king
 *      - Penalty for not being castled
 * 9) Mobility:
 *      - all pieces get a large penalty if they are very immobile (< 2 moves
 *        available). Encorporate enemy pawn attacks into this.
 *      - Each side gets a mobility score: the side with the most move choices
 *        gets a bonus
 * 
 */
int Board::evaluatePosition() const {
    assert(boardIsValid());
    const uint64 allPieces = colorBitboards[BOTH_COLORS];
    int eval = material[WHITE] - material[BLACK];
    {
        // TODO: see if I can delete these if I loop through every
        // piece in other parts of this function
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
    }
    // ------------------------------------------------------------------------
    // ---------------------------- WHITE -------------------------------------
    // ------------------------------------------------------------------------
    uint64 friendlyPawns = pieceBitboards[WHITE_PAWN];
    uint64 enemyPawns = pieceBitboards[BLACK_PAWN];
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
    uint64 whitePawns = pieceBitboards[WHITE_PAWN];
    while (whitePawns) {
        int pawn = getLSB(whitePawns);
        if (pawnIsIsolated(pawn, friendlyPawns))       eval -= 15;
        if (pawnIsDoubled(pawn, friendlyPawns))        eval -= 5;
        if (whitePawnIsProtected(pawn, friendlyPawns)) eval += 5;
        if (whitePawnIsPassed(pawn, enemyPawns))       eval += 20;
        if (pieces[pawn + 8] != INVALID) {
            eval -= 3;
            ++blockedPawns;
        } else if (whitePawnIsBackwards(pawn, friendlyPawns, enemyPawns)) {
            eval -= 10;
        }
        whitePawns &= whitePawns - 1;
    }
    uint64 whiteKnights = pieceBitboards[WHITE_KNIGHT];
    while (whiteKnights) {
        int knight = getLSB(whiteKnights);
        int file = knight & 0x7, rank = knight >> 3;
        int distToKing = std::abs(file - std::get<0>(targets[0]))
                       + std::abs(rank - std::get<1>(targets[0]));
        eval -= 2 * distToKing;
        whiteKnights &= whiteKnights - 1;
        eval += blockedPawns * 3;
    }
    uint64 whiteRooks = pieceBitboards[WHITE_ROOK];
    while (whiteRooks) {
        int rook = getLSB(whiteRooks);
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        assert(!((1ULL << rook) & attacks));
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
        whiteRooks &= whiteRooks - 1;
    }
    uint64 whiteQueens = pieceBitboards[WHITE_QUEEN];
    while (whiteQueens) {
        int queen = getLSB(whiteQueens);
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
        eval -= 2 * distToKing;
        whiteQueens &= whiteQueens - 1;
    }
    // ------------------------------------------------------------------------
    // ---------------------------- BLACK -------------------------------------
    // ------------------------------------------------------------------------
    friendlyPawns = pieceBitboards[BLACK_PAWN];
    enemyPawns = pieceBitboards[WHITE_PAWN];
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
    uint64 blackPawns = pieceBitboards[BLACK_PAWN];
    while (blackPawns) {
        int pawn = getLSB(blackPawns);
        if (pawnIsIsolated(pawn, friendlyPawns))       eval += 15;
        if (pawnIsDoubled(pawn, friendlyPawns))        eval += 5;
        if (blackPawnIsProtected(pawn, friendlyPawns)) eval -= 5;
        if (blackPawnIsPassed(pawn, enemyPawns))       eval -= 20;
        if (pieces[pawn - 8] != INVALID) {
            eval += 3;
            ++blockedPawns;
        } else if (blackPawnIsBackwards(pawn, friendlyPawns, enemyPawns)) {
            eval += 10;
        }
        blackPawns &= blackPawns - 1;
    }
    uint64 blackKnights = pieceBitboards[BLACK_KNIGHT];
    while (blackKnights) {
        int knight = getLSB(blackKnights);
        int file = knight & 0x7, rank = knight >> 3;
        int distToKing = std::abs(file - std::get<0>(targets[0]))
            + std::abs(rank - std::get<1>(targets[0]));
        eval += 2 * distToKing;
        blackKnights &= blackKnights - 1;
        eval -= blockedPawns * 3;
    }
    uint64 blackRooks = pieceBitboards[BLACK_ROOK];
    while (blackRooks) {
        int rook = getLSB(blackRooks);
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        assert(!((1ULL << rook) & attacks));
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
        blackRooks &= blackRooks - 1;
    }
    uint64 blackQueens = pieceBitboards[BLACK_QUEEN];
    while (blackQueens) {
        int queen = getLSB(blackQueens);
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
        eval += 2 * distToKing;
        blackQueens &= blackQueens - 1;
    }
    return sideToMove == WHITE ? eval : -eval;
}
