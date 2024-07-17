#include "Board.h"
#include <string>
#include <algorithm>

const std::string INITIAL_POSITION = 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Board::Board(const std::string& starting_fen = INITIAL_POSITION) {
    reset();
}

/*
 *
 * Reset the board's member variables to their default values. sideToMove is
 * set to BOTH_COLORS so that an error occurs if it is not set to either WHITE
 * or BLACK during initialization. The default value of the pieces array is
 * NO_PIECE. All other default values are 0.
 *
 */
void Board::reset() {
    std::fill_n(pieceBitboards, NUM_PIECE_TYPES, 0);
    std::fill_n(colorBitboards, 3, 0);
    std::fill_n(pieces, 64, NO_PIECE);
    sideToMove = Color::BOTH_COLORS;
    ply = searchPly = castlePerms = fiftyMoveCount = enPassantSquare = 0;
    material[0] = material[1] = 0;
    positionKey = 0;
    history.clear();
}
