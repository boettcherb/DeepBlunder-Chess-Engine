#include "MoveList.h"
#include "Defs.h"
#include "Board.h"


/******************************************************************************
 * Each move in a MoveList is a 32-bit integer with the following information:
 * 
 * 0000 0000 0000 0000 0000 0000 0011 1111   6 bits for the 'from' square
 * 0000 0000 0000 0000 0000 1111 1100 0000   6 bits for the 'to' square
 * 0000 0000 0000 0000 1111 0000 0000 0000   4 bits for the captured piece
 * 0000 0000 0000 1111 0000 0000 0000 0000   4 bits for the promoted piece
 * 0000 0000 0001 0000 0000 0000 0000 0000   1 bit for the capture flag
 * 0000 0000 0010 0000 0000 0000 0000 0000   1 bit for the promotion flag
 * 0000 0000 0100 0000 0000 0000 0000 0000   1 bit for the castle flag
 * 0000 0000 1000 0000 0000 0000 0000 0000   1 bit for the en passant flag
 * 0000 0001 0000 0000 0000 0000 0000 0000   1 bit for the pawn start flag
 * 1111 1110 0000 0000 0000 0000 0000 0000   7 bits for the move score
 * 
 * The piece that is moving originated on the 'from' square and is moving to
 * the 'to' square. If the move is a capture move, the captured piece is
 * recorded. If the move is a pawn promotion, the piece the pawn promotes to is
 * recorded. The 5 flags are used to quickly determine the type of the move:
 * capture, promotion, castle, en passant, pawn start (when a pawn moves 2
 * squares forward, or just a normal move if none of the flags are set.
 * 
 * A move will have a higher move score if it is likely to be a good move. (Ex:
 * captures, promotions, castling). Sorting moves by their move score will help
 * the search algorithm run faster, as more pruning can occur if the best moves
 * are considered first. Note that moves score are different from position
 * scores (or position evaluations). A position score is an estimate of which
 * side is winning in a certain position, and it determines which moves are
 * actually chosen by the alpha-beta algorithm. The move score just determines
 * the order in which the algorithm considers the available moves.
 *****************************************************************************/


/*
 * 
 * MoveList constructor. Given a board, fill the movelist with every legal and
 * pseudo-legal move that is available in the current position. A pseudo-legal
 * move is a move that would normally be legal but on the current board would
 * leave the king in check. These moves are removed later as moves are made and
 * unmade.
 * 
 */
MoveList::MoveList(const Board& board) {
    generateMoves(board);
}


/*
 * 
 * Given an index i, return the i'th move in the movelist. The given index must
 * be valid (0 <= i < the number of moves in the movelist).
 * 
 */
int MoveList::operator[](int index) const {
    assert(index > 0 && index < (int) moves.size());
    return moves[index];
}


/*
 * 
 * Combine all the parts of a move into a single 32-bit integer. This saves a
 * lot of memory per move and allows many more moves to be stored in the
 * transposition table.
 * 
 */
int MoveList::getMove(int from, int to, int cap, int prom, int flags) const {
    assert(from >= 0 && from < 64);
    assert(to >= 0 && to < 64);
    assert(cap == INVALID || (cap >= 0 && cap < NUM_PIECE_TYPES));
    assert(prom == INVALID || (prom >= 0 && prom < NUM_PIECE_TYPES));
    assert((flags & ~MOVE_FLAGS) == 0);
    cap = cap & 0xF;
    prom = prom & 0xF;
    int move = from | (to << 6) | (cap << 12) | (prom << 16) | flags;
    assert(validMove(move));
    return move;
}


/*
 * 
 * Given a move that is packed into a 32-bit integer, make sure that it is a
 * valid move. For example, the captured piece cannot be a king, and if the
 * promotion flag is set, then the promoted piece must be present. This is a
 * debug function which should only be called within asserts in debug mode.
 * 
 */
bool MoveList::validMove(int move) const {
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    int cap = (move >> 12) & 0xF;
    int prom = (move >> 16) & 0xF;

    if (move & CAPTURE_FLAG) {
        // assert(cap >= 0 && cap < NUM_PIECE_TYPES);
        // assert(cap != WHITE_KING && cap != BLACK_KING);
        if (cap < 0 || cap >= NUM_PIECE_TYPES
            || cap == WHITE_KING || cap == BLACK_KING) {
            std::cout << "Invalid captured piece (1)" << std::endl;
            return false;
        }
        // assert(!(move & EN_PASSANT_FLAG));
        // assert(!(move & CASTLE_FLAG));
        // assert(!(move & PAWN_START_FLAG));
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cout << "Invalid flags (1)" << std::endl;
            return false;
        }
    }
    else {
        // assert(cap == 0xF);
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (2)" << std::endl;
            return false;
        }
    }
    if (move & PROMOTION_FLAG) {
        // assert(prom >= 0 && prom < NUM_PIECE_TYPES);
        // assert(prom != WHITE_KING && prom != BLACK_KING);
        // assert(prom != WHITE_PAWN && prom != BLACK_PAWN);
        if (prom < 0 || prom >= NUM_PIECE_TYPES || prom == WHITE_KING
            || prom == WHITE_PAWN || prom == BLACK_KING || prom == BLACK_PAWN) {
            std::cerr << "Invalid promoted piece (1)" << std::endl;
            return false;
        }
        // assert(!(move & EN_PASSANT_FLAG));
        // assert(!(move & CASTLE_FLAG));
        // assert(!(move & PAWN_START_FLAG));
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cerr << "Invalid flags (2)" << std::endl;
            return false;
        }
        // assert((from < 16 && from >= 8 && to < 8)
        //        || (from >= 48 && from < 56 && to >= 56));
        if (!((from < 16 && from >= 8 && to < 8) ||
              (from >= 48 && from < 56 && to >= 56))) {
            std::cerr << "Invalid from/to squares for promotion" << std::endl;
            return false;
        }
    }
    else {
        // assert(prom == 0xF);
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (2)" << std::endl;
            return false;
        }
    }
    if (move & CASTLE_FLAG) {
        // assert(!(move & CAPTURE_FLAG));
        // assert(!(move & PROMOTION_FLAG));
        // assert(!(move & PAWN_START_FLAG));
        // assert(!(move & EN_PASSANT_FLAG));
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (3)" << std::endl;
            return false;
        }
        // assert(cap == 0xF);
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (3)" << std::endl;
            return false;
        }
        // assert(prom == 0xF);
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (3)" << std::endl;
            return false;
        }
        // assert((from == E1 && (to == G1 || to == C1))
        //     || (from == E8 && (to == G8 || to == C8)));
        if (!(from == E1 && (to == G1 || to == C1))
            && !(from == E8 && (to == G8 || to == C8))) {
            std::cerr << "Invalid from/to squares for castling" << std::endl;
        }
    }
    if (move & PAWN_START_FLAG) {
        // assert(!(move & CAPTURE_FLAG));
        // assert(!(move & PROMOTION_FLAG));
        // assert(!(move & CASTLE_FLAG));
        // assert(!(move & EN_PASSANT_FLAG));
        if (move & (CAPTURE_AND_PROMOTION_FLAG | CASTLE_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (4)" << std::endl;
            return false;
        }
        // assert(cap == 0xF);
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (4)" << std::endl;
            return false;
        }
        // assert(prom == 0xF);
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (4)" << std::endl;
            return false;
        }
        // assert(from & 0x00FF00000000FF00);
        // assert(to & 0x000000FFFF000000);
        // assert(std::abs(from - to) == 16);
        if (!((from & 0x00FF00000000FF00) && (to & 0x000000FFFF000000)
              && (std::abs(from - to) == 16))) {
            std::cerr << "Invalid from/to squares for pawn start" << std::endl;
            return false;
        }
    }
    if (move & EN_PASSANT_FLAG) {
        // assert(!(move & CAPTURE_FLAG));
        // assert(!(move & PROMOTION_FLAG));
        // assert(!(move & CASTLE_FLAG));
        // assert(!(move & PAWN_START_FLAG));
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | CASTLE_FLAG)) {
            std::cerr << "Invalid flags (5)" << std::endl;
            return false;
        }
        // assert(cap == 0xF);
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (5)" << std::endl;
            return false;
        }
        // assert(prom == 0xF);
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (5)" << std::endl;
            return false;
        }
        // assert(from & 0x000000FFFF000000);
        // assert(to & 0x0000FF0000FF0000);
        // assert(std::abs(from - to) == 7 || std::abs(from - to) == 9);
        if (!((from & 0x000000FFFF000000) && (to & 0x0000FF0000FF0000)
              && (std::abs(from - to) == 7 || std::abs(from - to) == 9))) {
            std::cerr << "Invalid from/to squares for en passant" << std::endl;
            return false;
        }
    }
    return true;
}


void MoveList::generateMoves(const Board& board) {
    moves.clear();
    // ...
}
