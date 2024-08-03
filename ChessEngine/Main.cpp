#include "Engine.h"

int main() {
    Engine chessEngine = Engine();
    chessEngine.setupBoard("r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1");
    SearchInfo info;
    info.maxDepth = 6;
    chessEngine.searchPosition(info);
}
