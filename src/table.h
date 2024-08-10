#pragma once

#include "defs.h"
#include <vector>

class TranspositionTable {

    struct Entry {
        uint64 key;
        int move;
    };

    std::vector<Entry> table;

public:
    TranspositionTable() = default;

    void initialize(int numEntries = 0x20000);
    void clear();
    void store(uint64 key, int move);
    int retrieve(uint64 key) const;
};
