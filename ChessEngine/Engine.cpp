#include "Engine.h"
#include "Board.h"
#include "HashKey.h"
#include "Attack.h"
#include <string>


/*
 * 
 * Engine constructor. Initialize the engine by initializing the hashkeys and
 * the bishop and rook attack tables.
 * 
 */
Engine::Engine() {
    static_assert(sizeof(uint64) == 8);
    hashkey::initHashKeys();
    attack::initializeBishopAttackTable();
    attack::initializeRookAttackTable();
}


/*
 * 
 * Setup the board according to the given position by calling setToFEN(). If
 * the position cannot be set up, return false.
 * 
 */
bool Engine::setupBoard(const std::string& fen) {
    return board.setToFEN(fen);
}
