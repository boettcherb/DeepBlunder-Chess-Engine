#include "table.h"
#include "defs.h"


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
int TranspositionTable::initialize() {
    assert(sizeInMB >= 0);
    if (!initialized) {
        int numEntries = (sizeInMB * 0x100000ULL) / sizeof(Entry);
        table = std::vector<Entry>(numEntries);
        initialized = true;
        return numEntries;
    }
    return 0;
}


/*
 *
 * For a position with the position key 'key', store the best move in the
 * position, the position's evaluation, the depth searched to, and the node
 * type. The node type is EXACT if the position was searched fully, UPPER_BOUND
 * if there was a beta cutoff, and LOWER_BOUND if there was an alpha cutoff.
 *
 */
void TranspositionTable::store(uint64 key, int move, int eval,
                               int depth, NodeType type) {
    assert(initialized);
    int index = static_cast<int>(key % table.size());
    table[index] = { key, move, static_cast<short>(eval),
        static_cast<unsigned char>(depth), type };
}


/*
 *
 * Retrieve a move and an evaluation from the Transposition Table for the
 * position whose position key is equal to 'key'. First, generate an index into
 * the table using the position key: key % table.size(). Then, check to see if
 * the entry at that index is for the current position. If not, we cannot
 * retrieve anything. If we are able to use the stored evaluation (eval is
 * exact, or a lower bound but still >= beta, or an upper bound but still <
 * alpha) then return the best move in the position and the evaluation. If we
 * are not able to use the stored evaluation, still return the best move so we
 * can use it for move ordering.
 *
 */
bool TranspositionTable::retrieve(uint64 key, int depth, int alpha, int beta,
                                 int& bestMove, int& eval) const {
    assert(initialized);
    int index = static_cast<int>(key % table.size());
    const Entry& entry = table[index];
    if (key == entry.key) {
        bestMove = entry.move;
        if (entry.depth >= depth) {
            if (entry.type == EXACT) {
                eval = entry.eval;
                return true;
            }
            if (entry.type == LOWER_BOUND && entry.eval >= beta) {
                eval = beta;
                return true;
            }
            if (entry.type == UPPER_BOUND && entry.eval <= alpha) {
                eval = alpha;
                return true;
            }
        }
    }
    return false;
}


/*
 * 
 * Retrieve only the best move stored for the current position. This is used to
 * get the principal variation line after the search completes.
 * 
 */
int TranspositionTable::retrieveMove(uint64 key) const {
    assert(initialized);
    int index = static_cast<int>(key % table.size());
    return table[index].key == key ? table[index].move : INVALID;
}
