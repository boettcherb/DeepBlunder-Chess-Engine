#pragma once

#include "defs.h"
#include <vector>

class TranspositionTable {

    struct Entry {
        uint64 key;
        int move;
    };

    std::vector<Entry> table;
    int sizeInMB;
    bool initialized;

public:
    TranspositionTable();

    void setSize(int sizeInMB);
    void initialize();
    void store(uint64 key, int move);
    int retrieve(uint64 key) const;
};
