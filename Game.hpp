#ifndef SOL_GAME_H
#define SOL_GAME_H

/*
	This file defines the interfaces for the Klondike Solitaire game.  It defines such things
	as a Card, a Pile, and a Move.  It defines the Game containing several piles:
	stock       the pile from which a player draws
	waste       the pile on which to lay a drawn card if it is not played elsewhere
	foundation  the four piles, one for each suit, to which one wishes to move all the cards
	tableau     the seven piles originally dealt with one card turned up.

	
	Any solvers defined here will operate on these objects.
*/ 
#include <string>
#include <vector>
#include <algorithm>
#include <array>

enum Rank_t : unsigned char
 {
	ACE = 0,
	TWO,
	THREE,
	FOUR,
	FIVE,
	SIX,
	SEVEN,
	EIGHT,
	NINE,
	TEN,
	JACK,
	QUEEN,
	KING
};
enum Suit_t : char {
	CLUBS = 0,
	DIAMONDS,
	SPADES,
	HEARTS,
};

class Card
{
private:
	// "Major" here means spades or hearts, rather than clubs or diamonds. 
	unsigned char _suit;
	unsigned char _rank;
	unsigned char _value;
	unsigned char _isRed:1;
	unsigned char _isMajor:1;

public:
	static bool FromString(const std::string& s0, Card & card);

	Card(Suit_t suit,unsigned char rank) : 
		_suit(suit),
		_rank(rank),
		_value(rank*4+suit),
		_isRed(suit&1),
		_isMajor(suit>>1)
		{}

	Card(unsigned int value):
		_suit(value%4),
		_rank(value/4),
		_value(value),
		_isRed(_suit&1),
		_isMajor(_suit>>1)
		{}


	unsigned char Suit() const {return _suit;}
	unsigned char Rank() const {return _rank;}
	unsigned Value() const {return _value;}
	bool IsRed() const {return _isRed;}
	bool IsMajor() const{return _isMajor;}
	std::string AsString() const;       // Returns a string like "ha" or "d2"
	bool Covers(const Card & other) const
		{return _isRed != other ._isRed && _rank+1 == other._rank;}
	bool operator==(const Card& other) const {return _suit==other._suit && _rank==other._rank;}
	bool operator!=(const Card& other) const {return ! (other == *this);}
};

// Make from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
// Ignores characters that cannot appear in a valid card string.
// Returns true if it succeeds.
bool FromString(std::string s, Card & card);


// Directions for a move.  Game::AvailableMoves() creates these.
// Game::UnMakeMove() cannot infer the value of the from tableau pile's
// up count before the move (because of flips), so Game::AvailableMoves() 
// includes that in any Move from a tableau pile.
class Move
{
private:
	unsigned char _from;
	unsigned char _to;
	unsigned char _n;
	unsigned char _fromUpCount;

public:
	Move(unsigned char from, unsigned char to, unsigned char n, unsigned char fromUpCount=0)
		: _from(from)
		, _to(to)
		, _n(n)
		, _fromUpCount(fromUpCount)
		{}

	unsigned From() const {return _from;}
	unsigned To()   const {return _to;}
	unsigned N()    const {return _n;}              // an N of 0 means do nothing
	unsigned FromUpCount() const {return _fromUpCount;}
	void SetFrom(unsigned fm)       {_from = fm;}
	void SetTo(unsigned to)         {_to = to;}
	void SetN(unsigned n)           {_n = n;}

};
typedef std::vector<Move> Moves;
typedef std::vector<Card> CardVec;


enum PileCode {
	WASTE = 0,
	TABLEAU = 1,
	TABLEAU1 = 1,
	TABLEAU2,
	TABLEAU3,
	TABLEAU4,
	TABLEAU5,
	TABLEAU6,
	TABLEAU7,
	STOCK = 8,
	FOUNDATION = 9,
	FOUNDATION1C = 9,
	FOUNDATION2D,
	FOUNDATION3S,
	FOUNDATION4H
};

class Pile
{
private:
	CardVec _cards;
	unsigned short _code;
	short _upCount;
	bool _isTableau;
	bool _isFoundation;

public:
	Pile(PileCode code)
	: _code(code)
	, _upCount(0)
	, _isTableau(TABLEAU <= code && code < TABLEAU+7)
	, _isFoundation(FOUNDATION <= code && code < FOUNDATION+4)
	{
		_cards.reserve(24);
	}

	unsigned Code() const 							{return _code;}
	int UpCount() const 							{return _upCount;}
	bool IsTableau() const 							{return _isTableau;}
	bool IsFoundation() const 						{return _isFoundation;}
	unsigned Size() const 							{return _cards.size();}

	void SetUpCount(int up)   						{_upCount = up;}
	void IncrUpCount(int c) 						{ _upCount += c;}
	const Card & operator[](unsigned which) const   {return _cards[which];}
	const CardVec& Cards() const 					{return _cards;}
	Card Pop()               {Card r = _cards.back(); _cards.pop_back(); return r;}
	void Push(const Card & c)              			{_cards.push_back(c);}
	CardVec Pop(unsigned n);
	CardVec Draw(unsigned n);         // like Pop(), but reverses order of cards drawn
	void Push( CardVec::const_iterator begin,  CardVec::const_iterator end)
											{_cards.insert(_cards.end(),begin,end);}
	void Push(const CardVec& cds)           {this->Push(cds.begin(),cds.end());}
	Card Top() const                        {return *(_cards.end()-_upCount);}
	Card Back() const                       {return _cards.back();}
	void ClearCards()                       {_cards.clear(); _upCount = 0;}
	void SwapCards(Pile & other)            {_cards.swap(other._cards);}
	void ReverseCards()                     {std::reverse(_cards.begin(),_cards.end());}
};

class Game
{
	Pile _waste;
	Pile _stock;
	std::array<Pile,7> _tableau;
	std::array<Pile,4> _foundation;
	CardVec _deck;
	unsigned _draw;             // number of cards to draw from stock (usually 1 or 3)
	std::array<Pile *,13> _allPiles;

public:
	Game(CardVec deck,unsigned draw=1);

	Pile& Waste()       							{return _waste;}
	Pile& Stock()       							{return _stock;}
	std::array<Pile,4>& Foundation()   				{return _foundation;}
	std::array<Pile,7>& Tableau()      				{return _tableau;}
	std::array<Pile*,13>& AllPiles()     			{return _allPiles;}
	const Pile & Waste() const       				{return _waste;}
	const Pile & Stock() const       				{return _stock;}
	const std::array<Pile,4>& Foundation() const   	{return _foundation;}
	const std::array<Pile,7>& Tableau() const      	{return _tableau;}
	const std::array<Pile*,13>& AllPiles() const   	{return _allPiles;}
	unsigned Draw() const            				{return _draw;}

	void Deal();
	Moves AvailableMoves() const;
	void  MakeMove(const Move & mv);
	void  UnMakeMove(const Move & mv);
	unsigned FoundationCardCount() const;
	bool  GameOver() const {return _foundation[0].Size() == 13
								&& _foundation[1].Size() == 13 
								&& _foundation[2].Size() == 13
								&& _foundation[3].Size() == 13;}
};
#endif