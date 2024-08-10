# DeepBlunder Chess Engine

[![Release][release-badge]][release-link]

A homemade UCI chess engine written from scratch in C++.

Author: Brandon Boettcher

### Challenge DeepBlunder to a game on [Lichess][lichess-link]!

## Building

On windows, you can open DeepBlunderChessEngine.sln with Visual Studio. Alternatively, this engine can be built with the makefile:

```bash
git clone https://github.com/boettcherb/DeepBlunder-Chess-Engine.git
cd ChessEngine/src
make release
./deepblunder
```

## Engine Strength

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
 - Add an Opening Book (such as PolyGlot) to save clock time at the beginning of the game and ensure a decent position.
 - Add an Endgame Database/Tablebase (such as Syzygy). Currently, DeepBlunder cannot find a win in a King+Queen vs King game, and instead draws by the 50 move rule. An endgame database will ensure best play in endgames where a checkmate is too deep in the search tree to find with a normal search.
 - Improve the transposition table. Currently, the transposition table stores only the best move in a position, and this move is used for move ordering. In the future I want to store additional information (such as the evaluation) and use it to prune redundant branches of the search tree.
 - Implement multithreading to speed up the search function.
 - In the iterative deepening algorithm, estimate if we are able to complete another depth based on our remaining time. If no, immediately return the best move for the previous depth. This will hopefully save a small amount of time on every move.
 - Add UCI options, such as hash size, number of threads, move overhead, and pondering. 

## Credit

Most of the inspiration and knowledge of how to build a chess engine came from [BlueFever Software][bluefever-YT-profile-link]'s series of videos on YouTube: [Chess engine in C][bluefever-YT-playlist-link]. The basic structure of this engine follows the structure of his engine [Vice][vice-link].

Additionally:
 - [Chess Programming Wiki][cpw-link]
 - [TalkChess forums][talkchess-link]

[release-badge]: https://img.shields.io/badge/Current_Release-v1.1.6-blue
[release-link]: https://github.com/boettcherb/DeepBlunder-Chess-Engine/releases/latest
[lichess-link]: https://lichess.org/@/DeepBlunder-Bot
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
