#include "Engine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>

constexpr inline int VERSION_MAJOR = 1;
constexpr inline int VERSION_MINOR = 0;

static bool hasNextToken(std::stringstream& ss) {
    ss >> std::ws;
    return !ss.eof();
}


static void search(Engine& chessEngine, SearchInfo info) {
    chessEngine.searchPosition(info);
}


static void uci() {
    Engine engine;
    std::thread searchThread;
    std::cout << "id name DeepBlunder " << VERSION_MAJOR <<
        "." << VERSION_MINOR << std::endl;
    std::cout << "id author Brandon Boettcher" << std::endl;
    std::cout << "uciok" << std::endl;
    std::string input;
    std::getline(std::cin, input);
    std::getline(std::cin, input);
    assert(input == "isready");
    engine.initialize();
    std::cout << "readyok" << std::endl;
    while (true) {
        std::getline(std::cin, input);
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        while (hasNextToken(ss)) {
            std::string tok;
            ss >> tok;
            tokens.push_back(tok);
        }
        if (tokens.size() == 0) {
            continue;
        }
        if (tokens[0] == "isready") {
            assert(tokens.size() == 1);
            std::cout << "readyok" << std::endl;
        } else if (tokens[0] == "ucinewgame") {
            assert(tokens.size() == 1);
            engine.setupBoard();
        } else if (tokens[0] == "position") {
            assert(tokens.size() > 1);
            int index = 1;
            if (tokens[index] == "fen") {
                assert(tokens.size() > 2);
                engine.setupBoard(tokens[++index]);
            } else {
                assert(tokens[index] == "startpos");
                engine.setupBoard();
            }
            if (tokens.size() > ++index) {
                assert(tokens[index] == "moves");
                std::vector<std::string> moves;
                while (++index < tokens.size()) {
                    moves.push_back(tokens[index]);
                }
                engine.makeMoves(moves);
            }
        } else if (tokens[0] == "go") {
            SearchInfo info;
            int index = 0;
            while (++index < tokens.size()) {
                if (tokens[index] == "depth") {
                    assert(index + 1 < tokens.size());
                    info.maxDepth = std::stoi(tokens[++index]);
                } else if (tokens[index] == "winc") {
                    assert(index + 1 < tokens.size());
                    info.inc[WHITE] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "binc") {
                    assert(index + 1 < tokens.size());
                    info.inc[BLACK] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "wtime") {
                    assert(index + 1 < tokens.size());
                    info.time[WHITE] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "btime") {
                    assert(index + 1 < tokens.size());
                    info.time[BLACK] = std::stoi(tokens[++index]);
                } else if (tokens[index] == "movetime") {
                    assert(index + 1 < tokens.size());
                    info.movetime = std::stoi(tokens[++index]);
                } else if (tokens[index] == "movestogo") {
                    assert(index + 1 < tokens.size());
                    info.movestogo = std::stoi(tokens[++index]);
                }
            }
            if (searchThread.joinable()) {
                searchThread.join();
            }
            searchThread = std::thread(search, std::ref(engine), info);
        } else if (tokens[0] == "stop") {
            assert(tokens.size() == 1);
            engine.stopSearch();
        } else if (tokens[0] == "quit") {
            assert(tokens.size() == 1);
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
    }
    else {
        std::cout << "Error: unrecognized protocol" << std::endl;
        Engine engine;
        engine.initialize();
        engine.runPerftTests();
    }
}
