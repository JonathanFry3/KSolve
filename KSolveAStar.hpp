// KSolveAStar.hpp declares a Klondike Solitaire solver function and auxiliaries.
// This solver function uses the A* search algorithm.

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

#ifndef KSOLVEASTAR_HPP
#define KSOLVEASTAR_HPP

#include "Game.hpp"		// for Game, Card, Pile, Move etc.
namespace KSolveNames {
// Solves the game of Klondike Solitaire for minimum moves if possible.
// Returns a result code and a Moves vector and some statistics. 
// The vector contains the minimum solution if the code returned
// is SolvedMinimal. It will contain a solution that may not be 
// minimal if the code is Solved. Otherwise, it will be empty.
//
// For some insight into how it works, look up the A* algorithm.
//
// This function uses an unpredictable amount of main memory. You can
// control this behavior to some degree by specifying MoveTreeLimit. 
//
// The statistics returns are:
//
//      _stateCount is the number of game states in the "closed list",
//      the list of the game states previously encounted, and their 
//      heuristic values.
//
//      _moveTreeSize is the number of move specifications stored
//      in the move tree. This the the size you control with the
//      second argument in the call.
//
//      _finalFringeSize is the final number of move specifications
//      in the fringe (the task queue). This will be zero for unsolvable 
//      games.
//      
//      _advances is the number of trips though the main loop.  Each trip
//      pops a move spec off the fringe, recreates the game state it lead to,
//      and makes moves up to the first state with more than one possible
//      next move.  It tries each of those moves, tests it against the
//      closed list, and if it qualifies, pushes it to the fringe. The starting
//      move spec and each move spec that does not have siblings is moved to the
//      move tree.

enum KSolveAStarCode {SolvedMinimal, Solved, Impossible, GaveUp};

struct KSolveAStarResult
{
public:        
    Moves _solution;
    KSolveAStarCode _code;
    unsigned _stateCount{0};
    unsigned _moveTreeSize{0};
    unsigned _finalFringeSize{0};
    unsigned _advances;

    KSolveAStarResult(KSolveAStarCode code, 
                const Moves& moves, 
                unsigned branchCount,
                unsigned moveCount,
                unsigned finalFringeSize,
                unsigned loopCount)  noexcept
        : _code(code)
        , _solution(moves)
        , _stateCount(branchCount)
        , _moveTreeSize(moveCount)
        , _finalFringeSize(finalFringeSize)
        , _advances(loopCount)
        {}
};
KSolveAStarResult KSolveAStar(
        Game& gm, 			// The game to be played
        unsigned moveTreeLimit=12'000'000,// Give up if the size of the move tree
                                        // exceeds this.
        unsigned threads=0) noexcept;   // Use as many threads as the hardware will run concurrently

unsigned DefaultThreads() noexcept;

unsigned MinimumMovesLeft(const Game& game) noexcept;
}       // namespace KSolveNames


#endif    // KSOLVEASTAR_HPP 