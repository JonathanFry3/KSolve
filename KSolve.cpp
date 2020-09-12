#include "KSolve.hpp"
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include <shared_mutex>		// for std::shared_mutex, std::shared_lock
#include "robin_hood.h"		// for robin_hood::unordered_node_map
#include "mf_vector.hpp"
#include "fixed_capacity_deque.hpp"
#include <thread>
#ifdef KSOLVE_TRACE
#include <iostream>			// for cout
#endif  //KSOLVE_TRACE

class Hasher
{
public:
	size_t operator() (const GameState & gs) const 
	{
		size_t result = robin_hood::hash<GameState::PartType>()(
							 gs._part[0]
					  	   ^ gs._part[1]
						   ^ gs._part[2]);
		return result;
	}
};

enum {maxMoves = 512};
typedef fixed_capacity_deque<Move,maxMoves> MoveSequenceType;

class MoveStorage
{
	typedef std::uint_fast32_t NodeX;
	struct MoveNode
	{
		Move _move;

		NodeX _prevNode;
		MoveNode(const Move& mv, NodeX prevNode)
			: _move(mv)
			, _prevNode(prevNode)
			{}
	};
	mf_vector<MoveNode,1024*32> _moveTree;
	// Stack of indexes to leaf nodes in _moveTree
	typedef mf_vector<NodeX> LeafNodeStack;
	// The leaf nodes waiting to grow new branches.  Each LeafNodeStack
	// stores nodes with the same minimum number of moves in any
	// completed game that can grow from them.  MoveStorage uses it
	// to implement a priority queue ordered by the minimum move count.
	mf_vector<LeafNodeStack,64> _fringe;
	unsigned _startStackIndex;
	unsigned _offset;		// offset of stack on last Dequeue
	bool _firstTime;
	MoveSequenceType _currentSequence;
	NodeX _leafIndex;			// index of current sequence's leaf node in _moveTree
public:
	// Constructor.
	MoveStorage();
	// Push the given move to the back of the current sequence of moves.
	void Push(Move move);
	// Remove the last move from the current move sequence.
	void Pop();
	// File the current move sequence under the given index number.
	// Calls after the first may not use an index less than the first.
	// Expects index > 0.
	void EnqueueMoveSequence(unsigned index);
	// Fetch a move sequence with the lowest available index, make it
	// the current move sequence and remove it from the queue.  
	// Return its index number, or zero if no more sequences are available.
	unsigned DequeueMoveSequence(); 
	// Make all the moves in the current sequence
	void MakeSequenceMoves(Game&game);
	// Return the current move sequence in a vector.
	Moves MovesVector() const;
	// Return a const reference to the current move sequence in its
	// native type.
	const MoveSequenceType& MoveSequence() const {return _currentSequence;}
};

struct KSolveState {
	Game _game;
	// _moveStorage stores the portion of the move tree that has been generated.
	// Each node has a move and a reference to the node with
	// the move before it.  The leaves are indexed by the minimum number of 
	// moves possible in any finished game that might grow from that leaf.
	// _moveStorage also stores the sequence of moves we are currently working on.
	MoveStorage _moveStorage;
	// _game_state_memory remembers the minimum move count at each game state we have
	// already visited.  If we get to that state again, we look at the current minimum
	// move count. If it is lower than the stored count, we keep our current node and store
	// its move count here.  If not, we forget the current node - we already have a
	// way to get to the same state that is at least as short.
	typedef robin_hood::unordered_node_map<
			GameState, 								// key type
			unsigned short, 						// mapped type
			Hasher									// hash function
		> MapType;
	MapType _game_state_memory;
	unsigned _maxStates;

	Moves & _minSolution;
	unsigned _minSolutionCount;

	bool _blewMemory;

	explicit KSolveState(  Game & gm, 
			Moves& solution,
			unsigned maxStates)
		: _minSolution(solution)
		, _game(gm)
		, _maxStates(maxStates)
		{
			_game_state_memory.reserve(maxStates);
			_minSolution.clear();
			_minSolutionCount = -1;
			_blewMemory = false;
		}
			
	QMoves MakeAutoMoves();
	void CheckForMinSolution();
	bool IsShortPathToState(unsigned minMoveCount);
	bool SkippableMove(Move mv);
	QMoves FilteredAvailableMoves();
};

KSolveResult KSolve(
		Game& game,
		unsigned maxStates)
{
	Moves solution;
	KSolveState state(game,solution,maxStates);

	const unsigned startMoves = state._game.MinimumMovesLeft();

	state._moveStorage.EnqueueMoveSequence(startMoves);	// pump priming
	try {
		// Main loop
		unsigned minMoves0;
		while (state._game_state_memory.size() < state._maxStates
				&& (minMoves0 = state._moveStorage.DequeueMoveSequence())    // <- side effect
				&& minMoves0 < state._minSolutionCount) { 

			// Restore state._game to the state it had when this move
			// sequence was enqueued.
			state._game.Deal();
			state._moveStorage.MakeSequenceMoves(state._game);

			QMoves availableMoves = state.MakeAutoMoves();

			if (availableMoves.size() == 0 && state._game.GameOver()) {
				// We have a solution.  See if it is a new champion
				state.CheckForMinSolution();
				// See if it the final winner.  It nearly always is.
				if (minMoves0 == state._minSolutionCount)
					break;
			}
			
			const unsigned movesMadeCount = MoveCount(state._moveStorage.MoveSequence());

			// Save the result of each of the possible next moves.
			for (auto mv: availableMoves){
				state._game.MakeMove(mv);
				const unsigned made = movesMadeCount + mv.NMoves();
				const unsigned remaining = state._game.MinimumMovesLeft();
				assert(minMoves0 <= made+remaining);
				if (state.IsShortPathToState(made)) {
					state._moveStorage.Push(mv);
					state._moveStorage.EnqueueMoveSequence(made+remaining); 
					state._moveStorage.Pop();
				}
				state._game.UnMakeMove(mv);
			}
		}
	} 
	catch(std::bad_alloc) {
		state._blewMemory = true;
	}

	KSolveCode outcome;
	if (state._blewMemory) {
		outcome = MEMORY_EXCEEDED;
	} else if (state._game_state_memory.size() >= maxStates){
		outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
	} else {
		outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
	}
	return KSolveResult(outcome,state._game_state_memory.size(),solution);
}
MoveStorage::MoveStorage()
	: _leafIndex(-1)
	, _startStackIndex(0)
	, _offset(0)
	, _firstTime(true)
	{}
void MoveStorage::Push(Move move)
{
	_currentSequence.push_back(move);
	NodeX ind;
	ind = _moveTree.size();
	_moveTree.emplace_back(move, _leafIndex);
	_leafIndex = ind;
}
void MoveStorage::Pop()
{
	_currentSequence.pop_back();
	_leafIndex = _moveTree[_leafIndex]._prevNode;
}
void MoveStorage::EnqueueMoveSequence(unsigned index)
{
	if (_firstTime) {
		_firstTime = false;
		_startStackIndex = index;
	}
	assert(_startStackIndex <= index);
	const unsigned offset = index - _startStackIndex;
	assert(_offset <= offset);

	// Grow the fringe as needed.
	while (!(offset < _fringe.size())){
		_fringe.emplace_back();
	}
	_fringe[offset].push_back(_leafIndex);
}
unsigned MoveStorage::DequeueMoveSequence()
{
	unsigned result = 0;
	_leafIndex = -1;
	// Find the first non-empty leaf node stack, pop its top into _leafIndex.
	const unsigned size = _fringe.size();
	for (; _offset < size && _fringe[_offset].empty(); _offset += 1) {}

	if (_offset < size) {
		auto & stack = _fringe[_offset];
		_leafIndex = stack.back();
		stack.pop_back();
		result = _offset+_startStackIndex;
	} 
	// Follow the links to recover all of its preceding moves in reverse order.
	_currentSequence.clear();
	for (NodeX node = _leafIndex; node != -1; node = _moveTree[node]._prevNode){
		const Move &mv = _moveTree[node]._move;
		_currentSequence.push_front(mv);
	}
	return result;
}
void MoveStorage::MakeSequenceMoves(Game&game)
{
	for (auto & move: _currentSequence){
		game.MakeMove(move);
	}
}
Moves MoveStorage::MovesVector() const
{
	Moves result(_currentSequence.begin(), _currentSequence.end());
	return result;
}


QMoves KSolveState::MakeAutoMoves()
{
	QMoves availableMoves;
	while ((availableMoves = FilteredAvailableMoves()).size() == 1)
	{
		_moveStorage.Push(availableMoves[0]);
		_game.MakeMove(availableMoves[0]);
	}
	return availableMoves;
}

// Return a vector of the available moves that pass the SkippableMove filter
QMoves KSolveState::FilteredAvailableMoves()
{
	QMoves availableMoves = _game.AvailableMoves();
	for (auto i = availableMoves.begin(); i < availableMoves.end(); ){
		if (SkippableMove(*i)) {
			availableMoves.erase(i);
		} else {
			i += 1;
		}
	}
	return availableMoves;
}


// Return true if this move cannot be in a minimum solution.
bool KSolveState::SkippableMove(Move trial)
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

	// Was the move from A to C possible at T0? Yes if no intervening
	// move has changed pile C.

	// Since nothing says A cannot equal C, this test catches 
	// moves that exactly reverse previous moves.
	const auto B = trial.From();
	if (B == STOCK || B == WASTE) return false; 
	const auto C = trial.To();
	const auto &movesMade = _moveStorage.MoveSequence();
	for (auto imv = movesMade.crbegin(); imv != movesMade.crend(); ++imv){
		const Move mv = *imv;
		if (mv.To() == B){
			// candidate T0 move
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

// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void KSolveState::CheckForMinSolution(){
	const unsigned nmv = MoveCount(_moveStorage.MoveSequence());
	{
		const unsigned x = _minSolution.size();
		if (x == 0 || nmv < _minSolutionCount) {
			_minSolution = _moveStorage.MovesVector();
			_minSolutionCount = nmv;
		}
	}
}

// Returns true if the current move sequence is the shortest path found
// so far to the current game state.
bool KSolveState::IsShortPathToState(unsigned moveCount)
{
	const GameState state{_game};
	// operator[] below returns a reference to the value in the 
	// hash map mapped to the key value 'state'.
	// If that is a new key, the value will be initialized to zero.
	unsigned short& mapped_value = _game_state_memory[state];
	const bool result = mapped_value == 0 || moveCount < mapped_value;
	if (result) mapped_value = moveCount;
	return result;
}
