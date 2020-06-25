#ifndef KSOLVE_HPP
#define KSOLVE_HPP
// Solver.hpp declares a Klondike Solitaire solver function and auxiliaries

#include <Game.hpp>
#include <cstdint>		// for std::uint32_t
#include <utility>		// for std::pair


// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code and a Moves vector.  The vector contains
// the minimum solution if the code retured is SOLVED. It will contain
// a solution that may not be minimal if the code is GAVEUP_SOLVED.
// Otherwise, it will be empty.
enum KSolveResult {SOLVED, GAVEUP_SOLVED, GAVEUP_UNSOLVED, IMPOSSIBLE};
std::pair<KSolveResult,Moves> KSolve(
		const std::vector<Card> & deck, // The deck to be played
		unsigned draw=1,				// number of cards to draw from stock at a time
		unsigned maxMoves=512,			// give up if the minimum possible number
										// of moves in any solution exceeds this.
		unsigned maxStates=10000000);	// Give up is the number of unique game states
										// examined exceeds this.


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