#pragma once

#include "Board.h"
#include <string>

class Engine {

    Board board;

public:
    Engine();
    Engine(const std::string& starting_fen);

    void setupBoard(const std::string& fen);
};
