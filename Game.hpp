#ifndef SOL_GAME_H
#define SOL_GAME_H

/*
	This file defines the interfaces for the Klondike Solitaire game.  It defines such things
	as a Card, a Pile, and a Move.  It defines the Game containing several piles:
	stock       the pile from which a player draws
	waste       the pile on which to lay a drawn card if it is not played elsewhere
	foundation  the four piles, one for each suit, to which one wishes to move all the cards
	tableau     the seven piles originally dealt with one card turned up.

	The talon is the stock and waste piles considered as a single entity.

	Any solvers implemented here will operate on these objects.
*/ 
#include <string>
#include <vector>
#include <array>
#include <cassert>

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
enum Suit_t : unsigned char {
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
	unsigned char _isMajor;
	unsigned char _parity;

public:
	Card(){}

	Card(unsigned char suit,unsigned char rank) : 
		_suit(suit),
		_rank(rank),
		_isMajor(suit>>1),
		_parity((rank&1)^(suit&1))
		{}

	Card(unsigned int value):
		_suit(value%4),
		_rank(value/4),
		_isMajor(_suit>>1),
		_parity((_rank&1)^(_suit&1))
		{}


	unsigned char Suit() const 			{return _suit;}
	unsigned char Rank() const 			{return _rank;}
	bool IsMajor() const				{return _isMajor;}
	bool OddRed() const 				{return _parity;}  // true for card that fits on stacks where odd cards are red
	unsigned Value() const				{return 4*_rank+_suit;}
	std::string AsString() const;       // Returns a string like "ha" or "d2"
	bool Covers(const Card & other) const // can other be moved onto this card on a tableau pile?
		{return _parity == other._parity && _rank+1 == other._rank;}
	bool operator==(const Card& other) const {return _suit==other._suit && _rank==other._rank;}
	bool operator!=(const Card& other) const {return ! (other == *this);}

	// Make from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
	// Ignores characters that cannot appear in a valid card string.
	// Suit may come before or after rank, and letters may be in
	// upper or lower case, or mixed.
	// Returns true and the specified Card if it succeeds, 
	// false and garbage if it fails,
	static std::pair<bool,Card> FromString(const std::string& s);
};


// CardVec is a very specialized vector.  Its capacity is always 24
// and it does not check for overfilling.
class CardVec {
	std::uint32_t _size;
	Card _cds[24];

public:
	CardVec() : _size(0), _cds(){}
	Card & operator[](unsigned i)					{return _cds[i];}
	const Card& operator[](unsigned i) const		{return _cds[i];}
	Card* begin()									{return _cds;}
	const Card* begin() const						{return _cds;}
	size_t size() const								{return _size;}
	Card* end()										{return _cds+_size;}
	const Card* end() const							{return _cds+_size;}
	Card & back()									{return *(_cds+_size-1);}
	const Card& back() const						{return *(_cds+_size-1);}
	void pop_back()									{_size -= 1;}
	void pop_back(unsigned n)						{_size -= n;}
	void push_back(const Card& cd)					{_cds[_size] = cd; _size += 1;}
	void append(const Card* begin, const Card* end)	
					{for (auto i=begin;i<end;++i){_cds[_size++]=*i;}}
	void clear()									{_size = 0;}
	bool operator==(const CardVec& other) const
					{	
						if (_size != other._size) return false;
						for(unsigned i = 0; i < _size; ++i){
							if ((*this)[i] != other[i]) return false;
						}
						return true;
					}
	CardVec& operator=(const CardVec& other) 
					{
						std::copy(other._cds,other._cds+other._size,_cds);
						_size = other._size;
						return *this;
					}
};	// end class CardVec

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
	unsigned short _upCount;
	bool _isTableau;
	bool _isFoundation;

public:
	Pile(PileCode code)
	: _code(code)
	, _upCount(0)
	, _isTableau(TABLEAU <= code && code < TABLEAU+7)
	, _isFoundation(FOUNDATION <= code && code < FOUNDATION+4)
	{}

	unsigned Code() const 							{return _code;}
	unsigned UpCount() const 						{return _upCount;}
	bool IsTableau() const 							{return _isTableau;}
	bool IsFoundation() const 						{return _isFoundation;}
	unsigned Size() const 							{return _cards.size();}

	void SetUpCount(unsigned up)					{_upCount = up;}
	void IncrUpCount(int c) 						{_upCount += c;}
	const Card & operator[](unsigned which) const   {return _cards[which];}
	const CardVec& Cards() const 					{return _cards;}
	Card Pop()               {Card r = _cards.back(); _cards.pop_back(); return r;}
	void Push(const Card & c)              			{_cards.push_back(c);}
	CardVec Pop(unsigned n);
	CardVec Draw(unsigned n);         // like Pop(), but reverses order of cards drawn
	void Push(const Card* begin, const Card* end)
											{_cards.append(begin,end);}
	void Push(const CardVec& cds)           {this->Push(cds.begin(),cds.end());}
	Card Top() const                        {return *(_cards.end()-_upCount);}
	Card Back() const                       {return _cards.back();}
	void ClearCards()                       {_cards.clear(); _upCount = 0;}
	void Draw(Pile & from)					{_cards.push_back(from._cards.back()); from._cards.pop_back();}
	void Draw(Pile & from, int nCards);
};

// Returns a string to visualize a pile in a debugger.
std::string Peek(const Pile& pile);

// Directions for a move.  Game::AvailableMoves() creates these.
// Game::UnMakeMove() cannot infer the value of the from tableau pile's
// up count before the move (because of flips), so Game::AvailableMoves() 
// includes that in any Move from a tableau pile.
//
// Game::AvailableMoves creates Moves around the talon (the waste and
// stock piles) that must be counted as multiple moves.  The number
// of actual moves implied by a Move object is given by NMoves().
class Move
{
private:
	unsigned char _from;
	unsigned char _to;
	unsigned char _nMoves;
	union {
		struct {
			// Non-talon move
			unsigned char _n:4;
			unsigned char _fromUpCount:4;
		};
		signed char _draw;			// draw this many cards (may be negative)
	};

public:
	// Construct a talon move.  Represents 'nMoves'-1 draws + recycles.
	// Their cumulative effect is to draw 'draw' cards (may be negative)
	// from stock. One card is then moved from the waste pile to the "to" pile.
	Move(unsigned to, unsigned nMoves, int draw)
		: _from(STOCK)
		, _to(to)
		, _nMoves(nMoves)
		, _draw(draw)
		{}
	// Construct a non-talon move
	Move(unsigned from, unsigned to, unsigned n, unsigned fromUpCount)
		: _from(from)
		, _to(to)
		, _nMoves(1)
		, _n(n)
		, _fromUpCount(fromUpCount)
		{
			assert(from != STOCK);
		}

	unsigned From() const 			{return _from;}
	unsigned To()   const			{return _to;}
	unsigned NCards()    const 		{return (_from == STOCK) ? 1 : _n;}     
	unsigned FromUpCount() const 	{return _fromUpCount;}
	unsigned NMoves() const			{return _nMoves;}
	int Draw() const				{return _draw;}
};
typedef std::vector<Move> Moves;

// Returns the number of actual moves implied by a series of Moves.
unsigned MoveCount(const Moves & moves);

// Return a string to visualize a move in a debugger
std::string Peek(const Move& mv);

// Return a string to visualize a Moves vector
std::string Peek(const Moves & mvs);

// A vector of Moves is not very useful for enumerating the moves
// required to solve a game.  What's wanted for that is
// a vector of XMoves - objects that can simply be listed in
// various formats.
//
// Moves are numbered from 1.  The numbers will often not be
// consecutive, as drawing multiple cards from the stock pile
// is represented as a single XMove.
//
// Pile numbers are given by the enum PileCode.
//
// Flips of tableau cards are not counted as moves, but they
// are flagged on the move from the pile of the flip.

class XMove
{
	unsigned _moveNum;
	unsigned _from;
	unsigned _to;
	unsigned char _nCards;
	unsigned char _flip;		// tableau flip?
public:
	XMove()
		: _nCards(0)
		{}
	XMove(    unsigned moveNum
			, unsigned from
			, unsigned to
			, unsigned nCards
			, bool flip)
		: _moveNum(moveNum)
		, _from(from)
		, _to(to)
		, _nCards(nCards)
		, _flip(flip)
		{}
	unsigned  MoveNum() const		{return _moveNum;}
	unsigned  From() const			{return _from;}
	unsigned  To() const			{return _to;}
	unsigned  NCards() const		{return _nCards;}
	bool 	  Flip() const			{return _flip;}
};

typedef std::vector<XMove> XMoves;
XMoves MakeXMoves(const Moves & moves, unsigned draw);

class Game
{
	Pile _waste;
	Pile _stock;
	std::array<Pile,7> _tableau;
	std::array<Pile,4> _foundation;
	std::vector<Card> _deck;
	unsigned _draw;             // number of cards to draw from stock (usually 1 or 3)
	std::array<Pile *,13> _allPiles;

public:
	Game(const std::vector<Card>& deck,unsigned draw=1);

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
	void MakeMove(const XMove& xmv);
};
#endif