#pragma once

#include <string>

#undef COMPILER_MSVS
#undef COMPILER_GCC
#if defined(_MSC_VER)
#define COMPILER_MSVS
#elif defined(__GNUC__)
#define COMPILER_GCC
#endif

#include <cassert>
#if !defined(NDEBUG)
#include <iostream>
#endif

using uint64 = unsigned long long;

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

inline constexpr int NO_PIECE = -1;
inline constexpr int NUM_PIECE_TYPES = 12;

inline constexpr int pieceColor[NUM_PIECE_TYPES] = {
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
};

inline constexpr int material[NUM_PIECE_TYPES] = {
    1, 3, 3, 5, 9, 0, 1, 3, 3, 5, 9, 0
};

inline constexpr int pieces[Color::BOTH_COLORS][NUM_PIECE_TYPES] = {
    {
        WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP,
        WHITE_ROOK, WHITE_QUEEN, WHITE_KING
    },
    {
        BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP,
        BLACK_ROOK, BLACK_QUEEN, BLACK_KING
    },
};

inline const std::string INITIAL_POSITION =
"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
