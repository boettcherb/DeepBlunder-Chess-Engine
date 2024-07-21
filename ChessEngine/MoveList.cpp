#include "MoveList.h"
#include "Board.h"


MoveList::MoveList(const Board& board) {
    generateMoves(board);
}


void MoveList::generateMoves(const Board& board) {
    moves.clear();
    // ...
}


int MoveList::operator[](int index) const {
    assert(index > 0 && index < (int) moves.size());
    return moves[index];
}
