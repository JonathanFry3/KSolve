// KSolveResult.hpp declares the result structure returned by the 
// Klondike solvers defined here.

// MIT License

// Copyright (c) 2022 Jonathan B. Fry (@JonathanFry3)

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

#ifndef KSOLVERESULT_HPP
#define KSOLVERESULT_HPP

#include "Game.hpp"		// for Moves

// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code and a Moves vector.  The vector contains
// the minimum solution if the code returned is SolvedMinimal. It will contain
// a solution that may not be minimal if the code is Solved.
// Otherwise, it will be empty.
//
// This function uses an unpredictable amount of main memory. You can
// control this behavior to some degree by specifying maxStates. The number
// of unique game states stored is returned in _stateCount.

enum KSolveResultCode {SolvedMinimal, Solved, Impossible, GaveUp, MemoryExceeded};
struct KSolveResult
{
    KSolveResultCode _code;
    unsigned _stateCount;
    Moves _solution;

    KSolveResult(KSolveResultCode code, unsigned stateCount, const Moves& moves)
        : _code(code)
        , _stateCount(stateCount)
        , _solution(moves)
        {}
};

#endif    // KSOLVERESULT_HPP