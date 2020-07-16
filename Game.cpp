/*
	Implements Game.hpp.
*/

#include "Game.hpp"
#include <cassert>
#include <sstream> 		// for stringstream

const std::string suits("cdsh");
const std::string ranks("a23456789tjqka");


static unsigned RoundUpQuotient(unsigned numerator, unsigned denominator)
{
	return (numerator+denominator-1)/denominator;
}

// Returns a string composed only of the characters in input that also appear in filter
static std::string Filtered(std::string input, std::string filter)
{
	std::string result;
	for (auto ic = input.begin(); ic<input.end(); ++ic)	{
		if (filter.find(*ic) != std::string::npos) {
			result += *ic;
		}
	}
	return result;
}

static std::string LowerCase(const std::string & in)
{
	std::string result;
	result.reserve(in.length());
	for (auto ich = in.begin(); ich < in.end(); ++ich)	{
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
std::pair<bool,Card> Card::FromString(const std::string& s0)    
{
	std::string s1 = Filtered(LowerCase(s0),suits+ranks+"10");
	Suit_t suit;
	Rank_t rank;
	bool ok = s1.length() == 2 || s1.length() == 3;
	std::string rankStr;
	if (ok)	{
		auto suitIndex = suits.find(s1[0]);
		if (suitIndex != std::string::npos)	{
			// input has suit first
			suit = static_cast<Suit_t>(suitIndex);
			rankStr = s1.substr(1);
		} else {
			// suit does not appear first in input
			{
				// assume it is last
				suitIndex = suits.find(s1.back());
				ok = suitIndex != std::string::npos;
				if (ok) {
					suit = static_cast<Suit_t>(suitIndex);
					rankStr = s1.substr(0,s1.length()-1);
				}
			}
		}
	}
	if (ok)	{
		if (rankStr == "10") {rankStr = "t";}
		auto rankIndex = ranks.find(rankStr);
		ok = rankIndex != std::string::npos;
		if (ok)	{
			rank = static_cast<Rank_t>(rankIndex);
		}
	}
	Card card;
	if (ok)
		card = Card(suit,rank);
	return std::pair<bool,Card>(ok,card);
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
	CardVec result;
	for (unsigned i = 0; i < n; ++i)
	{
		result.push_back(_cards.back());
		_cards.pop_back();
	}
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
static void SetAllPiles(Game& game)
{
	auto & allPiles = game.AllPiles();
	allPiles[WASTE] = &game.Waste();
	allPiles[STOCK] = &game.Stock();
	for (int ip = 0; ip < 4; ++ip) allPiles[FOUNDATION+ip] = &game.Foundation()[ip];
	for (int ip = 0; ip < 7; ++ip) allPiles[TABLEAU+ip] = &game.Tableau()[ip];
}

Game::Game(const std::vector<Card> &deck,unsigned draw,unsigned talonLookAheadLimit)
	: _deck(deck)
	, _waste(WASTE)
	, _stock(STOCK)
	, _draw(draw)
	, _talonLookAheadLimit(talonLookAheadLimit)
	, _foundation{FOUNDATION1C,FOUNDATION2D,FOUNDATION3S,FOUNDATION4H}
	, _tableau{TABLEAU1,TABLEAU2,TABLEAU3,TABLEAU4,TABLEAU5,TABLEAU6,TABLEAU7}
{
	SetAllPiles(*this);
	Deal();
}
Game::Game(const Game& orig)
	:_deck(orig._deck)
	, _waste(orig._waste)
	, _stock(orig._stock)
	, _draw(orig._draw)
	, _foundation(orig._foundation)
	, _tableau(orig._tableau)
	{
		SetAllPiles(*this);
	}

// Deal the cards for Klondike Solitaire.
void Game::Deal()
{
	for (auto pile: _allPiles)	{
		pile->ClearCards();
	}
	unsigned ideck = 0;
	for (unsigned i = 0; i<7; ++i) {
		for (unsigned icd = i; icd < 7; ++icd)	{
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
		auto n = mv.NCards();
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
		auto n = mv.NCards();
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

void Game::MakeMove(const XMove & xmv)
{
	auto from = xmv.From();
	auto to = xmv.To();
	unsigned n = xmv.NCards();
	Pile& toPile = *_allPiles[to];
	Pile& fromPile = *_allPiles[from];
	if (from == STOCK || to == STOCK)
		toPile.Push(fromPile.Draw(n));
	else
		toPile.Push(fromPile.Pop(n));
	toPile.IncrUpCount(n);
	fromPile.IncrUpCount(-n);
	if (xmv.Flip()){
		fromPile.SetUpCount(1);    // flip the top card
	}
}

// Return the height of the shortest foundation pile
static unsigned ShortFndLen(const Game& gm){
	const auto& fnd = gm.Foundation();
	int minFoundationSize = fnd[0].Size();
	for (int ifnd = 1; ifnd < 4; ++ifnd) {
		unsigned sz = fnd[ifnd].Size();
		if (sz < minFoundationSize) { 
			minFoundationSize = sz;
		}
	}
	return minFoundationSize;
}

// Look for a move to the shortest foundation pile or one one card higher.  
// For the same reason that putting an ace or deuce on its foundation pile 
// is always right, these are, too. Returns a Moves vector that may be empty 
// or contain one such move.
static Moves ShortFoundationMove(const Game & gm, unsigned minFoundationSize)
{
	Moves result;
	result.reserve(20);  // a favor for AvailableMoves
	const auto & fnd = gm.Foundation();
	const auto & allPiles = gm.AllPiles();
	for (int iPile = WASTE; iPile<TABLEAU+7 && result.size() == 0; ++iPile) {
		const Pile &pile = *allPiles[iPile] ;
		if (pile.Size()) {
			const Card& card = pile.Back();
			unsigned suit = card.Suit();
			if (card.Rank() <= minFoundationSize+1) {
				if (fnd[suit].Size() == card.Rank()) {
					unsigned up = (iPile == WASTE) ? 0 : pile.UpCount();
					result.emplace_back(iPile,FOUNDATION+suit,1,up);
				}
			}
		}
	}
	return result;
}

struct TalonFuture {
	Card _card;
	unsigned char _nMoves;
	signed char _draw;

	TalonFuture(const Card& card, unsigned nMoves, int draw)
		: _card(card)
		, _nMoves(nMoves)
		, _draw(draw)
		{}
};

// Return a vector of all the cards that can be played
// from the talon (the stock and waste piles), along
// with the number of moves required to reach each one
// and the number of cards that must be drawn (or undrawn)
// to reach each one.
static std::vector<TalonFuture> TalonMoves(const Game & game)
{
	std::vector<TalonFuture> result;
	unsigned talonSize = game.Waste().Size() + game.Stock().Size();
	if (talonSize == 0) return result;

	result.reserve(talonSize);
	Pile waste = game.Waste();
	Pile stock = game.Stock();
	unsigned nMoves = 0;
	unsigned nRecycles = 0;
	unsigned originalWasteSize = waste.Size();
	unsigned draw = game.Draw();

	do {
		if (waste.Size()) {
			result.emplace_back(waste.Back(), nMoves, 
				waste.Size()-originalWasteSize);
		}	
		if (stock.Size()) {
			// Draw from the stock pile
			nMoves += 1;
			waste.Draw(stock, std::min<unsigned>(draw,stock.Size()));
		} else {
			// Recycle the waste pile
			nMoves += 1;
			nRecycles += 1;
			stock.Draw(waste, waste.Size());
		}
	} while (waste.Size() != originalWasteSize && nRecycles < 2);
	return result;
}

// See if we need any more empty tableau columns.  We don't if
// the number of empty columns plus the number with a king at
// the top and no face-down cards is at least four.
static bool NeedEmptyColumn(const std::array<Pile,7>& tableau)
{
	unsigned nEmpty = 0;
	for (const Pile& pile : tableau){
		unsigned size = pile.Size();
		nEmpty +=  (size == 0) ||
			(size == pile.UpCount() && pile[0].Rank() == KING);
		if (nEmpty == 4) return false;
	}
	return true;
}

// If any short-foundation moves exist, returns one of those.
// Otherwise, returns a list of moves that are legal and not
// known to be wasted.  Rather than generate individual draws from
// stock to waste, it generates Move objects that represent one or more
// draws and that expose a playable top waste card and then play that card.
Moves Game::AvailableMoves() const 
{
	Moves result;
	if (GameOver()) return result;

	unsigned minFoundationSize = ShortFndLen(*this);
	result = ShortFoundationMove(*this,minFoundationSize);
	if (result.size()) return result;

	// look for moves from tableau to foundation
	for (const Pile & pile: _tableau) {
		if (pile.Size() > 0) {
			const Card& card = pile.Back();
			const Pile& foundation = _foundation[card.Suit()];
			if (foundation.Size() == card.Rank()) {
				unsigned ct = pile.UpCount();
				result.emplace_back(pile.Code(),foundation.Code(),1,ct);
			}
		}
	}

	// Look for moves between tableau piles.  These may involve
	// multiple cards.
	for (const Pile& fromPile: _tableau) {
		// skip empty from piles
		if (fromPile.Size() == 0) continue;
		auto up = fromPile.UpCount();

		bool kingMoved = false;     // prevents moving the same king twice
		for (const Pile& toPile: _tableau) {
			if (&fromPile == &toPile) continue;

			if (toPile.Size() == 0) { 
				if (!kingMoved 
						&& fromPile.Top().Rank() == KING 
						&& fromPile.Size() > up) {
					// toPile is empty, a king sits atop fromPile's face-up
					// cards, and it is covering at least one face-down card.
					result.emplace_back(fromPile.Code(),toPile.Code(),up,up);
					kingMoved = true;
				}
			} else {
				// Other moves follow the opposite-color-and-next-lower-rank rule.
				// We move from one tableau pile to another only to 
				// (a) move all the face-up cards on the from pile to 
				//		(1) expose a face-down card, or
				//		(2) make an empty column if we still need any, or
				// (b) expose a card on the from pile that can be moved to a foundation
				// 	   pile.
				Card cardToCover = toPile.Back();
				const auto& fromCds = fromPile.Cards();
				// See whether any of the from pile's up cards can be moved
				// to the to pile.
				Card fromBase = fromPile.Top();
				Card fromTip = fromPile.Back();
				unsigned toRank = cardToCover.Rank();
				if (fromTip.Rank() < toRank && toRank <= fromBase.Rank()+1
						&& (fromTip.OddRed() == cardToCover.OddRed())){
					// Some face-up card in the from pile covers the top card
					// in the to pile, so a move is possible.
					int moveCount = cardToCover.Rank() -  fromTip.Rank();
					assert(0 < moveCount && moveCount <= up);
					if (moveCount == up){
						if (up < fromPile.Size() || 
							(fromBase.Rank()!=KING && NeedEmptyColumn(_tableau))){
							// This move will expose a face-down card or
							// clear a column to which a king may be moved.
							// Move all the face-up cards on the from pile.
							assert(fromPile.Top().Covers(cardToCover));
							result.emplace_back(fromPile.Code(),toPile.Code(),up,up);
						}
					} else {
						Card exposed = *(fromCds.end()-moveCount-1);
						if (exposed.Rank() == _foundation[exposed.Suit()].Size()){
							// This move will expose a card that can be moved to 
							// its foundation pile.
							assert((fromCds.end()-moveCount)->Covers(cardToCover));
							result.emplace_back(fromPile.Code(),toPile.Code(),moveCount,up);
						}
					}
				}
			}
		}
	}
	// Look for move from waste to tableau or foundation, including moves that become available 
	// after one or more draws.  
	std::vector<TalonFuture> talon(TalonMoves(*this));
	for (const TalonFuture & mv : talon){

		// Stop generating talon moves if they require too many moves
		// and there are alternative moves.  The ungenerated moves will get
		// their chances later if we get that far before we find a minimum,
		// although that may require an extra move or more.
		if (result.size() > 1 && mv._nMoves > _talonLookAheadLimit) break;

		unsigned cardSuit = mv._card.Suit();
		unsigned cardRank = mv._card.Rank();
		if (cardRank == _foundation[cardSuit].Size()){
			unsigned pileNo = FOUNDATION+cardSuit;
			result.emplace_back(pileNo,mv._nMoves+1,mv._draw);
			if (cardRank <= minFoundationSize+1){
				if (this->_draw == 1) {
					// This is a short-foundation move that ShortFoundationMove()
					// can't find.
					if (result.size() == 1) return result;
					break;		// This is best next move from among the remaining talon cards
				}
				else
					continue;  	// This is best move for this card.  A card further on might be a better move.
			}
		}
			
		for (const Pile& tPile : _tableau) {
			if ((tPile.Size() > 0)) {
				if (mv._card.Covers(tPile.Back())) {
					result.emplace_back(tPile.Code(),mv._nMoves+1,mv._draw);
				}
			} else if (cardRank == KING) {
				result.emplace_back(tPile.Code(),mv._nMoves+1,mv._draw);
				break;  // move that king to just one empty pile
			}
		}
	}

	// Look for moves from foundation piles to tableau piles,
	// even though they rarely appear in optimal solutions.
	for (const Pile& fPile: _foundation) {
		if (fPile.Size() > minFoundationSize+1) {  
			const Card& top = fPile.Back();
			for (const Pile& tPile: _tableau) {
				if (tPile.Size() > 0) {
					if (top.Covers(tPile.Back())) {
						result.emplace_back(fPile.Code(),tPile.Code(),1,0);
					}
				} else {
					if (top.Rank() == KING) {
						result.emplace_back(fPile.Code(),tPile.Code(),1,0);
						break;  // don't move same king to another tableau pile
					}
				}
			}
		}
	}
	return result;
}

unsigned MoveCount(const Moves& moves)
{
	unsigned result = 0;
	for (const auto & mv: moves){
		result += mv.NMoves();
	}
	return result;
}

// Counts the number of times a card is higher in the stack
// than a lower card of the same suit.  Remember that the 
// stack tops are at the back.
static unsigned MisorderCount(const Card *begin, const Card *end)
{
	unsigned char mins[4] {14,14,14,14};
	unsigned result = 0;
	for (auto i = begin; i != end; ++i){
		const Card& cd = *i;
		auto rank = cd.Rank();
		auto suit = cd.Suit();
		if (rank < mins[suit])
			mins[suit] = rank;
		else
			result += 1;
	}
	return result;
}

// Return a lower bound on the number of moves required to complete
// this game.  This function must return a result that does not 
// decrease by more than one after any single Move.  The sum of 
// this result plus the number of moves made (from MoveCount())
// must never decrease when a new move is made (monotonicity).
// If it does, we won't know when to stop.
unsigned Game::MinimumMovesLeft() const
{
	unsigned draw = Draw();
	const CardVec& waste = _waste.Cards();
	const CardVec& stock = _stock.Cards();
	unsigned talonCount = waste.size() + stock.size();

	unsigned result = talonCount + RoundUpQuotient(stock.size(),draw);

	if (draw == 1) {
		// This can fail the monotonicity test for draw > 1.
		result += MisorderCount(waste.begin(), waste.end());
	}

	for (const Pile & tpile: _tableau) {
		auto begin = tpile.Cards().begin();
		unsigned downCount = tpile.Size() - tpile.UpCount();
		result += tpile.Size() + MisorderCount(begin, begin+downCount+1);
	}
	return result;
}

// Return the number of cards on the foundaton
unsigned Game::FoundationCardCount() const
{
	unsigned result = 0;
	for (const Pile& fPile : _foundation) {
		result += fPile.Size();
	}
	return result;
}
// Enumerate the moves in a vector of XMoves.
std::vector<XMove> MakeXMoves(const Moves& solution, unsigned draw)
{
	unsigned stockSize = 24;
	unsigned wasteSize = 0;
	unsigned mvnum = 0;
	std::array<unsigned char,7> upCount {1,1,1,1,1,1,1};
	std::array<unsigned char,7> totalCount {1,2,3,4,5,6,7};
	std::vector<XMove> result;

	for (auto mv : solution){
		unsigned from = mv.From();
		unsigned to = mv.To();
		if (from != STOCK){
			unsigned n = mv.NCards();
			bool flip = false;
			if (IsTableau(from)) {
				assert(totalCount[from-TABLEAU] >= n);
				assert(upCount[from-TABLEAU] >= n);
				totalCount[from-TABLEAU] -= n;
				upCount[from-TABLEAU] -= n;
				if (totalCount[from-TABLEAU] && !upCount[from-TABLEAU]){
					flip = true;
					upCount[from-TABLEAU] = 1;
				}
			}
			if (IsTableau(to)) {
				totalCount[to-TABLEAU] += n;
				upCount[to-TABLEAU] += n;
			}
			result.emplace_back(++mvnum,from, to, n,flip);
			if (mv.From() == WASTE){
				assert (wasteSize >= 1);
				wasteSize -= 1;
			}
		} else {
			assert(stockSize+wasteSize > 0);
			unsigned nTalonMoves = mv.NMoves()-1;
			unsigned stockMovesLeft = RoundUpQuotient(stockSize,draw);
			if (nTalonMoves > stockMovesLeft) {
				// Draw all remaining cards from stock
				if (stockSize) {
					result.emplace_back(++mvnum,STOCK,WASTE,stockSize,false);
					mvnum += stockMovesLeft-1;
					wasteSize += stockSize;
					stockSize = 0;
				}
				// Recycle the waste pile
				result.emplace_back(++mvnum,WASTE,STOCK,wasteSize,false);
				stockSize = wasteSize;
				wasteSize = 0;
				nTalonMoves -= stockMovesLeft+1;
			}
			if (nTalonMoves > 0) {
				unsigned nMoved = std::min<unsigned>(stockSize,nTalonMoves*draw);
				result.emplace_back(++mvnum,STOCK,WASTE,nMoved,false);
				assert (stockSize >= nMoved && wasteSize+nMoved <= 24);
				assert(stockSize >= nMoved);
				stockSize -= nMoved;
				wasteSize += nMoved;
				assert(wasteSize <= 24);
				mvnum += nTalonMoves-1;
			}
			result.emplace_back(++mvnum,WASTE,to,1,false);
			assert(wasteSize >= 1);
			wasteSize -= 1;
			if (IsTableau(to)) {
				totalCount[to-TABLEAU] += 1;
				upCount[to-TABLEAU] += 1;
			}
		}
	}
	return result;
}

static std::string PileNames[] 
{
	"st",
	"wa",
	"t1",
	"t2",
	"t3",
	"t4",
	"t5",
	"t6",
	"t7",
	"cb",
	"di",
	"sp",
	"ht"
};

std::string Peek(const Pile& pile) 
{
	std::stringstream outStr;
	outStr << PileNames[pile.Code()] << ":";
	for (auto ip = pile.Cards().begin(); ip < pile.Cards().end(); ++ip)
	{
		char sep(' ');
		if (pile.IsTableau())
		{
			if (ip == pile.Cards().end()-pile.UpCount()) {sep = '|';}
		}
		outStr << sep << ip->AsString();
	}
	return outStr.str();
}

std::string Peek(const Move & mv)
{
	std::stringstream outStr;
	if (mv.From() == STOCK){
		outStr << "+" << mv.NMoves() << "d" << mv.Draw() << ">" << PileNames[mv.To()];
	} else {
		outStr << PileNames[mv.From()] << ">" << PileNames[mv.To()];
		unsigned n = mv.NCards();
		if (n != 1) outStr << "x" << n;
		if (mv.FromUpCount()) outStr << "u" << mv.FromUpCount();
	}
	return outStr.str();
}

std::string Peek(const Moves & mvs)
{
	std::stringstream outStr;
	outStr << "(";
	if (mvs.size()) {
		outStr << Peek(mvs[0]);
		for (unsigned imv = 1; imv < mvs.size(); ++imv)
			outStr << "," <<  Peek(mvs[imv]);
	}
	outStr << ")";
	return outStr.str();
}

std::string Peek (const Game&game)
{
	std::stringstream out;
	const auto & piles = game.AllPiles();
	for (const Pile* pile: piles){
		out << Peek(*pile) << "\n";
	} 
	return out.str();
}
