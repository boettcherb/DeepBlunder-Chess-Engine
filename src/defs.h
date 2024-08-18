#pragma once

using uint64 = unsigned long long;

#include <string>
#include <cassert>

#if !defined(NDEBUG)
#include <iostream>
    // debug.cpp
    void printBitboard(uint64 bitboard);
    std::string getSquareString(int square);
#endif


enum Color {
    WHITE, BLACK, BOTH_COLORS,
};

enum Piece {
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP,
    WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP,
    BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};

inline constexpr int INVALID = -1;
inline constexpr int NUM_PIECE_TYPES = 12;
inline constexpr int MAX_SEARCH_DEPTH = 128;
inline constexpr int MOVE_FLAGS = 0x1F00000;
inline constexpr int CAPTURE_FLAG = 0x0100000;
inline constexpr int PROMOTION_FLAG = 0x0200000;
inline constexpr int CAPTURE_AND_PROMOTION_FLAG = 0x0300000;
inline constexpr int CASTLE_FLAG = 0x0400000;
inline constexpr int EN_PASSANT_FLAG = 0x0800000;
inline constexpr int PAWN_START_FLAG = 0x1000000;

inline constexpr int CASTLE_WK = 0x1;
inline constexpr int CASTLE_WQ = 0x2;
inline constexpr int CASTLE_BK = 0x4;
inline constexpr int CASTLE_BQ = 0x8;

inline constexpr int defaultPieces[64] = {
    WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK,
    WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,  WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,
    INVALID,    INVALID,      INVALID,      INVALID,     INVALID,    INVALID,      INVALID,      INVALID,
    INVALID,    INVALID,      INVALID,      INVALID,     INVALID,    INVALID,      INVALID,      INVALID,
    INVALID,    INVALID,      INVALID,      INVALID,     INVALID,    INVALID,      INVALID,      INVALID,
    INVALID,    INVALID,      INVALID,      INVALID,     INVALID,    INVALID,      INVALID,      INVALID,
    BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,  BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,
    BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK,
};

inline constexpr int pieceColor[NUM_PIECE_TYPES] = {
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
};

inline constexpr int pieceMaterial[NUM_PIECE_TYPES] = {
    100, 325, 330, 500, 900, 0, 100, 325, 330, 500, 900, 0
};
inline constexpr int startingMaterial = 4010;

inline constexpr int pieceType[Color::BOTH_COLORS][NUM_PIECE_TYPES] = {
    {
        WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP,
        WHITE_ROOK, WHITE_QUEEN, WHITE_KING
    },
    {
        BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP,
        BLACK_ROOK, BLACK_QUEEN, BLACK_KING
    },
};

inline constexpr char pieceChar[NUM_PIECE_TYPES] = {
    'p', 'n', 'b', 'r', 'q', 'k', 'p', 'n', 'b', 'r', 'q', 'k'
};

int getLSB(uint64 bitboard);
int getMSB(uint64 bitboard);
int countBits(uint64 bitboard);
uint64 currentTime();
