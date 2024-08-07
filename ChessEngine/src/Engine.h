#pragma once

#include "Board.h"
#include "Table.h"
#include <string>
#include <vector>


/*
 * 
 * A struct that stores information about the current search and information
 * about the current state of the game clock. The values maxDepth, inc, time,
 * movetime, and movestogo are given as input from the GUI via the UCI protocol.
 * These values allow us to configure the search to only go to a certain depth
 * or to only run for a certain amount of time (this is done in setupSearch()).
 * 
 * nodes:     The total number of nodes visited in the search tree.
 * stop:      True if we should stop the current search.
 * timeSet:   True if there is a time limit for the search.
 * startTime: The time the search started.
 * stopTime:  The time we should stop searching (if timeSet is true).
 * maxDepth:  The maximum depth to search to (if specified).
 * inc:       The time increment added to the clock per move (for both sides).
 * time:      The time remaining on the clock (for both sides).
 * movetime:  The time that should be spent on the current move.
 * movestogo: The number of moves remaining for the current time control.
 * fh, fhf:   fail high and fail high first. The ratio fhf/fh is used to see
 *            how good our move ordering is in the Alpha-Beta algorithm.
 * 
 */
struct SearchInfo {
    uint64 nodes;
    bool stop;
    bool timeSet;
    uint64 startTime;
    uint64 stopTime;
    int maxDepth;
    int inc[2];
    int time[2];
    int movetime;
    int movestogo;
#ifndef NDEBUG
    float fh, fhf;    
#endif
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
    int alphaBeta(int alpha, int beta, int depth);
    int quiescence(int alpha, int beta);
    std::vector<std::string> getPVLine(int depth);
};