#include "Engine.h"
#include "Board.h"
#include "HashKey.h"
#include <string>

/*
 * 
 * Engine constructor. First, call initHashKeys to generate the hash keys.
 * Then, If a starting position was given (as a FEN string), setup the board
 * according to that position.
 * 
 */
Engine::Engine() {
    static_assert(sizeof(uint64) == 8);
    hashkey::initHashKeys();
}
Engine::Engine(const std::string& starting_fen) :Engine() {
    setupBoard(starting_fen);
}


/*
 * 
 * Setup the board according to the given position by calling Board::setToFEN()
 * 
 */
void Engine::setupBoard(const std::string& fen) {
    board.setToFEN(fen);
}
