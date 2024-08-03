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
 * Engine constructor. Initialize the engine by initializing the hashkeys and
 * the bishop and rook attack tables.
 * 
 */
Engine::Engine() {
    static_assert(sizeof(uint64) == 8);
    hashkey::initHashKeys();
    attack::initializeBishopAttackTable();
    attack::initializeRookAttackTable();
}


/*
 * 
 * Setup the board according to the given position by calling setToFEN(). If
 * the position cannot be set up, return false.
 * 
 */
bool Engine::setupBoard(const std::string& fen) {
    return board.setToFEN(fen);
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
 * Setup some of the variables and data structures used during the alpha-beta
 * search, such as the variables in 'info' and the transposition table.
 * 
 */
void Engine::setupSearch(SearchInfo& info) {
    info.startTime = currentTime();
    info.nodes = 0;
    info.stop = info.quit = false;
    table.initialize();
}


/*
 *
 * The Alpha-Beta search algorithm. Recursively search to depth 'depth' for the
 * best move in the current position. Return the evaluation of the current
 * position based on the best sequence of moves for the current side to move.
 * 
 */
int Engine::alphaBeta(SearchInfo& info, int depth, int alpha, int beta, bool max) {
    ++info.nodes;
    if (depth <= 0) {
        return board.evaluatePosition();
    }
    if (board.getFiftyMoveCount() >= 100 || board.is3foldRepetition()) {
        return 0;
    }
    MoveList moveList(board);
    // TODO: retrieve the bestMove for the current position from the transposition
    // table and make it first in the movelist ordering
    int numMoves = moveList.numMoves(), legalMoves = 0;
    int bestEval = 0, bestMove = INVALID;
    int oldAlpha = alpha, oldBeta = beta;
    if (max) {
        assert(board.side() == WHITE);
        bestEval = -INF;
        for (int i = 0; i < numMoves; ++i) {
            int move = moveList[i];
            if (board.makeMove(move)) {
                int eval = alphaBeta(info, depth - 1, alpha, beta, false);
                if (eval > bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
                ++legalMoves;
                board.undoMove();
                if (alpha < eval) {
                    alpha = eval;
                    if (beta <= alpha) {
                        return beta;
                    }
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
    }
    else {
        assert(board.side() == BLACK);
        bestEval = INF;
        for (int i = 0; i < numMoves; ++i) {
            int move = moveList[i];
            if (board.makeMove(move)) {
                int eval = alphaBeta(info, depth - 1, alpha, beta, true);
                if (eval < bestEval) {
                    bestEval = eval;
                    bestMove = move;
                }
                ++legalMoves;
                board.undoMove();
                if (beta > eval) {
                    beta = eval;
                    if (beta <= alpha) {
                        return alpha;
                    }
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
    }
    return bestEval;
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
 * The SearchInfo struct contains information about the search and flags that
 * can be set by the caller to tell the search to stop
 * 
 */
void Engine::searchPosition(SearchInfo& info) {
    setupSearch(info);
    std::cout << "Searching position...\n";
    for (int depth = 1; depth <= info.maxDepth; ++depth) {
        int eval = alphaBeta(info, depth, -INF, INF, board.side() == WHITE);
        uint64 endTime = currentTime();
        std::vector<std::string> pvLine = getPVLine(depth);

        std::cout << "Depth " << depth << ":\n";
        std::cout << "\tEvaluation: " << eval << '\n';
        std::cout << "\tBest line: ";
        for (std::string moveString : pvLine) {
            std::cout << moveString << ' ';
        } std::cout << '\n';
        std::cout << "\tTime taken: " << (endTime - info.startTime) << " ms\n";
        std::cout << "\tNodes searched: " << info.nodes << "\n";
    }
}
