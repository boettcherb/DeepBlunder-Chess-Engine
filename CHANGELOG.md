### v1.2.0 (19 Aug 2024):

- Improve the evaluation function:
	- Pawn structure: Bonus for passed and protected pawns. Penalties for backwards, doubled, isolated, and blocked pawns.
	- Knights: Bonus for being closer to the enemy king and for the number of blocked pawns (favor closed positions).
	- Bishops: Bonus for the bishop pair, being on the same diagonal as enemy king/queens/rooks, and for having friendly pawns on opposite color. Penalty for being blocked by a friendly pawn and having friendly pawns on same color.
	- Rooks: Bonus for being on an open file, being connected to another rook, and being on the same rank or file as enemy king/queens.
	- Queens: Bonus for being closer to the enemy king and for being on the same file/rank/diagonal as enemy king.
	- King: Penalty for not being castled, being on an open file or diagonal, and for pushing pawns in front of the king.
	- Bonus for controlling central squares and for attacking squares around the enemy king.
	- Large penalty for pieces that have very few legal moves.
- Increase material values for knights and bishops (to discourage giving up a piece for 3 pawns)
- Add endgame-specific pieceSquare table for kings
- Move debug functions to debug.cpp
- Add member variable 'hasCastled[2]' to the Board class
- Improve history heuristic

### v1.1.9 (14 Aug 2024):

- Add countermove move ordering heuristic
- Improve history heuristic
- Add Move Overhead UCI option
- Update movelist to store the move and the move score separately
- Update move scores
- Change default log file to deepblunder.log
- Improve parsing of UCI's position command
- Fix bug in Makefile

### v1.1.8 (12 Aug 2024):

- Add support for the UCI option "Hash"
- Add support for the UCI option "Log File"
- Write all received UCI commands and engine output to log file
- Improve UCI command parsing
- Update Transposition table to take in a size in MB
- Add 'initialized' variable to Transposition table.

### v1.1.7 (11 Aug 2024):

- Fix warning in movelist.cpp: unused variable MAX_MOVE_SCORE
- Remove unused variable from defs.h: INITIAL_POSITION
- Replace compiler-specific functions in defs.cpp with C++20 functions
- Update .gitignore
- Update Makefile
- Make file names lowercase; delete unnecessary directory
- Add a README

### v1.1.6 (9 Aug 2024):

- fix comments
- Add a Makefile
- Fix signed-unsigned conversion warnings
- Add missing header to Board.cpp: `<cstring>`
- Improve move ordering with killer and history heuristics
- Always print debug statement about time controls
- Add MAX_SEARCH_DEPTH to Defs.h
- Use PV move in move ordering
- Stop searching if we found checkmate
- Convert AlphaBeta algorithm to Negamax algorithm
- Fix bug in parsing fen from uci
- Move all files to src folder
- Set a more reasonable maxDepth value: 128
- Add patch number
- Fix bug in uci()

### v1.0.0 (7 Aug 2024):

- Initial release