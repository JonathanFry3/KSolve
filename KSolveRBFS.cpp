// KSolveRBFS.cpp - implements a Klondike Solitaire solver using
// Recursive Best First Search (RBFS).  This should use less 
// space than the A* search implemented by KSolveAStar, although 
// it will probably use more time.

#include "KSolveRBFS.hpp"
#include "mf_vector.hpp"
#include <algorithm>        // *_heap, min
#include <cassert>

#include <sstream>
#include <iostream>

using namespace KSolveRBFS;

typedef unsigned NodeIndex;
typedef unsigned short Count;
enum {Solved = 0, Failed = 512};
enum {NullIndex = -1};

struct Node
{
    Move _move;
    NodeIndex _successors;
    unsigned short _nSuccessors;
    Count _f;

    Node(const Move& move, Count f)
        : _move(move)
        , _successors(NullIndex)        // successors are maintained in a heap
        , _nSuccessors(0)
        , _f(f)
        {}
    bool operator <(const Node & o){
        return o._f < _f;               // backward, as we want a min heap
    }
};

struct SolverState
{
    Game _game;
    Moves _movesMade;
    Moves _solution;
    mf_vector<Node,16*1024> _graph;

    explicit SolverState(const Game & game)
        : _game(game)
        {}
    Count NewF(NodeIndex start, Count bound);
    void PushMove(Move mv);
    void PopMove();
    NodeIndex ExtendGraph(NodeIndex node);
    NodeIndex AddNode(Move move, NodeIndex parent, Count nMoves, bool autoMove);
    QMoves FilteredAvailableMoves() noexcept;
    bool SkippableMove(Move trial) noexcept;
};


std::string Peek(const Node& node)
{
    std::stringstream s;
    s << "Node(" << Peek(node._move) << ", " << node._f;

    s << ", ->";
    if (node._successors == NullIndex)  s << "Null";
    else                                s << node._successors;
    s << "(" << node._nSuccessors << "))";
    return s.str();
}


Count SolverState::NewF(NodeIndex start, Count bound)
{
    // Insures moves made here are unmade even if returns are executed
    // anywhere.
    struct Unwinder
    {
        SolverState& _state;
        unsigned _size;
        Unwinder(SolverState& state)
            : _state(state)
            , _size(state._movesMade.size())
            {}
        ~Unwinder() {
            while (_size < _state._movesMade.size())
            {
                _state.PopMove();
            }
        }
    } unWinder(*this);

    Node & node = _graph[start];
    std::cout << "NewF(" << Peek(node) << ")\n";
    if (node._f == Solved || node._f > bound) return node._f;
    NodeIndex n = node._successors;
    if (n == NullIndex)
        n = ExtendGraph(start);
    else {
        // walk to next branching node
        while (_graph[n]._nSuccessors == 1) {
            std::cout << "Automove[" << n << "]: " << Peek(_graph[n]) << std::endl;
            PushMove(_graph[n]._move);
            n = _graph[n]._successors;
        }
        PushMove(_graph[n]._move);
    }
    Node & parent = _graph[n];   
    std::cout << "parent[" << n << "]: " << Peek(parent) << std::endl;
    std::cout << "bound: " << bound << std::endl;
    if (parent._nSuccessors == 0 && _game.GameOver()) return Solved;
    if (parent._nSuccessors == 0) return Failed;                   
    if (parent._f > bound) return parent._f;
    auto begin = _graph.begin()+parent._successors;
    auto end = begin + parent._nSuccessors;
    Node & front = *begin;
    NodeIndex backx = parent._successors+parent._nSuccessors-1;
    Node & back = _graph[backx];
    while (Solved < front._f && front._f <= bound) {
        std::pop_heap(begin,end);
        std::cout << "Best[" << backx << "]: " << Peek(back) << std::endl;
        std::cout << "Second[" << parent._successors << "]: " << Peek(front) << std::endl;
        Count bound1 = std::min(bound, front._f); // front._f is second-best _f
        assert(back._f <= front._f);
        PushMove(back._move);
        back._f = NewF(backx,bound1);
        std::cout << "NewF(" << backx << "," << bound1 << ") -> " << back._f << std::endl;
        PopMove();
        std::push_heap(begin,end);
    }
    return front._f;
}
// Extend the graph to the next branching node
NodeIndex SolverState::ExtendGraph(NodeIndex nodex)
{
    Node * parent = &_graph[nodex];
    QMoves avail = FilteredAvailableMoves();
    Count nMoves = MoveCount(_movesMade);
    // Make automoves
    while (avail.size() == 1) {
        nodex = AddNode(avail[0], nodex, nMoves, true);
        std::cout << "Add Automove[" << nodex << "]: ";
        std::cout << Peek(_graph.back()) << std::endl;
        nMoves += avail[0].NMoves();
        parent = &_graph[nodex];
        avail = FilteredAvailableMoves();
    }
    if (avail.size() == 0) {
        if (_game.GameOver()) {
            _solution = _movesMade;
            parent->_f = Solved;
        } else {
            parent->_f = Failed;        // dead end
        }
    } else {
        // branching node
        for (Move move: avail) {
            NodeIndex loc = AddNode(move,nodex,nMoves,false);
            std::cout << "Add Node[" << loc << "] " << Peek(_graph[loc]) << std::endl;
            PopMove();
        }
        auto begin = _graph.begin() + _graph[nodex]._successors;
        auto end = begin + _graph[nodex]._nSuccessors;
        std::make_heap(begin,end);
    }
    return nodex;
}

NodeIndex SolverState::AddNode(Move move, NodeIndex p, Count nMoves, bool autoMove)
{
    PushMove(move);
    Node & parent = _graph[p];
    parent._nSuccessors += 1;
    Count est = autoMove
                ? parent._f
                : nMoves+move.NMoves()+_game.MinimumMovesLeft();
    if (est < parent._f) {
        std::cout << "after " << Peek(move) << ", est = " << est;
        std::cout << ", parent._f = " << parent._f << std::endl;
    }
    assert(est>=parent._f);      // move estimates are non-decreasing
    NodeIndex result = _graph.size();
    _graph.emplace_back(move,est);
    if (parent._successors == NullIndex)
        parent._successors = result;
    return result;
}

// Return a vector of the available moves that pass the SkippableMove filter
QMoves SolverState::FilteredAvailableMoves() noexcept
{
	QMoves availableMoves = _game.AvailableMoves();
	for (auto i = availableMoves.begin(); i < availableMoves.end(); ){
		if (SkippableMove(*i)) {
			availableMoves.erase(i);
		} else {
			++i;
		}
	}
	return availableMoves;
}


// Return true if this move cannot be in a minimum solution.
bool SolverState::SkippableMove(Move trial) noexcept
{
	// Consider a move at time T0 from A to B and the next move
	// from B, which goes to C at time Tn.  The move at Tn is
	// skippable if the same result could have been achieved 
	// at T0 by moving the same cards directly from A to C.

	// We are now at Tn looking back for a T0 move.  B is our from pile
	// and C is our to pile.  A candidate T0 move is one that moves
	// to our from pile (pile B).

	// Do those two moves move the same set of cards?.  Yes if
	// no intervening move has changed pile B and the two moves
	// move the same number of cards.

	// Was the move from A to C possible at T0? Yes if neither that move
	// nor an intervening move has changed pile C.

	// Since nothing says A cannot equal C, this test catches 
	// moves that exactly reverse previous moves.
	const auto B = trial.From();
	if (B == STOCK || B == WASTE) return false; 
	const auto C = trial.To();
	const auto &movesMade = _movesMade;
	for (auto imv = movesMade.crbegin(); imv != movesMade.crend(); ++imv){
		const Move mv = *imv;
		if (mv.To() == B){
			// candidate T0 move
			if (mv.From() == C) {
				// If A=C and the A to B move flipped a tableau card
				// face up, then it changed C.
				if (IsTableau(C) && mv.NCards() == mv.FromUpCount())
					return false;
			}
			return  mv.NCards() == trial.NCards();
		} else {
			// intervening move
			if (mv.To() == C || mv.From() == C)
				return false;			// trial move's to pile (C) has changed
			if (mv.From() == B) 
				return false;			// trial move's from pile (B) has changed
		}
	}
	return false;

	// If you find another profitable way to filter available moves, you could
	// call this one ABC_Move. 
	//
	// AvailableMoves() generates moves among tableau files for only two purposes:
	// to move all the face-up cards, or to expose a card that can be moved to the 
	// foundation.  I have tried filtering out later moves that would re-cover a 
	// card that had been exposed in that fashion.  That did not break anything, but
	// cost more time than it saved.
	// Jonathan Fry 7/12/2020
}

void SolverState::PushMove(Move mv) 
{
    _game.MakeMove(mv);
    _movesMade.push_back(mv);
    std::cout << "PushMove: " << Peek(_movesMade) << std::endl;
}

void SolverState::PopMove() 
{
    assert(_movesMade.size());
    _game.UnMakeMove(_movesMade.back());
    _movesMade.pop_back();
    std::cout << "PopMove: " << Peek(_movesMade) << std::endl;
}



Result KSolveRBFS::Solve(const Game& game)
{
    SolverState state(game);
    Count est = game.MinimumMovesLeft();
    state._graph.emplace_back(Move(), est);
    Node & root = state._graph[0];
    std::cout << "root == " << Peek(root) << std::endl;
    while (root._f != Solved && root._f != Failed)
        root._f = state.NewF(0, root._f);
    Result::Code code(state._solution.size()
                ? Result::Solved
                : Result::Impossible);
    Result result{code,state._graph.size(),state._solution};
    std::cout << "Result: (" << result._code << ", " << result._graphSize;
    std::cout << ", " << Peek(result._solution) << ")\n\n";
    return result;
}
