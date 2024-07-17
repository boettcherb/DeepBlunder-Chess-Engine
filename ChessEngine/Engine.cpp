#include "Engine.h"
#include "Board.h"
#include <string>

Engine::Engine() : board{ Board() } {
}

Engine::Engine(const std::string& starting_fen)
    : board{ Board(starting_fen) } {
}
