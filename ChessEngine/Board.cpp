#include "Board.h"
#include "Defs.h"
#include "HashKey.h"
#include "Attack.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>


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
 * permissions, and the en passant square.
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
    ply = searchPly = fiftyMoveCount = 0;
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
    ++searchPly;
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
    --searchPly;
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
 * Helper function that splits the given string 'str' into a vector of strings
 * based on the given delimneter 'delim'. This is used to parse FEN strings and
 * the piece layout portion of FEN strings.
 * 
 */
static std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> substrings;
    size_t start = 0, pos;
    while ((pos = str.find(delim, start)) != std::string::npos) {
        substrings.push_back(str.substr(start, pos - start));
        start = pos + 1;
    }
    substrings.push_back(str.substr(start));
    return substrings;
}


/*
 * 
 * Helper function to determine if the string 'str' only contains characters in
 * the string 'chars'. This is used in Board::setToFEN() to check if a token of
 * the fen string contains only valid characters.
 * 
 */
static bool containsOnly(const std::string& str, const std::string& chars) {
    return std::all_of(str.begin(), str.end(), [chars](char c) -> bool {
        return chars.find(c) != std::string::npos;
    });
}


/* 
 * 
 * Set up a chessboard to the position given by the FEN string. Forsyth–Edwards
 * Notation (FEN) is standard notation for describing a particular board
 * position of a chess game. A FEN string has 6 parts: (1) the piece layout,
 * (2) side to move, (3) castling permissions for both sides, (4) en passant
 * square (if there is one), (5) the number of half moves since the last
 * capture or pawn move, and (6) the move number. After this function executes,
 * the board should be a valid chess position that exactly matches the FEN
 * string. If something goes wrong the function prints an error message and
 * returns false.
 * 
 */
bool Board::setToFEN(const std::string& fen) {
    reset();
    std::vector<std::string> tokens = split(fen, ' ');
    if (tokens.size() != 6) {
        std::cerr << "Error: invalid fen: requires 6 tokens, found " <<
            tokens.size() << "\nFEN: \"" << fen << '\"' << std::endl;
        return false;
    }

    // piece layout token
    if (!containsOnly(tokens[0], "PNBRQKpnbrqk/12345678")) {
        std::cerr << "Error: invalid fen: invalid character found in piece "
            "layout token\nFEN: \"" << fen << '\"' << std::endl;
        std::cerr << "layout token: '" << tokens[0] << "'" << std::endl;
        return false;
    }
    std::vector<std::string> rows = split(tokens[0], '/');
    if (rows.size() != 8) {
        std::cerr << "Error: invalid fen: piece layout token does not contain "
            "8 rows\nFEN: \"" << fen << '\"' << std::endl;
        return false;
    }
    for (int row = 0; row < 8; ++row) {
        int cnt = 0;
        for (char c : rows[row]) {
            int num = c - '0';
            cnt += num >= 1 && num <= 8 ? num : 1;
        }
        if (cnt != 8) {
            std::cerr << "Error: invalid fen: invalid piece layout token\n"
                << "FEN: \"" << fen << '\"' << std::endl;
            return false;
        }
        int square = (7 - row) * 8;
        for (char c : rows[row]) {
            switch (c) {
                case 'P': pieces[square++] = WHITE_PAWN; break;
                case 'N': pieces[square++] = WHITE_KNIGHT; break;
                case 'B': pieces[square++] = WHITE_BISHOP; break;
                case 'R': pieces[square++] = WHITE_ROOK; break;
                case 'Q': pieces[square++] = WHITE_QUEEN; break;
                case 'K': pieces[square++] = WHITE_KING; break;
                case 'p': pieces[square++] = BLACK_PAWN; break;
                case 'n': pieces[square++] = BLACK_KNIGHT; break;
                case 'b': pieces[square++] = BLACK_BISHOP; break;
                case 'r': pieces[square++] = BLACK_ROOK; break;
                case 'q': pieces[square++] = BLACK_QUEEN; break;
                case 'k': pieces[square++] = BLACK_KING; break;
                default: square += c - '0';
            }
        }
    }

    // side to move token
    if (tokens[1] == "w") {
        sideToMove = WHITE;
    } else if (tokens[1] == "b") {
        sideToMove = BLACK;
    } else {
        std::cerr << "Error: invalid fen: invalid side to move token\n"
            << "FEN: \"" << fen << '\"' << std::endl;
        return false;
    }

    // castle permissions token
    std::string possibleCastlePerms[16] = {
        "-", "K", "Q", "KQ", "k", "Kk", "Qk", "KQk", "q", "Kq", "Qq", 
        "KQq", "kq", "Kkq", "Qkq", "KQkq"
    };
    for (int i = 0; i < 16; ++i) {
        if (possibleCastlePerms[i] == tokens[2]) {
            castlePerms = i;
        }
    }
    if (castlePerms == INVALID) {
        std::cerr << "Error: invalid fen: invalid castle permissions token\n"
            << "FEN: \"" << fen << '\"' << std::endl;
        return false;
    }

    // en passant square token
    if (tokens[3] == "-") {
        enPassantSquare = INVALID;
    } else {
        if (tokens[3].length() != 2 || tokens[3][0] < 'a' || tokens[3][0] > 'h' ||
            (tokens[3][1] != '3' && tokens[3][1] != '6')) {
            std::cerr << "Error: invalid fen: invalid en passant square token\n"
                << "FEN: \"" << fen << '\"' << std::endl;
            return false;
        }
        enPassantSquare = (tokens[3][0] - 'a') + (tokens[3][1] - '1') * 8;
    }


    // fifty move count token
    if (!containsOnly(tokens[4], "0123456789")) {
        std::cerr << "Error: invalid fen: invalid character found in fifty "
            "move count token\nFEN: \"" << fen << '\"' << std::endl;
        return false;
    }
    fiftyMoveCount = std::stoi(tokens[4]);
    if (fiftyMoveCount < 0 || fiftyMoveCount > 100) {
        std::cerr << "Error: invalid fen: fifty move count token should be an "
            "integer from 0 to 100\nFEN: \"" << fen << '\"' << std::endl;
        return false;
    }

    // move number token
    if (!containsOnly(tokens[5], "0123456789")) {
        std::cerr << "Error: invalid fen: invalid character found in move "
            "number token\nFEN: \"" << fen << '\"' << std::endl;
        return false;
    }

    for (int sq = 0; sq < 64; ++sq) {
        if (pieces[sq] == INVALID) continue;
        assert(pieces[sq] >= 0 && pieces[sq] < NUM_PIECE_TYPES);
        material[pieceColor[pieces[sq]]] += pieceMaterial[pieces[sq]];
        pieceBitboards[pieces[sq]] |= 1ULL << sq;
        colorBitboards[pieceColor[pieces[sq]]] |= 1ULL << sq;
        colorBitboards[BOTH_COLORS] |= 1ULL << sq;
    }
    positionKey = generatePositionKey();
    assert(boardIsValid());
    return true;
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
    if (ply < 0 || searchPly < 0) {
        std::cerr << "Ply and searchPly must be positive" << std::endl;
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
