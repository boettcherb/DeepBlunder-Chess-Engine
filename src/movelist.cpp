#include "movelist.h"
#include "defs.h"
#include "board.h"
#include "attack.h"
#include <iostream>
#include <algorithm>


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
 * 0111 1110 0000 0000 0000 0000 0000 0000   6 bits for the move score
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


static inline constexpr int MAX_MOVE_SCORE = 63;


/*
 * 
 * MoveList constructor. Given a board, fill the movelist with every legal and
 * pseudo-legal move that is available in the current position. A pseudo-legal
 * move is a move that would normally be legal but on the current board would
 * leave the king in check. These moves are removed later as moves are made and
 * unmade. If we want to only generate capture moves (for Quiescence Search),
 * then we can set 'onlyCaptures' to true.
 * 
 */
MoveList::MoveList(const Board& board, bool onlyCaptures) {
    if (onlyCaptures) {
        generateCaptureMoves(board);
    } else {
        generateMoves(board);
    }
}


/*
 *
 * Return the number of moves in the MoveList.
 *
 */
int MoveList::numMoves() const {
    return (int) moves.size();
}


/*
 * 
 * Given an index i, return the i'th move in the movelist. The given index must
 * be valid (0 <= i < the number of moves in the movelist).
 * 
 */
int MoveList::operator[](int index) const {
    assert(index >= 0 && index < (int) moves.size());
    assert(validMove(moves[index]));
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
 * promotion flag is set, then the promoted piece must be present, etc. This is
 * a debug function which should only be called within asserts in debug mode.
 * 
 */
bool MoveList::validMove(int move) const {
    int from = move & 0x3F;
    int to = (move >> 6) & 0x3F;
    int cap = (move >> 12) & 0xF;
    int prom = (move >> 16) & 0xF;

    if (move & CAPTURE_FLAG) {
        if (cap < 0 || cap >= NUM_PIECE_TYPES
            || cap == WHITE_KING || cap == BLACK_KING) {
            std::cout << "Invalid captured piece (1)" << std::endl;
            return false;
        }
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cout << "Invalid flags (1)" << std::endl;
            return false;
        }
    }
    else {
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (2)" << std::endl;
            return false;
        }
    }
    if (move & PROMOTION_FLAG) {
        if (prom < 0 || prom >= NUM_PIECE_TYPES || prom == WHITE_KING
            || prom == WHITE_PAWN || prom == BLACK_KING || prom == BLACK_PAWN) {
            std::cerr << "Invalid promoted piece (1)" << std::endl;
            return false;
        }
        if (move & (EN_PASSANT_FLAG | CASTLE_FLAG | PAWN_START_FLAG)) {
            std::cerr << "Invalid flags (2)" << std::endl;
            return false;
        }
        if (!((from < 16 && from >= 8 && to < 8) ||
              (from >= 48 && from < 56 && to >= 56))) {
            std::cerr << "Invalid from/to squares for promotion" << std::endl;
            return false;
        }
    }
    else {
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (2)" << std::endl;
            return false;
        }
    }
    if (move & CASTLE_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (3)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (3)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (3)" << std::endl;
            return false;
        }
        if (!(from == E1 && (to == G1 || to == C1))
            && !(from == E8 && (to == G8 || to == C8))) {
            std::cerr << "Invalid from/to squares for castling" << std::endl;
        }
    }
    if (move & PAWN_START_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | CASTLE_FLAG | EN_PASSANT_FLAG)) {
            std::cerr << "Invalid flags (4)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (4)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (4)" << std::endl;
            return false;
        }
        uint64 F = 1ULL << from;
        uint64 T = 1ULL << to;
        if (!((F & 0x00FF00000000FF00) && (T & 0x000000FFFF000000)
              && (std::abs(from - to) == 16))) {
            std::cerr << "Invalid from/to squares for pawn start" << std::endl;
            return false;
        }
    }
    if (move & EN_PASSANT_FLAG) {
        if (move & (CAPTURE_AND_PROMOTION_FLAG | PAWN_START_FLAG | CASTLE_FLAG)) {
            std::cerr << "Invalid flags (5)" << std::endl;
            return false;
        }
        if (cap != 0xF) {
            std::cerr << "Invalid captured piece (5)" << std::endl;
            return false;
        }
        if (prom != 0xF) {
            std::cout << "Invalid promoted piece (5)" << std::endl;
            return false;
        }
        uint64 F = 1ULL << from;
        uint64 T = 1ULL << to;
        if (!((F & 0x000000FFFF000000) && (T & 0x0000FF0000FF0000)
              && (std::abs(from - to) == 7 || std::abs(from - to) == 9))) {
            std::cerr << "Invalid from/to squares for en passant" << std::endl;
            return false;
        }
    }
    return true;
}


/*
 *
 * Variables storing the move score for each move type. A move with a higher
 * move score is more likely to be better than a move with a lower move score
 * (likely, though not always. The alpha-beta algorithm will determine whether
 * the move is actually better. For example, a move where a white knight
 * captures a black queen has a higher move score than a move where a white
 * knight captures a black rook:
 *      captureScore[WHITE_KNIGHT][BLACK_QUEEN] == 61
 *      captureScore[WHITE_KNIGHT][BLACK_ROOK] == 54
 * moveScore[] is for normal moves (not captures, promotions, castling, etc.).
 * For example, a pawn move (moveScore[WHITE_PAWN] == 6) is prioritized over a
 * king move (moveScore[WHITE_KING] == 1).
 * promotionScore[] is for promotion moves. A promotion to a white queen
 * (promotionScore[WHITE_QUEEN] == 62) is prioritized over a promotion to a
 * white rook (promotionScore[WHITE_ROOK] == 59).
 *
 */
static inline constexpr int captureScore[NUM_PIECE_TYPES][NUM_PIECE_TYPES] = {
    {  0,  0,  0,  0,  0,  0, 46, 55, 56, 59, 62,  0 },
    {  0,  0,  0,  0,  0,  0, 40, 49, 50, 54, 61,  0 },
    {  0,  0,  0,  0,  0,  0, 39, 47, 48, 53, 60,  0 },
    {  0,  0,  0,  0,  0,  0, 38, 44, 45, 51, 58,  0 },
    {  0,  0,  0,  0,  0,  0, 37, 41, 42, 43, 52,  0 },
    {  0,  0,  0,  0,  0,  0, 34, 35, 36, 36, 57,  0 },
    { 46, 55, 56, 59, 62,  0,  0,  0,  0,  0,  0,  0 },
    { 40, 49, 50, 54, 61,  0,  0,  0,  0,  0,  0,  0 },
    { 39, 47, 48, 53, 60,  0,  0,  0,  0,  0,  0,  0 },
    { 38, 44, 45, 51, 58,  0,  0,  0,  0,  0,  0,  0 },
    { 37, 41, 42, 43, 52,  0,  0,  0,  0,  0,  0,  0 },
    { 34, 35, 36, 36, 57,  0,  0,  0,  0,  0,  0,  0 },
};
static inline constexpr int moveScore[NUM_PIECE_TYPES] = {
    6, 5, 4, 3, 2, 1, 6, 5, 4, 3, 2, 1,
};
static inline constexpr int promotionScore[NUM_PIECE_TYPES] = {
    0, 55, 56, 59, 62, 0, 0, 55, 56, 59, 62, 0,
};
static inline constexpr int enPassantScore = 46;
static inline constexpr int castleScore = 8;
static inline constexpr int pawnStartScore = 7;
static inline constexpr int killerScore1 = 33;
static inline constexpr int killerScore2 = 32;
static inline constexpr int historyScore = 9;


/*
 *
 * Return true if two moves are the same. When comparing moves, we do not take
 * into account the move score. This is because the same move can have a
 * different score in different positions (or even the same position at
 * different depths) because the move score is updated based on how well a move
 * does in the Alpha-Beta algorithm.
 *
 */
static bool sameMove(int m1, int m2) {
    constexpr int mask = 0x01FFFFFF;
    return (m1 & mask) == (m2 & mask);
}


/*
 *
 * Add a move (and its move score) to the MoveList. The move score is shifted
 * into the correct position and then the move is added to the back of the move
 * list.
 *
 */
void MoveList::addMove(int move, int score) {
    assert(validMove(move));
    assert(score > 0 && score <= MAX_MOVE_SCORE);
    moves.push_back(move | (score << 25));
}


/*
 *
 * Given a square with a piece (either a knight, bishop, rook, queen, or king)
 * and a bitboard of squares that it attacks, generate all the moves for the
 * piece. If a square being attacked has a piece on it, generate a capture
 * move. Otherwise, generate a normal move.
 *
 */
void MoveList::generatePieceMoves(int sq, uint64 attacks) {
    assert(sq >= 0 && sq < 64);
    assert(board[sq] != INVALID);
    while (attacks) {
        int to = getLSB(attacks);
        if (board[to] == INVALID) {
            int move = getMove(sq, to, INVALID, INVALID, 0);
            addMove(move, moveScore[board[sq]]);
        } else {
            int move = getMove(sq, to, board[to], INVALID, CAPTURE_FLAG);
            addMove(move, captureScore[board[sq]][board[to]]);
        }
        attacks &= attacks - 1;
    }
}


/*
 *
 * Check to see if a pawn move results in the pawn reaching the end of the
 * board. If so, convert the move into a promotion move by adding in the
 * promotion flag, adding in the promoted piece, and updating the move score.
 * Do this for all 4 promotion pieces (knight, bishop, rook, queen). If the
 * pawn move is not a pawn promotion, then just add the move without changes.
 *
 */
void MoveList::addPawnMove(int move, int score) {
    assert(validMove(move));
    assert(score > 0 && score < 0x3F);
    int to = (move >> 6) & 0x3F;
    if ((1ULL << to) & 0xFF000000000000FF) {
        move = (move & 0xFFF0FFFF) | PROMOTION_FLAG;
        int knight = pieceType[board.side()][KNIGHT];
        int bishop = pieceType[board.side()][BISHOP];
        int rook = pieceType[board.side()][ROOK];
        int queen = pieceType[board.side()][QUEEN];
        addMove(move | (knight << 16), promotionScore[KNIGHT]);
        addMove(move | (bishop << 16), promotionScore[BISHOP]);
        addMove(move | (rook << 16), promotionScore[ROOK]);
        addMove(move | (queen << 16), promotionScore[QUEEN]);
    } else {
        addMove(move, score);
    }
}


/*
 *
 * Generate pawn moves from the given board. Pawns get their own move generation
 * functions because they move uniquely from the other pieces. They are the only
 * piece whose normal moves and attacks are separated, and there are also many
 * special rules regarding pawns: en passant, pawn starts (moving 2 spaces
 * forward on the first turn), and pawn promotion.
 *
 */
void MoveList::generateWhitePawnMoves() {
    uint64 allPieces = board.getColorBitboard(BOTH_COLORS);
    uint64 pawns = board.getPieceBitboard(WHITE_PAWN);
    uint64 pawnMoves = (pawns << 8) & ~allPieces;
    uint64 pawnStarts = ((pawnMoves & 0x0000000000FF0000) << 8) & ~allPieces;
    while (pawnMoves) {
        int to = getLSB(pawnMoves);
        int move = getMove(to - 8, to, INVALID, INVALID, 0);
        addPawnMove(move, moveScore[WHITE_PAWN]);
        pawnMoves &= pawnMoves - 1;
    }
    uint64 enemyPieces = board.getColorBitboard(BLACK);
    uint64 attacksLeft = attack::getWhitePawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getWhitePawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to - 7, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to - 9, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    while (pawnStarts) {
        int to = getLSB(pawnStarts);
        int move = getMove(to - 16, to, INVALID, INVALID, PAWN_START_FLAG);
        addMove(move, pawnStartScore);
        pawnStarts &= pawnStarts - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 47 && board[ep - 7] == WHITE_PAWN) {
            int move = getMove(ep - 7, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
        if (ep != 40 && board[ep - 9] == WHITE_PAWN) {
            int move = getMove(ep - 9, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
    }
}
void MoveList::generateBlackPawnMoves() {
    uint64 allPieces = board.getColorBitboard(BOTH_COLORS);
    uint64 pawns = board.getPieceBitboard(BLACK_PAWN);
    uint64 pawnMoves = (pawns >> 8) & ~allPieces;
    uint64 pawnStarts = ((pawnMoves & 0x0000FF0000000000) >> 8) & ~allPieces;
    while (pawnMoves) {
        int to = getLSB(pawnMoves);
        int move = getMove(to + 8, to, INVALID, INVALID, 0);
        addPawnMove(move, moveScore[BLACK_PAWN]);
        pawnMoves &= pawnMoves - 1;
    }
    uint64 enemyPieces = board.getColorBitboard(WHITE);
    uint64 attacksLeft = attack::getBlackPawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getBlackPawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to + 7, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to + 9, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    while (pawnStarts) {
        int to = getLSB(pawnStarts);
        int move = getMove(to + 16, to, INVALID, INVALID, PAWN_START_FLAG);
        addMove(move, pawnStartScore);
        pawnStarts &= pawnStarts - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 16 && board[ep + 7] == BLACK_PAWN) {
            int move = getMove(ep + 7, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
        if (ep != 23 && board[ep + 9] == BLACK_PAWN) {
            int move = getMove(ep + 9, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
    }
}


/*
 * 
 * Generate castle moves for the given board. There are several conditions that
 * must be met for a castle move to be legal: neither the king or rook has
 * previously moved, there cannot be any pieces in between the king and the
 * rook, and the king cannot be in check, pass through check, or end up in
 * check.
 * 
 */
void MoveList::generateWhiteCastleMoves() {
    int castlePerms = board.getCastlePerms();
    uint64 allPieces = board.getColorBitboard(BOTH_COLORS);

    if (castlePerms & CASTLE_WK) {
        if (!(allPieces & 0x60) && !board.squaresAttacked(0x70, BLACK)) {
            int move = getMove(E1, G1, INVALID, INVALID, CASTLE_FLAG);
            addMove(move, castleScore);
        }
    }
    if (castlePerms & CASTLE_WQ) {
        if (!(allPieces & 0xE) && !board.squaresAttacked(0x1C, BLACK)) {
            int move = getMove(E1, C1, INVALID, INVALID, CASTLE_FLAG);
            addMove(move, castleScore);
        }
    }
}
void MoveList::generateBlackCastleMoves() {
    int castlePerms = board.getCastlePerms();
    uint64 allPieces = board.getColorBitboard(BOTH_COLORS);
    if (castlePerms & CASTLE_BK) {
        if (!(allPieces & 0x6000000000000000ULL) &&
            !board.squaresAttacked(0x7000000000000000ULL, WHITE)) {
            int move = getMove(E8, G8, INVALID, INVALID, CASTLE_FLAG);
            addMove(move, castleScore);
        }
    }
    if (castlePerms & CASTLE_BQ) {
        if (!(allPieces & 0x0E00000000000000ULL) &&
            !board.squaresAttacked(0x1C00000000000000ULL, WHITE)) {
            int move = getMove(E8, C8, INVALID, INVALID, CASTLE_FLAG);
            addMove(move, castleScore);
        }
    }
}


/*
 *
 * Generate all legal and pseudo-legal moves for the current position. (A
 * pseudo-legal move is a move that is normally legal, but in the current
 * position it would leave the king in check). Each move has a 'from' square
 * (the square that the moved piece starts on), a 'to' square (the square that
 * the moved piece ends up on), the captured piece (if there is one), the 
 * promoted piece (if the move is a pawn promotion), and bit flags indicating
 * if the move was a special move (capture, promotion, castle, en passant, or
 * pawn start). Each piece of information about a move, as well as the move
 * score, is combined into a single 32-bit integer and stored in the MoveList.
 * Then, the moves are sorted in descending order by their move score.
 *
 */
void MoveList::generateMoves(const Board& b) {
    moves.clear();
    moves.reserve(40);
    board = b;
    uint64 samePieces, allPieces = board.getColorBitboard(BOTH_COLORS);
    uint64 knights, bishops, rooks, queens, king;
    if (board.side() == WHITE) {
        knights = board.getPieceBitboard(WHITE_KNIGHT);
        bishops = board.getPieceBitboard(WHITE_BISHOP);
        rooks = board.getPieceBitboard(WHITE_ROOK);
        queens = board.getPieceBitboard(WHITE_QUEEN);
        king = board.getPieceBitboard(WHITE_KING);
        samePieces = board.getColorBitboard(WHITE);
        generateWhitePawnMoves();
        generateWhiteCastleMoves();
    } else {
        knights = board.getPieceBitboard(BLACK_KNIGHT);
        bishops = board.getPieceBitboard(BLACK_BISHOP);
        rooks = board.getPieceBitboard(BLACK_ROOK);
        queens = board.getPieceBitboard(BLACK_QUEEN);
        king = board.getPieceBitboard(BLACK_KING);
        samePieces = board.getColorBitboard(BLACK);
        generateBlackPawnMoves();
        generateBlackCastleMoves();
    }
    while (knights) {
        int knight = getLSB(knights);
        uint64 attacks = attack::getKnightAttacks(knight);
        generatePieceMoves(knight, attacks & ~samePieces);
        knights &= knights - 1;
    }
    while (bishops) {
        int bishop = getLSB(bishops);
        uint64 attacks = attack::getBishopAttacks(bishop, allPieces);
        generatePieceMoves(bishop, attacks & ~samePieces);
        bishops &= bishops - 1;
    }
    while (rooks) {
        int rook = getLSB(rooks);
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        generatePieceMoves(rook, attacks & ~samePieces);
        rooks &= rooks - 1;
    }
    while (queens) {
        int queen = getLSB(queens);
        uint64 attacks = attack::getQueenAttacks(queen, allPieces);
        generatePieceMoves(queen, attacks & ~samePieces);
        queens &= queens - 1;
    }
    uint64 kingAttacks = attack::getKingAttacks(king) & ~samePieces;
    generatePieceMoves(getLSB(king), kingAttacks);
}


/*
 *
 * Functions similar to generateWhitePawnMoves() and generateBlackPawnMoves()
 * except now we are only generating capture moves (not normal pawn moves or
 * pawn starts. These functions are used for Quiescence Search.
 *
 */
void MoveList::generateWhitePawnCaptureMoves() {
    uint64 pawns = board.getPieceBitboard(WHITE_PAWN);
    uint64 enemyPieces = board.getColorBitboard(BLACK);
    uint64 attacksLeft = attack::getWhitePawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getWhitePawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to - 7, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to - 9, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 47 && board[ep - 7] == WHITE_PAWN) {
            int move = getMove(ep - 7, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
        if (ep != 40 && board[ep - 9] == WHITE_PAWN) {
            int move = getMove(ep - 9, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
    }
}
void MoveList::generateBlackPawnCaptureMoves() {
    uint64 pawns = board.getPieceBitboard(BLACK_PAWN);
    uint64 enemyPieces = board.getColorBitboard(WHITE);
    uint64 attacksLeft = attack::getBlackPawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getBlackPawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to + 7, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to + 9, to, board[to], INVALID, CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 16 && board[ep + 7] == BLACK_PAWN) {
            int move = getMove(ep + 7, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
        if (ep != 23 && board[ep + 9] == BLACK_PAWN) {
            int move = getMove(ep + 9, ep, INVALID, INVALID, EN_PASSANT_FLAG);
            addMove(move, enPassantScore);
        }
    }
}


/*
 *
 * Generate only capture moves. This function is similar to generateMoves()
 * except we are generating pawn captures instead of all pawn moves, we are
 * not generating castle moves, and for each piece type we limiting the attacks
 * to capture moves with (attacks & enemyPieces).
 *
 */
void MoveList::generateCaptureMoves(const Board& b) {
    moves.clear();
    moves.reserve(40);
    board = b;
    uint64 enemyPieces, allPieces = board.getColorBitboard(BOTH_COLORS);
    uint64 knights, bishops, rooks, queens, king;
    if (board.side() == WHITE) {
        knights = board.getPieceBitboard(WHITE_KNIGHT);
        bishops = board.getPieceBitboard(WHITE_BISHOP);
        rooks = board.getPieceBitboard(WHITE_ROOK);
        queens = board.getPieceBitboard(WHITE_QUEEN);
        king = board.getPieceBitboard(WHITE_KING);
        enemyPieces = board.getColorBitboard(BLACK);
        generateWhitePawnCaptureMoves();
    } else {
        knights = board.getPieceBitboard(BLACK_KNIGHT);
        bishops = board.getPieceBitboard(BLACK_BISHOP);
        rooks = board.getPieceBitboard(BLACK_ROOK);
        queens = board.getPieceBitboard(BLACK_QUEEN);
        king = board.getPieceBitboard(BLACK_KING);
        enemyPieces = board.getColorBitboard(WHITE);
        generateBlackPawnCaptureMoves();
    }
    while (knights) {
        int knight = getLSB(knights);
        uint64 attacks = attack::getKnightAttacks(knight);
        generatePieceMoves(knight, attacks & enemyPieces);
        knights &= knights - 1;
    }
    while (bishops) {
        int bishop = getLSB(bishops);
        uint64 attacks = attack::getBishopAttacks(bishop, allPieces);
        generatePieceMoves(bishop, attacks & enemyPieces);
        bishops &= bishops - 1;
    }
    while (rooks) {
        int rook = getLSB(rooks);
        uint64 attacks = attack::getRookAttacks(rook, allPieces);
        generatePieceMoves(rook, attacks & enemyPieces);
        rooks &= rooks - 1;
    }
    while (queens) {
        int queen = getLSB(queens);
        uint64 attacks = attack::getQueenAttacks(queen, allPieces);
        generatePieceMoves(queen, attacks & enemyPieces);
        queens &= queens - 1;
    }
    uint64 kingAttacks = attack::getKingAttacks(king) & enemyPieces;
    generatePieceMoves(getLSB(king), kingAttacks);
}


/*
 *
 * Sort the moves based on their move score. If the current position was
 * previously searched and a best move was found, that move can be passed in
 * here and it will receive the highest score possible, ensuring that it is
 * considered first by the Alpha-Beta algorithm. If there are any non-capture
 * moves that match those stored by the killer move heuristic or the search
 * history heuristic, increase their scores as well. This will improve our move
 * ordering, which can lead to more pruning.
 *
 */
void MoveList::orderMoves(int bestMove, int killers[MAX_SEARCH_DEPTH][2],
                          int searchHistory[NUM_PIECE_TYPES][64]) {
    for (int& move : moves) {
        if (sameMove(move, bestMove)) {
            move |= 0x7E000000;
        }
        else if (sameMove(move, killers[board.getSearchPly()][0])) {
            move = (move & 0x01FFFFFF) | (killerScore1 << 25);
        }
        else if (sameMove(move, killers[board.getSearchPly()][1])) {
            move = (move & 0x01FFFFFF) | (killerScore2 << 25);
        }
        else if (!(move & (CAPTURE_FLAG | EN_PASSANT_FLAG))) {
            int piece = board[move & 0x3F];
            int to = (move >> 6) & 0x3F;
            if (searchHistory[piece][to] > 0) {
                int newScore = historyScore + searchHistory[piece][to];
                move = (move & 0x01FFFFFF) | (newScore << 25);
            }
        }
    }
    auto compareMoves = [](const int& m1, const int& m2) -> bool {
        return (m1 >> 25) > (m2 >> 25);
    };
    std::sort(moves.begin(), moves.end(), compareMoves);
}


/*
 *
 * Check to see if the given move is legal on the current board. If the move is
 * possible in the current position and it does not leave the king in check,
 * then return true. Otherwise return false.
 *
 */
bool MoveList::moveExists(int move) {
    assert(validMove(move));
    for (int m : moves) {
        if (sameMove(m, move)) {
            if (board.makeMove(m)) {
                board.undoMove();
                return true;
            }
            return false;
        }
    }
    return false;
}