/*
	Implements Game.hpp.
*/

#include "Game.hpp"
#include <cassert>
#include <sstream> 		// for stringstream

const std::string suits("cdsh");
const std::string ranks("a23456789tjqka");


static unsigned QuotientRoundedUp(unsigned numerator, unsigned denominator)
{
	return (numerator+denominator-1)/denominator;
}

// Returns a string composed only of the characters in input that also appear in filter
static std::string Filtered(std::string input, std::string filter)
{
	std::string result;
	for (auto ic = input.begin(); ic<input.end(); ic+=1)	{
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
	for (auto ich = in.begin(); ich < in.end(); ich+=1)	{
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
	for (unsigned i = 0; i < n; i+=1)
	{
		result.push_back(_cards.back());
		_cards.pop_back();
	}
	return result;
}

void Pile::Draw(Pile& other, int n)
{
	/*Pile & to = (n>0) ? *this : other;
	Pile & fm = (n>0) ? other : *this;
	unsigned nm = (n>0) ? n : -n;
	assert(nm <= fm.Size());
	for (unsigned i = 0; i < nm; i+=1)
		to.Draw(fm);
	*/
	if (n < 0) {
		assert(-n <= Size());
		for (unsigned i = 0; i < -n; ++i)
			other.Draw(*this);
	} else {
		assert(n <= other.Size());
		for (unsigned i = 0; i < n; ++i)
			Draw(other);
	}
}
static void SetAllPiles(Game& game)
{
	auto & allPiles = game.AllPiles();
	allPiles[WASTE] = &game.Waste();
	allPiles[STOCK] = &game.Stock();
	for (int ip = 0; ip < 4; ip+=1) allPiles[FOUNDATION+ip] = &game.Foundation()[ip];
	for (int ip = 0; ip < 7; ip+=1) allPiles[TABLEAU+ip] = &game.Tableau()[ip];
}

Game::Game(const std::vector<Card> &deck,unsigned draw,unsigned talonLookAheadLimit)
	: _deck(deck)
	, _waste(WASTE)
	, _stock(STOCK)
	, _drawSetting(draw)
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
	, _drawSetting(orig._drawSetting)
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
	for (unsigned i = 0; i<7; i+=1) {
		for (unsigned icd = i; icd < 7; icd+=1)	{
			_tableau[icd].Push(_deck[ideck]);
			ideck += 1;
		}
		_tableau[i].IncrUpCount(1);      // turn up the top card
	}
	for (unsigned ic = 51; ic >= 28; ic-=1)
		_stock.Push(_deck[ic]);
}

void Game::MakeMove(Move mv)
{
	auto from = mv.From();
	auto to = mv.To();
	Pile& toPile = *_allPiles[to];
	if (from == STOCK) {
		_waste.Draw(_stock,mv.DrawCount());
		toPile.Push(_waste.Pop());
		toPile.IncrUpCount(1);
	} else {
		auto n = mv.NCards();
		Pile& fromPile = *_allPiles[from];
		toPile.Push(fromPile.Pop(n));
		// For tableau piles, UpCount counts face-up cards.  
		// For other piles, it is undefined.
		toPile.IncrUpCount(n);
		fromPile.IncrUpCount(-n);
		if (fromPile.UpCount() == 0 && fromPile.Size()!= 0){
			fromPile.SetUpCount(1);    // flip the top card
		}
	}
}

void  Game::UnMakeMove(Move mv)
{
	auto from = mv.From();
	auto to = mv.To();
	Pile & toPile = *_allPiles[to];
	if (from == STOCK) {
		_waste.Push(toPile.Pop());
		toPile.IncrUpCount(-1);
		_waste.Draw(_stock,-mv.DrawCount());
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
	unsigned minFoundationSize = fnd[0].Size();
	for (int ifnd = 1; ifnd < 4; ifnd+=1) {
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
static QMoves ShortFoundationMove(const Game & gm, unsigned minFoundationSize)
{
	QMoves result;
	const auto & fnd = gm.Foundation();
	const auto & allPiles = gm.AllPiles();
	for (int iPile = WASTE; iPile<TABLEAU+7 && result.size() == 0; iPile+=1) {
		const Pile &pile = *allPiles[iPile] ;
		if (pile.Size()) {
			const Card& card = pile.Back();
			unsigned suit = card.Suit();
			if (card.Rank() <= minFoundationSize+1) {
				if (fnd[suit].Size() == card.Rank()) {
					unsigned up = (iPile == WASTE) ? 0 : pile.UpCount();
					result.push_back(Move(iPile,FOUNDATION+suit,1,up));
				}
			}
		}
	}
	return result;
}

struct TalonFuture {
	Card _card;
	unsigned short _nMoves;
	signed short _drawCount;

	TalonFuture(const Card& card, unsigned nMoves, int draw)
		: _card(card)
		, _nMoves(nMoves)
		, _drawCount(draw)
		{}
};

// Return a vector of all the cards that can be played
// from the talon (the stock and waste piles), along
// with the number of moves required to reach each one
// and the number of cards that must be drawn (or undrawn)
// to reach each one.
//
// Although it may be tempting to try to optimize this function,
// it takes only about 3% of the run time.
static std::vector<TalonFuture> TalonCards(const Game & game)
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
	unsigned draw = game.DrawSetting();

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
			nRecycles += 1;
			stock.Draw(waste, waste.Size());
		}
	} while (waste.Size() != originalWasteSize && nRecycles < 2);
	return result;
}

// If any short-foundation moves exist, returns one of those.
// Otherwise, returns a list of moves that are legal and not
// known to be wasted.  Rather than generate individual draws from
// stock to waste, it generates Move objects that represent one or more
// draws and that expose a playable top waste card and then play that card.
QMoves Game::AvailableMoves() const 
{
	QMoves result;
	if (GameOver()) return result;

	unsigned minFoundationSize = ShortFndLen(*this);
	result = ShortFoundationMove(*this,minFoundationSize);
	if (result.size()) return result;

	// Look for moves from tableau piles.
	for (const Pile& fromPile: _tableau) {
		// skip empty from piles
		if (fromPile.Size() == 0) continue;

		Card fromTip = fromPile.Back();
		auto upCount = fromPile.UpCount();

		// look for moves from tableau to foundation
		const Pile& foundation = _foundation[fromTip.Suit()];
		if (foundation.Size() == fromTip.Rank()) {
			result.push_back(Move(fromPile.Code(),foundation.Code(),1,upCount));
		}

		// Look for moves between tableau piles.  These may involve
		// moving multiple cards.
		bool kingMoved = false;     // prevents moving the same king twice
		for (const Pile& toPile: _tableau) {
			if (&fromPile == &toPile) continue;

			if (toPile.Size() == 0) { 
				if (!kingMoved 
						&& fromPile.Top().Rank() == KING 
						&& fromPile.Size() > upCount) {
					// toPile is empty, a king sits atop fromPile's face-up
					// cards, and it is covering at least one face-down card.
					result.push_back(Move(fromPile.Code(),toPile.Code(),upCount,upCount));
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
				unsigned toRank = cardToCover.Rank();
				if (fromTip.Rank() < toRank && toRank <= fromBase.Rank()+1
						&& (fromTip.OddRed() == cardToCover.OddRed())){
					// Some face-up card in the from pile covers the top card
					// in the to pile, so a move is possible.
					int moveCount = cardToCover.Rank() -  fromTip.Rank();
					assert(0 < moveCount && moveCount <= upCount);
					if (moveCount == upCount){
						// This move will expose a face-down card or
						// clear a column.
						// Move all the face-up cards on the from pile.
						assert(fromPile.Top().Covers(cardToCover));
						result.push_back(Move(fromPile.Code(),toPile.Code(),upCount,upCount));
					} else {
						Card exposed = *(fromCds.end()-moveCount-1);
						if (exposed.Rank() == _foundation[exposed.Suit()].Size()){
							// This move will expose a card that can be moved to 
							// its foundation pile.
							assert((fromCds.end()-moveCount)->Covers(cardToCover));
							result.push_back(Move(fromPile.Code(),toPile.Code(),moveCount,upCount));
						}
					}
				}
			}
		}
	}
	// Look for move from waste to tableau or foundation, including moves that become available 
	// after one or more draws.  
	std::vector<TalonFuture> talon(TalonCards(*this));
	for (const TalonFuture & talonCard : talon){

		// Stop generating talon moves if they require too many moves
		// and there are alternative moves.  The ungenerated moves will get
		// their chances later if we get that far before we find a minimum,
		// although that may require an extra move or more.
		if (result.size() > 1 && talonCard._nMoves > _talonLookAheadLimit) break;

		unsigned cardSuit = talonCard._card.Suit();
		unsigned cardRank = talonCard._card.Rank();
		if (cardRank == _foundation[cardSuit].Size()){
			unsigned pileNo = FOUNDATION+cardSuit;
			result.push_back(Move(pileNo,talonCard._nMoves+1,talonCard._drawCount));
			if (cardRank <= minFoundationSize+1){
				if (this->_drawSetting == 1) {
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
				if (talonCard._card.Covers(tPile.Back())) {
					result.push_back(Move(tPile.Code(),talonCard._nMoves+1,talonCard._drawCount));
				}
			} else if (cardRank == KING) {
				result.push_back(Move(tPile.Code(),talonCard._nMoves+1,talonCard._drawCount));
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
						result.push_back(Move(fPile.Code(),tPile.Code(),1,0));
					}
				} else {
					if (top.Rank() == KING) {
						result.push_back(Move(fPile.Code(),tPile.Code(),1,0));
						break;  // don't move same king to another tableau pile
					}
				}
			}
		}
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
	for (auto i = begin; i != end; i+=1){
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
	unsigned draw = DrawSetting();
	const CardVec& waste = _waste.Cards();
	const CardVec& stock = _stock.Cards();
	unsigned talonCount = waste.size() + stock.size();

	unsigned result = talonCount + QuotientRoundedUp(stock.size(),draw);

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
// Enumerate the moves in a vector of Moves.
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
			mvnum += 1;
			result.emplace_back(mvnum,from, to, n,flip);
			if (mv.From() == WASTE){
				assert (wasteSize >= 1);
				wasteSize -= 1;
			}
		} else {
			assert(stockSize+wasteSize > 0);
			unsigned nTalonMoves = mv.NMoves()-1;
			unsigned stockMovesLeft = QuotientRoundedUp(stockSize,draw);
			if (nTalonMoves > stockMovesLeft && stockSize) {
				// Draw all remaining cards from stock
				mvnum += 1;
				result.emplace_back(mvnum,STOCK,WASTE,stockSize,false);
				mvnum += stockMovesLeft-1;
				wasteSize += stockSize;
				stockSize = 0;
				nTalonMoves -= stockMovesLeft;
			}
			if (nTalonMoves > 0) {
				mvnum += 1;
				if (stockSize == 0) {
					// Recycle the waste pile
					result.emplace_back(mvnum,WASTE,STOCK,wasteSize,false);
					stockSize = wasteSize;
					wasteSize = 0;
				}
				unsigned nMoved = std::min<unsigned>(stockSize,nTalonMoves*draw);
				result.emplace_back(mvnum,STOCK,WASTE,nMoved,false);
				assert (stockSize >= nMoved && wasteSize+nMoved <= 24);
				assert(stockSize >= nMoved);
				stockSize -= nMoved;
				wasteSize += nMoved;
				assert(wasteSize <= 24);
				mvnum += nTalonMoves-1;
			}
			mvnum += 1;
			result.emplace_back(mvnum,WASTE,to,1,false);
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
	for (auto ip = pile.Cards().begin(); ip < pile.Cards().end(); ip+=1)
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
		outStr << "+" << mv.NMoves() << "d" << mv.DrawCount() << ">" << PileNames[mv.To()];
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
		for (unsigned imv = 1; imv < mvs.size(); imv+=1)
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

static void InsertionSort(std::array<uint64_t,7>& array) {
	int j;
	for(int i = 1; i<7; i++) {
		auto key = array[i];//take value
		j = i;
		while(j > 0 && array[j-1]<key) {	// des
			array[j] = array[j-1];
			j--;
		}
		array[j] = key;   //insert in right place
	}
}


GameStateType::GameStateType(const Game& game)
{
	std::array<uint64_t,7> tableauState;
	const auto& tableau = game.Tableau();
	for (unsigned i = 0; i<7; i+=1) {
		const auto& pile = tableau[i];
		unsigned upCount = pile.UpCount();
		if (upCount == 0) {
			tableauState[i] = 0;
		} else {
			const auto& cards = pile.Cards();
			unsigned isMajor = 0;
			for (auto j = cards.end()-upCount+1;j < cards.end(); j+=1){
				isMajor = isMajor<<1 | j->IsMajor();
			}
			Card top = pile.Top();
			tableauState[i] = ((isMajor<<4 | upCount)<<4 | top.Rank())<<2 | top.Suit();
		}
	}
	InsertionSort(tableauState);

	_part[0] = ((tableauState[0]<<21| tableauState[1])<<21) | tableauState[2];
	_part[1] = ((tableauState[3]<<21| tableauState[4])<<21) | tableauState[5];
	_part[2] = (tableauState[6]<<5) | game.Stock().Size();
	const auto& fnd = game.Foundation();
	for (auto& pile: fnd){
		_part[2] =_part[2]<<4 | pile.Size();
	}
}
