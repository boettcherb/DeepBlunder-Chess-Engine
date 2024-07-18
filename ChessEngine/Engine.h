#pragma once

#include "Board.h"
#include "MoveGenerator.h"
#include "Search.h"

#include <string>

class Engine {

    Board board;
    MoveGenerator moveGen;
    Search search;

public:
    Engine();
    Engine(const std::string& starting_fen);

    void setupBoard(const std::string& fen);
};
