#pragma once

#include "defs.h"
#include <vector>

enum NodeType : unsigned char {
    EXACT, UPPER_BOUND, LOWER_BOUND, NONE
};

class TranspositionTable {


    struct Entry {
        uint64 key;
        int move;
        short eval;
        unsigned char depth;
        NodeType type;
    };

    std::vector<Entry> table;
    int sizeInMB;
    bool initialized;

public:
    TranspositionTable();

    void setSize(int sizeInMB);
    int initialize();
    void store(uint64 key, int move, int score,
               int depth, NodeType type);
    bool retrieve(uint64 key, int depth, int alpha, int beta,
                  int& bestMove, int& eval) const;
};
