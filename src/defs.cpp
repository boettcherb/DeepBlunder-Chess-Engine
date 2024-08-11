#include "defs.h"
#include <chrono>
#include <bit>


/*
 *
 * Retrieve the index of the given bitboard's least significant bit. Ex:
 * getLSB(0x1) = 0, getLSB(0x4) = 2, getLSB(0xC00) = 10. There is undefined
 * behavior if the bitboard is equal to 0.
 *
 */
int getLSB(uint64 bitboard) {
    assert(bitboard != 0);
    return std::countr_zero(bitboard);
}


/*
 *
 * Retrieve the index of the given bitboard's most significant bit. Ex:
 * getMSB(0x1) = 63, getLSB(0x8) = 60. There is undefined behavior if the
 * bitboard is equal to 0.
 *
 */
int getMSB(uint64 bitboard) {
    assert(bitboard != 0);
    return 63 - std::countl_zero(bitboard);
}


/*
 *
 * Count and return the number of bits that are set to 1 in the given
 * bitboard. Ex: countBits(11011100101) = 7, countBits(45) = 4.
 *
 */
int countBits(uint64 bitboard) {
    return std::popcount(bitboard);
}


/*
 *
 * Return a time value in milliseconds. Call this function twice, before and
 * after some block of code, to get the time in milliseconds that code took to
 * run.
 *
 */
uint64 currentTime() {
    using namespace std::chrono;
    auto curTime = high_resolution_clock::now().time_since_epoch();
    return static_cast<uint64>(duration_cast<milliseconds>(curTime).count());
}
