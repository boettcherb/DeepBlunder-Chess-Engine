#include "engine.h"
#include "defs.h"
#include "board.h"
#include "movelist.h"
#include "hashkey.h"
#include "attack.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

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
    stop = timeSet = false;
    maxDepth = -1;
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
    table.setSize(hashTableSize);
    logger_mutex.lock();
    logger.open(logFile, std::ios::out | std::ios::app);
    logger_mutex.unlock();
}


/*
 * 
 * Initialize the engine. Create the hash keys for Zobrist hashing, create the
 * bishop and rook attack tables, and initialize the transposition table. This
 * function can be called multiple times and it will have no effect after the
 * first time unless a uci option has changed.
 * 
 */
void Engine::initialize() {
    hashkey::initHashKeys();
    attack::initializeBishopAttackTable();
    attack::initializeRookAttackTable();
    uint64 entries = table.initialize();
    if (entries > 0) {
        log("Hash table initialized to " + std::to_string(entries) + " entries");
    }
}


/*
 * 
 * Set the Transposition table to a certain size in MB. This is called whenever
 * the engine receives a "setoption name Hash ..." command from the GUI.
 * 
 */
void Engine::setHashTableSize(int sizeInMB) {
    assert(sizeInMB > 0 && sizeInMB <= 4096);
    hashTableSize = sizeInMB;
    table.setSize(sizeInMB);
    log("Setting hash table size to " + std::to_string(sizeInMB) + " MB");
}


/*
 * 
 * Change the 'moveOverhead' value. This value is used to reserve a small
 * portion of time on each move in case of sudden lag or internet delays. On
 * each move, this value is subtracted from the calculted time remaining
 * (occurs in setupSearch()).
 * 
 */
void Engine::setMoveOverhead(int overhead) {
    assert(overhead >= 0 && overhead <= 5000);
    moveOverhead = overhead;
    log("Setting move overhead to " + std::to_string(overhead) + " ms");
}


/*
 * 
 * Set the path to the log file. If the file path is empty, then disable
 * logging. This function is called whenever we receive a "setoption name Log
 * File ..." command from the GUI.
 * 
 */
void Engine::setLogFile(const std::string& path) {
    if (path == logFile) {
        return;
    }
    if (path.empty()) {
        log("Disabling logging...");
    }
    logFile = path;
    logger_mutex.lock();
    logger.close();
    if (!logFile.empty()) {
        logger.open(logFile, std::ios::out | std::ios::app);
    }
    logger_mutex.unlock();
}


/*
 * 
 * Output a message to the log file, if there is one open.
 * 
 */
void Engine::log(const std::string& message) {
    logger_mutex.lock();
    if (logger.is_open()) {
        logger << message << '\n';
    }
    logger_mutex.unlock();
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
#ifndef NDEBUG
        bool moveMade = board.makeMove(move);
        assert(moveMade);
#else
        board.makeMove(move);
#endif
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
        int storedMove = table.retrieveMove(board.getPositionKey());
        if (storedMove != INVALID && board.makeMove(storedMove)) {
            moves.push_back(board.getMoveString(storedMove));
        } else {
            break;
        }
    }
    for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
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
 * via the UCI protocol. If a specific movetime was given, set the time of the
 * current side to that value
 * 
 */
void Engine::setupSearch() {
    initialize();
    pvMove = INVALID;
    info.nodes = 0;
    info.stop = false;
    info.startTime = currentTime();
    int side = board.side();
    if (info.movetime != -1) {
        info.time[side] = info.movetime;
        info.movestogo = 1;
    }
    if (info.maxDepth == -1) {
        info.maxDepth = MAX_SEARCH_DEPTH;
    }
    if (info.time[side] != -1) {
        info.timeSet = true;
        info.time[side] /= info.movestogo;
        info.stopTime = info.startTime + info.time[side] + info.inc[side];
        info.stopTime -= moveOverhead;
    }
    board.resetSearchPly();
    std::memset(searchHistory, 0, sizeof(searchHistory));
    std::memset(searchKillers, 0, sizeof(searchKillers));
    std::string time_info = "timeSet: " + std::to_string(info.timeSet) + ", ";
    if (side == WHITE) {
        time_info += "wtime: " + std::to_string(info.time[side]) + ", ";
        time_info += "winc: " + std::to_string(info.inc[side]) + ", ";
    } else {
        time_info += "btime: " + std::to_string(info.time[side]) + ", ";
        time_info += "binc: " + std::to_string(info.inc[side]) + ", ";
    }
    time_info += "allocated time: " + std::to_string(info.stopTime - info.startTime) + ", ";
    time_info += "depth: " + std::to_string(info.maxDepth);
    log(time_info);
    std::cout << time_info << std::endl;
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
int Engine::quiescence(int alpha, int beta) {
    ++info.nodes;
    if ((info.nodes & 0xFFF) == 0) {
        checkup();
    }
    if (info.stop || (board.getSearchPly() > 0 && board.isRepetition())
        || board.getFiftyMoveCount() >= 100) {
        return 0;
    }
    int bestEval = board.evaluatePosition();
    if (bestEval > alpha) {
        if (bestEval >= beta) {
            return beta;
        }
        alpha = bestEval;
    }
    MoveList moveList(board, true);
    moveList.orderMoves(INVALID, searchKillers, searchHistory, counterMoves);
    int numMoves = moveList.numMoves();
    for (int i = 0; i < numMoves; ++i) {
        assert(moveList[i] & (CAPTURE_FLAG | EN_PASSANT_FLAG));
        if (!board.makeMove(moveList[i])) {
            continue;
        }
        int eval = -quiescence(-beta, -alpha);
        board.undoMove();
        if (info.stop) {
            return 0;
        }
        if (eval > alpha) {
            if (eval >= beta) {
                return beta;
            }
            alpha = eval;
        }
    }
    return alpha;
}


/*
 *
 * The Alpha-Beta search algorithm. Recursively search to depth 'depth' for the
 * best move in the current position. Return the evaluation of the current
 * position based on the best sequence of moves for the current side to move.
 * 
 * In normal alpha-beta, alpha is the best possible score for white in the
 * starting position, and beta is the best possible score for black in the
 * starting position. If it is white's move and we find a position with an
 * evaluation > alpha, we update alpha. Similarly, if it is black's move and
 * the evaluation is < beta, we update beta. However, if alpha becomes >= beta,
 * then we've encountered a position that is 'too good' for one side. The other
 * side will have a chance to avoid that path by choosing a different move. If
 * this occurs, the current branch can be pruned.
 * 
 * This function uses a variant of alpha-beta called Negamax, where alpha is
 * the best possible score for the current side to move, and beta is the best
 * possible score for the other side. For the recursive call, we negate and
 * swap alpha and beta, and negate the evaluation for the other side to move.
 * 
 */
int Engine::alphaBeta(int alpha, int beta, int depth) {
    if (depth <= 0) {
        return quiescence(alpha, beta);
    }
    ++info.nodes;
    if ((info.nodes & 0xFFF) == 0) {
        checkup();
    }
    if (info.stop || (board.getSearchPly() > 0 && board.isRepetition())
        || board.getFiftyMoveCount() >= 100) {
        return 0;
    }
    int bestMove = INVALID;
    int bestEval = -INF;
    if (table.retrieve(board.getPositionKey(), depth, alpha, beta,
                       bestMove, bestEval)) {
        pvMove = board.getSearchPly() == 0 ? bestMove : pvMove;
        return bestEval;
    }
    MoveList moveList(board);
    moveList.orderMoves(bestMove, searchKillers, searchHistory, counterMoves);
    int numMoves = moveList.numMoves(), legalMoves = 0, oldAlpha = alpha;
    for (int i = 0; i < numMoves; ++i) {
        if (!board.makeMove(moveList[i])) {
            continue;
        }
        int eval = -alphaBeta(-beta, -alpha, depth - 1);
        board.undoMove();
        if (info.stop) {
            return 0;
        }
        ++legalMoves;
        if (eval > bestEval) {
            bestEval = eval;
            bestMove = moveList[i];
            pvMove = board.getSearchPly() == 0 ? bestMove : pvMove;
            if (eval > alpha) {
                if (eval >= beta) {
                    if (!(moveList[i] & (CAPTURE_FLAG | EN_PASSANT_FLAG))) {
                        int sp = board.getSearchPly();
                        searchKillers[sp][1] = searchKillers[sp][0];
                        searchKillers[sp][0] = moveList[i];
                        int prevMove = board.getPreviousMove();
                        if (prevMove != INVALID) {
                            int prevTo = (prevMove >> 6) & 0x3F;
                            int prevPiece = board[prevTo];
                            assert(prevPiece != NO_PIECE);
                            counterMoves[prevPiece][prevTo] = moveList[i];
                        }
                    }
                    table.store(board.getPositionKey(), bestMove, beta,
                                depth, LOWER_BOUND);
                    return beta;
                }
                alpha = eval;
                if (!(moveList[i] & (CAPTURE_FLAG | EN_PASSANT_FLAG))) {
                    int to = (bestMove >> 6) & 0x3F;
                    int piece = board[bestMove & 0x3F];
                    assert(piece != NO_PIECE);
                    searchHistory[piece][to] += depth * depth;
                }
            }
        }
    }
    if (legalMoves == 0) {
        int kingPiece = board.side() == WHITE ? WHITE_KING : BLACK_KING;
        uint64 king = board.getPieceBitboard(kingPiece);
        if (board.squaresAttacked(king, board.side() ^ 1)) {
            return -(MATE - board.getSearchPly());
        }
        return 0;
    }
    if (alpha != oldAlpha) {
        assert(bestMove != INVALID);
        table.store(board.getPositionKey(), bestMove, bestEval, depth, EXACT);
    } else {
        table.store(board.getPositionKey(), bestMove, alpha, depth, UPPER_BOUND);
    }
    return alpha;
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
 * Once the search completes, print out the best move to the console so that
 * it can be read by the GUI.
 * 
 */
void Engine::searchPosition(const SearchInfo& searchInfo) {
    info = searchInfo;
    setupSearch();
    for (int depth = 1; depth <= info.maxDepth; ++depth) {
        int eval = alphaBeta(-INF, INF, depth);
        if (info.stop) {
            break;
        }
        std::stringstream ss;
        if (eval > 20000) {
            ss << "info score mate " << (MATE - eval + 1) / 2;
        } else if (eval < -20000) {
            ss << "info score mate " << -((MATE + eval + 1) / 2);
        } else {
            ss << "info score cp " << eval;
        }
        uint64 time = currentTime() - info.startTime;
        ss << " depth " << depth;
        ss << " nodes " << info.nodes;
        ss << " time " << time;
        if (time > 0) {
            ss << " nps " << (info.nodes * 1000 / time);
        }
        std::vector<std::string> pvLine = getPVLine(depth);
        assert(pvLine.size() > 0);
        ss << " pv";
        for (std::string moveString : pvLine) {
            ss << " " << moveString;
        }
        std::string info_string = ss.str();
        std::cout << info_string << std::endl;
        log(info_string);
        if (eval > 20000) {
            break;
        }
    }
    if (pvMove == INVALID) {
        log("Error: pvMove not set");
    }
    std::string bestMove = board.getMoveString(pvMove);
    std::cout << "bestmove " << bestMove << std::endl;
    log("bestmove " + bestMove);
}
