#pragma once

#include "board.h"
#include <vector>

class MoveList {

    struct Move {
        int move;
        int score;
        Move() = default;
        Move(int m, int s) : move{ m }, score{ s } {}
    };

    std::vector<Move> moves;
    Board board;

public:
    MoveList(const Board& b, bool onlyCaptures = false);
    int operator[](int index) const;
    int numMoves() const;
    void generateMoves(const Board& b);
    void generateCaptureMoves(const Board& b);
    void orderMoves(int bestMove, int killers[MAX_SEARCH_DEPTH][2],
                    int searchHistory[NUM_PIECE_TYPES][64]);
    bool moveExists(int move);

private:
    int getMove(int from, int to, int cap, int prom, int flags) const;
    bool validMove(int move) const;
    void addPawnMove(int move, int score);
    void generatePieceMoves(int sq, uint64 attacks);
    void generateWhiteCastleMoves();
    void generateBlackCastleMoves();
    void generateWhitePawnMoves();
    void generateBlackPawnMoves();
    void generateWhitePawnCaptureMoves();
    void generateBlackPawnCaptureMoves();
};
