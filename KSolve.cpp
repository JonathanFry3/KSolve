#include <KSolve.hpp>
#include <cassert>
#include <stack>
#include <unordered_map>

typedef std::stack<Moves> HistoryStack;

class Hasher
{
public:
  size_t operator() (const GameStateType & gs) const
  {
	size_t result = std::hash<std::uint32_t>()(gs._psts[0]);
	for (unsigned i = 1; i < 7; ++i) {
		result << 1;
		result ^= std::hash<std::uint32_t>()(gs._psts[i]);
	}
	return result;
  }
};

struct State {
	std::vector<HistoryStack> _histories;
	std::unordered_map<GameStateType,unsigned,Hasher> _previousStates;
	Game _game;
	Moves _movesMade;
	Moves & _minSolution;
	unsigned _minSolutionCount;
	unsigned _stateSize;			// expected value of _previousStates.size()
	unsigned _stateWins;			// count of previously encountered states have lower minimum move counts

	State(const CardVec & deck, 
			Moves& solution, 
			unsigned draw, 
			unsigned maxMoves, 
			unsigned maxStates)
		: _histories(maxMoves,HistoryStack())
		, _previousStates(maxStates,Hasher())
		, _minSolution(solution)
		, _game(deck,draw)
		, _minSolutionCount(maxMoves)
		, _stateSize(0)
		, _stateWins(0)
		{
			_movesMade.reserve(maxMoves);
			_minSolution.clear();
		}

	Moves MakeAutoMoves();
	void CheckForMinSolution();
	void RecordState(unsigned minMoveCount);
	unsigned MinimumMoves();
};

KSolveResult KSolve(const CardVec & deck,
		Moves& solution,
		unsigned draw,
		unsigned maxMoves,
		unsigned maxStates)
{
	State state(deck,solution,draw,maxMoves,maxStates);

	Moves avail = state.MakeAutoMoves();

	if (avail.size() == 0) {
		solution = state._movesMade;
		return state._game.GameOver() ? SOLVED : IMPOSSIBLE;
	}
	assert(avail.size() > 1);

	unsigned startMoves = state.MinimumMoves();
	state._histories[startMoves].push(state._movesMade);

	unsigned ih;
	for  (ih= startMoves; ih < state._minSolutionCount; ++ih) {
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
				// We have a solution.  See if it is a new champion.
				state.CheckForMinSolution();
				// See if it the final winner.
				if (ih == state._minSolutionCount)
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
						if (minMoveCount < ih)
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
	if (ih >= maxMoves || state._previousStates.size() > maxStates){
		outcome = state._minSolution.size() ? GAVEUP_SOLVED : GAVEUP_UNSOLVED;
	} else {
		outcome = state._minSolution.size() ? SOLVED : IMPOSSIBLE;
	}
	return outcome;
}

// Return a lower bound on the total number of moves required to complete
// a game that begins as this one has begun.
unsigned State::MinimumMoves()
{
	unsigned result = 52 - _game.FoundationCardCount() + _movesMade.size();
	unsigned draw = _game.Draw();
	unsigned draws = _game.Stock().Size();
	result += (draw == 1) ? draws : (draws+draw-1)/draw;
	return result;
}
Moves State::MakeAutoMoves()
{
	Moves avail;
	while (_movesMade.size() < _minSolutionCount && 
			(avail = _game.AvailableMoves()).size() == 1)
	{
		_movesMade.push_back(avail[0]);
		_game.MakeMove(avail[0]);
	}
	return avail;
}
// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void State::CheckForMinSolution(){
	unsigned x = _minSolution.size();
	unsigned nmv = _movesMade.size();
	if (x == 0 || nmv < x) {
		_minSolution = _movesMade;
		_minSolutionCount = nmv;
	}
}
void State::RecordState(unsigned minMoveCount)
{
	GameStateType pState(_game);
	unsigned & storedMinimumCount = _previousStates[pState];
	_stateSize += storedMinimumCount == 0;
	if (_stateSize != _previousStates.size())
		assert(_stateSize == _previousStates.size());
	if (storedMinimumCount == 0 || minMoveCount < storedMinimumCount) {
		storedMinimumCount = minMoveCount;
		_histories[minMoveCount].push(_movesMade);
	} 
	else ++_stateWins;
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
		uint32_t chi;
	} p;
	for (unsigned i = 0; i<7; ++i){
		p.chi = 0;	// clear all bits
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
		_psts[i] = p.chi;
	}
	for (unsigned i = 0; i < 4; ++i) {
		p.chi = _psts[i+2];
		p._other = game.Foundation()[i].Size();
		_psts[i+2] = p.chi;
	}
	p.chi = _psts[6];
	p._other = game.Stock().Size();
	_psts[6] = p.chi;
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

