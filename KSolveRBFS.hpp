// KSolveRBFS.hpp declares a Klondike Solitaire solver function and auxiliaries.
// This solver function uses the Recursive Best First Search (RBFS) algorithm.

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

#ifndef KSOLVERBFS_HPP
#define KSOLVERBFS_HPP

#include "Game.hpp"		// for Game

// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code, a Moves vector, and a graph size.  The vector contains
// the minimum solution if the code returned is Solved. Otherwise, it will be empty.
//
// For some insight into how it works, look up the RBFS algorithm.

struct KSolveRBFSResult
{
    enum Code {Solved, Impossible, MemoryExceeded};
	Moves _solution;
	size_t _graphSize;
	Code _code;

	KSolveRBFSResult(Code code, size_t graphSize, const Moves& moves)
		: _code(code)
		, _graphSize(graphSize)
		, _solution(moves)
		{}
};
KSolveRBFSResult KSolveRBFS(
		const Game& gm); 						// The game to be solved

#endif    // KSOLVERBFS_HPP