/*
	Implements Game.hpp.
*/

#include <Game.hpp>
#include <cassert>

const std::string suits("cdsh");
const std::string ranks("a23456789tjqka");


// Returns a string composed only of the characters in input that also appear in filter
static std::string Filtered(std::string input, std::string filter)
{
	std::string result;
	for (auto ic = input.begin(); ic<input.end(); ++ic)
	{
		if (filter.find(*ic) != std::string::npos)
		{
			result += *ic;
		}
	}
	return result;
}

static std::string LowerCase(const std::string & in)
{
	std::string result;
	result.reserve(in.length());
	for (auto ich = in.begin(); ich < in.end(); ++ich)
	{
		result.push_back(std::tolower(*ich));
	}
	return result;
}

// Return a string like "d5" or "ca" given a Card
std::string Card::AsString() const
{
	return suits.substr(_suit,1) + ranks[_rank];
}

// Make a Card from a string like "ah" or "s8".
// Returns true if it succeeds.
bool Card::FromString(const std::string& s0, Card & card)    
{
	std::string s1 = Filtered(LowerCase(s0),suits+ranks+"10");
	Suit_t suit;
	Rank_t rank;
	bool result = s1.length() == 2 || s1.length() == 3;
	std::string rankStr;
	if (result)
	{
		auto suitIndex = suits.find(s1[0]);
		if (suitIndex != std::string::npos)
		{
			// input has suit first
			suit = static_cast<Suit_t>(suitIndex);
			rankStr = s1.substr(1);
		}
		else
		{
			// suit does not appear first in input
			{
				// assume it is last
				suitIndex = suits.find(s1.back());
				suit = static_cast<Suit_t>(suitIndex);
				result = suitIndex != std::string::npos;
				rankStr = s1.substr(0,s1.length()-1);
			}
		}
	}
	if (result)
	{
		if (rankStr == "10") {rankStr = "t";}
		auto rankIndex = ranks.find(rankStr);
		result = rankIndex != std::string::npos;
		if (result)
		{
			rank = static_cast<Rank_t>(rankIndex);
		}
	}
	if (result)
	{
		card = Card(suit,rank);
	}
	return result;
}

CardVec Pile::Pop(unsigned n)
{
	CardVec result;
	result.append(_cards.end()-n, _cards.end());
	_cards.pop_back(n);
	return result;
}

CardVec Pile::Draw(unsigned n)
{
	CardVec result = this->Pop(n);
	if (n > 1)
		std::reverse(result.begin(),result.end());
	return result;
}

void Pile::Draw(Pile& other, int n)
{
	Pile & to = (n>0) ? *this : other;
	Pile & fm = (n>0) ? other : *this;
	unsigned nm = (n>0) ? n : -n;
	assert(nm <= fm.Size());
	for (unsigned i = 0; i < nm; ++i)
		to.Draw(fm);
}

unsigned MoveCount(const Moves& moves)
{
	unsigned result = 0;
	for (const auto & mv: moves){
		result += mv.NMoves();
	}
	return result;
}
Game::Game(const std::vector<Card> &deck,unsigned draw)
	: _deck(deck)
	, _waste(WASTE)
	, _stock(STOCK)
	, _draw(draw)
	, _foundation{FOUNDATION1C,FOUNDATION2D,FOUNDATION3S,FOUNDATION4H}
	, _tableau{TABLEAU1,TABLEAU2,TABLEAU3,TABLEAU4,TABLEAU5,TABLEAU6,TABLEAU7}
{
	_allPiles[WASTE] = &_waste;
	_allPiles[STOCK] = &_stock;
	for (int ip = 0; ip < 4; ++ip) _allPiles[FOUNDATION+ip] = &_foundation[ip];
	for (int ip = 0; ip < 7; ++ip) _allPiles[TABLEAU+ip] = &_tableau[ip];
	Deal();
}

// Deal the cards for Klondike Solitaire.
void Game::Deal()
{
	for (auto pile: _allPiles)
	{
		pile->ClearCards();
	}
	unsigned ideck = 0;
	for (unsigned i = 0; i<7; ++i)
	{
		for (unsigned icd = i; icd < 7; ++icd)
		{
			_tableau[icd].Push(_deck[ideck++]);
		}
		_tableau[i].IncrUpCount(1);      // turn up the top card
	}
	_waste.Push(_deck.data()+28,_deck.data()+_deck.size());
	_stock.Draw(_waste,24);
	_stock.SetUpCount(24);
}

void Game::MakeMove(const Move & mv)
{
	auto from = mv.From();
	auto to = mv.To();
	Pile& toPile = *_allPiles[to];
	if (from == STOCK) {
		_waste.Draw(_stock,mv.Draw());
		toPile.Push(_waste.Pop());
		toPile.IncrUpCount(1);
		_waste.SetUpCount(_waste.Size());
		_stock.SetUpCount(_stock.Size());
	} else {
		auto n = mv.N();
		Pile& fromPile = *_allPiles[from];
		toPile.Push(fromPile.Pop(n));
		// For tableau piles, UpCount counts face-up cards.  
		// For other piles, it counts cards.
		toPile.IncrUpCount(n);
		fromPile.IncrUpCount(-n);
		if (fromPile.UpCount() == 0 && fromPile.Size()!= 0){
			fromPile.SetUpCount(1);    // flip the top card
		}
	}
}

void  Game::UnMakeMove(const Move & mv)
{
	auto from = mv.From();
	auto to = mv.To();
	Pile & toPile = *_allPiles[to];
	if (from == STOCK) {
		_waste.Push(toPile.Pop());
		toPile.IncrUpCount(-1);
		_waste.Draw(_stock,-mv.Draw());
		_waste.SetUpCount(_waste.Size());
		_stock.SetUpCount(_stock.Size());
	} else {
		auto n = mv.N();
		Pile & fromPile = *_allPiles[from];
		fromPile.Push(toPile.Pop(n));
		if (fromPile.IsTableau()) {
			fromPile.SetUpCount(mv.FromUpCount());
		} else {
			fromPile.IncrUpCount(n);
		}
		toPile.IncrUpCount(-n);
	}
}

// Return the height of the shortest foundation pile
static unsigned ShortFndLen(const Game& gm){
	const auto& fnd = gm.Foundation();
	int shortLen = fnd[0].Size();
	for (int ifnd = 1; ifnd < 4; ++ifnd) {
		unsigned sz = fnd[ifnd].Size();
		if (sz < shortLen) { 
			shortLen = sz;
		}
	}
	return shortLen;
}

// Look for a move to the shortest foundaton pile.  For the same reason 
// that putting an ace on its foundation pile is always right, these are, too.
// Returns a Moves vector that may be empty or contain one such move.
static Moves ShortFoundationMove(const Game & gm, unsigned shortLen)
{
	Moves result;
	const auto & fnd = gm.Foundation();
	const auto & allPiles = gm.AllPiles();
	for (int iPile = WASTE; iPile<TABLEAU+7 && result.size() == 0; ++iPile) {
		const Pile &pile = *allPiles[iPile] ;
		if (pile.Size()) {
			const Card& card = pile.Back();
			unsigned suit = card.Suit();
			if (card.Rank() == shortLen) {
				if (fnd[suit].Size() == shortLen) {
					unsigned up = (iPile == WASTE) ? 0 : pile.UpCount();
					result.push_back(Move(iPile,FOUNDATION+suit,1,up));
				}
			}
		}
	}
	return result;
}

// Return a vector of all the cards that can be played
// from the talon (the stock and waste piles), along
// with the number of moves requied to reach each one
// and the number of cards that must be drawn (or undrawn)
// to reach each one.
std::vector<Game::TalonFuture> Game::TalonMoves() const
{
	unsigned talonSize = _waste.Size() + _stock.Size();
	std::vector<Game::TalonFuture> result;
	result.reserve(talonSize);

	if (talonSize == 0) return result;

	unsigned nMoves = 0;
	unsigned nRecycles = 0;
	unsigned originalWasteSize = _waste.Size();
	Pile waste = _waste;
	Pile stock = _stock;

	do {
		if (waste.Size()) {
			result.push_back(TalonFuture(waste.Back(), nMoves, 
				waste.Size()-originalWasteSize));
		}	
		if (stock.Size()) {
			// Draw from the stock pile
			nMoves += 1;
			waste.Draw(stock, std::min<unsigned>(_draw,stock.Size()));
		} else {
			// Recycle the waste pile
			nMoves += 1;
			nRecycles += 1;
			stock.Draw(waste, waste.Size());
		}
	} while (waste.Size() != originalWasteSize && nRecycles < 2);
	return result;
}

/*
If any short-foundation moves exist, returns one of those.
Otherwise, returns a list of moves that are legal and not
known to be sub-optimal.  Rather than generate individual draws from
stock to waste, it generates Move objects that represent one or more
draws and that expose a playable top waste card.
*/
Moves Game::AvailableMoves() const 
{
	Moves result;
	if (GameOver()) return result;

	unsigned shortLen = ShortFndLen(*this);
	result = ShortFoundationMove(*this,shortLen);
	if (result.size()) return result;

	// look for moves from tableau to foundation
	for (const Pile & pile: _tableau) {
		if (pile.Size() > 0) {
			const Card& card = pile.Back();
			const Pile& foundation = _foundation[card.Suit()];
			if (foundation.Size() == card.Rank()) {
				unsigned ct = pile.UpCount();
				result.push_back(Move(pile.Code(),foundation.Code(),1,ct));
			}
		}
	}
	// Look for moves between tableau piles.  These may involve
	// multiple cards
	for (const Pile& fromPile: _tableau) {
		// skip empty from piles
		if (fromPile.Size() == 0) continue;
		auto up = fromPile.UpCount();

		bool kingMoved = false;     // prevents moving the same king twice
		for (const Pile& toPile: _tableau) {
			if (&fromPile == &toPile) continue;

			if (toPile.Size() == 0) { 
				if (!kingMoved && fromPile.Size() > up &&
					fromPile.Top().Rank() == KING) {
					// toPile is empty, a king sits atop the from pile's up
					// cards, and it is covering at least one down card.
					result.push_back(Move(fromPile.Code(),toPile.Code(),up,up));
					kingMoved = true;
				}
			} else {
				// Other moves follow the opposite-color-and-next-lower-rank rule.
				// We move from one tableau pile to another only to 
				// (a) move all the up cards on the from pile, or
				// (b) expose a card on the from pile that can be moved to a foundation
				// 	   pile.
				Card cardToCover = toPile.Back();
				if (cardToCover.Rank() <= TWO) { // never cover a deuce
					continue;
				}
				const auto& fromCds = fromPile.Cards();
				// See whether any of the from pile's up cards can be moved
				// to the to pile.
				Card fromBase = fromPile.Top();
				Card fromTip = fromPile.Back();
				unsigned toRank = cardToCover.Rank();
				if (!(fromTip.Rank() < toRank && toRank <= fromBase.Rank()+1
						&& (fromTip.OddRed() == cardToCover.OddRed())))
					continue;
				// Some face-up card in the from pile covers the top card
				// in the to pile, so a move is possible.
				int moveCount = cardToCover.Rank() -  fromTip.Rank();
				assert(0 < moveCount && moveCount <= up);
				if (moveCount == up){
					// move all the up cards on the from pile.
					assert(fromPile.Top().Covers(cardToCover));
					result.push_back(Move(fromPile.Code(),toPile.Code(),up,up));
					continue;
				}
				Card exposed = *(fromCds.end()-moveCount-1);
				if (exposed.Rank() == _foundation[exposed.Suit()].Size()){
					// This move will expose a card that can be moved to its foundation pile.
					assert((fromCds.end()-moveCount)->Covers(cardToCover));
					result.push_back(Move(fromPile.Code(),toPile.Code(),moveCount,up));
				}
			}
		}
	}
	// Look for move from waste to tableau or foundation, including moves that become available 
	// after one or more draws.  
	std::vector<TalonFuture> talon(TalonMoves());
	for (const TalonFuture & mv : talon){
		if (mv._card.Rank() == _foundation[mv._card.Suit()].Size()){
			unsigned pileNo = FOUNDATION+mv._card.Suit();
			result.push_back(Move(pileNo,mv._nMoves+1,mv._draw));
			if (_foundation[mv._card.Suit()].Size() == shortLen)
				break;
		}
		bool kingToMove = mv._card.Rank() == KING;
		for (const Pile& tab : _tableau) {
			if ((tab.Size() > 0)) {
				if (mv._card.Covers(tab.Back())) {
					result.push_back(Move(tab.Code(),mv._nMoves+1,mv._draw));
				}
			} else if (kingToMove) {
				result.push_back(Move(tab.Code(),mv._nMoves+1,mv._draw));
				break;  // move that king to just one empty pile
			}
		}
	}

	// Look for moves from foundation piles to tableau piles,
	// even though they rarely appear in optimal solutions.
	for (const Pile& f: _foundation) {
		if (f.Size() > std::max<unsigned>(2,shortLen)) {  
			const Card& top = f.Back();
			for (const Pile& t: _tableau) {
				if (t.Size() > 0) {
					if (top.Covers(t.Back())) {
						result.push_back(Move(f.Code(),t.Code(),1,0));
					}
				} else {
					if (top.Rank() == KING) {
						result.push_back(Move(f.Code(),t.Code(),1,0));
						break;  // don't move same king to another tableau pile
					}
				}
			}
		}
	}
	return result;
}

// Return the number of cards on the foundaton
unsigned Game::FoundationCardCount() const
{
	unsigned result = 0;
	for (const Pile& f : _foundation) {
		result += f.Size();
	}
	return result;
}
