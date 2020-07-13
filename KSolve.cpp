#include "KSolve.hpp"
#include <stack>
#include <algorithm>        // for sort
#include "robin_hood.h"     // for unordered_node_map
#ifdef KSOLVE_TRACE
#include <iostream>			// for cout
#endif  //KSOLVE_TRACE

typedef std::stack<Moves> HistoryStack;

class Hasher
{
public:
  size_t operator() (const GameStateType & gs) const
  {
	size_t result = robin_hood::hash<std::uint32_t>()(gs._psts[0]);
	for (unsigned i = 1; i < 7; i+=2) {
		result ^= robin_hood::hash<std::uint32_t>()(gs._psts[i]);
		result += robin_hood::hash<std::uint32_t>()(gs._psts[i+1]);
	}
	return result;
  }
};

struct KSolveState {
	std::vector<HistoryStack> _open_histories;
	robin_hood::unordered_node_map<GameStateType,unsigned,Hasher> _closed_previousStates;
	Game &_game;
	Moves _movesMade;
	Moves & _minSolution;
	unsigned _minSolutionCount;
	unsigned _stateWins;
	unsigned _closedStates;
	unsigned _skippableWins;

	KSolveState(  Game & gm, 
			Moves& solution, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _open_histories(maxMoves,HistoryStack())
		, _closed_previousStates()
		, _minSolution(solution)
		, _game(gm)
		, _minSolutionCount(maxMoves)
		, _stateWins(0)
		, _skippableWins(0)
		, _closedStates(0)
		{
			_movesMade.reserve(maxMoves);
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
				solution = state._movesMade;
				KSolveCode rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
				return KSolveResult(rc,state._closed_previousStates.size(), solution);
			}
			assert(avail.size() > 1);
		}

		unsigned startMoves = MoveCount(state._movesMade)
						+ state._game.MinimumMovesLeft();

		state._open_histories[startMoves].push(state._movesMade);

		unsigned ih;
		for  (ih= startMoves; ih < state._minSolutionCount
				&& state._closed_previousStates.size() <maxStates; ++ih) {
			auto &h = state._open_histories[ih];
			// scan histories from shortest to longest
			while (h.size() && state._closed_previousStates.size() <maxStates) {
				state._game.Deal();
				state._movesMade = h.top();	
				h.pop();
				
				for (const auto& mv: state._movesMade){
					state._game.MakeMove(mv);
				}
				Moves avail = state.MakeAutoMoves();

				if (avail.size() == 0 && state._game.GameOver()) {
					// We have a solution.  See if it is a new champion
					state.CheckForMinSolution();
					// See if it the final winner.
					if (ih == state._minSolutionCount)
						break;
				}
				
				unsigned movesMadeCount = MoveCount(state._movesMade);
				unsigned minMoveCount = movesMadeCount+state._game.MinimumMovesLeft();

				if (minMoveCount < state._minSolutionCount)	{
					// There is still hope for this subtree.
					// Save the result of each of the possible next moves.
					for (auto mv: avail){
						state._movesMade.push_back(mv);
						state._game.MakeMove(mv);
						unsigned minMoveCount = movesMadeCount + mv.NMoves()
												+ state._game.MinimumMovesLeft();
						if (minMoveCount < state._minSolutionCount){
							assert(ih <= minMoveCount);
							state.RecordState(minMoveCount);
						}
						state._game.UnMakeMove(mv);
						state._movesMade.pop_back();
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
Moves KSolveState::MakeAutoMoves()
{
	Moves avail;
	while ((avail = FilteredAvailableMoves()).size() == 1)
	{
		_movesMade.push_back(avail[0]);
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
			++_skippableWins;
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
	for (auto imv = _movesMade.crbegin(); imv != _movesMade.crend(); ++imv){
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
	unsigned nmv = MoveCount(_movesMade);
	if (x == 0 || nmv < _minSolutionCount) {
		_minSolution = _movesMade;
		_minSolutionCount = nmv;
	}
}

void KSolveState::RecordState(unsigned minMoveCount)
{
	GameStateType pState(_game);
	unsigned & storedMinimumCount = _closed_previousStates[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_open_histories[minMoveCount].push(_movesMade);
		++_closedStates;
#ifdef KSOLVE_TRACE
		if (_closedStates%1000000 == 999999){
			std::cout << "Stage " << _closedStates;
			std::cout << " improvements = " << _closedStates - _closed_previousStates.size();
			std::cout << " minMoveCount = " << minMoveCount;
			std::cout << " _stateWins = " << _stateWins;
			std::cout << " _skippableWins = " << _skippableWins;
			std::cout << std::endl;
			std::cout << Peek(_game);
			std::cout << Peek(_movesMade) << std::endl << std::endl;
			std::cout << std::flush;
		}
#endif // KSOLVE_TRACE
	} else ++_stateWins;
}

GameStateType::GameStateType(const Game& game)
{
	union {
		struct {
			unsigned _upCount:4;
			unsigned _topRank:4;
			unsigned _topSuit:2;
			unsigned _isMajor:12;
			unsigned int _other : 10;
		};
		uint32_t asUnsigned;
	} p;
	for (unsigned i = 0; i<7; ++i){
		p.asUnsigned = 0;	// clear all bits
		const Pile& tp = game.Tableau()[i];
		unsigned upCount = tp.UpCount();
		p._upCount = upCount;
		if (upCount > 0) {
			const CardVec & cards = tp.Cards();
			const Card& topCard = *(cards.end()-upCount);
			p._topSuit = topCard.Suit();
			p._topRank = topCard.Rank();

			unsigned isMajorBits = 0;
			for (auto icd = cards.end()-upCount+1; icd < cards.end(); ++icd) {
				isMajorBits <<= 1;
				isMajorBits |= icd->IsMajor();
			}
			p._isMajor = isMajorBits;
		}
		_psts[i] = p.asUnsigned;
	}

	std::sort(_psts.begin(),_psts.end());

	for (unsigned i = 0; i < 4; ++i) {
		p.asUnsigned = _psts[i+2];
		p._other = game.Foundation()[i].Size();
		_psts[i+2] = p.asUnsigned;
	}
	p.asUnsigned = _psts[6];
	p._other = game.Stock().Size();
	_psts[6] = p.asUnsigned;
}

bool GameStateType::operator==(const GameStateType& other) const
{
	return _psts[0] == other._psts[0]
	    && _psts[1] == other._psts[1]
	    && _psts[2] == other._psts[2]
	    && _psts[3] == other._psts[3]
	    && _psts[4] == other._psts[4]
	    && _psts[5] == other._psts[5]
	    && _psts[6] == other._psts[6];
}
