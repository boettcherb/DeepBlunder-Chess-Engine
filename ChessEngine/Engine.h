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
    uint64 nodes;     // the total number of nodes visited in the search tree

    bool stop;        // true if we should stop the current search

    bool timeSet;     // true if we've set a time limit for the search
    uint64 startTime; // the time the search started
    uint64 stopTime;  // the time we should stop searching (if timeset is true)

    int maxDepth;     // the max depth to search to (if specified)

    float fh, fhf;    // fail high and fail high first. The ratio fhf/fh is
                      // used to see how good our move ordering is in the
                      // alpha-beta algorithm. > 0.9 is optimal

    int inc[2];
    int time[2];
    int movetime;
    int movestogo;

    SearchInfo();
};


class Engine {

    Board board;
    TranspositionTable table;
    SearchInfo info;

public:
    Engine();

    void initialize();
    bool setupBoard(const std::string& fen = START_POS);
    void makeMoves(const std::vector<std::string>& moves);
    void searchPosition(const SearchInfo& searchInfo);
    void stopSearch();

    // Perft.cpp
    void runPerftTests() const;

private:
    int parseMoveString(const std::string& moveString) const;
    void setupSearch();
    void checkup();
    int alphaBeta(int alpha, int beta, int depth, bool max);
    int quiescence(int alpha, int beta, bool max);
    std::vector<std::string> getPVLine(int depth);
};
