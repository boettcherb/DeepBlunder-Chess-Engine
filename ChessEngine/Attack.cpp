#include "Attack.h"
#include "Defs.h"

namespace attack {

    /*
     *
     * For each index (0-63) there is a 1-bit in the positions that make up a ray
     * headed in a certain direction from that index. There 8 tables for the 8
     * cardinal and intercardinal directions (North, South, East, West, Northeast,
     * Northwest, Southeast, and Southwest). These tables can be used to generate
     * attack bitboards for sliding pieces. However, this method is relatively slow
     * compared to magic bitboards, and are thus only used to initialize the rook
     * and bishop attack tables during the initialization of the chess engine.
     * Remember: bit 0 = A1, bit 1 = B1, ..., bit 62 = G8, bit 63 = H8.
     *
     *     Ex: rayNorth[F2] =           |         Ex: raySouthEast[C7] =
     *      0 0 0 0 0 1 0 0             |            0 0 0 0 0 0 0 0
     *      0 0 0 0 0 1 0 0             |            0 0 0 0 0 0 0 0
     *      0 0 0 0 0 1 0 0             |            0 0 0 1 0 0 0 0
     *      0 0 0 0 0 1 0 0             |            0 0 0 0 1 0 0 0
     *      0 0 0 0 0 1 0 0             |            0 0 0 0 0 1 0 0
     *      0 0 0 0 0 1 0 0             |            0 0 0 0 0 0 1 0
     *      0 0 0 0 0 0 0 0             |            0 0 0 0 0 0 0 1
     *      0 0 0 0 0 0 0 0             |            0 0 0 0 0 0 0 0
     *
     */
    static inline constexpr uint64 rayNorth[64] = {
        0x0101010101010100, 0x0202020202020200, 0x0404040404040400, 0x0808080808080800,
        0x1010101010101000, 0x2020202020202000, 0x4040404040404000, 0x8080808080808000,
        0x0101010101010000, 0x0202020202020000, 0x0404040404040000, 0x0808080808080000,
        0x1010101010100000, 0x2020202020200000, 0x4040404040400000, 0x8080808080800000,
        0x0101010101000000, 0x0202020202000000, 0x0404040404000000, 0x0808080808000000,
        0x1010101010000000, 0x2020202020000000, 0x4040404040000000, 0x8080808080000000,
        0x0101010100000000, 0x0202020200000000, 0x0404040400000000, 0x0808080800000000,
        0x1010101000000000, 0x2020202000000000, 0x4040404000000000, 0x8080808000000000,
        0x0101010000000000, 0x0202020000000000, 0x0404040000000000, 0x0808080000000000,
        0x1010100000000000, 0x2020200000000000, 0x4040400000000000, 0x8080800000000000,
        0x0101000000000000, 0x0202000000000000, 0x0404000000000000, 0x0808000000000000,
        0x1010000000000000, 0x2020000000000000, 0x4040000000000000, 0x8080000000000000,
        0x0100000000000000, 0x0200000000000000, 0x0400000000000000, 0x0800000000000000,
        0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    };
    static inline constexpr uint64 raySouth[64] = {
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000001, 0x0000000000000002, 0x0000000000000004, 0x0000000000000008,
        0x0000000000000010, 0x0000000000000020, 0x0000000000000040, 0x0000000000000080,
        0x0000000000000101, 0x0000000000000202, 0x0000000000000404, 0x0000000000000808,
        0x0000000000001010, 0x0000000000002020, 0x0000000000004040, 0x0000000000008080,
        0x0000000000010101, 0x0000000000020202, 0x0000000000040404, 0x0000000000080808,
        0x0000000000101010, 0x0000000000202020, 0x0000000000404040, 0x0000000000808080,
        0x0000000001010101, 0x0000000002020202, 0x0000000004040404, 0x0000000008080808,
        0x0000000010101010, 0x0000000020202020, 0x0000000040404040, 0x0000000080808080,
        0x0000000101010101, 0x0000000202020202, 0x0000000404040404, 0x0000000808080808,
        0x0000001010101010, 0x0000002020202020, 0x0000004040404040, 0x0000008080808080,
        0x0000010101010101, 0x0000020202020202, 0x0000040404040404, 0x0000080808080808,
        0x0000101010101010, 0x0000202020202020, 0x0000404040404040, 0x0000808080808080,
        0x0001010101010101, 0x0002020202020202, 0x0004040404040404, 0x0008080808080808,
        0x0010101010101010, 0x0020202020202020, 0x0040404040404040, 0x0080808080808080,
    };
    static inline constexpr uint64 rayEast[64] = {
        0x00000000000000FE, 0x00000000000000FC, 0x00000000000000F8, 0x00000000000000F0,
        0x00000000000000E0, 0x00000000000000C0, 0x0000000000000080, 0x0000000000000000,
        0x000000000000FE00, 0x000000000000FC00, 0x000000000000F800, 0x000000000000F000,
        0x000000000000E000, 0x000000000000C000, 0x0000000000008000, 0x0000000000000000,
        0x0000000000FE0000, 0x0000000000FC0000, 0x0000000000F80000, 0x0000000000F00000,
        0x0000000000E00000, 0x0000000000C00000, 0x0000000000800000, 0x0000000000000000,
        0x00000000FE000000, 0x00000000FC000000, 0x00000000F8000000, 0x00000000F0000000,
        0x00000000E0000000, 0x00000000C0000000, 0x0000000080000000, 0x0000000000000000,
        0x000000FE00000000, 0x000000FC00000000, 0x000000F800000000, 0x000000F000000000,
        0x000000E000000000, 0x000000C000000000, 0x0000008000000000, 0x0000000000000000,
        0x0000FE0000000000, 0x0000FC0000000000, 0x0000F80000000000, 0x0000F00000000000,
        0x0000E00000000000, 0x0000C00000000000, 0x0000800000000000, 0x0000000000000000,
        0x00FE000000000000, 0x00FC000000000000, 0x00F8000000000000, 0x00F0000000000000,
        0x00E0000000000000, 0x00C0000000000000, 0x0080000000000000, 0x0000000000000000,
        0xFE00000000000000, 0xFC00000000000000, 0xF800000000000000, 0xF000000000000000,
        0xE000000000000000, 0xC000000000000000, 0x8000000000000000, 0x0000000000000000,
    };
    static inline constexpr uint64 rayWest[64] = {
        0x0000000000000000, 0x0000000000000001, 0x0000000000000003, 0x0000000000000007,
        0x000000000000000F, 0x000000000000001F, 0x000000000000003F, 0x000000000000007F,
        0x0000000000000000, 0x0000000000000100, 0x0000000000000300, 0x0000000000000700,
        0x0000000000000F00, 0x0000000000001F00, 0x0000000000003F00, 0x0000000000007F00,
        0x0000000000000000, 0x0000000000010000, 0x0000000000030000, 0x0000000000070000,
        0x00000000000F0000, 0x00000000001F0000, 0x00000000003F0000, 0x00000000007F0000,
        0x0000000000000000, 0x0000000001000000, 0x0000000003000000, 0x0000000007000000,
        0x000000000F000000, 0x000000001F000000, 0x000000003F000000, 0x000000007F000000,
        0x0000000000000000, 0x0000000100000000, 0x0000000300000000, 0x0000000700000000,
        0x0000000F00000000, 0x0000001F00000000, 0x0000003F00000000, 0x0000007F00000000,
        0x0000000000000000, 0x0000010000000000, 0x0000030000000000, 0x0000070000000000,
        0x00000F0000000000, 0x00001F0000000000, 0x00003F0000000000, 0x00007F0000000000,
        0x0000000000000000, 0x0001000000000000, 0x0003000000000000, 0x0007000000000000,
        0x000F000000000000, 0x001F000000000000, 0x003F000000000000, 0x007F000000000000,
        0x0000000000000000, 0x0100000000000000, 0x0300000000000000, 0x0700000000000000,
        0x0F00000000000000, 0x1F00000000000000, 0x3F00000000000000, 0x7F00000000000000,
    };
    static inline constexpr uint64 rayNorthWest[64] = {
        0x0000000000000000, 0x0000000000000100, 0x0000000000010200, 0x0000000001020400,
        0x0000000102040800, 0x0000010204081000, 0x0001020408102000, 0x0102040810204000,
        0x0000000000000000, 0x0000000000010000, 0x0000000001020000, 0x0000000102040000,
        0x0000010204080000, 0x0001020408100000, 0x0102040810200000, 0x0204081020400000,
        0x0000000000000000, 0x0000000001000000, 0x0000000102000000, 0x0000010204000000,
        0x0001020408000000, 0x0102040810000000, 0x0204081020000000, 0x0408102040000000,
        0x0000000000000000, 0x0000000100000000, 0x0000010200000000, 0x0001020400000000,
        0x0102040800000000, 0x0204081000000000, 0x0408102000000000, 0x0810204000000000,
        0x0000000000000000, 0x0000010000000000, 0x0001020000000000, 0x0102040000000000,
        0x0204080000000000, 0x0408100000000000, 0x0810200000000000, 0x1020400000000000,
        0x0000000000000000, 0x0001000000000000, 0x0102000000000000, 0x0204000000000000,
        0x0408000000000000, 0x0810000000000000, 0x1020000000000000, 0x2040000000000000,
        0x0000000000000000, 0x0100000000000000, 0x0200000000000000, 0x0400000000000000,
        0x0800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    };
    static inline constexpr uint64 rayNorthEast[64] = {
        0x8040201008040200, 0x0080402010080400, 0x0000804020100800, 0x0000008040201000,
        0x0000000080402000, 0x0000000000804000, 0x0000000000008000, 0x0000000000000000,
        0x4020100804020000, 0x8040201008040000, 0x0080402010080000, 0x0000804020100000,
        0x0000008040200000, 0x0000000080400000, 0x0000000000800000, 0x0000000000000000,
        0x2010080402000000, 0x4020100804000000, 0x8040201008000000, 0x0080402010000000,
        0x0000804020000000, 0x0000008040000000, 0x0000000080000000, 0x0000000000000000,
        0x1008040200000000, 0x2010080400000000, 0x4020100800000000, 0x8040201000000000,
        0x0080402000000000, 0x0000804000000000, 0x0000008000000000, 0x0000000000000000,
        0x0804020000000000, 0x1008040000000000, 0x2010080000000000, 0x4020100000000000,
        0x8040200000000000, 0x0080400000000000, 0x0000800000000000, 0x0000000000000000,
        0x0402000000000000, 0x0804000000000000, 0x1008000000000000, 0x2010000000000000,
        0x4020000000000000, 0x8040000000000000, 0x0080000000000000, 0x0000000000000000,
        0x0200000000000000, 0x0400000000000000, 0x0800000000000000, 0x1000000000000000,
        0x2000000000000000, 0x4000000000000000, 0x8000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
    };
    static inline constexpr uint64 raySouthWest[64] = {
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000001, 0x0000000000000002, 0x0000000000000004,
        0x0000000000000008, 0x0000000000000010, 0x0000000000000020, 0x0000000000000040,
        0x0000000000000000, 0x0000000000000100, 0x0000000000000201, 0x0000000000000402,
        0x0000000000000804, 0x0000000000001008, 0x0000000000002010, 0x0000000000004020,
        0x0000000000000000, 0x0000000000010000, 0x0000000000020100, 0x0000000000040201,
        0x0000000000080402, 0x0000000000100804, 0x0000000000201008, 0x0000000000402010,
        0x0000000000000000, 0x0000000001000000, 0x0000000002010000, 0x0000000004020100,
        0x0000000008040201, 0x0000000010080402, 0x0000000020100804, 0x0000000040201008,
        0x0000000000000000, 0x0000000100000000, 0x0000000201000000, 0x0000000402010000,
        0x0000000804020100, 0x0000001008040201, 0x0000002010080402, 0x0000004020100804,
        0x0000000000000000, 0x0000010000000000, 0x0000020100000000, 0x0000040201000000,
        0x0000080402010000, 0x0000100804020100, 0x0000201008040201, 0x0000402010080402,
        0x0000000000000000, 0x0001000000000000, 0x0002010000000000, 0x0004020100000000,
        0x0008040201000000, 0x0010080402010000, 0x0020100804020100, 0x0040201008040201,
    };
    static inline constexpr uint64 raySouthEast[64] = {
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000,
        0x0000000000000002, 0x0000000000000004, 0x0000000000000008, 0x0000000000000010,
        0x0000000000000020, 0x0000000000000040, 0x0000000000000080, 0x0000000000000000,
        0x0000000000000204, 0x0000000000000408, 0x0000000000000810, 0x0000000000001020,
        0x0000000000002040, 0x0000000000004080, 0x0000000000008000, 0x0000000000000000,
        0x0000000000020408, 0x0000000000040810, 0x0000000000081020, 0x0000000000102040,
        0x0000000000204080, 0x0000000000408000, 0x0000000000800000, 0x0000000000000000,
        0x0000000002040810, 0x0000000004081020, 0x0000000008102040, 0x0000000010204080,
        0x0000000020408000, 0x0000000040800000, 0x0000000080000000, 0x0000000000000000,
        0x0000000204081020, 0x0000000408102040, 0x0000000810204080, 0x0000001020408000,
        0x0000002040800000, 0x0000004080000000, 0x0000008000000000, 0x0000000000000000,
        0x0000020408102040, 0x0000040810204080, 0x0000081020408000, 0x0000102040800000,
        0x0000204080000000, 0x0000408000000000, 0x0000800000000000, 0x0000000000000000,
        0x0002040810204080, 0x0004081020408000, 0x0008102040800000, 0x0010204080000000,
        0x0020408000000000, 0x0040800000000000, 0x0080000000000000, 0x0000000000000000,
    };


    /*
     *
     * For each index (0-63) there is a 1-bit in the positions that a bishop (for
     * bishopAttacks[]) or a rook (for rookAttacks[]) at that index would be able
     * to attack. These arrays are only used during the initialization of the chess
     * engine, in the functions bishopAttacksSlow(), initializeBishopAttackTable(),
     * rookAttacksSlow(), and initializeRookAttackTable().
     * Remember: bit 0 = A1, bit 1 = B1, ... , bit 62 = G8, bit 63 = H8.
     *
     *    Ex: bishopAttacks[D4] =     |       Ex: rookAttacks[D4] =
     *        0 0 0 0 0 0 0 1         |         0 0 0 1 0 0 0 0
     *        1 0 0 0 0 0 1 0         |         0 0 0 1 0 0 0 0
     *        0 1 0 0 0 1 0 0         |         0 0 0 1 0 0 0 0
     *        0 0 1 0 1 0 0 0         |         0 0 0 1 0 0 0 0
     *        0 0 0 0 0 0 0 0         |         1 1 1 0 1 1 1 1
     *        0 0 1 0 1 0 0 0         |         0 0 0 1 0 0 0 0
     *        0 1 0 0 0 1 0 0         |         0 0 0 1 0 0 0 0
     *        1 0 0 0 0 0 1 0         |         0 0 0 1 0 0 0 0
     *
     */
    static inline constexpr uint64 bishopAttacks[64] = {
        0x8040201008040200, 0x0080402010080500, 0x0000804020110A00, 0x0000008041221400,
        0x0000000182442800, 0x0000010204885000, 0x000102040810A000, 0x0102040810204000,
        0x4020100804020002, 0x8040201008050005, 0x00804020110A000A, 0x0000804122140014,
        0x0000018244280028, 0x0001020488500050, 0x0102040810A000A0, 0x0204081020400040,
        0x2010080402000204, 0x4020100805000508, 0x804020110A000A11, 0x0080412214001422,
        0x0001824428002844, 0x0102048850005088, 0x02040810A000A010, 0x0408102040004020,
        0x1008040200020408, 0x2010080500050810, 0x4020110A000A1120, 0x8041221400142241,
        0x0182442800284482, 0x0204885000508804, 0x040810A000A01008, 0x0810204000402010,
        0x0804020002040810, 0x1008050005081020, 0x20110A000A112040, 0x4122140014224180,
        0x8244280028448201, 0x0488500050880402, 0x0810A000A0100804, 0x1020400040201008,
        0x0402000204081020, 0x0805000508102040, 0x110A000A11204080, 0x2214001422418000,
        0x4428002844820100, 0x8850005088040201, 0x10A000A010080402, 0x2040004020100804,
        0x0200020408102040, 0x0500050810204080, 0x0A000A1120408000, 0x1400142241800000,
        0x2800284482010000, 0x5000508804020100, 0xA000A01008040201, 0x4000402010080402,
        0x0002040810204080, 0x0005081020408000, 0x000A112040800000, 0x0014224180000000,
        0x0028448201000000, 0x0050880402010000, 0x00A0100804020100, 0x0040201008040201,
    };
    static inline constexpr uint64 rookAttacks[64] = {
        0x01010101010101FE, 0x02020202020202FD, 0x04040404040404FB, 0x08080808080808F7,
        0x10101010101010EF, 0x20202020202020DF, 0x40404040404040BF, 0x808080808080807F,
        0x010101010101FE01, 0x020202020202FD02, 0x040404040404FB04, 0x080808080808F708,
        0x101010101010EF10, 0x202020202020DF20, 0x404040404040BF40, 0x8080808080807F80,
        0x0101010101FE0101, 0x0202020202FD0202, 0x0404040404FB0404, 0x0808080808F70808,
        0x1010101010EF1010, 0x2020202020DF2020, 0x4040404040BF4040, 0x80808080807F8080,
        0x01010101FE010101, 0x02020202FD020202, 0x04040404FB040404, 0x08080808F7080808,
        0x10101010EF101010, 0x20202020DF202020, 0x40404040BF404040, 0x808080807F808080,
        0x010101FE01010101, 0x020202FD02020202, 0x040404FB04040404, 0x080808F708080808,
        0x101010EF10101010, 0x202020DF20202020, 0x404040BF40404040, 0x8080807F80808080,
        0x0101FE0101010101, 0x0202FD0202020202, 0x0404FB0404040404, 0x0808F70808080808,
        0x1010EF1010101010, 0x2020DF2020202020, 0x4040BF4040404040, 0x80807F8080808080,
        0x01FE010101010101, 0x02FD020202020202, 0x04FB040404040404, 0x08F7080808080808,
        0x10EF101010101010, 0x20DF202020202020, 0x40BF404040404040, 0x807F808080808080,
        0xFE01010101010101, 0xFD02020202020202, 0xFB04040404040404, 0xF708080808080808,
        0xEF10101010101010, 0xDF20202020202020, 0xBF40404040404040, 0x7F80808080808080,
    };


    /*
     *
     * Given the square index of a sliding-piece (a bishop or queen when using
     * bishopAttacksSlow() and a rook or queen when using rookAttacksSlow()) and a
     * bitboard of every piece on the chessboard, return a bitboard containing the
     * attacks of that sliding piece.
     *
     * Example: given the bitboard of all pieces on the chessboard, find the
     *          possible attacks for a queen on E3:
     *
     * All pieces on the chessboard:    |    Attacks of a queen on E3:
     *       1 0 1 0 0 0 1 0            |         0 0 0 0 0 0 0 0
     *       1 0 0 0 0 0 1 0            |         0 0 0 0 0 0 0 0
     *       0 1 0 0 1 0 1 1            |         0 1 0 0 0 0 0 1
     *       0 0 0 1 1 0 0 0            |         0 0 1 0 1 0 1 0
     *       0 0 0 0 0 0 0 0            |         0 0 0 1 1 1 0 0
     *       0 1 1 1 1 0 0 0            |         0 0 0 1 0 1 1 1
     *       0 1 0 0 0 1 0 0            |         0 0 0 1 1 1 0 0
     *       0 0 1 0 0 0 1 0            |         0 0 1 0 1 0 0 0
     *
     * Note that the possible attacks of a sliding piece include the first blocker
     * in any direction. For example, the first blocker to the North of the queen
     * on E3 is on E5, and this square is included in the attacks bitboard for the
     * queen. This is so that captures are included in the attacks. If this piece
     * is the same color as the queen, we can remove it later from the attack
     * bitboard using bitwise operators. For Example: if the queen is white:
     * queenAttacks & ~colorBitboards[WHITE] will remove captures of white pieces.
     *
     * Generating an attack bitboard for sliding pieces (rook, bishop, queen) is
     * more difficult than for non-sliding pieces (king, knight, pawn) because we
     * must take into account the positions of every other piece on the board.
     * Pieces that limit the movement of sliding pieces are called "blockers" and
     * can be found using the ray bitboards (ex: bitboard & rayNorth[sq] gives the
     * blockers to the north of a rook on the square 'sq'). Since there can be
     * multiple blockers in a single direction, use the getLSB() and getMSB()
     * functions (defined in Defs.c) to find the blocker that is closest to the
     * sliding piece. Use this blocker and the ray bitboards to limit the attacks
     * of the sliding piece and to generate the correct attack bitboard.
     *
     * These functions (bishopAttacksSlow() and rookAttacksSlow()) and the ray
     * bitboards are only used during the intialization of the chess engine to
     * fill the sliding piece attack tables. The functions getBishopAttacks(),
     * getRookAttacks(), and getQueenAttacks() will be used during runtime to
     * quickly query attack bitboards from the attack tables.
     *
     */
    static uint64 bishopAttacksSlow(int square, uint64 allPieces) {
        assert(square >= 0 && square < 64);
        uint64 bishopMoves = bishopAttacks[square];
        uint64 blockersNorthEast = rayNorthEast[square] & allPieces;
        uint64 blockersNorthWest = rayNorthWest[square] & allPieces;
        uint64 blockersSouthEast = raySouthEast[square] & allPieces;
        uint64 blockersSouthWest = raySouthWest[square] & allPieces;
        if (blockersNorthEast) {
            bishopMoves &= ~rayNorthEast[getLSB(blockersNorthEast)];
        }
        if (blockersNorthWest) {
            bishopMoves &= ~rayNorthWest[getLSB(blockersNorthWest)];
        }
        if (blockersSouthEast) {
            bishopMoves &= ~raySouthEast[getMSB(blockersSouthEast)];
        }
        if (blockersSouthWest) {
            bishopMoves &= ~raySouthWest[getMSB(blockersSouthWest)];
        }
        return bishopMoves;
    }
    static uint64 rookAttacksSlow(int square, uint64 allPieces) {
        assert(square >= 0 && square < 64);
        uint64 rookMoves = rookAttacks[square];
        uint64 blockersNorth = rayNorth[square] & allPieces;
        uint64 blockersSouth = raySouth[square] & allPieces;
        uint64 blockersEast = rayEast[square] & allPieces;
        uint64 blockersWest = rayWest[square] & allPieces;
        if (blockersNorth) {
            rookMoves &= ~rayNorth[getLSB(blockersNorth)];
        }
        if (blockersSouth) {
            rookMoves &= ~raySouth[getMSB(blockersSouth)];
        }
        if (blockersEast) {
            rookMoves &= ~rayEast[getLSB(blockersEast)];
        }
        if (blockersWest) {
            rookMoves &= ~rayWest[getMSB(blockersWest)];
        }
        return rookMoves;
    }


    /*
     * Sliding piece blockers. A "blocker" is a piece that limits the movement of a
     * sliding piece. A blocker must be on the same diagonal as a bishop and the
     * same rank/file as a rook. Pieces on the edge of the board are not blockers.
     * For example, a rook on D4 is "blocked" by a piece on C4 and B4, but not by a
     * piece on A4. The rook can "see" A4 regardless of whether a piece is there.
     * There is no queenBlockers[] because a queen is just the combination of a
     * rook and bishop.
     * Remember: bit 0 = A1, bit 1 = B1, ... , bit 62 = G8, bit 63 = H8.
     *
     *    Ex: bishopBlockers[D4]                      Ex: rookBlockers[D4]
     *       0 0 0 0 0 0 0 0               |            0 0 0 0 0 0 0 0
     *       0 0 0 0 0 0 1 0               |            0 0 0 1 0 0 0 0
     *       0 1 0 0 0 1 0 0               |            0 0 0 1 0 0 0 0
     *       0 0 1 0 1 0 0 0               |            0 0 0 1 0 0 0 0
     *       0 0 0 0 0 0 0 0               |            0 1 1 0 1 1 1 0
     *       0 0 1 0 1 0 0 0               |            0 0 0 1 0 0 0 0
     *       0 1 0 0 0 1 0 0               |            0 0 0 1 0 0 0 0
     *       0 0 0 0 0 0 0 0               |            0 0 0 0 0 0 0 0
     */
    static inline constexpr uint64 bishopBlockers[64] = {
        0x0040201008040200, 0x0000402010080400, 0x0000004020100A00, 0x0000000040221400,
        0x0000000002442800, 0x0000000204085000, 0x0000020408102000, 0x0002040810204000,
        0x0020100804020000, 0x0040201008040000, 0x00004020100A0000, 0x0000004022140000,
        0x0000000244280000, 0x0000020408500000, 0x0002040810200000, 0x0004081020400000,
        0x0010080402000200, 0x0020100804000400, 0x004020100A000A00, 0x0000402214001400,
        0x0000024428002800, 0x0002040850005000, 0x0004081020002000, 0x0008102040004000,
        0x0008040200020400, 0x0010080400040800, 0x0020100A000A1000, 0x0040221400142200,
        0x0002442800284400, 0x0004085000500800, 0x0008102000201000, 0x0010204000402000,
        0x0004020002040800, 0x0008040004081000, 0x00100A000A102000, 0x0022140014224000,
        0x0044280028440200, 0x0008500050080400, 0x0010200020100800, 0x0020400040201000,
        0x0002000204081000, 0x0004000408102000, 0x000A000A10204000, 0x0014001422400000,
        0x0028002844020000, 0x0050005008040200, 0x0020002010080400, 0x0040004020100800,
        0x0000020408102000, 0x0000040810204000, 0x00000A1020400000, 0x0000142240000000,
        0x0000284402000000, 0x0000500804020000, 0x0000201008040200, 0x0000402010080400,
        0x0002040810204000, 0x0004081020400000, 0x000A102040000000, 0x0014224000000000,
        0x0028440200000000, 0x0050080402000000, 0x0020100804020000, 0x0040201008040200,
    };
    static inline constexpr uint64 rookBlockers[64] = {
        0x000101010101017E, 0x000202020202027C, 0x000404040404047A, 0x0008080808080876,
        0x001010101010106E, 0x002020202020205E, 0x004040404040403E, 0x008080808080807E,
        0x0001010101017E00, 0x0002020202027C00, 0x0004040404047A00, 0x0008080808087600,
        0x0010101010106E00, 0x0020202020205E00, 0x0040404040403E00, 0x0080808080807E00,
        0x00010101017E0100, 0x00020202027C0200, 0x00040404047A0400, 0x0008080808760800,
        0x00101010106E1000, 0x00202020205E2000, 0x00404040403E4000, 0x00808080807E8000,
        0x000101017E010100, 0x000202027C020200, 0x000404047A040400, 0x0008080876080800,
        0x001010106E101000, 0x002020205E202000, 0x004040403E404000, 0x008080807E808000,
        0x0001017E01010100, 0x0002027C02020200, 0x0004047A04040400, 0x0008087608080800,
        0x0010106E10101000, 0x0020205E20202000, 0x0040403E40404000, 0x0080807E80808000,
        0x00017E0101010100, 0x00027C0202020200, 0x00047A0404040400, 0x0008760808080800,
        0x00106E1010101000, 0x00205E2020202000, 0x00403E4040404000, 0x00807E8080808000,
        0x007E010101010100, 0x007C020202020200, 0x007A040404040400, 0x0076080808080800,
        0x006E101010101000, 0x005E202020202000, 0x003E404040404000, 0x007E808080808000,
        0x7E01010101010100, 0x7C02020202020200, 0x7A04040404040400, 0x7608080808080800,
        0x6E10101010101000, 0x5E20202020202000, 0x3E40404040404000, 0x7E80808080808000,
    };


    /*
     *
     * Arrays to hold the maximum possible number of blockers for a rook or bishop
     * on the given square. A value of n on the square sq means that there are n
     * squares that could contain blockers for the square sq, and that the total
     * number of unique blocker bitboards for the sqare sq is 2^n (each of the n
     * squares either contains a blocker or doesn't).
     *
     */
    static inline constexpr int numBishopBlockers[64] = {
        6, 5, 5, 5, 5, 5, 5, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 9, 9, 7, 5, 5,
        5, 5, 7, 7, 7, 7, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 5, 5, 5, 5, 5, 5, 6,
    };
    static inline constexpr int numRookBlockers[64] = {
        12, 11, 11, 11, 11, 11, 11, 12,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        11, 10, 10, 10, 10, 10, 10, 11,
        12, 11, 11, 11, 11, 11, 11, 12,
    };


    /*
     *
     * Magic number tables. These magic numbers are used when querying into the
     * bishop and rook attack tables to retrieve an attack bitboard. A blocker
     * bitboard, when multiplied by a magic number, generates a bitboard where the
     * upper n bits can be used as an index into the attack tables. This bitboard
     * can then be shifted left by 64 - n to retrieve the index.
     *
     * https://www.chessprogramming.org/Magic_Bitboards
     *
     */
    static inline constexpr uint64 bishopMagics[64] = {
        0x89a1121896040240, 0x2004844802002010, 0x2068080051921000, 0x62880a0220200808,
        0x0004042004000000, 0x0100822020200011, 0xc00444222012000a, 0x0028808801216001,
        0x0400492088408100, 0x0201c401040c0084, 0x00840800910a0010, 0x0000082080240060,
        0x2000840504006000, 0x30010c4108405004, 0x1008005410080802, 0x8144042209100900,
        0x0208081020014400, 0x004800201208ca00, 0x0F18140408012008, 0x1004002802102001,
        0x0841000820080811, 0x0040200200a42008, 0x0000800054042000, 0x88010400410c9000,
        0x0520040470104290, 0x1004040051500081, 0x2002081833080021, 0x000400c00c010142,
        0x941408200c002000, 0x0658810000806011, 0x0188071040440a00, 0x4800404002011c00,
        0x0104442040404200, 0x0511080202091021, 0x0004022401120400, 0x80c0040400080120,
        0x8040010040820802, 0x0480810700020090, 0x0102008e00040242, 0x0809005202050100,
        0x8002024220104080, 0x0431008804142000, 0x0019001802081400, 0x0200014208040080,
        0x3308082008200100, 0x041010500040c020, 0x4012020c04210308, 0x208220a202004080,
        0x0111040120082000, 0x6803040141280a00, 0x2101004202410000, 0x8200000041108022,
        0x0000021082088000, 0x0002410204010040, 0x0040100400809000, 0x0822088220820214,
        0x0040808090012004, 0x00910224040218c9, 0x0402814422015008, 0x0090014004842410,
        0x0001000042304105, 0x0010008830412a00, 0x2520081090008908, 0x40102000a0a60140,
    };
    static inline constexpr uint64 rookMagics[64] = {
        0x0A8002C000108020, 0x06C00049B0002001, 0x0100200010090040, 0x2480041000800801,
        0x0280028004000800, 0x0900410008040022, 0x0280020001001080, 0x2880002041000080,
        0xA000800080400034, 0x0004808020004000, 0x2290802004801000, 0x0411000D00100020,
        0x0402800800040080, 0x000B000401004208, 0x2409000100040200, 0x0001002100004082,
        0x0022878001E24000, 0x1090810021004010, 0x0801030040200012, 0x0500808008001000,
        0x0A08018014000880, 0x8000808004000200, 0x0201008080010200, 0x0801020000441091,
        0x0000800080204005, 0x1040200040100048, 0x0000120200402082, 0x0D14880480100080,
        0x0012040280080080, 0x0100040080020080, 0x9020010080800200, 0x0813241200148449,
        0x0491604001800080, 0x0100401000402001, 0x4820010021001040, 0x0400402202000812,
        0x0209009005000802, 0x0810800601800400, 0x4301083214000150, 0x204026458E001401,
        0x0040204000808000, 0x8001008040010020, 0x8410820820420010, 0x1003001000090020,
        0x0804040008008080, 0x0012000810020004, 0x1000100200040208, 0x430000A044020001,
        0x0280009023410300, 0x00E0100040002240, 0x0000200100401700, 0x2244100408008080,
        0x0008000400801980, 0x0002000810040200, 0x8010100228810400, 0x2000009044210200,
        0x4080008040102101, 0x0040002080411D01, 0x2005524060000901, 0x0502001008400422,
        0x489A000810200402, 0x0001004400080A13, 0x4000011008020084, 0x0026002114058042,
    };


    /*
     *
     * Sliding piece attack tables. For all 64 squares on the board, and for every
     * possible bitboard of blockers (512 for bishops, 4096 for rooks) for a piece
     * on that square, store all the possible attacks of that piece. A "blocker" is
     * any piece that could potentially limit the movement of a sliding piece. A
     * blocker bitboard (a bitboard with only blockers) is converted into an index
     * using magic numbers, and that index is used to index into the attack tables.
     *
     * There are up to 512 blocker bitboards for bishops because the maximum number
     * of blockers for a single bishop is 9 (as seen in numBishopBlockers[]), and
     * 2^9 == 512. Similarly, there are up to 4096 blocker bitboards for rooks
     * because the maximum number of blockers for a single rook is 12 (as seen in
     * numRookBlockers[]), and 2^12 == 4096.
     *
     */
    static uint64 bishopAttackTable[64][512];
    static uint64 rookAttackTable[64][4096];


    /*
     *
     * Initialize the bishop and rook attack tables. In these functions, every
     * possible blocker bitboard is generated for every square, and the functions
     * getBishopAttacksSlow() and getRookAttacksSlow() are used to generate an
     * attack bitboard for each blocker bitboard. These attack bitboards are
     * stored in the bishop and rook attack tables, and during execution are
     * queried using blocker bitboards and magic numbers.
     *
     */
    void initializeBishopAttackTable() {
        static bool initializedBishopAttackTable = false;
        if (initializedBishopAttackTable) {
            return;
        }
        initializedBishopAttackTable = true;
        for (int sq = 0; sq < 64; ++sq) {
            int numBlockerBoards = 1 << numBishopBlockers[sq];
            for (int blockerIdx = 0; blockerIdx < numBlockerBoards; ++blockerIdx) {
                uint64 innerAttacks = bishopBlockers[sq];
                uint64 blockers = 0ULL;
                for (int i = 0; innerAttacks; i++) {
                    if (blockerIdx & (1 << i)) {
                        blockers |= (1ULL << getLSB(innerAttacks));
                    }
                    innerAttacks &= innerAttacks - 1;
                }
                int shift = 64 - numBishopBlockers[sq];
                int index = static_cast<int>((blockers * bishopMagics[sq]) >> shift);
                assert(index < numBlockerBoards);
                bishopAttackTable[sq][index] = bishopAttacksSlow(sq, blockers);
            }
        }
    }
    void initializeRookAttackTable() {
        static bool initializedRookAttackTable = false;
        if (initializedRookAttackTable) {
            return;
        }
        initializedRookAttackTable = true;
        for (int sq = 0; sq < 64; ++sq) {
            int numBlockerBoards = 1 << numRookBlockers[sq];
            for (int blockerIdx = 0; blockerIdx < numBlockerBoards; ++blockerIdx) {
                uint64 innerAttacks = rookBlockers[sq];
                uint64 blockers = 0ULL;
                for (int i = 0; innerAttacks; i++) {
                    if (blockerIdx & (1 << i)) {
                        blockers |= (1ULL << getLSB(innerAttacks));
                    }
                    innerAttacks &= innerAttacks - 1;
                }
                int shift = 64 - numRookBlockers[sq];
                int index = static_cast<int>((blockers * rookMagics[sq]) >> shift);
                assert(index < numBlockerBoards);
                rookAttackTable[sq][index] = rookAttacksSlow(sq, blockers);
            }
        }
    }


    /*
     *
     * Given the position of a sliding piece and a bitboard of all the pieces on
     * the board, find the attack bitboard of that piece. Use getBishopAttacks()
     * for bishops, getRookAttacks() for rooks, and getQueenAttacks() for queens.
     * These functions use the allPieces bitboard to generate a bitboard of
     * blockers, then use this blocker bitboard to generate an index into the
     * attack tables.
     *
     */
    uint64 getBishopAttacks(int sq, uint64 allPieces) {
        assert(sq >= 0 && sq < 64);
        assert(allPieces & (1ULL << sq));
        uint64 blockers = allPieces & bishopBlockers[sq];
        int shift = 64 - numBishopBlockers[sq];
        int index = static_cast<int>((blockers * bishopMagics[sq]) >> shift);
        assert(index >= 0 && index < (1 << numBishopBlockers[sq]));
        return bishopAttackTable[sq][index];
    }
    uint64 getRookAttacks(int sq, uint64 allPieces) {
        assert(sq >= 0 && sq < 64);
        assert(allPieces & (1ULL << sq));
        uint64 blockers = allPieces & rookBlockers[sq];
        int shift = 64 - numRookBlockers[sq];
        int index = static_cast<int>((blockers * rookMagics[sq]) >> shift);
        assert(index >= 0 && index < (1 << numRookBlockers[sq]));
        return rookAttackTable[sq][index];
    }
    uint64 getQueenAttacks(int sq, uint64 allPieces) {
        assert(sq >= 0 && sq < 64);
        assert(allPieces & (1ULL << sq));
        return getBishopAttacks(sq, allPieces) | getRookAttacks(sq, allPieces);
    }

};