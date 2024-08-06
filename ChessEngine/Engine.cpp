#include "Engine.h"
#include "Board.h"
#include "MoveList.h"
#include "HashKey.h"
#include "Attack.h"
#include <string>
#include <vector>
#include <iostream>

static inline constexpr int INF = 1000000000;
static inline constexpr int MATE = 30000;

/*
 * 
 * Constructor for SearchInfo. Initialize all member variables to default
 * values. These variables should be set to reasonable values in setupSearch()
 * before every search.
 * 
 */
SearchInfo::SearchInfo() {
    nodes = startTime = stopTime = 0;
    stop = false;
    timeSet = false;
    maxDepth = -1;
    fh = fhf = 0.0f;
    inc[WHITE] = inc[BLACK] = 0;
    time[WHITE] = time[BLACK] = -1;
    movetime = -1;
    movestogo = 30;
}


/*
 * 
 * Engine constructor. Create the engine but don't initialize anything yet
 * (wait for the UCI protocol to tell us to initialize).
 * 
 */
Engine::Engine() {
    static_assert(sizeof(uint64) == 8);
}


/*
 * 
 * Initialize the engine. Create the hash keys for zobrist hashing, the bishop
 * and rook sliding piece attack tables, and the transposition table.
 * 
 */
void Engine::initialize() {
    hashkey::initHashKeys();
    attack::initializeBishopAttackTable();
    attack::initializeRookAttackTable();
    table.initialize();
}


/*
 * 
 * Setup the board according to the given position by calling setToFEN(). The
 * default position is the starting position. If the position cannot be set up,
 * return false.
 * 
 */
bool Engine::setupBoard(const std::string& fen) {
    return board.setToFEN(fen);
}


/*
* 
* Convert a moveString into an integer representing the move. A movestring is
* made up of the 'from' and 'to' squares of the move. For example: e2e4, e1g1,
* b7b8q, etc.
* 
*/
int Engine::parseMoveString(const std::string& moveString) const {
    assert(moveString.length() == 4 || moveString.length() == 5);
    int from = static_cast<int>(moveString[1] - '1') * 8;
    from += static_cast<int>(moveString[0] - 'a');
    int to = static_cast<int>(moveString[3] - '1') * 8;
    to += static_cast<int>(moveString[2] - 'a');
    assert(from >= 0 && from < 64);
    assert(to >= 0 && to < 64);
    MoveList moveList(board);
    int numMoves = moveList.numMoves();
    for (int i = 0; i < numMoves; ++i) {
        int move = moveList[i];
        if ((move & 0x3F) == from && ((move >> 6) & 0x3F) == to) {
            if (moveString.length() == 4) {
                return move;
            }
            assert(move & PROMOTION_FLAG);
            int promotedPiece = (move >> 16) & 0xF;
            assert(promotedPiece >= 0 && promotedPiece < NUM_PIECE_TYPES);
            if (moveString[4] == pieceChar[promotedPiece]) {
                return move;
            }
        }
    }
    return INVALID;
}


/*
 * 
 * Given a list of move strings, make each move in order. The UCI protocol may
 * ask us to set up the board with a starting position, plus a list of moves
 * made from that position. This function takes in a list of moves, converts
 * them from strings to integers, then makes the moves on the board.
 * 
 */
void Engine::makeMoves(const std::vector<std::string>& moves) {
    for (const std::string& moveString : moves) {
        int move = parseMoveString(moveString);
        assert(move != INVALID);
        board.makeMove(move);
    }
}


/*
 * 
 * Look in the hash table for the best line (principal variation) from the
 * current position, up to a certain depth. The principal variation is the best
 * sequence of moves available for the current side to move. Normally, the line
 * will be of length 'depth' but it is possible for it to be shorter if there
 * was a hash collision and one of the moves in the hash table was overridden.
 * 
 */
std::vector<std::string> Engine::getPVLine(int depth) {
    std::vector<std::string> moves;
    while (depth--) {
        int storedMove = table.retrieve(board.getPositionKey());
        if (storedMove == INVALID) {
            break;
        }
        MoveList moveList(board);
        if (!moveList.moveExists(storedMove)) {
            break;
        }
        moves.push_back(board.getMoveString(storedMove));
#ifndef NDEBUG
        bool moveMade = board.makeMove(storedMove);
        assert(moveMade);
#else
        board.makeMove(storedMove);
#endif
    }
    for (int i = 0; i < (int) moves.size(); ++i) {
        board.undoMove();
    }
    return moves;
}


/*
* 
* Call this function to stop the current search. This function must be used if
* the search is not based on a specific time per move or a maximum depth. This
* is called whenever we receive a "stop" or "quit" command from the GUI via the
* UCI protocol.
* 
*/
void Engine::stopSearch() {
    info.stop = true;
}


/*
 * 
 * Setup the search time controls based on information passed in from the GUI
 * via the UCI protocol. 
 * 
 */
void Engine::setupSearch() {
    info.nodes = 0;
    info.stop = false;
    info.fh = info.fhf = 0.0f;
    info.startTime = currentTime();

    int side = board.side();
    if (info.movetime != -1) {
        info.time[side] = info.movetime;
        info.movestogo = 1;
    }
    if (info.maxDepth == -1) {
        info.maxDepth = INF;
    }
    if (info.time[side] != -1) {
        info.timeSet = true;
        info.time[side] /= info.movestogo;
        info.time[side] -= 50;
        info.stopTime = info.startTime + info.time[side] + info.inc[side];
    }

    std::cout << "timeSet: " << info.timeSet << ", ";
    std::cout << "time: " << info.time[side] << ", ";
    std::cout << "startTime: " << info.startTime << ", ";
    std::cout << "stopTime: " << info.stopTime << ", ";
    std::cout << "depth: " << info.maxDepth << std::endl;
}


/*
 * 
 * Check if we have run out of time. This is called in the Alpha-Beta and
 * quiescence functions every 4096 nodes searched.
 * 
 */
void Engine::checkup() {
    if (info.timeSet && currentTime() > info.stopTime) {
        info.stop = true;
    }
}


/*
 * 
 * Quiescence Search. Search only capture moves in order to find a 'quiet'
 * position. This function is used to eliminate the Horizon Effect. Say white
 * captures a pawn with a queen on the final depth of the Alpha-Beta search.
 * White would evaluate the position as being up a pawn. However, that pawn may
 * have been defended, so on the next move white would have lost a queen.
 * Quiesence search is very similar to Alpha-Beta search except we do not have a
 * max depth and we only consider capture moves. We also do not check for 
 * checkmate or stalemate since not all moves are being generated.
 * 
 */
int Engine::quiescence(int alpha, int beta, bool max) {
    if ((info.nodes & 0xFFF) == 0) {
        checkup();
    }
    ++info.nodes;
    if (board.getFiftyMoveCount() >= 100 || board.isRepetition()) {
        return 0;
    }
    int eval = board.evaluatePosition();
    if (max) {
        assert(board.side() == WHITE);
        if (eval > alpha) {
            alpha = eval;
            if (beta <= alpha) {
                return beta;
            }
        }
        MoveList moveList(board, true);
        // TODO: retrieve the bestMove for the current position from the transposition
        // table and make it first in the movelist ordering
        int numMoves = moveList.numMoves(), legalMoves = 0;
        int bestMove = INVALID;
        int oldAlpha = alpha;
        for (int i = 0; i < numMoves; ++i) {
            assert((moveList[i] & CAPTURE_FLAG) || (moveList[i] & EN_PASSANT_FLAG));
            if (board.makeMove(moveList[i])) {
                eval = quiescence(alpha, beta, false);
                ++legalMoves;
                board.undoMove();
                if (info.stop) {
                    return 0;
                }
                if (eval > alpha) {
                    alpha = eval;
                    if (beta <= alpha) {
                        info.fhf += legalMoves == 1;
                        ++info.fh;
                        return beta;
                    }
                    bestMove = moveList[i];
                }
            }
        }
        if (alpha != oldAlpha) {
            assert(bestMove != INVALID);
            table.store(board.getPositionKey(), bestMove);
        }
        return alpha;
    }
    assert(board.side() == BLACK);
    if (eval < beta) {
        beta = eval;
        if (beta <= alpha) {
            return alpha;
        }
    }
    MoveList moveList(board, true);
    // TODO: retrieve the bestMove for the current position from the transposition
    // table and make it first in the movelist ordering
    int numMoves = moveList.numMoves(), legalMoves = 0;
    int bestMove = INVALID;
    int oldBeta = beta;
    for (int i = 0; i < numMoves; ++i) {
        assert((moveList[i] & CAPTURE_FLAG) || (moveList[i] & EN_PASSANT_FLAG));
        if (board.makeMove(moveList[i])) {
            eval = quiescence(alpha, beta, true);
            ++legalMoves;
            board.undoMove();
            if (info.stop) {
                return 0;
            }
            if (eval < beta) {
                beta = eval;
                if (beta <= alpha) {
                    info.fhf += legalMoves == 1;
                    ++info.fh;
                    return alpha;
                }
                bestMove = moveList[i];
            }
        }
    }
    if (beta != oldBeta) {
        assert(bestMove != INVALID);
        table.store(board.getPositionKey(), bestMove);
    }
    return beta;
}


/*
 *
 * The Alpha-Beta search algorithm. Recursively search to depth 'depth' for the
 * best move in the current position. Return the evaluation of the current
 * position based on the best sequence of moves for the current side to move.
 * 
 * Alpha is the best possible score for white in the starting position, and
 * beta is the best possible score for black in the starting position. If it is
 * white's move and we find a position with an evaluation > alpha, we update
 * alpha. Similarly, if it is black's move and the evaluation is < beta, we
 * update beta. However, if alpha becomes >= beta, then we've encountered a
 * position that is 'too good' for white. Black has a chance to go down a
 * different branch with a lower evaluation, and so black will never go down
 * this branch, which means it can be pruned.
 * 
 * The bool 'max' is true if it is currently the maximizing player's move (ie.
 * white's move) and false if it is the minimizing player's (black's) move.
 * 
 */
int Engine::alphaBeta(int alpha, int beta, int depth, bool max) {
    if (depth <= 0) {
        return quiescence(alpha, beta, max);
    }
    if ((info.nodes & 0xFFF) == 0) {
        checkup();
    }
    ++info.nodes;
    if (board.getFiftyMoveCount() >= 100 || board.isRepetition()) {
        return 0;
    }
    MoveList moveList(board);
    // TODO: retrieve the bestMove for the current position from the transposition
    // table and make it first in the movelist ordering
    int numMoves = moveList.numMoves(), legalMoves = 0;
    int bestMove = INVALID;
    int oldAlpha = alpha, oldBeta = beta;
    if (max) {
        assert(board.side() == WHITE);
        for (int i = 0; i < numMoves; ++i) {
            if (board.makeMove(moveList[i])) {
                int eval = alphaBeta(alpha, beta, depth - 1, false);
                ++legalMoves;
                board.undoMove();
                if (info.stop) {
                    return 0;
                }
                if (eval > alpha) {
                    alpha = eval;
                    if (beta <= alpha) {
                        info.fhf += legalMoves == 1;
                        ++info.fh;
                        return beta;
                    }
                    bestMove = moveList[i];
                }
            }
        }
        if (legalMoves == 0) {
            uint64 king = board.getPieceBitboard(WHITE_KING);
            if (board.squaresAttacked(king, BLACK)) {
                return -(MATE + depth);
            }
            return 0;
        }
        if (alpha != oldAlpha) {
            assert(bestMove != INVALID);
            table.store(board.getPositionKey(), bestMove);
        }
        return alpha;
    }
    assert(board.side() == BLACK);
    for (int i = 0; i < numMoves; ++i) {
        if (board.makeMove(moveList[i])) {
            int eval = alphaBeta(alpha, beta, depth - 1, true);
            ++legalMoves;
            board.undoMove();
            if (info.stop) {
                return 0;
            }
            if (eval < beta) {
                beta = eval;
                if (beta <= alpha) {
                    info.fhf += legalMoves == 1;
                    ++info.fh;
                    return alpha;
                }
                bestMove = moveList[i];
            }
        }
    }
    if (legalMoves == 0) {
        uint64 king = board.getPieceBitboard(BLACK_KING);
        if (board.squaresAttacked(king, WHITE)) {
            return MATE + depth;
        }
        return 0;
    }
    if (beta != oldBeta) {
        assert(bestMove != INVALID);
        table.store(board.getPositionKey(), bestMove);
    }
    return beta;
}


/*
 *
 * Search for the best move in the current position using iterative deepening.
 * Iterative deepening is a technique where you do a full search to depth 1,
 * then depth 2, then depth 3, etc. With this approach, if we run out of time
 * to complete a search of depth 10, we can return the best move found on the
 * search to depth 9. Also, we can use the results of previous depths to speed
 * up the current depth by using the principal variation and other heuristics
 * to alter the order in which the alpha-beta algorithm considers moves. Move
 * ordering is critical because more nodes can be pruned from the search tree
 * if the best moves are considered first.
 * 
 */
void Engine::searchPosition(const SearchInfo& searchInfo) {
    info = searchInfo;
    setupSearch();
    std::string bestMove;
    for (int depth = 1; depth <= info.maxDepth; ++depth) {
        int eval = alphaBeta(-INF, INF, depth, board.side() == WHITE);
        if (info.stop) {
            break;
        }
        std::vector<std::string> pvLine = getPVLine(depth);
        assert(pvLine.size() > 0);
        bestMove = pvLine[0];
        std::cout << "info score cp " << eval << " depth " << depth;
        std::cout << " nodes " << info.nodes << " time ";
        std::cout << (currentTime() - info.startTime) << " pv ";
        for (std::string moveString : pvLine) {
            std::cout << moveString << ' ';
        } std::cout << '\n';
        printf("\tordering: %.2f\n", info.fh == 0.0f ? 0.0f : info.fhf / info.fh);
    }
    assert(bestMove != "");
    std::cout << "bestmove " << bestMove << '\n';
}
