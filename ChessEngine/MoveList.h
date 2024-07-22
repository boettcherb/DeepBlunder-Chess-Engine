#pragma once

#include "Board.h"
#include <vector>

class MoveList {

    std::vector<int> moves;

public:
    MoveList(const Board& board);
    int operator[](int index) const;
    void generateMoves(const Board& board);

private:
    int getMove(int from, int to, int cap, int prom, int flags) const;
    bool validMove(int move) const;
};
