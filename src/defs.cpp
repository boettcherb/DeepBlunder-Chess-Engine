#include "defs.h"
#include <chrono>


/*
 *
 * Retrieve the index of the given bitboard's least significant bit. Ex:
 * getLSB(0x1) = 0, getLSB(0x4) = 2, getLSB(0xC00) = 10. There is undefined
 * behavior if the bitboard is equal to 0.
 *
 */
int getLSB(uint64 bitboard) {
    assert(bitboard != 0);
#if defined(COMPILER_GCC)
    return __builtin_ctzll(bitboard);
#elif defined(COMPILER_MSVS)
    unsigned long index;
    _BitScanForward64(&index, bitboard);
    return static_cast<int>(index);
#else
    int index = 0;
    while (!(bitboard & 0x1)) {
        bitboard >>= 1;
        ++index;
    }
    return index;
#endif
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
#if defined(COMPILER_GCC)
    return 63 - __builtin_clzll(bitboard);
#elif defined(COMPILER_MSVS)
    unsigned long index;
    _BitScanReverse64(&index, bitboard);
    return static_cast<int>(index);
#else
    int index = 63;
    while (!(bitboard & 0x8000000000000000ULL)) {
        bitboard <<= 1;
        --index;
    }
    return index;
#endif
}


/*
 *
 * Count and return the number of bits that are set to 1 in the given
 * bitboard. Ex: countBits(11011100101) = 7, countBits(45) = 4.
 *
 */
int countBits(uint64 bitboard) {
#if defined(COMPILER_GCC)
    return __builtin_popcountll(bitboard);
#elif defined(COMPILER_MSVS)
    return static_cast<int>(__popcnt64(bitboard));
#else
    int bits = 0;
    while (bitboard) {
        bitboard &= bitboard - 1;
        ++bits;
    }
    return bits;
#endif
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
