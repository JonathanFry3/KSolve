#include "KSolve.hpp"
#include <stack>
#include <deque>
#include <algorithm>        // for sort
#include "robin_hood.h"     // for unordered_node_map
#include "mf_vector.hpp"
#ifdef KSOLVE_TRACE
#include <iostream>			// for cout
#endif  //KSOLVE_TRACE

class Hasher
{
public:
	size_t operator() (const GameState & gs) const 
	{
		robin_hood::hash<GameState::PartType> hash;
		size_t result = hash(gs._part[0]
					  	   ^ gs._part[1]
						   ^ gs._part[2]);
		return result;
	}
};

typedef std::deque<Move> MoveSequenceType;
class MoveStorage
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
	// Stack of indexes to leaf nodes in _moveTree
	typedef std::stack<index_t, mf_vector<index_t> > LeafNodeStack;
	// The leaf nodes waiting to grow new branches.  Each LeafNodeStack
	// stores nodes with the same minimum number of moves in any
	// completed game that can grow from them.  The client accesses them
	// using that minimum number of moves.
	std::vector<LeafNodeStack> _fringe;
	MoveSequenceType _currentSequence;
	index_t _leafIndex;			// index of current sequence's leaf node in _moveTree
	unsigned _startStackIndex;
	unsigned _maxStackIndex;
public:
	// Constructor.  maxIndex is the maximum size for a File argument.
	MoveStorage(unsigned maxIndex);
	// Push the given move to the back of the current sequence of moves.
	void Push(Move move);
	// Remove the last move from the current move sequence.
	void Pop();
	// File the current move sequence under the given index number.
	// Calls after the first may not use an index less than the first.
	void File(unsigned index);
	// Make one of the move sequences filed under the given index number
	// the current move sequence and unfile it.  
	// Return false if there are no more sequences filed under that number.
	bool FetchMoveSequence(unsigned index); 
	// Make all the moves in the current sequence
	void MakeSequenceMoves(Game&game);
	// Return the current move sequence in a vector.
	Moves MovesVector() const;
	// Return a const reference to the current move sequence in its
	// native type.
	const MoveSequenceType& MoveSequence() const {return _currentSequence;}
};

struct KSolveState {
	// _moveTree stores the portion of the move tree that has been generated.
	// Each node has a move and a reference to the node with
	// the move before it.  The leaves are indexed by the minimum number of 
	// moves possible in any finished game that might grow from that leaf.
	// _moveTree also stores the sequence of moves we are currently working on.
	MoveStorage _moveTree;
	// _game_state_memory remembers the minimum move count at each game state we have
	// already visited.  If we get to that state again, we look at the current minimum
	// move count. If it is lower than the stored count, we keep our current node and store
	// its move count here.  If not, we forget the current node - we already have a
	// way to get to the same state that is at least as short.
	robin_hood::unordered_node_map<GameState, unsigned, Hasher> _game_state_memory;
	Game &_game;
	Moves & _minSolution;
	unsigned _minSolutionCount;

	// statistics not required for operation
	unsigned _stateWins;
	unsigned _rememberedStates;
	unsigned _skippableWins;

	KSolveState(  Game & gm, 
			Moves& solution, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _moveTree(maxMoves)
		, _game_state_memory()
		, _minSolution(solution)
		, _game(gm)
		, _minSolutionCount(maxMoves)
		, _stateWins(0)
		, _skippableWins(0)
		, _rememberedStates(0)
		{
			_game_state_memory.reserve(maxStates);
			_minSolution.clear();
		}

	QMoves MakeAutoMoves();
	void CheckForMinSolution();
	void RecordState(unsigned minMoveCount);
	bool SkippableMove(Move mv);
	QMoves FilteredAvailableMoves();
};


KSolveResult KSolve(
		Game& game,
		unsigned maxStates)
{
	Moves solution;
	enum {maxMoves = 512};
	KSolveState state(game,solution,maxMoves,maxStates);
	try	{
		{
			QMoves avail = state.MakeAutoMoves();

			if (avail.size() == 0) {
				KSolveCode rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
				if (rc == SOLVED) 
					solution = state._moveTree.MovesVector();

				return KSolveResult(rc,state._game_state_memory.size(), solution);
			}
			assert(avail.size() > 1);
		}

		unsigned startMoves = MoveCount(state._moveTree.MoveSequence())
						+ state._game.MinimumMovesLeft();

		state._moveTree.File(startMoves);

		// Main loop
		unsigned minMoves0;
		for  (minMoves0 = startMoves; minMoves0 < state._minSolutionCount
				&& state._game_state_memory.size() <maxStates; minMoves0+=1) {
			while (state._game_state_memory.size() <maxStates
				 && state._moveTree.FetchMoveSequence(minMoves0)) {
				state._game.Deal();
				state._moveTree.MakeSequenceMoves(state._game);

				QMoves avail = state.MakeAutoMoves();
				
				if (avail.size() == 0 && state._game.GameOver()) {
					// We have a solution.  See if it is a new champion
					state.CheckForMinSolution();
					// See if it the final winner.
					if (minMoves0 == state._minSolutionCount)
						break;
				}
				
				unsigned movesMadeCount = MoveCount(state._moveTree.MoveSequence());
				unsigned minMoveCount = movesMadeCount+state._game.MinimumMovesLeft();

				if (minMoveCount < state._minSolutionCount)	{
					// There is still hope for this subtree.
					// Save the result of each of the possible next moves.
					for (auto mv: avail){
						state._game.MakeMove(mv);
						unsigned minMoveCount = movesMadeCount + mv.NMoves()
												+ state._game.MinimumMovesLeft();
						if (minMoveCount < state._minSolutionCount){
							assert(minMoves0 <= minMoveCount);
							state._moveTree.Push(mv);
							state.RecordState(minMoveCount);
							state._moveTree.Pop();
						}
						state._game.UnMakeMove(mv);
					}
				}
			}
		}
		KSolveCode outcome;
		if (minMoves0 > maxMoves || state._game_state_memory.size() >= maxStates){
			outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
		} else {
			outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
		}
		return KSolveResult(outcome,state._game_state_memory.size(),solution);
	} catch(std::bad_alloc) {
		unsigned nStates = state._game_state_memory.size();
		state._game_state_memory.clear();
		return KSolveResult(MEMORY_EXCEEDED,nStates,Moves());
	}
}

MoveStorage::MoveStorage(unsigned maxIndex)
	: _maxStackIndex(maxIndex+1)
	, _leafIndex(-1)
	, _startStackIndex(maxIndex+1)
{}
void MoveStorage::Push(Move move)
{
	_currentSequence.push_back(move);
	index_t ind = _moveTree.size();
	_moveTree.emplace_back(move, _leafIndex);
	_leafIndex = ind;
}
void MoveStorage::Pop()
{
	_currentSequence.pop_back();
	_leafIndex = _moveTree[_leafIndex]._prevNode;
}
void MoveStorage::File(unsigned index)
{
	if (_fringe.size() == 0) {
		_startStackIndex = index;
		unsigned cap = _maxStackIndex-_startStackIndex+1;
		_fringe.reserve(cap);
		static LeafNodeStack emptyStack;
		for (unsigned i = 0; i < cap; ++i)
			_fringe.push_back(emptyStack);
	}
	assert(_startStackIndex <= index);
	_fringe[index-_startStackIndex].push(_leafIndex);
}
bool MoveStorage::FetchMoveSequence(unsigned index)
{
	if (_startStackIndex > _maxStackIndex) return false;
	assert(_startStackIndex <= index && index <= _maxStackIndex);
	LeafNodeStack & stack =  _fringe[index-_startStackIndex];
	bool result = stack.size() != 0;
	if (result) {
		_currentSequence.clear();
		_leafIndex = stack.top();
		for (index_t node = _leafIndex; node != -1; node = _moveTree[node]._prevNode){
			_currentSequence.push_front(_moveTree[node]._move);
		}
		stack.pop();
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
	QMoves avail;
	while ((avail = FilteredAvailableMoves()).size() == 1)
	{
		_moveTree.Push(avail[0]);
		_game.MakeMove(avail[0]);
	}
	return avail;
}

// Return a vector of the available moves that pass the SkippableMove filter
QMoves KSolveState::FilteredAvailableMoves()
{
	QMoves avail = _game.AvailableMoves();
	for (auto i = avail.begin(); i < avail.end(); ){
		if (SkippableMove(*i)) {
			avail.erase(i);
			_skippableWins+=1;
		} else {
			i += 1;
		}
	}
	return avail;
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
	auto &movesMade = _moveTree.MoveSequence();
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
	unsigned x = _minSolution.size();
	unsigned nmv = MoveCount(_moveTree.MoveSequence());
	if (x == 0 || nmv < _minSolutionCount) {
		_minSolution = _moveTree.MovesVector();
		_minSolutionCount = nmv;
	}
}

void KSolveState::RecordState(unsigned minMoveCount)
{
	GameState pState(_game);
	unsigned & storedMinimumCount = _game_state_memory[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_moveTree.File(minMoveCount);
		_rememberedStates+=1;
#ifdef KSOLVE_TRACE
		if (_rememberedStates%1000000 == 999999){
			std::cout << "Stage " << _rememberedStates;
			std::cout << " improvements = " << _rememberedStates - _game_state_memory.size();
			std::cout << " minMoveCount = " << minMoveCount;
			std::cout << " _stateWins = " << _stateWins;
			std::cout << " _skippableWins = " << _skippableWins;
			std::cout << std::endl;
			std::cout << Peek(_game);
			std::cout << Peek(_moveSequence) << std::endl << std::endl;
			std::cout << std::flush;
		}
#endif // KSOLVE_TRACE
	} else _stateWins+=1;
}
