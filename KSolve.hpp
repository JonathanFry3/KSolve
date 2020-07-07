#ifndef KSOLVE_HPP
#define KSOLVE_HPP
// Solver.hpp declares a Klondike Solitaire solver function and auxiliaries

#include "Game.hpp"
#include <cstdint>		// for std::uint32_t
#include <utility>		// for std::pair


// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code and a Moves vector.  The vector contains
// the minimum solution if the code returned is SOLVED. It will contain
// a solution that may not be minimal if the code is GAVEUP_SOLVED.
// Otherwise, it will be empty.
//
// This function uses an unpredictable amount of main memory. You can
// control this behavior to some degree by specifying maxStates.
enum KSolveResult {SOLVED, GAVEUP_SOLVED, GAVEUP_UNSOLVED, IMPOSSIBLE};
std::pair<KSolveResult,Moves> KSolve(
		Game& gm, 						// The game to be played
		unsigned maxStates=5000000,		// Give up if the number of unique game states
										// examined exceeds this.
		unsigned maxMoves=512)			// Give up if the minimum possible number
										// of moves in any solution exceeds this.
		noexcept(false);				// Known to throw std::bad_alloc.


// A compact representation of the current game state.
// It is possible, although tedious, to reconstruct the
// game state from one of these and the original deck.
//
// This is an implementation detail, but one sufficiently 
// complex that it needs unit tests.  The basic requirements
// for GameStateType are:
// 1.  Any difference in the foundation piles, the face-up cards
//     in the tableau piles, or in the stock pile length
//     should be reflected in the GameStateType.
// 2.  It should be quite compact, as we will usually be storing
//     millions to tens of millions of instances.
typedef std::uint32_t PileState;
struct GameStateType {
	std::array<PileState,7> _psts;
	GameStateType(const Game& game);
	bool operator==(const GameStateType& other) const;
};

#endif