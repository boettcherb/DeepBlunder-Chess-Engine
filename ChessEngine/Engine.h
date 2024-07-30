#pragma once

#include "Board.h"
#include <string>

class Engine {

    Board board;

public:
    Engine();

    bool setupBoard(const std::string& fen);
    void runPerftTests() const;
};
