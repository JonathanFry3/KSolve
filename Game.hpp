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
// It is safe to use only where the problem limits the size needed.
#include <cstdint> 		// for uint_fast32_t, uint_fast64_t

template <class T, unsigned Capacity>
class fixed_capacity_vector{
    uint_fast32_t _size;
    T _elem[Capacity];
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    fixed_capacity_vector() 						: _size(0){}
    ~fixed_capacity_vector()						{clear();}
    template <class V>
        fixed_capacity_vector(const V& donor) 
        : _size(0)
        {append(donor.begin(),donor.end());}
    size_t capacity() const noexcept				{return Capacity;}
    T & operator[](unsigned i) noexcept				{assert(i<_size); return _elem[i];}
    const T& operator[](unsigned i) const noexcept	{assert(i<_size); return _elem[i];}
    iterator begin() noexcept						{return _elem;}
    const_iterator begin() const noexcept			{return _elem;}
    size_t size() const	noexcept					{return _size;}
    iterator end() noexcept							{return _elem+_size;}
    const_iterator end() const noexcept				{return _elem+_size;}
    T & back() noexcept								{return _elem[_size-1];}
    const T& back() const noexcept					{return _elem[_size-1];}
    void pop_back()	noexcept						{assert(_size); _size -= 1; _elem[_size].~T();}
    void push_back(const T& cd)	noexcept			{emplace_back(cd);}
    void clear() noexcept							{while (_size) pop_back();}
    void erase(iterator x) noexcept
                    {x->~T();for (iterator y = x+1; y < end(); ++y) *(y-1) = *y; _size-=1;}
    template <class V>
    bool operator==(const V& other) const noexcept
                    {	
                        if (_size != other.size()) return false;
                        auto iv = other.begin();
                        for(auto ic=begin();ic!=end();ic+=1,iv+=1){
                            if (*ic != *iv) return false;
                        }
                        return true;
                    }
    template <class V>
    fixed_capacity_vector<T,Capacity>& operator=(const V& other) noexcept
                    {
                        assert(other.size()<=Capacity);
                        clear();
                        for (const auto & m: other) emplace_back(m);
                        return *this;
                    }
    template <class ... Args>
    void emplace_back(Args ... args) noexcept
                    {
                        assert(_size < Capacity);
                        new(end()) T(args...);
                        _size += 1;
                    }
    // Functions not part of the std::vector API

    // Push the elements in [begin,end) to the back, preserving order.
    template <typename Iterator>
    void append(Iterator begin, Iterator end) noexcept	
                    {
                        assert(_size+(end-begin)<=Capacity);
                        for (auto i=begin;i<end;i+=1){
                            _elem[_size]=*i;
                            _size+=1;
                        }
                    }
    // Move the last n elements from the argument vector to this, preserving order.
    template <typename V>
    void take_back(V& donor, unsigned n) noexcept
                    {
                        assert(donor.size() >= n);
                        append(donor.end()-n, donor.end());
                        donor._size -= n;
                    }
};

enum Rank_t : unsigned char
 {
    Ace = 0,
    King = 12
};
enum Suit_t : unsigned char {
    Clubs = 0,
    Diamonds,
    Spades,
    Hearts,
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


    unsigned char Suit() const noexcept	{return _suit;}
    unsigned char Rank() const noexcept	{return _rank;}
    bool IsMajor() const noexcept		{return _isMajor;}
    bool OddRed() const noexcept		// true for card that fits on stacks where odd cards are red
                                        {return _parity;}
    unsigned Value() const noexcept		{return 13*_suit+_rank;}
    std::string AsString() const;       // Returns a string like "ha" or "d2"
    bool Covers(Card c) const noexcept	// can this card be moved onto c on a tableau pile?
                                        {return _parity == c._parity && _rank+1 == c._rank;}
    bool operator==(Card o) const noexcept	{return _suit==o._suit && _rank==o._rank;}
    bool operator!=(Card o) const noexcept	{return ! (o == *this);}

    // Make from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
    // Ignores characters that cannot appear in a valid card string.
    // Suit may come before or after rank, and letters may be in
    // upper or lower case, or mixed.
    // Returns true and the specified Card if it succeeds, 
    // false and garbage if it fails,
    static std::pair<bool,Card> FromString(const std::string& s) noexcept;
};

// Type to hold the cards in a pile after the deal.  None ever exceeds 24 cards.
typedef fixed_capacity_vector<Card,24> PileVec;

// Type to hold a complete deck
struct CardDeck : fixed_capacity_vector<Card,52> 
{
    CardDeck () = default;
    CardDeck (const std::vector<Card> vec) 
        : fixed_capacity_vector<Card,52>(vec)
    {
        assert(vec.size() == 52);
    }
};

// Function to generate a randomly shuffled deck
CardDeck NumberedDeal(uint_fast32_t seed);

enum PileCode {
    Stock = 0,
    Waste,
    TableauBase = 2,
    Tableau1 = 2,
    Tableau2,
    Tableau3,
    Tableau4,
    Tableau5,
    Tableau6,
    Tableau7,
    FoundationBase = 9,
    Foundation1C = 9,
    Foundation2D,
    Foundation3S,
    Foundation4H
};

static bool IsTableau(unsigned pile) noexcept
{
    return TableauBase <= pile && pile < TableauBase+7;
}

class Pile
{
private:
    PileVec _cards;
    unsigned short _code;
    unsigned short _upCount;
    bool _isTableau;
    bool _isFoundation;

public:
    Pile(PileCode code)
    : _code(code)
    , _upCount(0)
    , _isTableau(::IsTableau(code))
    , _isFoundation(FoundationBase <= code && code < FoundationBase+4)
    {}

    unsigned Code() const noexcept					{return _code;}
    unsigned UpCount() const noexcept				{return _upCount;}
    bool IsTableau() const noexcept					{return _isTableau;}
    bool IsFoundation() const noexcept				{return _isFoundation;}
    unsigned Size() const noexcept					{return _cards.size();}

    void SetUpCount(unsigned up) noexcept			{_upCount = up;}
    void IncrUpCount(int c) noexcept				{_upCount += c;}
    const Card & operator[](unsigned which) const noexcept  {return _cards[which];}
    const PileVec& Cards() const noexcept			{return _cards;}
    Card Pop() noexcept			{Card r = _cards.back(); _cards.pop_back(); return r;}
    void Push(Card c) noexcept             			{_cards.push_back(c);}
    void Take(Pile& donor, unsigned n)		{_cards.take_back(donor._cards, n);}
    void Push(PileVec::const_iterator begin, PileVec::const_iterator end) noexcept
                                            {_cards.append(begin,end);}
    Card Top() const  noexcept              {return *(_cards.end()-_upCount);}
    Card Back() const noexcept				{return _cards.back();}
    void ClearCards() noexcept              {_cards.clear(); _upCount = 0;}
    PileVec Draw(unsigned n) noexcept; 		// like Take(), but reverses order of cards drawn
    void Draw(Pile & from) noexcept			{_cards.push_back(from._cards.back()); from._cards.pop_back();}
    void Draw(Pile & from, int nCards) noexcept;
};


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
    unsigned char _recycle:1;
    unsigned char _nMoves:7;
    union {
        struct {
            // Non-talon move
            unsigned char _n:4;
            unsigned char _fromUpCount:4;
        };
        signed char _drawCount;			// draw this many cards (may be negative)
    };

public:
    Move() = default;
    // Construct a talon move.  Represents 'nMoves'-1 draws
    // Their cumulative effect is to draw 'draw' cards (may be negative)
    // from stock. One card is then moved from the waste pile to the "to" pile.
    // All talon moves, and only talon moves, are from the stock pile.
    Move(unsigned to, unsigned nMoves, int draw) noexcept
        : _from(Stock)
        , _to(to)
        , _nMoves(nMoves)
        , _drawCount(draw)
        , _recycle(0)
        {}
    // Construct a non-talon move.  UnMakeMove() can't infer the count
    // of face-up cards in a tableau pile, so AvailableMoves() saves it.
    Move(unsigned from, unsigned to, unsigned n, unsigned fromUpCount) noexcept
        : _from(from)
        , _to(to)
        , _nMoves(1)
        , _n(n)
        , _fromUpCount(fromUpCount)
        , _recycle(0)
        {
            assert(from != Stock);
        }

    void SetRecycle(bool r) noexcept    {_recycle = r;}      

    bool IsTalonMove() const noexcept	{return _from==Stock;}
    unsigned From() const noexcept		{return _from;}
    unsigned To()   const noexcept		{return _to;}
    unsigned NCards()    const noexcept	{return (_from == Stock) ? 1 : _n;}     
    unsigned FromUpCount() const noexcept{return _fromUpCount;}
    unsigned NMoves() const	noexcept	{return _nMoves;}
    bool Recycle() const noexcept       {return _recycle;}
    int DrawCount() const noexcept		{return _drawCount;}
};

typedef std::vector<Move> Moves;

// A limited-size Moves type for AvailableMoves to return
typedef fixed_capacity_vector<Move,74> QMoves; 

// Return the number of actual moves implied by a sequence of Moves.
template <class SeqType>
unsigned MoveCount(const SeqType& moves) noexcept
{
    unsigned result = 0;
    for (auto & move: moves)
        result += move.NMoves();
    return result;
}

// Return the number of stock recycles implied by a sequence of Moves.
template <class SeqType>
unsigned RecycleCount(const SeqType& moves) noexcept
{
    unsigned result = 0;
    for (auto & move: moves)
        result += move.Recycle();
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
    CardDeck _deck;
    Pile _waste;
    Pile _stock;
    std::array<Pile,7> _tableau;
    std::array<Pile,4> _foundation;
    std::array<Pile *,13> _allPiles; 	// pile numbers from enum PileCode
    unsigned _drawSetting;             	// number of cards to draw from stock (usually 1 or 3)
    unsigned _talonLookAheadLimit;
    unsigned _recycleLimit;             // max number of recycles allowed
    unsigned _recycleCount;             // n of recycles so far
    unsigned _kingSpaces;

    bool NeedKingSpace() const noexcept;

public:
    Game(CardDeck deck,unsigned draw=1,unsigned talonLookAheadLimit=24, unsigned recyleLimit=-1);
    Game(const Game&);

    Pile& WastePile()       						{return _waste;}
    Pile& StockPile()       						{return _stock;}
    std::array<Pile,4>& Foundation()   				{return _foundation;}
    std::array<Pile,7>& Tableau()      				{return _tableau;}
    std::array<Pile*,13>& AllPiles()     			{return _allPiles;}
    const Pile & WastePile() const     				{return _waste;}
    const Pile & StockPile() const     				{return _stock;}
    const std::array<Pile,4>& Foundation() const   	{return _foundation;}
    const std::array<Pile,7>& Tableau() const      	{return _tableau;}
    const std::array<Pile*,13>& AllPiles() const   	{return _allPiles;}
    unsigned DrawSetting() const            		{return _drawSetting;}
    unsigned TalonLookAheadLimit() const			{return _talonLookAheadLimit;}
    unsigned RecycleLimit() const                   {return _recycleLimit;}
    unsigned RecycleCount() const                   {return _recycleCount;}

    void Deal();
    QMoves AvailableMoves() noexcept;
    void  MakeMove(Move mv) noexcept;
    void  UnMakeMove(Move mv) noexcept;
    unsigned MinimumMovesLeft() const noexcept;
    void MakeMove(const XMove& xmv) noexcept;
    unsigned MinFoundationPileSize() const noexcept;
    bool GameOver() const noexcept;
};

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
    typedef std::uint_fast64_t PartType;
    std::array<PartType,3> _part;
    GameState(const Game& game) noexcept;
    bool operator==(const GameState& other) const noexcept
    {
        return _part[0] == other._part[0]
            && _part[1] == other._part[1]
            && _part[2] == other._part[2];
    }
};
struct Hasher
{
    size_t operator() (const GameState & gs) const noexcept
    {
        return 	  gs._part[0]
                ^ gs._part[1]
                ^ gs._part[2];
    }
};

#endif      // GAME_HPP