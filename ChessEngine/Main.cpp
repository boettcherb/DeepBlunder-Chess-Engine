#include "Engine.h"

int main() {
    Engine chessEngine = Engine();
    //chessEngine.setupBoard("r1b1k2r/ppppnppp/2n2q2/2b5/3NP3/2P1B3/PP3PPP/RN1QKB1R w KQkq - 0 1");
    //chessEngine.setupBoard("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    chessEngine.setupBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //chessEngine.setupBoard("r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1");
    //chessEngine.setupBoard("Q7/5p2/5P1p/5PPN/6Pk/4N1Rp/7P/6K1 w - - 0 1");
    chessEngine.searchPosition();
}
