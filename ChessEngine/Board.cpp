#include "Board.h"
#include "Defs.h"
#include "HashKey.h"
#include <string>
#include <algorithm>
#include <vector>

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
 * Reset the board's member variables to their default values. sideToMove is
 * set to BOTH_COLORS so that an error occurs if it is not set to either WHITE
 * or BLACK during initialization. castlePerms, enPassantSquare, and the pieces
 * in the pieces array are set to INVALID. All other default values are 0.
 *
 */
void Board::reset() {
    std::fill_n(pieceBitboards, NUM_PIECE_TYPES, 0);
    std::fill_n(colorBitboards, 3, 0);
    std::fill_n(pieces, 64, static_cast<unsigned char>(INVALID));
    sideToMove = Color::BOTH_COLORS;
    castlePerms = enPassantSquare = INVALID;
    ply = searchPly = fiftyMoveCount = 0;
    material[0] = material[1] = 0;
    positionKey = 0;
    history.clear();
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
    if (tokens[3] != "-" && (tokens[3].length() != 2 || tokens[3][0] < 'a' ||
        tokens[3][0] > 'h' || (tokens[3][1] != '3' && tokens[3][1] != '6'))) {
        std::cerr << "Error: invalid fen: invalid en passant square token\n"
            << "FEN: \"" << fen << '\"' << std::endl;
        return false;
    }
    enPassantSquare = (tokens[3][0] - 'a') * 8 + (tokens[3][1] - '1');

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
        colorBitboards[pieceColor[BOTH_COLORS]] |= 1ULL << sq;
    }
    positionKey = generatePositionKey();
    assert(validBoard());
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
uint64 Board::generatePositionKey() {
    uint64 key = sideToMove == WHITE ? hashkey::getSideKey() : 0ULL;
    for (int sq = 0; sq < 64; ++sq) {
        if (pieces[sq] != INVALID) {
            key ^= hashkey::getPieceKey(pieces[sq], sq);
        }
    }
    key ^= hashkey::getCastleKey(castlePerms);
    if (enPassantSquare != INVALID) {
        key |= hashkey::getEnPassantKey(enPassantSquare);
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
bool Board::validBoard() {
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
        for (int pieceType = 0; pieceType < NUM_PIECE_TYPES; ++pieceType) {
            if (pieceBitboards[pieceType] & (1ULL << sq)) {
                ++count;
                piece = pieceType;
            }
        }
        if (count > 1 || piece != pieces[sq]) {
            std::cerr << "Invalid pieces[] array" << std::endl;
            return false;
        }
        if (piece != INVALID) {
            ++pieceCount[piece];
            if (pieceColor[piece] == WHITE) {
                materialWhite += pieceColor[piece];
            } else {
                materialBlack += pieceColor[piece];
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
        if (sideToMove == WHITE) {
            if ((1ULL << enPassantSquare) & 0xFFFF00FFFFFFFFFF ||
                pieces[enPassantSquare - 8] != BLACK_PAWN) {
                std::cerr << "Invalid en passant square (2)" << std::endl;
                return false;
            }
        }
        else {
            if ((1ULL << enPassantSquare) & 0xFFFFFFFFFF00FFFF ||
                pieces[enPassantSquare + 8] != WHITE_PAWN) {
                std::cerr << "Invalid en passant square (3)" << std::endl;
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
        if (pieces[E8] != WHITE_KING || pieces[A8] != BLACK_ROOK) {
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
