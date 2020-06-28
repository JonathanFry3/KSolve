#include "KSolve.hpp"
#include <stack>
#include "robin_hood.h"     // for unordered_map

typedef std::stack<Moves> HistoryStack;

class Hasher
{
public:
  size_t operator() (const GameStateType & gs) const
  {
	size_t result = robin_hood::hash<std::uint32_t>()(gs._psts[0]);
	for (unsigned i = 1; i < 7; ++i) {
		result ^= robin_hood::hash<std::uint32_t>()(gs._psts[i]);
	}
	return result;
  }
};

struct State {
	std::vector<HistoryStack> _histories;
	robin_hood::unordered_flat_map<GameStateType,unsigned,Hasher> _previousStates;
	Game &_game;
	Moves _movesMade;
	Moves & _minSolution;
	unsigned _minSolutionCount;
	unsigned _stateWins;
	unsigned _skippableWins;

	State(  Game & gm, 
			Moves& solution, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _histories(maxMoves,HistoryStack())
		, _previousStates(maxStates*5/3,Hasher())
		, _minSolution(solution)
		, _game(gm)
		, _minSolutionCount(maxMoves)
		, _stateWins(0)
		, _skippableWins(0)
		{
			_movesMade.reserve(maxMoves);
			_minSolution.clear();
		}

	Moves MakeAutoMoves();
	void CheckForMinSolution();
	void RecordState(unsigned minMoveCount);
	unsigned MinimumMoves();
	bool SkippableMove(const Move& mv);
	Moves FilteredAvailableMoves();
};


std::pair<KSolveResult,Moves> KSolve(
		Game& game,
		unsigned maxMoves,
		unsigned maxStates)
{
	Moves solution;
	State state(game,solution,maxMoves,maxStates);

	Moves avail = state.MakeAutoMoves();

	if (avail.size() == 0) {
		solution = state._movesMade;
		KSolveResult rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
		return std::pair<KSolveResult,Moves>(rc,solution);
	}
	assert(avail.size() > 1);

	unsigned startMoves = state.MinimumMoves();
	state._histories[startMoves].push(state._movesMade);

	unsigned ih;
	for  (ih= startMoves; ih <= state._minSolutionCount
			 && state._previousStates.size() <maxStates; ++ih) {
		auto &h = state._histories[ih];
		// scan histories from shortest to longest
		while (h.size() && state._previousStates.size() <maxStates) {
			state._game.Deal();
			state._movesMade = h.top();	
			h.pop();
			
			for (const auto& mv: state._movesMade){
				state._game.MakeMove(mv);
			}
			Moves avail = state.MakeAutoMoves();

			if (state._game.GameOver()) {
				// We have a solution.  See if it is a new champion
				state.CheckForMinSolution();
				// See if it the final winner.
				if (ih == MoveCount(state._movesMade))
					break;
			}
			unsigned minMoveCount = state.MinimumMoves();
			if (minMoveCount < state._minSolutionCount)	{
				// There is still hope for this one.
				// Save the result of each of the possible next moves.
				for (auto mv: avail){
					state._movesMade.push_back(mv);
					state._game.MakeMove(mv);
					minMoveCount = state.MinimumMoves();
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
	KSolveResult outcome;
	if (ih > maxMoves || state._previousStates.size() >= maxStates){
		outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
	} else {
		outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
	}
	return std::pair<KSolveResult,Moves>(outcome,solution);
}

// Return a lower bound on the total number of moves required to complete
// a game that begins as this one has begun.
unsigned State::MinimumMoves()
{
	unsigned result = 52 - _game.FoundationCardCount() + MoveCount(_movesMade);
	unsigned draw = _game.Draw();
	unsigned draws = _game.Stock().Size();
	result += (draw == 1) ? draws : (draws+draw-1)/draw;
	return result;
}
Moves State::MakeAutoMoves()
{
	Moves avail;
	unsigned mvCount = MoveCount(_movesMade);
	while (mvCount < _minSolutionCount && 
			(avail = FilteredAvailableMoves()).size() == 1)
	{
		_movesMade.push_back(avail[0]);
		_game.MakeMove(avail[0]);
		mvCount += avail[0].NMoves();
	}
	return avail;
}

// Return a vector of the available moves that pass the SkippableMove filter
Moves State::FilteredAvailableMoves()
{
	Moves avail = _game.AvailableMoves();
	for (auto i = avail.begin(); i < avail.end(); ++i){
		if (SkippableMove(*i)) {
			avail.erase(i);
			++_skippableWins;
		}	
	}
	return avail;
}


// Return true if this move cannot be in a minimum solution.
bool State::SkippableMove(const Move& trial)
{
	/* 
	Consider a move at time T0 from A to B and the next move
	from B, which goes to C at time Tn.  The move at Tn is
	skippable if the same result could have been achieved 
	at T0 by moving the same cards directly from A to C.

	We are now at Tn looking back for a T0 move.  B is our from pile
	and C is our to pile.  A candidate T0 move is one that moves
	to our from pile (pile B).

	Do those two moves move the same set of cards?.  Yes if
	no intervening move has changed pile B and the two moves
	move the same number of cards.

	Was the move from A to C possible at T0? Yes if no intervening
	move has changed pile C.
	*/
	auto B = trial.From();
	if (B == STOCK || B == WASTE) return false;
	auto C = trial.To();
	for (auto imv = _movesMade.crbegin(); imv != _movesMade.crend(); ++imv){
		Move mv = *imv;
		if (mv.To() == B){
			// candidate T0 move
			return  mv.N() == trial.N();
		} else {
			// intervening move
			if (mv.To() == C || mv.From() == C)
				return false;			// trial move's to pile (C) has changed
			if (mv.From() == B) 
				return false;			// trial move's from pile (B) has changed
		}
	}
	return false;
}

// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void State::CheckForMinSolution(){
	unsigned x = _minSolution.size();
	unsigned nmv = MoveCount(_movesMade);
	if (x == 0 || nmv < x) {
		_minSolution = _movesMade;
		_minSolutionCount = nmv;
	}
}

void State::RecordState(unsigned minMoveCount)
{
	GameStateType pState(_game);
	unsigned & storedMinimumCount = _previousStates[pState];
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_histories[minMoveCount].push(_movesMade);
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
		if (p._upCount > 0) {
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
