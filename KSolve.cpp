#include "KSolve.hpp"
#include <stack>
#include <deque>
#include <algorithm>        // for sort
#include "robin_hood.h"     // for unordered_node_map
#ifdef KSOLVE_TRACE
#include <iostream>			// for cout
#endif  //KSOLVE_TRACE

class Hasher
{
public:
	size_t operator() (const GameStateType & gs) const 
	{
		robin_hood::hash<GameStateType::PartType> hash;
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
	std::deque<MoveNode> _moveTree;
	// Stack of indexes to leaf nodes in _moveTree
	typedef std::stack<index_t> HistoryStack;
	// The filing system
	std::vector<HistoryStack> _files;
	MoveSequenceType _currentSequence;
	index_t _leafIndex;			// index of current sequence's leaf node in _moveTree
	unsigned _startStackIndex;
	unsigned _maxStackIndex;
public:
	// Constructor.  maxIndex is the maximum size for a File argument.
	MoveStorage(unsigned maxIndex);
	// Push the given move to the back of the current sequence of moves.
	void Push(const Move& move);
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
	const MoveSequenceType& MoveSequence() const;
};

struct KSolveState {
	MoveStorage _open;
	robin_hood::unordered_node_map<GameStateType,unsigned,Hasher> _closed;
	Game &_game;
	Moves & _minSolution;
	unsigned _minSolutionCount;

	// statistics not required for operation
	unsigned _stateWins;
	unsigned _closedStates;
	unsigned _skippableWins;

	KSolveState(  Game & gm, 
			Moves& solution, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _open(maxMoves)
		, _closed()
		, _minSolution(solution)
		, _game(gm)
		, _minSolutionCount(maxMoves)
		, _stateWins(0)
		, _skippableWins(0)
		, _closedStates(0)
		{
			_closed.reserve(maxStates);
			_minSolution.clear();
		}

	Moves MakeAutoMoves();
	void CheckForMinSolution();
	void RecordState(unsigned minMoveCount);
	bool SkippableMove(const Move& mv);
	Moves FilteredAvailableMoves();
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
			Moves avail = state.MakeAutoMoves();

			if (avail.size() == 0) {
				KSolveCode rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
				if (rc == SOLVED) 
					solution = state._open.MovesVector();

				return KSolveResult(rc,state._closed.size(), solution);
			}
			assert(avail.size() > 1);
		}

		unsigned startMoves = MoveCount(state._open.MoveSequence())
						+ state._game.MinimumMovesLeft();

		state._open.File(startMoves);

		unsigned iOpen;
		for  (iOpen= startMoves; iOpen < state._minSolutionCount
				&& state._closed.size() <maxStates; iOpen+=1) {
			while (state._closed.size() <maxStates
				 && state._open.FetchMoveSequence(iOpen)) {
				state._game.Deal();
				state._open.MakeSequenceMoves(state._game);

				Moves avail = state.MakeAutoMoves();

				if (avail.size() == 0 && state._game.GameOver()) {
					// We have a solution.  See if it is a new champion
					state.CheckForMinSolution();
					// See if it the final winner.
					if (iOpen == state._minSolutionCount)
						break;
				}
				
				unsigned movesMadeCount = MoveCount(state._open.MoveSequence());
				unsigned minMoveCount = movesMadeCount+state._game.MinimumMovesLeft();

				if (minMoveCount < state._minSolutionCount)	{
					// There is still hope for this subtree.
					// Save the result of each of the possible next moves.
					for (auto mv: avail){
						state._game.MakeMove(mv);
						unsigned minMoveCount = movesMadeCount + mv.NMoves()
												+ state._game.MinimumMovesLeft();
						if (minMoveCount < state._minSolutionCount){
							assert(iOpen <= minMoveCount);
							state._open.Push(mv);
							state.RecordState(minMoveCount);
							state._open.Pop();
						}
						state._game.UnMakeMove(mv);
					}
				}
			}
		}
		KSolveCode outcome;
		if (iOpen > maxMoves || state._closed.size() >= maxStates){
			outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
		} else {
			outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
		}
		return KSolveResult(outcome,state._closed.size(),solution);
	} catch(std::bad_alloc) {
		unsigned nStates = state._closed.size();
		state._closed.clear();
		return KSolveResult(MEMORY_EXCEEDED,nStates,Moves());
	}
}

MoveStorage::MoveStorage(unsigned maxIndex)
	: _maxStackIndex(maxIndex)
	, _leafIndex(-1)
	, _startStackIndex(maxIndex+1)
{}
void MoveStorage::Push(const Move& move)
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
	if (_files.size() == 0) {
		_startStackIndex = index;
		unsigned cap = _maxStackIndex-_startStackIndex+1;
		_files.reserve(cap);
		static HistoryStack emptyStack;
		for (unsigned i = 0; i < cap; ++i)
			_files.push_back(emptyStack);
	}
	assert(_startStackIndex <= index);
	_files[index-_startStackIndex].push(_leafIndex);
}
bool MoveStorage::FetchMoveSequence(unsigned index)
{
	if (_startStackIndex > _maxStackIndex) return false;
	assert(_startStackIndex <= index && index <= _maxStackIndex);
	HistoryStack & stack =  _files[index-_startStackIndex];
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
const MoveSequenceType& MoveStorage::MoveSequence() const
{
	return _currentSequence;
}


Moves KSolveState::MakeAutoMoves()
{
	Moves avail;
	while ((avail = FilteredAvailableMoves()).size() == 1)
	{
		_open.Push(avail[0]);
		_game.MakeMove(avail[0]);
	}
	return avail;
}

// Return a vector of the available moves that pass the SkippableMove filter
Moves KSolveState::FilteredAvailableMoves()
{
	Moves avail = _game.AvailableMoves();
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
bool KSolveState::SkippableMove(const Move& trial)
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
	auto &movesMade = _open.MoveSequence();
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
	unsigned nmv = MoveCount(_open.MoveSequence());
	if (x == 0 || nmv < _minSolutionCount) {
		_minSolution = _open.MovesVector();
		_minSolutionCount = nmv;
	}
}

void KSolveState::RecordState(unsigned minMoveCount)
{
	GameStateType pState(_game);
	unsigned & storedMinimumCount = _closed[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_open.File(minMoveCount);
		_closedStates+=1;
#ifdef KSOLVE_TRACE
		if (_closedStates%1000000 == 999999){
			std::cout << "Stage " << _closedStates;
			std::cout << " improvements = " << _closedStates - _closed.size();
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
