#include "KSolve.hpp"
#include <stack>
#include "robin_hood.h"     // for unordered_map

typedef std::stack<Moves> HistoryStack;

class Hasher
{
public:
  size_t operator() (const GameStateType & gs) const
  {
	robin_hood::hash<std::uint64_t> hshr;
	return 	hshr(gs._long1)^hshr(gs._long2)^hshr(gs._short);
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


std::pair<KSolveResult,Moves> KSolve(
		Game& game,
		unsigned maxStates,
		unsigned maxMoves)
{
	Moves solution;
	KSolveState state(game,solution,maxMoves,maxStates);

	{
		Moves avail = state.MakeAutoMoves();

		if (avail.size() == 0) {
			solution = state._movesMade;
			KSolveResult rc = state._game.GameOver() ? SOLVED : IMPOSSIBLE;
			return std::pair<KSolveResult,Moves>(rc,solution);
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

			if (state._game.GameOver()) {
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
	KSolveResult outcome;
	if (ih > maxMoves || state._closed_previousStates.size() >= maxStates){
		outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
	} else {
		outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
	}
	return std::pair<KSolveResult,Moves>(outcome,solution);
}
Moves KSolveState::MakeAutoMoves()
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
Moves KSolveState::FilteredAvailableMoves()
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
bool KSolveState::SkippableMove(const Move& trial)
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
	} else ++_stateWins;
}

GameStateType::GameStateType(const Game& game)
{
	auto & tableau = game.Tableau();
	std::uint64_t pileStates [7];
	for (unsigned iPile = 0; iPile < 7; ++iPile){
		const Pile& tPile(tableau[iPile]);
		unsigned upCount = tPile.UpCount();
		assert(upCount <= 12);
		unsigned size = tPile.Size();
		unsigned downCount = size-upCount;
		// Since the base face-up card was dealt to this pile,
		// the downCount implies what it is.
		unsigned isMajor = 0;
		for (unsigned i = downCount+1; i < size; ++i){
			isMajor <<= 1;
			isMajor |= tPile[i].IsMajor();
		}
		pileStates[iPile] = isMajor<<7 | upCount<<3 | downCount; // 18 bits max
	}
	auto& foundation(game.Foundation());
	_long1 = foundation[0].Size()|foundation[1].Size()<<4 |
			 pileStates[0]<<8 |
			 pileStates[1]<<(8+18) |
			 pileStates[2]<<(8+18+18);
	_long2 = foundation[2].Size()|foundation[3].Size()<<4 |
			 pileStates[3]<<8 |
			 pileStates[4]<<(8+18) |
			 pileStates[5]<<(8+18+18);
	_short = game.Waste().Size() |
			 pileStates[6]<<5;
}

bool GameStateType::operator==(const GameStateType& other) const
{
	return _long1 == other._long1
		&& _long2 == other._long2
		&& _short == other._short;
}
