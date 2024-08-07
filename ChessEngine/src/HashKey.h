#pragma once

#include "Defs.h"

namespace hashkey {

    void initHashKeys();
    uint64 getSideKey();
    uint64 getPieceKey(int piece, int square);
    uint64 getCastleKey(int castlePerm);
    uint64 getEnPassantKey(int square);

};