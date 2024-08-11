<div style="text-align: center;">
    <img src="img/deepblunder.png" alt="Deep Blunder Logo" width="200"/>
</div>

# DeepBlunder Chess Engine

[![Release][release-badge]][releases-link]

A homemade UCI chess engine written from scratch in C++.

### Challenge DeepBlunder to a game on [Lichess][lichess-link]!

## Credit

Most of the inspiration and knowledge of how to build a chess engine came from [BlueFever Software][bluefever-YT-profile-link]'s series of videos on YouTube: [Chess engine in C][bluefever-YT-playlist-link]. The basic structure of this engine follows the structure of his engine [Vice][vice-link].

Additionally:
 - [Chess Programming Wiki][cpw-link]
 - [TalkChess forums][talkchess-link]

## Building

Pre-built binaries for windows and linux can be found on the [releases][releases-link] page.

On windows, you can open DeepBlunder.sln with Visual Studio. Alternatively, this engine can be built with the Makefile:

```bash
git clone https://github.com/boettcherb/DeepBlunder-Chess-Engine.git
cd DeepBlunder-Chess-Engine/src
make release
```

## Running

#### UCI

Example starting a search from the initial chess position after running the program:

```bash
./deepblunder
uci
isready
position startpos
go depth 8
```

This engine uses the [UCI Protocol][uci-link] to communicate with users and GUIs. The UCI protocol involves a back-and-forth communication through standard input and output.

1. First, the user sends ```uci``` to the engine to let the engine know to use the UCI protocol. The engine responds ```uciok```.
2. Next, the user asks ```isready``` and the engine responds ```readyok``` once it is fully initialized.
3. Next, the user sends a board position to the engine so the engine knows which position to search: ```position startpos [moves ...]```. For example, if the user is white and they play pawn to e4, they will send ```position startpos moves e2e4``` to the engine. The engine will then search that position (after receiving a ```go``` command) and return a move as black (say, pawn to e5). Then the user will make another move (say, knight to f3), and they will send this position to the engine with ```position startpos moves e2e4 e7e5 g1f3```. Most GUIs use this method for the entire game. Alternatively, the user can send a specific position to the engine with ```position fen <FEN string> [moves ...]```. The engine will not print anything in response to the position command.
4. After sending a position to the engine, the user must tell the engine to start searching that position with the ```go``` command. There are many options for this command, but the simplest are ```go depth <max depth>``` to search to a specific depth, and ```go movetime <time in ms>``` to search for a specific amount of time. For GUIs, it is most common to send the time remaining and the increment for both sides. For example, if both sides have 3 minutes on the clock and there is a 2 second increment: ```go wtime 180000 btime 180000 winc 2000 binc 2000```. The engine will respond with ```bestmove <move>``` once it is done searching.
5. Repeat steps 3 and 4 for the entire game.

#### Using a GUI

Instead of using the console and having to send the position every move, you can instead download a GUI to automate the process (and so you can actually see the board).

Here are some of the most popular ones:
- [Arena][arena-gui-link]
- [CuteChess][cutechess-gui-link]
- [Tarrasch][tarrasch-gui-link]

#### Perft Tests

Currently, the UCI protocol is the only protocol this engine knows. If the first command given after running the program is not "uci", the engine will default to running its perft tests. Perft tests (performance tests) are used to debug the engine's move generation by counting the number of positions reachable from a starting position (up to a certain depth). To run the perft tests, simply enter an integer for the max depth to search to. ```3``` is recommended for the debug build and ```5``` is recommended for the release build.

## Engine Strength

The strength of chess players and engines is measured in [ELO][elo-link].

| Version | Bullet (lichess) | Blitz (lichess) | Rapid (lichess) |
|---|---|---|---|
| [v1.0.5][v1.0-link] | 1985 | 1841 | 1955 |

## Features

 - [Magic bitboards][magic-bitboards-link]
 - [Zobrist Hashing][zobrist-link]
 - [Alpha-Beta pruning][alpha-beta-link] ([Negamax][negamax-link] algorithm)
 - [Iterative Deepening][iterative-deepening-link]
 - [Quiescence Search][quiescence-link]
 - [Transposition Table][transposition-link]
 - Move Ordering:
    - [PV Move][pv-move-link]
    - [MVV-LVA][mvv-lva-link]
    - [Killer Move heuristic][killer-link]
    - [History Heuristic][history-link]
 - Very Basic Evaluation:
    - Material counts
    - [Piece-Square tables][piece-square-link]

## Upcoming Features

My plans to improve this engine in the future:
 - Further improve move ordering. There are more move-ordering techniques to test out, such as the Countermove Heuristic.
 - Add more pruning methods. There are many more pruning techniques than just Alpha-Beta pruning, such as null move pruning, late move reduction (LMR), aspiration windows, futility pruning, etc.
 - Improve the evaluation function. Implement evalution based on pawn structure, king safety, piece mobility, center control, etc. Also, adjust piece-square values for the different phases of the game: opening, middlegame, and endgame.
 - Add an Opening Book (such as PolyGlot) to save clock time at the beginning of the game and to ensure a decent position.
 - Add an Endgame Database/Tablebase (such as Syzygy). Currently, DeepBlunder cannot find a win in a King+Queen vs King game, and instead draws by the 50 move rule. An endgame database will ensure best play in endgames where a checkmate is too deep in the search tree to find with a normal search.
 - Improve the transposition table. Currently, the transposition table stores only the best move in a position, and this move is used for move ordering. In the future I want to store additional information (such as the evaluation) and use it to prune redundant branches of the search tree.
 - Implement multithreading to speed up the search function.
 - Improve time management. Possible Ideas:
   - If there is only one move that doesn't cause a massive disadvantage, then play it instantly.
   - If there is no more time to complete another depth in iterative deepening, then stop the search.
 - Add UCI options, such as hash size, number of threads, move overhead, and pondering. 

[release-badge]: https://img.shields.io/badge/Current_Release-v1.1.6-blue
[releases-link]: https://github.com/boettcherb/DeepBlunder-Chess-Engine/releases/latest
[lichess-link]: https://lichess.org/@/DeepBlunder-Bot
[uci-link]: https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
[elo-link]: https://www.chess.com/terms/elo-rating-chess
[arena-gui-link]: http://www.playwitharena.de/
[tarrasch-gui-link]: https://www.triplehappy.com/
[cutechess-gui-link]: https://github.com/cutechess/cutechess?tab=readme-ov-file
[magic-bitboards-link]: https://www.chessprogramming.org/Magic_Bitboards
[zobrist-link]: https://www.chessprogramming.org/Zobrist_Hashing
[alpha-beta-link]: https://www.chessprogramming.org/Alpha-Beta
[negamax-link]: https://www.chessprogramming.org/Negamax
[iterative-deepening-link]: https://www.chessprogramming.org/Iterative_Deepening
[quiescence-link]: https://www.chessprogramming.org/Quiescence_Search
[transposition-link]: https://www.chessprogramming.org/Transposition_Table
[pv-move-link]: https://www.chessprogramming.org/PV-Move
[mvv-lva-link]: https://www.chessprogramming.org/MVV-LVA
[killer-link]: https://www.chessprogramming.org/Killer_Heuristic
[history-link]: https://www.chessprogramming.org/History_Heuristic
[piece-square-link]: https://www.chessprogramming.org/Piece-Square_Tables
[bluefever-YT-profile-link]: https://www.youtube.com/@BlueFeverSoft
[bluefever-YT-playlist-link]: https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg
[vice-link]: https://github.com/bluefeversoft/vice?tab=readme-ov-file
[cpw-link]: https://www.chessprogramming.org/Main_Page
[talkchess-link]: https://www.talkchess.com/
[v1.0-link]: https://github.com/boettcherb/DeepBlunder-Chess-Engine/releases/tag/v1.0.0
