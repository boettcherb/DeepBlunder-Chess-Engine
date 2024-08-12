#include "table.h"


TranspositionTable::TranspositionTable() {
    sizeInMB = -1;
    initialized = false;
}


/*
* 
* Set the size of the transposition table, in MB. This is called whenever we
* receive a "setoption name Hash ..." command from the UCI protocol.
* 
*/
void TranspositionTable::setSize(int size) {
    assert(size >= 0);
    sizeInMB = size;
    initialized = false;
}


/*
 *
 * Initialize the transposition table with a size of 'sizeInMB' megabytes. The
 * default size is 128 MB, but this can be changed through the UCI protocol's
 * "setoption" command.
 *
 */
void TranspositionTable::initialize() {
    assert(sizeInMB >= 0);
    if (!initialized) {
        uint64 numEntries = (sizeInMB * 0x100000ULL) / sizeof(Entry);
        table = std::vector<Entry>(numEntries);
        initialized = true;
    }
}


/*
 *
 * Store a move in the transposition table. The move being stored is the best
 * move found by the Alpha-Beta algorithm for the position whose position key
 * is equal to 'key'. An index into the table is found using the position key:
 * key % table.size(). Then, at that index we store the best move along with
 * the position key. Note that we are overriding the previous value at this
 * index. The key is stored along with the move so that when we retrieve the
 * move, we know that it is for the correct position.
 * 
 */
void TranspositionTable::store(uint64 key, int move) {
    assert(initialized);
    int index = static_cast<int>(key % table.size());
    table[index] = { key, move };
}


/*
 * 
 * Retrieve a move from the transposition table for the position whose position
 * key is equal to 'key'. First, generate an index into the table using the
 * position key: key % table.size(). Then, check to see if the move stored at
 * that index belongs to the current position by comparing the key stored at 
 * that index to 'key'. If they match, then return the move. If they do not
 * match, then return -1.
 * 
 */
int TranspositionTable::retrieve(uint64 key) const {
    assert(initialized);
    int index = static_cast<int>(key % table.size());
    if (key == table[index].key) {
        return table[index].move;
    }
    return INVALID;
}
