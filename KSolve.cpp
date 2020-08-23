#include "KSolve.hpp"
#include <stack>
#include <deque>
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h"     // for 
#include "mf_vector.hpp"
#include <thread>
#ifdef KSOLVE_TRACE
#include <iostream>			// for cout
#endif  //KSOLVE_TRACE

typedef std::mutex Mutex;
typedef std::lock_guard<Mutex> Guard;

class Hasher
{
public:
	size_t operator() (const GameState & gs) const 
	{
		size_t result = 	 gs._part[0]
					  	   ^ gs._part[1]
						   ^ gs._part[2];
		return result;
	}
};

typedef std::deque<Move> MoveSequenceType;

class SharedMoveStorage
{
	typedef std::uint32_t index_t;
	struct MoveNode
	{
		Move _move;
		index_t _prevNode;
		MoveNode(const Move& mv, index_t prevNode)
			: _move(mv)
			, _prevNode(prevNode)
			{}
	};
	mf_vector<MoveNode> _moveTree;
	Mutex _moveTreeMutex;
	// Stack of indexes to leaf nodes in _moveTree
	typedef std::stack<index_t, mf_vector<index_t> > LeafNodeStack;
	// The leaf nodes waiting to grow new branches.  Each LeafNodeStack
	// stores nodes with the same minimum number of moves in any
	// completed game that can grow from them.  MoveStorage uses it
	// to implement a priority queue ordered by the minimum move count.
	std::vector<LeafNodeStack> _fringe;
	Mutex _fringeMutex;
	unsigned _startStackIndex;
	SharedMoveStorage() 
	: _startStackIndex(-1)
	{
		_fringe.reserve(100);
	}
	void Clear()
	{
		_moveTree.clear();
		_fringe.clear();
		_startStackIndex = -1;
	}
	friend class MoveStorage;
};
class MoveStorage
{
	typedef SharedMoveStorage::index_t index_t;
	typedef SharedMoveStorage::MoveNode MoveNode;
	typedef SharedMoveStorage::LeafNodeStack LeafNodeStack;
	static SharedMoveStorage k_shared;
	MoveSequenceType _currentSequence;
	index_t _leafIndex;			// index of current sequence's leaf node in _moveTree
public:
	// Constructor.  maxIndex is the maximum size for a File argument.
	MoveStorage();
	// Push the given move to the back of the current sequence of moves.
	void Push(Move move);
	// Remove the last move from the current move sequence.
	void Pop();
	// File the current move sequence under the given index number.
	// Calls after the first may not use an index less than the first.
	void File(unsigned index);
	// Fetch a move sequence with the lowest available index, make it
	// the current move sequence and unfile it.  
	// Return its index number, or zero if no more sequence are available.
	unsigned FetchMoveSequence(); 
	// Make all the moves in the current sequence
	void MakeSequenceMoves(Game&game);
	// Return the current move sequence in a vector.
	Moves MovesVector() const;
	// Return a const reference to the current move sequence in its
	// native type.
	const MoveSequenceType& MoveSequence() const {return _currentSequence;}
	// Resets to initial state
	void Clear();
};
SharedMoveStorage MoveStorage::k_shared;

struct KSolveState {
	// _moveStorage stores the portion of the move tree that has been generated.
	// Each node has a move and a reference to the node with
	// the move before it.  The leaves are indexed by the minimum number of 
	// moves possible in any finished game that might grow from that leaf.
	// _moveStorage also stores the sequence of moves we are currently working on.
	MoveStorage _moveStorage;
	// k_game_state_memory remembers the minimum move count at each game state we have
	// already visited.  If we get to that state again, we look at the current minimum
	// move count. If it is lower than the stored count, we keep our current node and store
	// its move count here.  If not, we forget the current node - we already have a
	// way to get to the same state that is at least as short.
	typedef phmap::parallel_flat_hash_map<GameState, unsigned short, Hasher,phmap::priv::hash_default_eq<GameState>,
		 phmap::priv::Allocator<phmap::priv::Pair<GameState,unsigned short> >, 4U, Mutex> 
		 MapType;
	static MapType k_game_state_memory;
	Game _game;
	unsigned _maxStates;

	Moves & _minSolution;
	static unsigned k_minSolutionCount;
	Mutex _minSolutionMutex;

	explicit KSolveState(  Game & gm, 
			Moves& solution,
			unsigned maxStates)
		: _minSolution(solution)
		, _game(gm)
		, _maxStates(maxStates)
		{
			k_game_state_memory.clear();
			k_game_state_memory.reserve(maxStates);
			_minSolution.clear();
			_moveStorage.Clear();
			k_minSolutionCount = -1;
		}
	explicit KSolveState(const KSolveState& orig)
		: _moveStorage()
		, _game(orig._game)
		, _minSolution(orig._minSolution)
		, _maxStates(orig._maxStates)
		{}
			
	QMoves MakeAutoMoves();
	void CheckForMinSolution();
	void RecordState(unsigned minMoveCount);
	bool SkippableMove(Move mv);
	QMoves FilteredAvailableMoves();
};
KSolveState::MapType KSolveState::k_game_state_memory; // initializers
unsigned KSolveState::k_minSolutionCount(-1);

void KSolveWorker(
		KSolveState* pMasterState);

KSolveResult KSolve(
		Game& game,
		unsigned maxStates)
{
	Moves solution;
	enum {maxMoves = 512};
	KSolveState state(game,solution,maxStates);
	QMoves avail = state.MakeAutoMoves();

	if (avail.size() == 0) {
		KSolveCode rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
		if (rc == SOLVED) 
			solution = state._moveStorage.MovesVector();

		return KSolveResult(rc,state.k_game_state_memory.size(), solution);
	}
	assert(avail.size() > 1);

	unsigned startMoves = MoveCount(state._moveStorage.MoveSequence())
					+ state._game.MinimumMovesLeft();

	state._moveStorage.File(startMoves);
	try	{
#ifndef NTHREADS
#define NTHREADS 1
#endif
		const unsigned nthreads = NTHREADS;
		std::vector<std::thread> threads;
		for (unsigned ithread = 0; ithread < nthreads; ++ithread) {
			threads.emplace_back(&KSolveWorker, &state);
			std::this_thread::sleep_for(std::chrono::milliseconds(23));
		}
		//KSolveWorker(&state);
		for (auto& thread: threads) 
			thread.join();

		KSolveCode outcome;
		if (state.k_game_state_memory.size() >= maxStates){
			outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
		} else {
			outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
		}
		return KSolveResult(outcome,state.k_game_state_memory.size(),solution);
	} 
	
	catch(std::bad_alloc) {
		unsigned nStates = state.k_game_state_memory.size();
		state.k_game_state_memory.clear();
		return KSolveResult(MEMORY_EXCEEDED,nStates,Moves());
	}
}

void KSolveWorker(
		KSolveState* pMasterState)
{
	KSolveState state(*pMasterState);

	// Main loop
	unsigned minMoves0;
	while (state.k_game_state_memory.size() <state._maxStates
			&& (minMoves0 = state._moveStorage.FetchMoveSequence())) {     // <- side effect
		state._game.Deal();
		state._moveStorage.MakeSequenceMoves(state._game);

		QMoves availableMoves = state.MakeAutoMoves();

		if (availableMoves.size() == 0 && state._game.GameOver()) {
			// We have a solution.  See if it is a new champion
			state.CheckForMinSolution();
			// See if it the final winner.
			if (minMoves0 == state.k_minSolutionCount)
				break;
		}
		
		unsigned movesMadeCount = MoveCount(state._moveStorage.MoveSequence());
		unsigned minMoveCount = movesMadeCount+state._game.MinimumMovesLeft();
		assert(((minMoves0 <= minMoveCount),"first"));

		if (minMoveCount < state.k_minSolutionCount){
			// There is still hope for this subtree.
			// Save the result of each of the possible next moves.
			for (auto mv: availableMoves){
				state._game.MakeMove(mv);
				unsigned minMoveCount = movesMadeCount + mv.NMoves()
										+ state._game.MinimumMovesLeft();
				if (minMoveCount < state.k_minSolutionCount){
					assert(((minMoves0 <= minMoveCount),"second"));
					state._moveStorage.Push(mv);
					state.RecordState(minMoveCount);
					state._moveStorage.Pop();
				}
				state._game.UnMakeMove(mv);
			}
		}
	}
}
MoveStorage::MoveStorage()
	: _leafIndex(-1)
	{}
void MoveStorage::Clear(){
	_leafIndex = -1;
	k_shared.Clear();
}
void MoveStorage::Push(Move move)
{
	_currentSequence.push_back(move);
	index_t ind;
	{
		Guard( k_shared._moveTreeMutex);
		ind = k_shared._moveTree.size();
		k_shared._moveTree.emplace_back(move, _leafIndex);
	}
	_leafIndex = ind;
}
void MoveStorage::Pop()
{
	_currentSequence.pop_back();
	_leafIndex = k_shared._moveTree[_leafIndex]._prevNode;
}
void MoveStorage::File(unsigned index)
{
	if (k_shared._fringe.size() == 0) {
		k_shared._startStackIndex = index;
	}
	assert(k_shared._startStackIndex <= index);
	unsigned offset = index - k_shared._startStackIndex;
	static LeafNodeStack emptyStack;
	{
		Guard(k_shared._fringeMutex);
		for (unsigned i = k_shared._fringe.size(); i <= offset; i+=1) 
			k_shared._fringe.push_back(emptyStack);
		k_shared._fringe[offset].push(_leafIndex);
	}
}
unsigned MoveStorage::FetchMoveSequence()
{
	unsigned offset, size;
	{
		Guard(k_shared._fringeMutex);
		size = k_shared._fringe.size();
		for (offset = 0; offset < size && k_shared._fringe[offset].empty(); offset += 1) {}
		if (offset < size) {
			LeafNodeStack & stack =  k_shared._fringe[offset];
			_leafIndex = stack.top();
			stack.pop();
		}
	}
	if (offset >= size) 
		return (0);

	_currentSequence.clear();
	for (index_t node = _leafIndex; node != -1; node = k_shared._moveTree[node]._prevNode){
		_currentSequence.push_front(k_shared._moveTree[node]._move);
	}
	return offset+k_shared._startStackIndex;
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
	auto B = trial.From();
	if (B == STOCK || B == WASTE) return false; 
	auto C = trial.To();
	auto &movesMade = _moveStorage.MoveSequence();
	for (auto imv = movesMade.crbegin(); imv != movesMade.crend(); imv+=1){
		Move mv = *imv;
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
	unsigned nmv = MoveCount(_moveStorage.MoveSequence());
	{
		Guard(this->_minSolutionMutex);
		unsigned x = _minSolution.size();
		if (x == 0 || nmv < k_minSolutionCount) {
			_minSolution = _moveStorage.MovesVector();
			k_minSolutionCount = nmv;
		}
	}
}

void KSolveState::RecordState(unsigned minMoveCount)
{
	GameState pState(_game);
	/*
	auto & storedMinimumCount = k_game_state_memory[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_moveStorage.File(minMoveCount); 
	}
	*/
	bool valueChanged{false};
	bool newKey = k_game_state_memory.try_emplace_l(
		pState,						// key
		[&](auto& mapped_value) {	// run behind lock when key found
			valueChanged = minMoveCount < mapped_value;
			if (valueChanged) 
				mapped_value = minMoveCount;
		},
		minMoveCount 				// c'tor run behind lock when key not found
	);
	if (newKey || valueChanged) 
		_moveStorage.File(minMoveCount); 
}
  