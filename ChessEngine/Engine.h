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

class Engine {

    struct SearchInfo {
        uint64 startTime;
        uint64 stopTime;
        uint64 nodes;
        int maxDepth;
        int depthSet;  // true if we've set a depth limit for the search
        int timeSet;   // true if we've set a time limit for the search
        int movesToGo;
        bool inifinite;
        bool quit;
        bool stop;
        float fh, fhf;
    };

    Board board;
    TranspositionTable table;
    SearchInfo info;

public:
    Engine();

    bool setupBoard(const std::string& fen);
    void searchPosition();

    // Perft.cpp
    void runPerftTests() const;

private:
    void setupSearch();
    int alphaBeta(int alpha, int beta, int depth, bool max);
    int quiescence(int alpha, int beta, bool max);
    std::vector<std::string> getPVLine(int depth);
};
