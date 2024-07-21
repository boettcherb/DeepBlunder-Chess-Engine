#pragma once

#include "Board.h"
#include <vector>

class MoveList {

    std::vector<int> moves;

public:
    MoveList() = default;
    MoveList(const Board& board);
    void generateMoves(const Board& board);
    int operator[](int index) const;
};
