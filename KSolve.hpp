#ifndef KSOLVE_HPP
#define KSOLVE_HPP
// Solver.hpp declares a Klondike Solitaire solver function and auxiliaries

#include <Game.hpp>
#include <cstdint>


enum KSolveResult {SOLVED, GAVEUP_SOLVED, GAVEUP_UNSOLVED, IMPOSSIBLE};
KSolveResult KSolve(const std::vector<Card> & deck,
		Moves& solution,
		unsigned draw=1,
		unsigned maxMoves=512,
		unsigned maxStates=10000000);


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