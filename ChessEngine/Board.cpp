#include "Board.h"
#include "Defs.h"
#include "HashKey.h"
#include "Attack.h"
#include <iostream>
#include <string>


/*
 * 
 * PrevMove constructor. Initializes all member variables of the PrevMove
 * struct to the given values.
 * 
 */
Board::PrevMove::PrevMove(int m, int c, int f, int e, uint64 p) : move{ m },
castlePerms{ c }, fiftyMoveCount{ f }, enPassantSquare{ e }, positionKey{ p } {}


/*
 *
 * Board constructor. Initializes all board member variables to a default
 * value. If a FEN string is given, the board is set up according to that
 * string.
 *
 */
Board::Board() {
    reset();
}
Board::Board(const std::string& starting_fen) : Board() {
    setToFEN(starting_fen);
}


/*
 *
 * Getter methods to retrieve the piece on the given square, the side to move,
 * the bitboard of a given piece, the bitboard of a given color, the castle
 * permissions, the en passant square, the fifty move rule count, and the
 * current position key.
 *
 */
int Board::operator[](int index) const {
    assert(index >= 0 && index < 64);
    assert(boardIsValid());
    return pieces[index];
}
int Board::side() const {
    assert(boardIsValid());
    return sideToMove;
}
uint64 Board::getPieceBitboard(int piece) const {
    assert(piece >= 0 && piece < NUM_PIECE_TYPES);
    assert(boardIsValid());
    return pieceBitboards[piece];
}
uint64 Board::getColorBitboard(int color) const {
    assert(color == WHITE || color == BLACK || color == BOTH_COLORS);
    assert(boardIsValid());
    return colorBitboards[color];
}
int Board::getCastlePerms() const {
    assert(boardIsValid());
    return castlePerms;
}
int Board::getEnPassantSquare() const {
    assert(boardIsValid());
    return enPassantSquare;
}
int Board::getFiftyMoveCount() const {
    assert(boardIsValid());
    return fiftyMoveCount;
}
uint64 Board::getPositionKey() const {
    assert(boardIsValid());
    return positionKey;
}


/*
 *
 * Reset the board's member variables to their default values. sideToMove is
 * set to BOTH_COLORS so that an error occurs if it is not set to either WHITE
 * or BLACK during initialization. castlePerms, enPassantSquare, and the pieces
 * in the pieces array are set to INVALID. All other default values are 0.
 *
 */
void Board::reset() {
    std::fill_n(pieceBitboards, NUM_PIECE_TYPES, 0);
    std::fill_n(colorBitboards, 3, 0);
    std::fill_n(pieces, 64, INVALID);
    sideToMove = Color::BOTH_COLORS;
    castlePerms = enPassantSquare = INVALID;
    ply = fiftyMoveCount = 0;
    material[0] = material[1] = 0;
    positionKey = 0;
    history.clear();
}


/*
 * 
 * Add the given piece to the given square. Update the board's member variables
 * to reflect the change. There must not be a piece already on the square.
 * 
 */
void Board::addPiece(int square, int piece) {
    assert(square >= 0 && square < 64);
    assert(piece >= 0 && piece < NUM_PIECE_TYPES);
    assert(pieces[square] == INVALID);
    pieces[square] = piece;
    uint64 mask = 1ULL << square;
    pieceBitboards[piece] |= mask;
    colorBitboards[pieceColor[piece]] |= mask;
    colorBitboards[BOTH_COLORS] |= mask;
    material[pieceColor[piece]] += pieceMaterial[piece];
    positionKey ^= hashkey::getPieceKey(piece, square);
}


/*
 *
 * Remove the piece on the given square. Update the board's member variables to
 * reflect the change. There must be a piece on the square.
 *
 */
void Board::clearPiece(int square) {
    assert(square >= 0 && square < 64);
    assert(pieces[square] != INVALID);
    int piece = pieces[square];
    pieces[square] = INVALID;
    uint64 mask = ~(1ULL << square);
    pieceBitboards[piece] &= mask;
    colorBitboards[pieceColor[piece]] &= mask;
    colorBitboards[BOTH_COLORS] &= mask;
    material[pieceColor[piece]] -= pieceMaterial[piece];
    positionKey ^= hashkey::getPieceKey(piece, square);
}


/*
 *
 * Move the piece from the square 'from' to the square 'to'. Update the board's
 * member variables to reflect the change. There must be a piece on the 'from'
 * square and the 'to' square must be empty.
 *
 */
void Board::movePiece(int from, int to) {
    assert(from >= 0 && from < 64);
    assert(to >= 0 && to < 64);
    assert(from != to);
    assert(pieces[from] != INVALID);
    assert(pieces[to] == INVALID);
    int piece = pieces[from];
    pieces[to] = piece;
    pieces[from] = INVALID;
    uint64 setMask = 1ULL << to;
    uint64 clearMask = ~(1ULL << from);
    pieceBitboards[piece] |= setMask;
    pieceBitboards[piece] &= clearMask;
    colorBitboards[pieceColor[piece]] |= setMask;
    colorBitboards[pieceColor[piece]] &= clearMask;
    colorBitboards[BOTH_COLORS] |= setMask;
    colorBitboards[BOTH_COLORS] &= clearMask;
    positionKey ^= hashkey::getPieceKey(piece, from);
    positionKey ^= hashkey::getPieceKey(piece, to);
}


/*
 * This array is used to update the castlePermissions member of the board
 * struct. Whenever a move is made, this operation:
 * board->castlePermissions &= castlePerms[from] & castlePerms[to];
 * is all that is needed to update the board's castle permissions. Note that
 * most squares (except the starting positions of the kings and rooks) are
 * 0xF, which causes the above operation to have no effect. This is because
 * the castle permissions of a chess board do not change if the rooks/kings
 * are not the pieces that are moving/being taken. Example: castlePerms[A1]
 * is 0xD (1101). If the rook on A1 moves/is taken, board->castlePermissions
 * will be updated from 1111 to 1101 with the above operation, which signifies
 * that white can no longer castle queenside.
 */
static inline constexpr int castlePermissions[64] = {
    0xD, 0xF, 0xF, 0xF, 0xC, 0xF, 0xF, 0xE,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0x7, 0xF, 0xF, 0xF, 0x3, 0xF, 0xF, 0xB,
};


/*
 *
 * Make a move on the chessboard. Return true if the move was a legal move and
 * it was made on the board. Return false if the move was not legal (left the
 * king in check). The move is passed in as a 32-bit integer that contains all
 * the necessary information about the move. First, store information about the
 * current position in the board's history array so the move can de undone.
 * Then, update the board's member variables to reflect the move. After the
 * move is made, make sure that the king was not left in check by the move. If
 * so, undo the move and return false. If the move was legal, return true.
 *
 */
bool Board::makeMove(int move) {
    assert(boardIsValid());
    assert(ply == (int) history.size());
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    history.emplace_back(move, castlePerms, fiftyMoveCount,
                         enPassantSquare, positionKey);
    ++ply;
    if (enPassantSquare != INVALID) {
        positionKey ^= hashkey::getEnPassantKey(enPassantSquare);
        enPassantSquare = INVALID;
    }
    if ((move & CAPTURE_FLAG) || pieces[from] == WHITE_PAWN
        || pieces[from] == BLACK_PAWN) {
        fiftyMoveCount = 0;
    } else {
        ++fiftyMoveCount;
    }
    positionKey ^= hashkey::getCastleKey(castlePerms);
    castlePerms &= castlePermissions[from] & castlePermissions[to];
    positionKey ^= hashkey::getCastleKey(castlePerms);
    switch (move & MOVE_FLAGS) {
        case CAPTURE_FLAG:
            clearPiece(to);
            break;
        case CAPTURE_AND_PROMOTION_FLAG:
            clearPiece(to);
            [[fallthrough]];
        case PROMOTION_FLAG:
            clearPiece(from);
            addPiece(from, (move >> 16) & 0xF);
            break;
        case CASTLE_FLAG:
            switch (to) {
                case G1: movePiece(H1, F1); break;
                case C1: movePiece(A1, D1); break;
                case G8: movePiece(H8, F8); break;
                case C8: movePiece(A8, D8); break;
                default: assert(false);
            }
            break;
        case PAWN_START_FLAG:
            enPassantSquare = (to + from) / 2;
            positionKey ^= hashkey::getEnPassantKey(enPassantSquare);
            break;
        case EN_PASSANT_FLAG:
            clearPiece(to + sideToMove * 16 - 8);
    }
    movePiece(from, to);
    int king = sideToMove == WHITE ? WHITE_KING : BLACK_KING;
    sideToMove = !sideToMove;
    positionKey ^= hashkey::getSideKey();
    assert(boardIsValid());
    if (squaresAttacked(pieceBitboards[king], sideToMove)) {
        undoMove();
        return false;
    }
    return true;
}


/*
 *
 * Undo the last move that was made on the board. At least one move must have
 * been made on the board before this function can be called. Information about
 * previous board states is stored in the board's history array.
 *
 */
void Board::undoMove() {
    assert(boardIsValid());
    assert(history.size() > 0);
    assert(ply == (int) history.size());
    --ply;
    sideToMove = !sideToMove;
    int move = history.back().move;
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    movePiece(to, from);
    switch (move & MOVE_FLAGS) {
        case CAPTURE_FLAG:
            addPiece(to, (move >> 12) & 0xF);
            break;
        case CAPTURE_AND_PROMOTION_FLAG:
            addPiece(to, (move >> 12) & 0xF);
            [[fallthrough]];
        case PROMOTION_FLAG:
            clearPiece(from);
            addPiece(from, pieceType[sideToMove][PAWN]);
            break;
        case CASTLE_FLAG:
            switch (to) {
                case G1: movePiece(F1, H1); break;
                case C1: movePiece(D1, A1); break;
                case G8: movePiece(F8, H8); break;
                case C8: movePiece(D8, A8); break;
            }
            break;
        case EN_PASSANT_FLAG:
            addPiece(to + sideToMove * 16 - 8, pieceType[!sideToMove][PAWN]);
    }
    castlePerms = history.back().castlePerms;
    fiftyMoveCount = history.back().fiftyMoveCount;
    enPassantSquare = history.back().enPassantSquare;
    positionKey = history.back().positionKey;
    history.pop_back();
    assert(boardIsValid());
}


/*
 * 
 * Return true if any of the squares marked by a 1 in 'squares' are attacked by
 * a piece of the side 'side'. This method generates all attack bitboards of
 * the pieces of side 'side', combines them into one bitboard, and checks if
 * this bitboard overlaps with 'squares'. If so, then there is a square in
 * 'squares' that is being attacked by 'side'. This method is used to determine
 * if a move is illegal because it left the king in check, if a castle move is
 * illegal because the king moved through an attacked square, or to determine
 * if a position is checkmate vs stalemate.
 * 
 */
bool Board::squaresAttacked(uint64 squares, int side) const {
    assert(boardIsValid());
    assert(side == WHITE || side == BLACK);
    uint64 attacks = 0ULL, knights, bishops, rooks, queens;
    if (side == WHITE) {
        attacks |= attack::getKingAttacks(pieceBitboards[WHITE_KING]);
        attacks |= attack::getWhitePawnAttacksLeft(pieceBitboards[WHITE_PAWN]);
        attacks |= attack::getWhitePawnAttacksRight(pieceBitboards[WHITE_PAWN]);
        knights = pieceBitboards[WHITE_KNIGHT];
        bishops = pieceBitboards[WHITE_BISHOP];
        rooks = pieceBitboards[WHITE_ROOK];
        queens = pieceBitboards[WHITE_QUEEN];
    } else {
        attacks |= attack::getKingAttacks(pieceBitboards[BLACK_KING]);
        attacks |= attack::getBlackPawnAttacksLeft(pieceBitboards[BLACK_PAWN]);
        attacks |= attack::getBlackPawnAttacksRight(pieceBitboards[BLACK_PAWN]);
        knights = pieceBitboards[BLACK_KNIGHT];
        bishops = pieceBitboards[BLACK_BISHOP];
        rooks = pieceBitboards[BLACK_ROOK];
        queens = pieceBitboards[BLACK_QUEEN];
    }
    uint64 allPieces = colorBitboards[BOTH_COLORS];
    while (knights) {
        attacks |= attack::getKnightAttacks(getLSB(knights));
        knights &= knights - 1;
    }
    while (bishops) {
        attacks |= attack::getBishopAttacks(getLSB(bishops), allPieces);
        bishops &= bishops - 1;
    }
    while (rooks) {
        attacks |= attack::getRookAttacks(getLSB(rooks), allPieces);
        rooks &= rooks - 1;
    }
    while (queens) {
        attacks |= attack::getQueenAttacks(getLSB(queens), allPieces);
        queens &= queens - 1;
    }
    return (attacks & squares) != 0ULL;
}


/*
 * 
 * Return true if this is the third time this position has occurred on the
 * board. We can compare the current position key with position keys stored in
 * the history array. We do not have to check all positions in the history
 * array. We only have to check positions with the same side to move (every
 * other position) up until the fifty move count was reset to 0 (because it is
 * not possible to have the same position after a capture or a pawn move). If
 * there are at least 2 positions in the history array equal to the current
 * position, then there is a 3-fold repetition on the board.
 * 
 */
bool Board::is3foldRepetition() const {
    assert(boardIsValid());
    int start = (int) history.size() - 2;
    int stop = start - (fiftyMoveCount - 2);
    int numRepetitions = 0;
    for (int i = start; i >= stop; i -= 2) {
        assert(i >= 0 && i < (int) history.size());
        numRepetitions += positionKey == history[i].positionKey;
    }
    return numRepetitions >= 2;
}


/*
 *
 * Generate an (almost) unique position key for the board. This position key
 * will be used to detect repetitions and as a key into the transposition
 * table. After every move the position key can be very quickly updated along
 * with the chessboard, which makes comparing two positions very efficient.
 *
 */
uint64 Board::generatePositionKey() const {
    uint64 key = sideToMove == WHITE ? hashkey::getSideKey() : 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        if (pieces[sq] != INVALID) {
            key ^= hashkey::getPieceKey(pieces[sq], sq);
        }
    }
    key ^= hashkey::getCastleKey(castlePerms);
    if (enPassantSquare != INVALID) {
        key ^= hashkey::getEnPassantKey(enPassantSquare);
    }
    return key;
}


/*
 * 
 * Return true if the board is a valid chessboard. Check to make sure that the
 * board's member variables have reasonable values, that the bitboards match
 * the pieces array, that the matieral counts match the pieces, etc. This
 * method currently does not allow for any variant of chess that might have
 * abnormal piece placements (such as Chess960) or abnormal amounts of pieces
 * such as Horde chess).
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
        }
        else {
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
