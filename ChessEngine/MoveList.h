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
    void addMove(int move, int score);
    void addPawnMove(const Board& board, int move, int score);
    void generatePieceMoves(const Board& board, int sq, uint64 attacks);
    void generateWhiteCastleMoves(const Board& board);
    void generateBlackCastleMoves(const Board& board);
    void generateWhitePawnMoves(const Board& board);
    void generateBlackPawnMoves(const Board& board);
};
