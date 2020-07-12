// KSolve.hpp declares a Klondike Solitaire solver function and auxiliaries

// MIT License

// Copyright (c) 2020 Jonathan B. Fry (@JonathanFry3)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef KSOLVE_HPP
#define KSOLVE_HPP

#include "Game.hpp"		// for Game
#include <cstdint>		// for std::uint32_t
#include <array>

// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code and a Moves vector.  The vector contains
// the minimum solution if the code returned is SOLVED. It will contain
// a solution that may not be minimal if the code is GAVEUP_SOLVED.
// Otherwise, it will be empty.
//
// This function uses an unpredictable amount of main memory. You can
// control this behavior to some degree by specifying maxStates. The number
// of unique game states stored is returned in _stateCount.
//
// Although there is code to catch std::bad_alloc exceptions and return
// the MEMORY_EXCEEDED code, if memory is exceeded, the process will probably 
// just die.
enum KSolveCode {SOLVED, GAVEUP_SOLVED, GAVEUP_UNSOLVED, IMPOSSIBLE, MEMORY_EXCEEDED};
struct KSolveResult
{
	KSolveCode _code;
	unsigned _stateCount;
	Moves _solution;

	KSolveResult(KSolveCode code, unsigned stateCount, const Moves& moves)
		: _code(code)
		, _stateCount(stateCount)
		, _solution(moves)
		{}
};
KSolveResult KSolve(
		Game& gm, 						// The game to be played
		unsigned maxStates=10000000);	// Give up if the number of unique game states
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