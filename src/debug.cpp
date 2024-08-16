#include "defs.h"
#include "board.h"
#include "movelist.h"
#include <iostream>
#include <cassert>

/*
 * 
 * Print out a bitboard to the console formatted as an 8x8 grid such that the
 * square A1 is the bottom left (visually) and H8 is in the top right.
 * 
 */
void printBitboard(uint64 bitboard) {
    std::cout << "bitboard: " << bitboard << std::endl;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            if (bitboard & (1ULL << square)) {
                std::cout << " 1";
            } else {
                std::cout << " 0";
            }
        }
        std::cout << std::endl;
    }
}


/*
 * 
 * return a string representing a square. For example, square 0 returns "A1",
 * square 63 returns "H8", and square 26 returns "C3".
 * 
 */
std::string getSquareString(int square) {
    assert(square >= 0 && square < 64);
    int file = square & 0x7, rank = square >> 3;
    char f = 'a' + static_cast<char>(file), r = '1' + static_cast<char>(rank);
    return std::string({ f, r });
}


/*
 *
 * Return true if the board is a valid chessboard. Check to make sure that the
 * board's member variables have reasonable values, that the bitboards match
 * the pieces array, that the matieral counts match the pieces, etc. This
 * function should only be called within asserts in debug mode.
 *
 */
bool Board::boardIsValid() const {
    if (sideToMove != WHITE && sideToMove != BLACK) {
        std::cerr << "Side to move is not WHITE or BLACK" << std::endl;
        return false;
    }
    if (ply < 0) {
        std::cerr << "Ply must be positive" << std::endl;
        return false;
    }
    if (fiftyMoveCount < 0 || fiftyMoveCount > 100) {
        std::cerr << "Fiftymovecount should be between 0 and 100" << std::endl;
        return false;
    }
    if (countBits(pieceBitboards[WHITE_KING]) != 1) {
        std::cerr << "White must have exactly 1 king" << std::endl;
        return false;
    }
    if (countBits(pieceBitboards[BLACK_KING]) != 1) {
        std::cerr << "Black must have exactly 1 king" << std::endl;
        return false;
    }
    int pieceCount[NUM_PIECE_TYPES] = { 0 };
    int materialWhite = 0, materialBlack = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int piece = INVALID, count = 0;
        for (int type = 0; type < NUM_PIECE_TYPES; ++type) {
            if (pieceBitboards[type] & (1ULL << sq)) {
                ++count;
                piece = type;
            }
        }
        if (count > 1 || piece != pieces[sq]) {
            std::cout << "pieces:\n";
            for (int i = 0; i < 64; ++i) {
                printf("%2d ", pieces[i]);
                if (i % 8 == 7) std::cout << "\n";
            }
            std::cerr << "Invalid pieces[] array" << std::endl;
            return false;
        }
        if (piece != INVALID) {
            ++pieceCount[piece];
            if (pieceColor[piece] == WHITE) {
                materialWhite += pieceMaterial[piece];
            } else {
                materialBlack += pieceMaterial[piece];
            }
        }
    }
    if (material[WHITE] != materialWhite || material[BLACK] != materialBlack) {
        std::cerr << "Material values are incorrect" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_PAWN] > 8 || pieceCount[BLACK_PAWN] > 8) {
        std::cerr << "Invalid pawn counts" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_KNIGHT] > 10 || pieceCount[BLACK_KNIGHT] > 10) {
        std::cerr << "Invalid knight counts" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_BISHOP] > 10 || pieceCount[BLACK_BISHOP] > 10) {
        std::cerr << "Invalid bishop counts" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_ROOK] > 10 || pieceCount[BLACK_ROOK] > 10) {
        std::cerr << "Invalid rook counts" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_QUEEN] > 9 || pieceCount[BLACK_QUEEN] > 9) {
        std::cerr << "Invalid queen counts" << std::endl;
        return false;
    }
    if (pieceCount[WHITE_KING] != 1 || pieceCount[BLACK_KING] != 1) {
        std::cerr << "Invalid king counts" << std::endl;
        return false;
    }
    uint64 c_white = 0, c_black = 0;
    for (int i = WHITE_PAWN; i <= WHITE_KING; ++i) {
        c_white |= pieceBitboards[i];
        c_black |= pieceBitboards[i + 6];
    }
    if (c_white != colorBitboards[WHITE] || c_black != colorBitboards[BLACK]
        || colorBitboards[BOTH_COLORS] != (c_white | c_black)) {
        std::cerr << "Invalid colorBitboards" << std::endl;
        return false;
    }
    if (enPassantSquare != INVALID) {
        if (enPassantSquare < 0 || enPassantSquare >= 64) {
            std::cerr << "Invalid en passant square (1)" << std::endl;
            return false;
        }
        if (pieces[enPassantSquare] != INVALID) {
            std::cerr << "Invalid en passant square (2)" << std::endl;
            return false;
        }
        if (sideToMove == WHITE) {
            if ((1ULL << enPassantSquare) & 0xFFFF00FFFFFFFFFF ||
                pieces[enPassantSquare - 8] != BLACK_PAWN) {
                std::cerr << "Invalid en passant square (3)" << std::endl;
                return false;
            }
        } else {
            if ((1ULL << enPassantSquare) & 0xFFFFFFFFFF00FFFF ||
                pieces[enPassantSquare + 8] != WHITE_PAWN) {
                std::cerr << "Invalid en passant square (4)" << std::endl;
                return false;
            }
        }
    }
    if (castlePerms & CASTLE_WK) {
        if (pieces[E1] != WHITE_KING || pieces[H1] != WHITE_ROOK) {
            std::cerr << "Invalid castle permissions (1)" << std::endl;
            return false;
        }
    }
    if (castlePerms & CASTLE_WQ) {
        if (pieces[E1] != WHITE_KING || pieces[A1] != WHITE_ROOK) {
            std::cerr << "Invalid castle permissions (2)" << std::endl;
            return false;
        }
    }
    if (castlePerms & CASTLE_BK) {
        if (pieces[E8] != BLACK_KING || pieces[H8] != BLACK_ROOK) {
            std::cerr << "Invalid castle permissions (3)" << std::endl;
            return false;
        }
    }
    if (castlePerms & CASTLE_BQ) {
        if (pieces[E8] != BLACK_KING || pieces[A8] != BLACK_ROOK) {
            std::cerr << "Invalid castle permissions (4)" << std::endl;
            return false;
        }
    }
    if (castlePerms & 0xFFFFFFF0) {
        std::cerr << "Invalid castle permissions (5)" << std::endl;
    }
    uint64 pawns = pieceBitboards[WHITE_PAWN] | pieceBitboards[BLACK_PAWN];
    if (pawns & 0xFF000000000000FF) {
        std::cerr << "Pawns are on the 1st or 8th rank" << std::endl;
        return false;
    }
    if (positionKey != generatePositionKey()) {
        std::cerr << "positionKey is incorrect" << std::endl;
        return false;
    }
    return true;
}


/*
 *
 * Given a move that is packed into a 32-bit integer, make sure that it is a
 * valid move. For example, the captured piece cannot be a king, and if the
 * promotion flag is set, then the promoted piece must be present, etc. This
 * function should only be called within asserts in debug mode.
 *
 */
bool MoveList::validMove(int move) const {
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    int cap = (move >> 12) & 0xF;
    int prom = (move >> 16) & 0xF;

    if (move & CAPTURE_FLAG) {
        if (cap < 0 || cap >= NUM_PIECE_TYPES
            || cap == WHITE_KING || cap == BLACK_KING) {
            std::cout << "Invalid captured piece (1)" << std::endl;
            return false;
        }
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cout << "Invalid flags (1)" << std::endl;
            return false;
        }
    } else {
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (2)" << std::endl;
            return false;
        }
    }
    if (move & PROMOTION_FLAG) {
        if (prom < 0 || prom >= NUM_PIECE_TYPES || prom == WHITE_KING
            || prom == WHITE_PAWN || prom == BLACK_KING || prom == BLACK_PAWN) {
            std::cerr << "Invalid promoted piece (1)" << std::endl;
            return false;
        }
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cerr << "Invalid flags (2)" << std::endl;
            return false;
        }
        if (!((from < 16 && from >= 8 && to < 8) ||
              (from >= 48 && from < 56 && to >= 56))) {
            std::cerr << "Invalid from/to squares for promotion" << std::endl;
            return false;
        }
    } else {
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (2)" << std::endl;
            return false;
        }
    }
    if (move & CASTLE_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (3)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (3)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (3)" << std::endl;
            return false;
        }
        if (!(from == E1 && (to == G1 || to == C1))
            && !(from == E8 && (to == G8 || to == C8))) {
            std::cerr << "Invalid from/to squares for castling" << std::endl;
        }
    }
    if (move & PAWN_START_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | CASTLE_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (4)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (4)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (4)" << std::endl;
            return false;
        }
        uint64 F = 1ULL << from;
        uint64 T = 1ULL << to;
        if (!((F & 0x00FF00000000FF00) && (T & 0x000000FFFF000000)
              && (std::abs(from - to) == 16))) {
            std::cerr << "Invalid from/to squares for pawn start" << std::endl;
            return false;
        }
    }
    if (move & EN_PASSANT_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | CASTLE_FLAG)) {
            std::cerr << "Invalid flags (5)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (5)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (5)" << std::endl;
            return false;
        }
        uint64 F = 1ULL << from;
        uint64 T = 1ULL << to;
        if (!((F & 0x000000FFFF000000) && (T & 0x0000FF0000FF0000)
              && (std::abs(from - to) == 7 || std::abs(from - to) == 9))) {
            std::cerr << "Invalid from/to squares for en passant" << std::endl;
            return false;
        }
    }
    return true;
}
