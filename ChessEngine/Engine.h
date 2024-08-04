#pragma once

#include "Board.h"
#include "Table.h"
#include <string>
#include <vector>


/*
 * 
 * A struct passed in to the Alpha-Beta search function that stores information
 * about the search, and that also allows the user/GUI to control when to stop
 * the search.
 * 
 * TODO: explain all of these variables
 * 
 */
struct SearchInfo {
    uint64 startTime;
    uint64 stopTime;
    uint64 nodes;
    int maxDepth;
    int depthSet;
    int timeSet;
    int movesToGo;
    bool inifinite;
    bool quit;
    bool stop;
    float fh, fhf;
};

class Engine {

    Board board;
    TranspositionTable table;

public:
    Engine();

    bool setupBoard(const std::string& fen);
    void searchPosition(SearchInfo& info);

    // Perft.cpp
    void runPerftTests() const;

private:
    void setupSearch(SearchInfo& info);
    int alphaBeta(SearchInfo& info, int alpha, int beta, int depth, bool max);
    std::vector<std::string> getPVLine(int depth);
};
