#ifndef GAME_HPP
#define GAME_HPP

/*
	This file defines the interfaces for the Klondike Solitaire card game.  It defines such
	things as a Card, a Pile, and a Move.  It defines the Game containing several piles:
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
#include <sstream> 		// for stringstream

// Template class fixed_capacity_vector
//
// One of these has much of the API of a std::vector,
// but has a fixed capacity.  It cannot be extended past that.
// It is safe to use only where the problem limits the size needed,
// as it does not check for overfilling.  It is designed for speed,
// so it does not check subscripts.
#include <cstdint> 		// for uint32_t, uint64_t
#include <algorithm>	// for std::copy()

template <class T, unsigned Capacity>
class fixed_capacity_vector{
	uint32_t _size;
	T _elem[Capacity];
public:
	typedef T* iterator;
	typedef const T* const_iterator;
	fixed_capacity_vector() : _size(0){}
	~fixed_capacity_vector(){clear();}
	size_t capacity() const {return Capacity;}
	T & operator[](unsigned i)						{assert(i<_size); return _elem[i];}
	const T& operator[](unsigned i) const			{return _elem[i];}
	iterator begin()								{return _elem;}
	const_iterator begin() const					{return _elem;}
	size_t size() const								{return _size;}
	iterator end()									{return _elem+_size;}
	const_iterator end() const						{return _elem+_size;}
	T & back()										{return _elem[_size-1];}
	const T& back() const							{return _elem[_size-1];}
	void pop_back()									{back().~T(); _size -= 1;}
	void pop_back(unsigned n)						{for (unsigned i=0;i<n;++i) {pop_back();}}
	void push_back(const T& cd)						{assert(_size<Capacity); _elem[_size] = cd; _size += 1;}
	void clear()									{while (_size) pop_back();}
	void append(const_iterator begin, const_iterator end)	
					{for (auto i=begin;i<end;i+=1){_elem[_size]=*i;_size+=1;}}
	void erase(iterator x)
					{x->~T();for (iterator y = x+1; y < end(); ++y) *(y-1) = *y; _size-=1;}
	template <class V>
	bool operator==(const V& other) const
					{	
						if (_size != other.size()) return false;
						auto iv = other.begin();
						for(auto ic=begin();ic!=end();ic+=1,iv+=1){
							if (*ic != *iv) return false;
						}
						return true;
					}
	template <class V>
	fixed_capacity_vector<T,Capacity>& operator=(const V& other) 
					{
						_size = other.size();
						std::copy(other.begin(),other.end(),begin());
						return *this;
					}
	template <class ... Args>
	void emplace_back(Args ... args)
					{
						new(end()) T(args...);
						_size += 1;
					}
};

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
		_suit(value/13),
		_rank(value%13),
		_isMajor(_suit>>1),
		_parity((_rank&1)^(_suit&1))
		{}


	unsigned char Suit() const 			{return _suit;}
	unsigned char Rank() const 			{return _rank;}
	bool IsMajor() const				{return _isMajor;}
	bool OddRed() const 				{return _parity;}  // true for card that fits on stacks where odd cards are red
	unsigned Value() const				{return 13*_suit+_rank;}
	std::string AsString() const;       // Returns a string like "ha" or "d2"
	bool Covers(Card other) const 		// can other be moved onto this card on a tableau pile?
		{return _parity == other._parity && _rank+1 == other._rank;}
	bool operator==(Card other) const {return _suit==other._suit && _rank==other._rank;}
	bool operator!=(Card other) const {return ! (other == *this);}

	// Make from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
	// Ignores characters that cannot appear in a valid card string.
	// Suit may come before or after rank, and letters may be in
	// upper or lower case, or mixed.
	// Returns true and the specified Card if it succeeds, 
	// false and garbage if it fails,
	static std::pair<bool,Card> FromString(const std::string& s);
};

// Type to hold the cards in a pile after the deal.  None ever exceeds 24 cards.
typedef fixed_capacity_vector<Card,24> CardVec;

enum PileCode {
	STOCK = 0,
	WASTE,
	TABLEAU = 2,
	TABLEAU1 = 2,
	TABLEAU2,
	TABLEAU3,
	TABLEAU4,
	TABLEAU5,
	TABLEAU6,
	TABLEAU7,
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
	void Push(Card c)              			{_cards.push_back(c);}
	CardVec Pop(unsigned n);
	void Push(CardVec::const_iterator begin, CardVec::const_iterator end)
											{_cards.append(begin,end);}
	void Push(const CardVec& cds)           {this->Push(cds.begin(),cds.end());}
	Card Top() const                        {return *(_cards.end()-_upCount);}
	Card Back() const                       {return _cards.back();}
	void ClearCards()                       {_cards.clear(); _upCount = 0;}
	CardVec Draw(unsigned n);         		// like Pop(), but reverses order of cards drawn
	void Draw(Pile & from)					{_cards.push_back(from._cards.back()); from._cards.pop_back();}
	void Draw(Pile & from, int nCards);
};

static bool IsTableau(unsigned pile)
{
	return TABLEAU <= pile && pile < TABLEAU+7;
}


// Returns a string to visualize a pile for debugging.
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
		signed char _drawCount;			// draw this many cards (may be negative)
	};

public:
	Move() {}
	// Construct a talon move.  Represents 'nMoves'-1 draws + recycles.
	// Their cumulative effect is to draw 'draw' cards (may be negative)
	// from stock. One card is then moved from the waste pile to the "to" pile.
	Move(unsigned to, unsigned nMoves, int draw)
		: _from(STOCK)
		, _to(to)
		, _nMoves(nMoves)
		, _drawCount(draw)
		{}
	// Construct a non-talon move.  UnMakeMove() can't infer the count
	// of face-up cards in a tableau pile, so AvailableMoves() saves it.
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
	int DrawCount() const			{return _drawCount;}
};

typedef std::vector<Move> Moves;

// A limited-size Moves type for AvailableMoves to return
typedef fixed_capacity_vector<Move,74> QMoves; 

// Return the number of actual moves implied by a vector of Moves.
template <class SeqType>
unsigned MoveCount(const SeqType& moves)
{
	unsigned result = 0;
	for (auto & move: moves)
		result += move.NMoves();
	return result;
}

// Return a string to visualize a move for debugging
std::string Peek(const Move& mv);

// Return a string to visualize a sequence of Moves
template <class Moves_t>
std::string Peek(const Moves_t & mvs)
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
	unsigned short _moveNum;
	unsigned char _from;
	unsigned char _to;
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
	unsigned _drawSetting;             	// number of cards to draw from stock (usually 1 or 3)
	unsigned _talonLookAheadLimit;
	std::array<Pile *,13> _allPiles; 	// pile numbers from enum PileCode

public:
	Game(const std::vector<Card>& deck,unsigned draw=1,unsigned talonLookAheadLimit=24);
	Game(const Game&);

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
	unsigned DrawSetting() const            		{return _drawSetting;}

	void Deal();
	QMoves AvailableMoves() const;
	void  MakeMove(Move mv);
	void  UnMakeMove(Move mv);
	unsigned MinimumMovesLeft() const;
	void MakeMove(const XMove& xmv);
	bool GameOver() const;
};

// Return the size of the shortest foundation pile
unsigned MinFoundationPileSize(const Game& gm);


// Return a string to visualize the state of a game
std::string Peek (const Game& game);

// A compact representation of the current game state.
// It is possible, although tedious, to reconstruct the
// game state from one of these and the original deck.
//
// The basic requirements for GameState are:
// 1.  Any difference in the foundation piles, the face-up cards
//     in the tableau piles, or in the stock pile length
//     should be reflected in the GameState.
// 2.  It should be quite compact, as we will usually be storing
//     millions or tens of millions of instances.
struct GameState {
	typedef std::uint64_t PartType;
	std::array<PartType,3> _part;
	GameState(const Game& game);
	bool operator==(const GameState& other) const
	{
		return _part[0] == other._part[0]
			&& _part[1] == other._part[1]
			&& _part[2] == other._part[2];
	}
};

#endif      // GAME_HPP