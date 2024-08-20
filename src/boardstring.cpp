#include "defs.h"
#include "board.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>


/*
 *
 * Return a string representing a move. The string is just a combination of the
 * 'from' and 'to' squares. For example, moving a knight from f3 to g5 would
 * return "f3g5". A pawn capture from e7 to d8 that results in a queen would
 * return "e7d8q".
 *
 */
std::string Board::getMoveString(int move) const {
    assert(boardIsValid());
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    std::string moveString;
    moveString.push_back('a' + static_cast<char>(from % 8));
    moveString.push_back('1' + static_cast<char>(from / 8));
    moveString.push_back('a' + static_cast<char>(to % 8));
    moveString.push_back('1' + static_cast<char>(to / 8));
    if (move & PROMOTION_FLAG) {
        int promotedPiece = (move >> 16) & 0xF;
        assert(promotedPiece >= 0 && promotedPiece < NUM_PIECE_TYPES);
        moveString.push_back(pieceChar[promotedPiece]);
    }
    return moveString;
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
    if (fen == START_POS) {
        reset();
        return true;
    }
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
                default:
                    int spaces = static_cast<int>(c - '0');
                    while (spaces--) {
                        pieces[square++] = NO_PIECE;
                    }
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

    for (int type = 0; type < NUM_PIECE_TYPES; ++type) {
        pieceBitboards[type] = 0ULL;
    }
    colorBitboards[WHITE] = colorBitboards[BLACK] = 0ULL;
    colorBitboards[BOTH_COLORS] = 0ULL;
    material[WHITE] = material[BLACK] = 0;

    for (int sq = 0; sq < 64; ++sq) {
        if (pieces[sq] == NO_PIECE) continue;
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
