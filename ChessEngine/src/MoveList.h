#pragma once

#include "Board.h"
#include <vector>

class MoveList {

    std::vector<int> moves;
    Board board;
    int pvMove;

public:
    MoveList(const Board& b, int bestMove = INVALID, bool onlyCaptures = false);
    int operator[](int index) const;
    int numMoves() const;
    void generateMoves(const Board& b);
    void generateCaptureMoves(const Board& b);
    bool moveExists(int move);

private:
    int getMove(int from, int to, int cap, int prom, int flags) const;
    bool validMove(int move) const;
    void addMove(int move, int score);
    void addPawnMove(int move, int score);
    void generatePieceMoves(int sq, uint64 attacks);
    void generateWhiteCastleMoves();
    void generateBlackCastleMoves();
    void generateWhitePawnMoves();
    void generateBlackPawnMoves();
    void generateWhitePawnCaptureMoves();
    void generateBlackPawnCaptureMoves();
};
