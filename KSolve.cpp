#include "KSolve.hpp"
#include <stack>
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
		robin_hood::hash<std::uint64_t> hash;
		size_t result = hash(gs._psts[0])
					  ^ hash(gs._psts[1])
					  + hash(gs._psts[2]);
		return result;
	}
};

typedef Moves MoveSequenceType;
class MoveStorage
{
	typedef std::stack<MoveSequenceType> HistoryStack;
	std::vector<HistoryStack> _stackVector;
	MoveSequenceType _moveSequence;
	unsigned _startIndex;
	unsigned _maxIndex;
public:
	MoveStorage(unsigned maxIndex);
	void Push(const Move& move);
	void Pop();
	void File(unsigned index);
	bool FetchMoveSequence(unsigned index); // returns false if no more moves available
	void MakeSequenceMoves(Game&game);
	unsigned CountOfMoves() const;
	Moves MovesVector() const;
	const MoveSequenceType& MoveSequence() const;
};

struct KSolveState {
	MoveStorage _open_histories;
	robin_hood::unordered_node_map<GameStateType,unsigned,Hasher> _closed_previousStates;
	Game &_game;
	Moves & _minSolution;
	unsigned _minSolutionCount;
	unsigned _stateWins;
	unsigned _closedStates;
	unsigned _skippableWins;

	KSolveState(  Game & gm, 
			Moves& solution, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _open_histories(maxMoves)
		, _closed_previousStates()
		, _minSolution(solution)
		, _game(gm)
		, _minSolutionCount(maxMoves)
		, _stateWins(0)
		, _skippableWins(0)
		, _closedStates(0)
		{
			_closed_previousStates.reserve(maxStates);
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
					solution = state._open_histories.MovesVector();

				return KSolveResult(rc,state._closed_previousStates.size(), solution);
			}
			assert(avail.size() > 1);
		}

		unsigned startMoves = state._open_histories.CountOfMoves()
						+ state._game.MinimumMovesLeft();

		state._open_histories.File(startMoves);

		unsigned ih;
		for  (ih= startMoves; ih < state._minSolutionCount
				&& state._closed_previousStates.size() <maxStates; ih+=1) {
			while (state._open_histories.FetchMoveSequence(ih)
				 && state._closed_previousStates.size() <maxStates) {
				state._game.Deal();
				state._open_histories.MakeSequenceMoves(state._game);

				Moves avail = state.MakeAutoMoves();

				if (avail.size() == 0 && state._game.GameOver()) {
					// We have a solution.  See if it is a new champion
					state.CheckForMinSolution();
					// See if it the final winner.
					if (ih == state._minSolutionCount)
						break;
				}
				
				unsigned movesMadeCount = state._open_histories.CountOfMoves();
				unsigned minMoveCount = movesMadeCount+state._game.MinimumMovesLeft();

				if (minMoveCount < state._minSolutionCount)	{
					// There is still hope for this subtree.
					// Save the result of each of the possible next moves.
					for (auto mv: avail){
						state._game.MakeMove(mv);
						unsigned minMoveCount = movesMadeCount + mv.NMoves()
												+ state._game.MinimumMovesLeft();
						if (minMoveCount < state._minSolutionCount){
							assert(ih <= minMoveCount);
							state._open_histories.Push(mv);
							state.RecordState(minMoveCount);
							state._open_histories.Pop();
						}
						state._game.UnMakeMove(mv);
					}
				}
			}
		}
		KSolveCode outcome;
		if (ih > maxMoves || state._closed_previousStates.size() >= maxStates){
			outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
		} else {
			outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
		}
		return KSolveResult(outcome,state._closed_previousStates.size(),solution);
	} catch(std::bad_alloc) {
		unsigned nStates = state._closed_previousStates.size();
		state._closed_previousStates.clear();
		return KSolveResult(MEMORY_EXCEEDED,nStates,Moves());
	}
}

MoveStorage::MoveStorage(unsigned maxIndex)
	: _maxIndex(maxIndex)
{}
void MoveStorage::Push(const Move& move)
{
	_moveSequence.push_back(move);
}
void MoveStorage::Pop()
{
	_moveSequence.pop_back();
}
void MoveStorage::File(unsigned index)
{
	if (_stackVector.size() == 0) {
		_startIndex = index;
		unsigned cap = _maxIndex-_startIndex+1;
		_stackVector.reserve(cap);
		HistoryStack emptyStack;
		for (unsigned i = 0; i < cap; ++i)
			_stackVector.push_back(emptyStack);
	}
	_stackVector[index-_startIndex].push(_moveSequence);
}
bool MoveStorage::FetchMoveSequence(unsigned index)
{
	HistoryStack & stack =  _stackVector[index-_startIndex];
	bool result = stack.size() != 0;
	if (result) {
		_moveSequence = stack.top();
		stack.pop();
	}
	return result;
}
void MoveStorage::MakeSequenceMoves(Game&game)
{
	for (auto & move: _moveSequence){
		game.MakeMove(move);
	}
}
unsigned MoveStorage::CountOfMoves() const
{
	return MoveCount(_moveSequence);
}
Moves MoveStorage::MovesVector() const
{
	return _moveSequence;
}
const MoveSequenceType& MoveStorage::MoveSequence() const
{
	return _moveSequence;
}
Moves KSolveState::MakeAutoMoves()
{
	Moves avail;
	while ((avail = FilteredAvailableMoves()).size() == 1)
	{
		_open_histories.Push(avail[0]);
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
	auto movesMade = _open_histories.MoveSequence();
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
	unsigned nmv = _open_histories.CountOfMoves();
	if (x == 0 || nmv < _minSolutionCount) {
		_minSolution = _open_histories.MovesVector();
		_minSolutionCount = nmv;
	}
}

void KSolveState::RecordState(unsigned minMoveCount)
{
	GameStateType pState(_game);
	unsigned & storedMinimumCount = _closed_previousStates[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_open_histories.File(minMoveCount);
		_closedStates+=1;
#ifdef KSOLVE_TRACE
		if (_closedStates%1000000 == 999999){
			std::cout << "Stage " << _closedStates;
			std::cout << " improvements = " << _closedStates - _closed_previousStates.size();
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
