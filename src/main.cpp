#include "engine.h"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <sstream>
#include <chrono>

constexpr auto VERSION = "v1.1.8";


static void search(Engine& engine, SearchInfo info) {
    engine.searchPosition(info);
}

static void uci_process_setoption(Engine& engine, std::stringstream& ss) {
    std::string token, name, value;
    ss >> token;
    assert(token == "name");
    while (ss >> token && token != "value") {
        name += (name.empty() ? "" : " ") + token;
    }
    while (ss >> token) {
        value += (value.empty() ? "" : " ") + token;
    }
    if (name == "Hash") {
        assert(!value.empty());
        engine.setHashTableSize(std::stoi(value));
    }
    if (name == "Log File") {
        assert(!value.empty());
        engine.setLogFile(value);
    }
}

static void uci_process_position(Engine& engine, std::stringstream& ss) {
    std::string token;
    ss >> token;
    if (token == "fen") {
        std::string fen;
        for (int i = 0; i < 6; ++i) {
            ss >> token;
            fen += token + " ";
        }
        fen.pop_back();
        engine.setupBoard(fen);
    } else {
        assert(token == "startpos");
        engine.setupBoard();
    }
    if (ss >> token) {
        assert(token == "moves");
        std::vector<std::string> moves;
        while (ss >> token) {
            moves.push_back(token);
        }
        engine.makeMoves(moves);
    }
}

static SearchInfo uci_process_go(std::stringstream& ss) {
    SearchInfo info;
    std::string token;
    while (ss >> token) {
        if (token == "depth") {
            ss >> token;
            info.maxDepth = std::stoi(token);
        }
        else if (token == "winc") {
            ss >> token;
            info.inc[WHITE] = std::stoi(token);
        }
        else if (token == "binc") {
            ss >> token;
            info.inc[BLACK] = std::stoi(token);
        }
        else if (token == "wtime") {
            ss >> token;
            info.time[WHITE] = std::stoi(token);
        }
        else if (token == "btime") {
            ss >> token;
            info.time[BLACK] = std::stoi(token);
        }
        else if (token == "movetime") {
            ss >> token;
            info.movetime = std::stoi(token);
        }
        else if (token == "movestogo") {
            ss >> token;
            info.movestogo = std::stoi(token);
        }
    }
    return info;
}

static void uci() {
    Engine engine;
    std::thread searchThread;
    std::cout << "id name DeepBlunder " << VERSION << '\n';
    std::cout << "id author Brandon Boettcher\n";
    std::cout << "option name Hash type spin default 128 min 1 max 4096\n";
    std::cout << "option name Log File type string default log.txt\n";
    std::cout << "uciok" << std::endl;
    bool quit = false;
    while (!quit) {
        std::string input, token;
        std::getline(std::cin, input);
        engine.log("Received uci command: " + input);
        std::stringstream ss(input);
        while (ss >> token) {
            if (token == "isready") {
                assert(!(ss >> token));
                engine.initialize();
                std::cout << "readyok" << std::endl;
            }
            else if (token == "setoption") {
                uci_process_setoption(engine, ss);
            }
            else if (token == "position") {
                uci_process_position(engine, ss);
            }
            else if (token == "go") {
                SearchInfo info = uci_process_go(ss);
                if (searchThread.joinable()) {
                    searchThread.join();
                }
                searchThread = std::thread(search, std::ref(engine), info);
            }
            else if (token == "stop") {
                assert(!(ss >> token));
                engine.stopSearch();
            }
            else if (token == "quit") {
                assert(!(ss >> token));
                engine.stopSearch();
                quit = true;
                break;
            }
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
