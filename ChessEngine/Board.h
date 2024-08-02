#pragma once

#include "Defs.h"
#include <string>
#include <vector>


/*
 * 
 * In this engine, a chessboard is represented using bitboards. Each bitboard
 * is a 64-bit number where the least significant bit (bit 0) represents square
 * A1, bit 1 represents square B1, ..., and the most significant bit (bit 63)
 * represents square H8. A bit is set to 1 if there is a piece on that square.
 *
 * pieceBitboards:    The bitboards for this chessboard. There are 12 of them,
 *                    one for each piece type.
 * colorBitboards:    3 bitboards to represent (1) all the white pieces, (2)
 *                    all the black pieces, and (3) all pieces of both colors.
 *                    These bitboards are used often and it is more efficient
 *                    to keep them updated with the piece bitboards as moves
 *                    are made then to calculate them when needed.
 * pieces:            An array of 64 chars to hold the piece type for each
 *                    square. This allows quick access of the piece type of a
 *                    given square and is also updated incrementally with the
 *                    piece bitboards.
 * sideToMove:        An integer that is either 0 (white) or 1 (black) denoting
 *                    whose turn it is in the current position.
 * ply:               An integer holding the number of half moves made to get
 *                    to the current board position.
 * searchPly:         An integer holding the number of half moves made in the
 *                    current search by the AlphaBeta algorithm.
 * castlePerms:       A combination of bit flags denoting which castling moves
 *                    are legal. Ex: If (castlePerms & CASTLE_WQ != 0), then
 *                    white can castle queenside in the current position.
 * fiftyMoveCount:    An integer holding the number of half moves since the
 *                    last capture or pawn move. Used for the fifty move rule.
 * enPassantSquare:   An integer holding the square that the current side to
 *                    move could attack by the en passant rule. NO_PIECE if
 *                    en passant is not possible in the current position.
 * material:          Two integers holding the overall material for each side.
 *                    (Q=9, R=5, B=3, N=3, P=1). Updated incrementally as moves
 *                    are made and unmade.
 * history:           A vector of PrevMove structs that stores info about how
 *                    to un-make each move made to get to the current position.
 * positionKey:       A 64-bit integer that is used as a hash key into the
 *                    board's transposition table. Updated incrementally as
 *                    moves are made and unmade.
 * 
 */
class Board {

    struct PrevMove {
        int move;
        int castlePerms;
        int fiftyMoveCount;
        int enPassantSquare;
        uint64 positionKey;
        PrevMove(int m, int c, int f, int e, uint64 p);
    };

    uint64 pieceBitboards[NUM_PIECE_TYPES];
    uint64 colorBitboards[3];
    int pieces[64];
    int sideToMove;
    int ply, searchPly;
    int castlePerms;
    int fiftyMoveCount;
    int enPassantSquare;
    int material[2];
    std::vector<PrevMove> history;
    uint64 positionKey;

public:
    Board();
    Board(const std::string& starting_fen);

    int operator[](int square) const;
    int side() const;
    uint64 getPieceBitboard(int piece) const;
    uint64 getColorBitboard(int color) const;
    int getCastlePerms() const;
    int getEnPassantSquare() const;

    void reset();
    bool makeMove(int move);
    void undoMove();
    bool squaresAttacked(uint64 squares, int side) const;
    bool setToFEN(const std::string& fen);

    // Evaluate.cpp
    int evaluatePosition() const;

private:
    void addPiece(int square, int piece);
    void clearPiece(int square);
    void movePiece(int from, int to);
    uint64 generatePositionKey() const;
    bool boardIsValid() const;
};
