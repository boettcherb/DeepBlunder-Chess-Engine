#include "HashKey.h"
#include "Defs.h"
#include <random>

namespace hashkey {

    /*
     *
     * These values are the hash keys that will be used to generate position keys
     * for each chess position. They are 64-bit values that are randomly generated
     * at the start of the program. A board's position key is generated using a
     * technique called Zobrist Hashing, which involves xor-ing many different hash
     * keys together. Every chess position has an (almost) unique position key,
     * which is used to quickly compare different board states without directly
     * comparing every member variable inside the Board class. Position keys take
     * into account the locations of every piece, the side to move, the castling
     * rights of each side, and the en passant square (if there is one).
     *
     * If 2 position keys from 2 boards are different, we can be certain that those
     * boards are different as well. This allows us to check for 3-fold repetition
     * and to compare hash table entries quickly by doing a single comparison.
     * However, it is possible, although VERY unlikely, that a hash collision can
     * occur, which will cause 2 different boards to generate the same position
     * key. Therefore, if 2 position keys are the same, we must manually verify
     * that the boards are equal as well. Since repetitions in chess are unlikely,
     * this should not have much of an impact on performance.
     *
     */
    static uint64 sideKey;
    static uint64 pieceKeys[NUM_PIECE_TYPES][64];
    static uint64 castleKeys[16];
    static uint64 enPassantKeys[64];


    static bool hash_keys_initialized = false;


    /*
     *
     * Initialize the hash keys that are used to generate a board's position key.
     * Initialization should happen only called once at the start of the program.
     * Each of the hash keys (sideKey, pieceKeys, castleKeys, enPassantKeys) are
     * given a random 64-bit value.
     *
     */
    void initHashKeys() {
        if (hash_keys_initialized) {
            return;
        }
        hash_keys_initialized = true;
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64> getRandUint64;

        sideKey = getRandUint64(rng);
        for (int i = 0; i < 16; ++i) {
            castleKeys[i] = getRandUint64(rng);
        }
        for (int i = 0; i < 64; ++i) {
            enPassantKeys[i] = getRandUint64(rng);
        }
        for (int i = 0; i < NUM_PIECE_TYPES; ++i) {
            for (int j = 0; j < 64; ++j) {
                pieceKeys[i][j] = getRandUint64(rng);
            }
        }
    }


    /*
     *
     * Retrieve the hash key that is used to factor in which side it is to move.
     * When it is white's move, the hash key is xor-ed into the board's position
     * key, and when it is black's move it is removed. This way, if 2 boards have
     * the same piece layouts but one has white to move and the other has black
     * to move, they will have a different position key.
     *
     */
    uint64 getSideKey() {
        assert(hash_keys_initialized);
        return sideKey;
    }


    /*
     *
     * Retrieve the hash key that is used to mark that a certain piece is on a
     * certain square. There are 64 * 12 = 768 different piece hash keys: one for
     * the 12 piece types on each of the 64 squares. These hash keys are used to
     * mark in the board's position key which pieces are on which squares so that
     * if 2 boards have different piece layouts, their position keys will also be
     * different.
     *
     */
    uint64 getPieceKey(int piece, int square) {
        assert(hash_keys_initialized);
        assert(piece >= 0 && piece < NUM_PIECE_TYPES);
        assert(square >= 0 && square < 64);
        return pieceKeys[piece][square];
    }


    /*
     *
     * Retrieve the hash key that is used to mark the castling permissions of a
     * chessboard. There are 16 castle hash keys, one for each of the 16 possible
     * permutations of castling permissions. If 2 boards have the same piece
     * layouts but different castling permissions, their 2 position keys will be
     * different.
     *
     */
    uint64 getCastleKey(int castlePerm) {
        assert(hash_keys_initialized);
        assert(castlePerm >= 0 && castlePerm < 16);
        return castleKeys[castlePerm];
    }


    /*
     *
     * Retrieve the hash key that is used to mark an en passant capture is possible
     * on the given square. There are 64 en passant hash keys, one for each square,
     * although only 16 of these will ever be used (the keys for the 3rd and 6th
     * ranks) because these are the only squares where en passant captures are
     * possible. If 2 boards have the same piece layouts but on one board an en
     * passant capture is possible, their 2 position keys will be different.
     *
     */
    uint64 getEnPassantKey(int square) {
        assert(hash_keys_initialized);
        assert(square >= 0 && square < 64);
        assert((1ULL << square) & 0x0000FF0000FF0000);
        return enPassantKeys[square];
    }

};
