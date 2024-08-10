#include "Engine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>

constexpr inline int VERSION_MAJOR = 1;
constexpr inline int VERSION_MINOR = 1;
constexpr inline int VERSION_PATCH = 5;


static bool hasNextToken(std::stringstream& ss) {
    ss >> std::ws;
    return !ss.eof();
}


static void search(Engine& chessEngine, SearchInfo info) {
    chessEngine.searchPosition(info);
}


static void uci() {
    Engine engine;
    bool initialized = false;
    std::thread searchThread;
    std::cout << "id name DeepBlunder v" << VERSION_MAJOR <<
        "." << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
    std::cout << "id author Brandon Boettcher" << std::endl;
    std::cout << "uciok" << std::endl;
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        while (hasNextToken(ss)) {
            std::string tok;
            ss >> tok;
            tokens.push_back(tok);
        }
        int numTokens = (int) tokens.size();
        if (numTokens == 0) {
            continue;
        }
        if (tokens[0] == "isready") {
            assert(numTokens == 1);
            if (!initialized) {
                engine.initialize();
                initialized = true;
            }
            std::cout << "readyok" << std::endl;
        } else if (tokens[0] == "position") {
            assert(numTokens > 1);
            int index = 1;
            if (tokens[index] == "fen") {
                assert(numTokens >= 8);
                std::string fen;
                for (int i = 0; i < 6; ++i) {
                    fen += tokens[++index] + " ";
                }
                fen.pop_back();
                engine.setupBoard(fen);
            } else {
                assert(tokens[index] == "startpos");
                engine.setupBoard();
            }
            if (numTokens > ++index) {
                assert(tokens[index] == "moves");
                std::vector<std::string> moves;
                while (++index < numTokens) {
                    moves.push_back(tokens[index]);
                }
                engine.makeMoves(moves);
            }
        } else if (tokens[0] == "go") {
            SearchInfo info;
            int index = 0;
            while (++index < numTokens) {
                if (tokens[index] == "depth") {
                    assert(index + 1 < numTokens);
                    info.maxDepth = std::stoi(tokens[++index]);
                } else if (tokens[index] == "winc") {
                    assert(index + 1 < numTokens);
                    info.inc[WHITE] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "binc") {
                    assert(index + 1 < numTokens);
                    info.inc[BLACK] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "wtime") {
                    assert(index + 1 < numTokens);
                    info.time[WHITE] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "btime") {
                    assert(index + 1 < numTokens);
                    info.time[BLACK] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "movetime") {
                    assert(index + 1 < numTokens);
                    info.movetime = std::stoi(tokens[++index]);
                } else if (tokens[index] == "movestogo") {
                    assert(index + 1 < numTokens);
                    info.movestogo = std::stoi(tokens[++index]);
                }
            }
            if (searchThread.joinable()) {
                searchThread.join();
            }
            searchThread = std::thread(search, std::ref(engine), info);
        } else if (tokens[0] == "stop") {
            assert(numTokens == 1);
            engine.stopSearch();
        } else if (tokens[0] == "quit") {
            assert(numTokens == 1);
            engine.stopSearch();
            break;
        }
    }
    if (searchThread.joinable()) {
        searchThread.join();
    }
}


int main() {
    std::string protocol;
    std::cin >> protocol;
    if (protocol == "uci") {
        uci();
    } else {
        std::cout << "Error: unrecognized protocol" << std::endl;
        std::cout << "Running perft tests instead." << std::endl;
        Engine engine;
        engine.initialize();
        engine.runPerftTests();
    }
}
