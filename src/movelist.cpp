#include "movelist.h"
#include "defs.h"
#include "board.h"
#include "attack.h"
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
 * 
 * The piece that is moving originated on the 'from' square and is moving to
 * the 'to' square. If the move is a capture move, the captured piece is
 * recorded. If the move is a pawn promotion, the piece the pawn promotes to is
 * recorded. The 5 flags are used to quickly determine the type of the move:
 * capture, promotion, castle, en passant, pawn start (when a pawn moves 2
 * squares forward), or just a normal move if none of the flags are set.
 * 
 * Each move also has a move score. A move will have a higher move score if it
 * is likely to be a good move. (Ex: captures, promotions, castling). Sorting
 * moves by their move score will help the search algorithm run faster, as more
 * pruning can occur if the best moves are considered first. Note that move
 * scores are different from position scores (or position evaluations). A
 * position evaluation is an estimate of which side is winning in a certain
 * position, and it determines which moves are actually chosen by the Alpha
 * Beta algorithm. The move score just determines the order in which the
 * algorithm considers the available moves.
 *****************************************************************************/


/*
 *
 * Variables storing the move score for each move type. A move with a higher
 * move score is more likely to be better than a move with a lower move score
 * (likely, though not always). The alpha-beta algorithm will determine whether
 * the move is actually better. For example, a move where a white knight
 * captures a black queen has a higher move score than a move where a white
 * knight captures a black rook:
 *      captureScore[WHITE_KNIGHT][BLACK_QUEEN] == 100000380
 *      captureScore[WHITE_KNIGHT][BLACK_ROOK] == 100000310
 * moveScore[] is for normal moves (not captures, promotions, castling, etc.).
 * For example, a pawn move (moveScore[WHITE_PAWN] == 6) is prioritized over a
 * king move (moveScore[WHITE_KING] == 1).
 * promotionScore[] is for promotion moves. A promotion to a white queen
 * (promotionScore[WHITE_QUEEN] == 100000385) is prioritized over a promotion
 * to a white rook (promotionScore[WHITE_ROOK] == 100000345).
 *
 */
static inline constexpr int captureScore[NUM_PIECE_TYPES][NUM_PIECE_TYPES] = {
    { 0, 0, 0, 0, 0, 0, 100000150, 100000320, 100000330, 100000350, 100000390, 0 },
    { 0, 0, 0, 0, 0, 0, 100000140, 100000240, 100000260, 100000310, 100000380, 0 },
    { 0, 0, 0, 0, 0, 0, 100000130, 100000230, 100000250, 100000300, 100000370, 0 },
    { 0, 0, 0, 0, 0, 0, 100000120, 100000200, 100000210, 100000270, 100000360, 0 },
    { 0, 0, 0, 0, 0, 0, 100000110, 100000180, 100000190, 100000220, 100000280, 0 },
    { 0, 0, 0, 0, 0, 0, 100000100, 100000160, 100000170, 100000290, 100000340, 0 },
    { 100000150, 100000320, 100000330, 100000350, 100000390, 0, 0, 0, 0, 0, 0, 0 },
    { 100000140, 100000240, 100000260, 100000310, 100000380, 0, 0, 0, 0, 0, 0, 0 },
    { 100000130, 100000230, 100000250, 100000300, 100000370, 0, 0, 0, 0, 0, 0, 0 },
    { 100000120, 100000200, 100000210, 100000270, 100000360, 0, 0, 0, 0, 0, 0, 0 },
    { 100000110, 100000180, 100000190, 100000220, 100000280, 0, 0, 0, 0, 0, 0, 0 },
    { 100000100, 100000160, 100000170, 100000290, 100000340, 0, 0, 0, 0, 0, 0, 0 },
};
static inline constexpr int moveScore[NUM_PIECE_TYPES] = {
    6, 5, 4, 3, 2, 1, 6, 5, 4, 3, 2, 1,
};
static inline constexpr int promotionScore[NUM_PIECE_TYPES] = {
    0, 100000315, 100000325, 100000345, 100000385, 0,
    0, 100000315, 100000325, 100000345, 100000385, 0,
};
static inline constexpr int enPassantScore = 1000000 + 155;
static inline constexpr int castleScore = 8;
static inline constexpr int pawnStartScore = 7;
static inline constexpr int pvScore = 2000000000;
static inline constexpr int killerScore1 = 100000205;
static inline constexpr int killerScore2 = 100000095;
static inline constexpr int counterMoveScore = 100000105;
static inline constexpr int historyScore = 100;


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
    return static_cast<int>(moves.size());
}


/*
 * 
 * Given an index i, return the i'th move in the movelist. The given index must
 * be valid (0 <= i < the number of moves in the movelist).
 * 
 */
int MoveList::operator[](int index) const {
    assert(index >= 0 && index < static_cast<int>(moves.size()));
    assert(validMove(moves[index].move));
    return moves[index].move;
}


/*
 * 
 * Combine all the parts of a move into a single 32-bit integer. This saves a
 * lot of memory per move and allows many more moves to be stored in the
 * transposition table. (The promoted piece is handled in addPawnMove(), so we
 * assume that it is NO_PIECE here).
 * 
 */
int MoveList::getMove(int from, int to, int cap, int flags) const {
    assert(from >= 0 && from < 64);
    assert(to >= 0 && to < 64);
    assert(cap == NO_PIECE || (cap >= 0 && cap < NUM_PIECE_TYPES));
    assert((flags & ~MOVE_FLAGS) == 0);
    int prom = NO_PIECE;
    int move = from | (to << 6) | (cap << 12) | (prom << 16) | flags;
    assert(validMove(move));
    return move;
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
    assert(board[sq] != NO_PIECE);
    while (attacks) {
        int to = getLSB(attacks);
        if (board[to] == NO_PIECE) {
            int move = getMove(sq, to, NO_PIECE, 0);
            moves.emplace_back(move, moveScore[board[sq]]);
        } else {
            int move = getMove(sq, to, board[to], CAPTURE_FLAG);
            moves.emplace_back(move, captureScore[board[sq]][board[to]]);
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
    assert(score > 0);
    int to = (move >> 6) & 0x3F;
    if ((1ULL << to) & 0xFF000000000000FF) {
        move = (move & 0xFFF0FFFF) | PROMOTION_FLAG;
        int knight = pieceType[board.side()][KNIGHT];
        int bishop = pieceType[board.side()][BISHOP];
        int rook = pieceType[board.side()][ROOK];
        int queen = pieceType[board.side()][QUEEN];
        moves.emplace_back(move | (knight << 16), score + promotionScore[KNIGHT]);
        moves.emplace_back(move | (bishop << 16), score + promotionScore[BISHOP]);
        moves.emplace_back(move | (rook << 16), score + promotionScore[ROOK]);
        moves.emplace_back(move | (queen << 16), score + promotionScore[QUEEN]);
    } else {
        moves.emplace_back(move, score);
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
        int move = getMove(to - 8, to, NO_PIECE, 0);
        addPawnMove(move, moveScore[WHITE_PAWN]);
        pawnMoves &= pawnMoves - 1;
    }
    uint64 enemyPieces = board.getColorBitboard(BLACK);
    uint64 attacksLeft = attack::getWhitePawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getWhitePawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to - 7, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to - 9, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    while (pawnStarts) {
        int to = getLSB(pawnStarts);
        int move = getMove(to - 16, to, NO_PIECE, PAWN_START_FLAG);
        moves.emplace_back(move, pawnStartScore);
        pawnStarts &= pawnStarts - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 47 && board[ep - 7] == WHITE_PAWN) {
            int move = getMove(ep - 7, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
        }
        if (ep != 40 && board[ep - 9] == WHITE_PAWN) {
            int move = getMove(ep - 9, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
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
        int move = getMove(to + 8, to, NO_PIECE, 0);
        addPawnMove(move, moveScore[BLACK_PAWN]);
        pawnMoves &= pawnMoves - 1;
    }
    uint64 enemyPieces = board.getColorBitboard(WHITE);
    uint64 attacksLeft = attack::getBlackPawnAttacksLeft(pawns) & enemyPieces;
    uint64 attacksRight = attack::getBlackPawnAttacksRight(pawns) & enemyPieces;
    while (attacksLeft) {
        int to = getLSB(attacksLeft);
        int move = getMove(to + 7, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to + 9, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    while (pawnStarts) {
        int to = getLSB(pawnStarts);
        int move = getMove(to + 16, to, NO_PIECE, PAWN_START_FLAG);
        moves.emplace_back(move, pawnStartScore);
        pawnStarts &= pawnStarts - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 16 && board[ep + 7] == BLACK_PAWN) {
            int move = getMove(ep + 7, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
        }
        if (ep != 23 && board[ep + 9] == BLACK_PAWN) {
            int move = getMove(ep + 9, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
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
            int move = getMove(E1, G1, NO_PIECE, CASTLE_FLAG);
            moves.emplace_back(move, castleScore);
        }
    }
    if (castlePerms & CASTLE_WQ) {
        if (!(allPieces & 0xE) && !board.squaresAttacked(0x1C, BLACK)) {
            int move = getMove(E1, C1, NO_PIECE, CASTLE_FLAG);
            moves.emplace_back(move, castleScore);
        }
    }
}
void MoveList::generateBlackCastleMoves() {
    int castlePerms = board.getCastlePerms();
    uint64 allPieces = board.getColorBitboard(BOTH_COLORS);
    if (castlePerms & CASTLE_BK) {
        if (!(allPieces & 0x6000000000000000ULL) &&
            !board.squaresAttacked(0x7000000000000000ULL, WHITE)) {
            int move = getMove(E8, G8, NO_PIECE, CASTLE_FLAG);
            moves.emplace_back(move, castleScore);
        }
    }
    if (castlePerms & CASTLE_BQ) {
        if (!(allPieces & 0x0E00000000000000ULL) &&
            !board.squaresAttacked(0x1C00000000000000ULL, WHITE)) {
            int move = getMove(E8, C8, NO_PIECE, CASTLE_FLAG);
            moves.emplace_back(move, castleScore);
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
    moves.reserve(50);
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
        int move = getMove(to - 7, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to - 9, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[WHITE_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 47 && board[ep - 7] == WHITE_PAWN) {
            int move = getMove(ep - 7, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
        }
        if (ep != 40 && board[ep - 9] == WHITE_PAWN) {
            int move = getMove(ep - 9, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
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
        int move = getMove(to + 7, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksLeft &= attacksLeft - 1;
    }
    while (attacksRight) {
        int to = getLSB(attacksRight);
        int move = getMove(to + 9, to, board[to], CAPTURE_FLAG);
        addPawnMove(move, captureScore[BLACK_PAWN][board[to]]);
        attacksRight &= attacksRight - 1;
    }
    int ep = board.getEnPassantSquare();
    if (ep != INVALID) {
        if (ep != 16 && board[ep + 7] == BLACK_PAWN) {
            int move = getMove(ep + 7, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
        }
        if (ep != 23 && board[ep + 9] == BLACK_PAWN) {
            int move = getMove(ep + 9, ep, NO_PIECE, EN_PASSANT_FLAG);
            moves.emplace_back(move, enPassantScore);
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
 * moves that match those stored by the killer move heuristic, the search
 * history heuristic, or the countermove heuristic, increase their scores as
 * well. This will improve our move ordering, which can lead to more pruning.
 *
 */
void MoveList::orderMoves(int bestMove, int killers[MAX_SEARCH_DEPTH][2],
                          int searchHistory[NUM_PIECE_TYPES][64],
                          int counterMove[NUM_PIECE_TYPES][64]) {
    for (Move& move : moves) {
        if (move.move == bestMove) {
            move.score = pvScore;
            continue;
        }
        if (move.move == killers[board.getSearchPly()][0]) {
            move.score = killerScore1;
            continue;
        }
        if (move.move == killers[board.getSearchPly()][1]) {
            move.score = killerScore2;
            continue;
        }
        if (!(move.move & (CAPTURE_FLAG | EN_PASSANT_FLAG))) {
            int prevMove = board.getPreviousMove();
            if (prevMove != INVALID) {
                int prevTo = (prevMove >> 6) & 0x3F;
                int prevPiece = board[prevTo];
                assert(prevPiece != INVALID);
                if (move.move == counterMove[prevPiece][prevTo]) {
                    move.score = counterMoveScore;
                    continue;
                }
            }
            int piece = board[move.move & 0x3F];
            int to = (move.move >> 6) & 0x3F;
            if (searchHistory[piece][to] > 0) {
                move.score = historyScore + searchHistory[piece][to];
            }
        }
    }
    auto compareMoves = [](const Move& m1, const Move& m2) -> bool {
        assert(m1.score > 0 && m2.score > 0);
        return m1.score > m2.score;
    };
    std::sort(moves.begin(), moves.end(), compareMoves);
}

