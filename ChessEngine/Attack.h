#pragma once

#include "Defs.h"

namespace attack {

    void initializeBishopAttackTable();
    void initializeRookAttackTable();

    uint64 getBishopAttacks(int sq, uint64 allPieces);
    uint64 getRookAttacks(int sq, uint64 allPieces);
    uint64 getQueenAttacks(int sq, uint64 allPieces);

};
