#pragma once

#include "Defs.h"

namespace attack {

    void initializeBishopAttackTable();
    void initializeRookAttackTable();

    uint64 getKingAttacks(uint64 king);
    uint64 getKnightAttacks(int square);
    uint64 getWhitePawnAttacksRight(uint64 pawns);
    uint64 getWhitePawnAttacksLeft(uint64 pawns);
    uint64 getBlackPawnAttacksRight(uint64 pawns);
    uint64 getBlackPawnAttacksLeft(uint64 pawns);
    uint64 getBishopAttacks(int sq, uint64 allPieces);
    uint64 getRookAttacks(int sq, uint64 allPieces);
    uint64 getQueenAttacks(int sq, uint64 allPieces);

};
