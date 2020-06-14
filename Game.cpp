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


std::vector<Card> Pile::Pop(unsigned n)
{
	std::vector<Card> result;
	result.insert(result.end(), _cards.end()-n, _cards.end());
	_cards.erase(_cards.end()-n, _cards.end());
	return result;
}

std::vector<Card> Pile::Draw(unsigned n)
{
	std::vector<Card> result = this->Pop(n);
	if (n > 1)
		std::reverse(result.begin(),result.end());
	return result;
}
Game::Game(CardVec deck,unsigned draw)
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
	_stock.Push(_deck.begin()+28,_deck.end());
	_stock.SetUpCount(24);
	_stock.ReverseCards();
}

void Game::MakeMove(const Move & mv)
{
	auto to = mv.To();
	auto from = mv.From();
	auto n = mv.N();
	if (n==0) return;

	Pile& toPile = *_allPiles[to];
	Pile& fromPile = *_allPiles[from];
	if (to == STOCK){
		// Validate a recycling of the waste pile
		assert(from==WASTE);
		assert(toPile.Size() == 0);
		// Recycle the waste pile
		toPile.SwapCards(fromPile);
		toPile.ReverseCards();
	} else if (from == STOCK) {
		toPile.Push(fromPile.Draw(n));
	} else {
		toPile.Push(fromPile.Pop(n));
	}
	// For tableau piles, UpCount counts face-up cards.  
	// For other piles, it counts cards.
	toPile.IncrUpCount(n);
	fromPile.IncrUpCount(-n);
	if (fromPile.UpCount() == 0 && fromPile.Size()!= 0){
		fromPile.SetUpCount(1);    // flip the top card
	}
}

void  Game::UnMakeMove(const Move & mv)
{
	auto to = mv.To();
	auto from = mv.From();
	auto n = mv.N();
	Pile & toPile = *_allPiles[to];
	Pile & fromPile = *_allPiles[from];

	if (to == STOCK) {
		// Reversing turning over the waste pile
		assert(from == WASTE);
		fromPile.SwapCards(toPile);
		fromPile.ReverseCards();
	} else {
		if (from == STOCK) {
			fromPile.Push(toPile.Draw(n));
		} else {
			fromPile.Push(toPile.Pop(n));
		}
		if (fromPile.IsTableau()) {
			fromPile.SetUpCount(mv.FromUpCount());
		} else {
			fromPile.IncrUpCount(n);
		}
		toPile.IncrUpCount(-n);
	}
}

// Look for a move to the shortest foundaton pile.  For the same reason 
// that putting an ace on its foundation pile is always right, these are, too.
// Returns a Moves vector that may be empty or contain one such move.
static Moves ShortFoundationMove(const Game & gm)
{
	int shortLen = 14;          // length of shortest foundation pile
	const auto& fnd = gm.Foundation();
	Moves result;
	for (int ifnd = 0; ifnd < 4; ++ifnd) {
		const Pile & fPile = fnd[ifnd];
		if (fPile.Size() < shortLen) { 
			shortLen = fPile.Size();
		}
	}
	const auto & allPiles = gm.AllPiles();
	for (int iPile = WASTE; iPile<TABLEAU+7 && result.size() == 0; ++iPile) {
		const Pile &pile = *allPiles[iPile] ;
		if (pile.Size()) {
			const Card& card = pile.Back();
			unsigned suit = card.Suit();
			if (card.Rank() == shortLen) {
				if (fnd[suit].Size() == shortLen) {
					result.push_back(Move(iPile,FOUNDATION+suit,1,0));
				}
			}
		}
	}
	return result;
}

/*
If any short-foundation moves exist, returns one of those.
Otherwise, returns all moves that are now legal and not on this 
silly list: 
	moves of aces or deuces off of foundation pile, 
	moves of aces to tableau piles, 
	moves of a king to more than one empty tableau pile, 
	moves of kings from tableau piles with no cards under them to other tableau piles.
*/
Moves Game::AvailableMoves() const
{
	Moves result = ShortFoundationMove(*this);
	if (result.size() == 0 && !GameOver())
	{
		// look for moves from waste or tableau to foundation
		for (int iPile = WASTE; iPile < TABLEAU+7; ++iPile) {
			const Pile& pile = *_allPiles[iPile];
			if (pile.Size() > 0) {
				const Card& card = pile.Back();
				const Pile& foundation = _foundation[card.Suit()];
				if (foundation.Size() == card.Rank()) {
					unsigned ct = iPile == WASTE ? 0 : pile.UpCount();
					result.push_back(Move(iPile,FOUNDATION+card.Suit(),1,ct));
				}
			}
		}
		// Look for moves between tableau piles.  These may involve
		// multiple cards
		for (const Pile& fromPile: _tableau) {
			// skip empty from piles
			if (fromPile.Size() == 0) continue;

			bool kingMoved = false;     // prevents moving the same king twice
			for (const Pile& toPile: _tableau) {
				if (&fromPile == &toPile) continue;

				if (toPile.Size() == 0) { 
					if (!kingMoved && fromPile.Size() > fromPile.UpCount() &&
						fromPile.Top().Rank() == KING) {
						// toPile is empty, a king sits atop the from pile's up
						// cards, and it is covering at least one down card.
						auto up = fromPile.UpCount();
						result.push_back(Move(fromPile.Code(),toPile.Code(),up,up));
					}
				} else {
					// Other moves follow the opposite-color-and-next-lower-rank rule
					auto cardToCover = toPile.Back();
					if (cardToCover.Rank() > TWO) { // don't cover a deuce
						const auto& fromCds = fromPile.Cards();
						for (auto ic = fromCds.end()-fromPile.UpCount(); ic < fromCds.end(); ++ic) {
							if (ic->Covers(cardToCover)) {
								result.push_back(Move(fromPile.Code(),toPile.Code(),
									fromCds.end()-ic, fromPile.UpCount()));
							}
						}
					}
				}
			}
		}
		// Look for move from waste to tableau
		if (_waste.Size() > 0) {
			auto wcard = _waste.Back();
			bool kingToMove = wcard.Rank() == KING;
			for (const Pile& tab : _tableau) {
				if (tab.Size() > 0) {
					if (wcard.Covers(tab.Back())) {
						result.push_back(Move(WASTE,tab.Code(),1,0));
					}
				} else if (kingToMove) {
					result.push_back(Move(WASTE,tab.Code(),1,0));
					break;  // move that king to just one pile
				}
			}
		}
		if (_stock.Size()) {
			// Add drawing from stock
			result.push_back(Move(STOCK,WASTE,std::min(_draw,_stock.Size()),0));
		} else if (_waste.Size()) {
			// Add recycling waste back to stock
			result.push_back(Move(WASTE,STOCK,_waste.Size(),0));
		}
		// Look for moves from foundation piles to tableau piles,
		// even though they rarely appear in optimal solutions.
		for (const Pile& f: _foundation) {
			if (f.Size() > 2) {  // never move an ace or deuce off foundation
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


