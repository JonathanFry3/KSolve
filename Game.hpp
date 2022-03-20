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

#include "frystl/static_vector.hpp"


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
    Card() :
        _suit(0),
        _rank(0),
        _isMajor(0),
        _parity(0)
        {}

    Card(const Card& orig) = default;

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

    // Make a Card from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
    // Ignores characters that cannot appear in a valid card string.
    // Suit may come before or after rank, and letters may be in
    // upper or lower case, or mixed.
    // Returns true and the specified Card if it succeeds, 
    // false and garbage if it fails,
    static std::pair<bool,Card> FromString(const std::string& s) noexcept;
};

// Type to hold the cards in a pile after the deal.  None ever exceeds 24 cards.
typedef frystl::static_vector<Card,24> PileVec;

// Type to hold a complete deck
struct CardDeck : frystl::static_vector<Card,52> 
{
    CardDeck () = default;
    CardDeck (const std::vector<Card> vec) 
        : static_vector<Card,52>(vec.begin(),vec.end())
    {
        assert(vec.size() == 52);
    }
    CardDeck(const frystl::static_vector<Card,52> &v)
        : static_vector<Card,52>(v)
    {
        assert(v.size() == 52);
    }
};

// Function to generate a randomly shuffled deck
CardDeck NumberedDeal(uint32_t seed);

enum PileCode {
    Waste = 0, 
    TableauBase,  // == 1.  Must == Waste+1
    Tableau1 = TableauBase,
    Tableau2,
    Tableau3,
    Tableau4,
    Tableau5,
    Tableau6,
    Tableau7,
    Stock,       // must == TableauBase+8.  See MovesToShortFoundationPile
    FoundationBase, // == 9
    Foundation1C = FoundationBase,
    Foundation2D,
    Foundation3S,
    Foundation4H
};

static bool IsTableau(unsigned pile) noexcept
{
    return TableauBase <= pile && pile < TableauBase+7;
}

class Pile : public PileVec
{
private:
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

    unsigned Code() const noexcept			{return _code;}
    unsigned UpCount() const noexcept		{return _upCount;}
    bool IsTableau() const noexcept			{return _isTableau;}
    bool IsFoundation() const noexcept		{return _isFoundation;}

    void SetUpCount(unsigned up) noexcept	{_upCount = up;}
    void IncrUpCount(int c) noexcept		{_upCount += c;}
    const PileVec& Cards() const noexcept	{return *this;}
    void Push(Card c) noexcept             	{push_back(c);}
    Card Pop() noexcept			            {Card r = back(); pop_back(); return r;}
    void Draw(Pile & from) noexcept			{push_back(from.back()); from.pop_back();}
    Card Top() const  noexcept              {return *(end()-_upCount);}
    void ClearCards() noexcept              {clear(); _upCount = 0;}
    // Take the last n cards from donor preserving order	
    void Take(Pile& donor, unsigned n) noexcept 
    {
        assert(n <= donor.size());
        for (auto p = donor.end()-n; p < donor.end(); ++p)
            push_back(*p);
        while (n--)
            donor.pop_back();
    }
    void Draw(Pile& other, int n) noexcept
    {
        if (n < 0) {
            while (n++) other.Draw(*this);
        } else {
            while (n--) Draw(other);
        }
    }
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
    unsigned char _nMoves:7;
    unsigned char _recycle:1;
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
        , _recycle(0)
        , _drawCount(draw)
        {}
    // Construct a non-talon move.  UnMakeMove() can't infer the count
    // of face-up cards in a tableau pile, so AvailableMoves() saves it.
    Move(unsigned from, unsigned to, unsigned n, unsigned fromUpCount) noexcept
        : _from(from)
        , _to(to)
        , _nMoves(1)
        , _recycle(0)
        , _n(n)
        , _fromUpCount(fromUpCount)
        {
            assert(from != Stock);
        }

    void SetRecycle(bool r) noexcept    {_recycle = r;}      

    bool IsTalonMove() const noexcept	{return _from==Stock;}
    unsigned From() const noexcept		{return _from;}
    unsigned To()   const noexcept		{return _to;}
    unsigned NCards()    const noexcept	{return (_from == Stock) ? 1 : _n;}     
    unsigned FromUpCount()const noexcept{return _fromUpCount;}
    unsigned NMoves() const	noexcept	{return _nMoves;}
    bool Recycle() const noexcept       {return _recycle;}
    int DrawCount() const noexcept		{return _drawCount;}
};

typedef std::vector<Move> Moves;

// A limited-size Moves type for AvailableMoves to return
typedef frystl::static_vector<Move,74> QMoves; 

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
    const CardDeck _deck;
    Pile _waste;
    Pile _stock;
    unsigned _drawSetting;             	// number of cards to draw from stock (usually 1 or 3)
    unsigned _talonLookAheadLimit;
    unsigned _recycleLimit;             // max number of recycles allowed
    unsigned _recycleCount;             // n of recycles so far
    unsigned _kingSpaces;
    std::array<Pile,7> _tableau;
    std::array<Pile,4> _foundation;
    std::array<Pile *,13> _allPiles; 	// pile numbers from enum PileCode

    // Return true if any more empty columns are needed for kings
    bool NeedKingSpace() const noexcept {return _kingSpaces < 4;}
    // Parts of AvailableMoves()
    void MovesToShortFoundationPile(QMoves & moves, unsigned minFndSize) const noexcept;
    void MovesFromTableau(QMoves & moves) const noexcept;
    bool MovesFromTalon(QMoves & moves, unsigned minFndSize) const noexcept;
    void MovesFromFoundation(QMoves & moves, unsigned minFndSize) const noexcept;
public:
    Game(CardDeck deck,
         unsigned draw=1,
         unsigned talonLookAheadLimit=24, 
         unsigned recyleLimit=-1);
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

    void        Deal();
    QMoves      AvailableMoves() const noexcept;
    void        MakeMove(Move mv) noexcept;
    void        UnMakeMove(Move mv) noexcept;
    unsigned    MinimumMovesLeft() const noexcept;
    void        MakeMove(const XMove& xmv) noexcept;
    unsigned    MinFoundationPileSize() const noexcept;
    bool        GameOver() const noexcept;
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
                ^ gs._part[2]
                ;
    }
};



// Return true if this move cannot be in a minimum solution.
template <class V>
bool ABC_Move(Move trial, const V& movesMade) noexcept
{
    // Consider a move at time T0 from A to B and the next move
    // from B, which goes to C at time Tn.  The move at Tn can
    // be skipped if the same result could have been achieved 
    // at T0 by moving the same cards directly from A to C.

    // We are now at Tn looking back for a T0 move.  B is our from pile
    // and C is our to pile.  A candidate T0 move is one that moves
    // to our from pile (pile B).

    // Do those two moves move the same set of cards?.  Yes if
    // no intervening move has changed pile B and the two moves
    // move the same number of cards.

    // Was the move from A to C possible at T0? Yes if pile
    // C has not changed since the cards moved at T0 were
    // lifted off pile A.  If the T0 move caused a face-down card
    // on pile C to be turned up (i.e., A==C, pile A is a tableau
    // pile, and the T0 move resulted in a flip of a face-down card), 
    // that changed pile C.

    // Since nothing says A cannot equal C, this test catches 
    // moves that exactly reverse previous moves.
    const auto B = trial.From();
    if (B == Stock || B == Waste) return false; 
    const auto C = trial.To();
    for (auto imv = movesMade.crbegin(); imv != movesMade.crend(); ++imv){
        const Move mv = *imv;
        if (mv.To() == B){
            // candidate T0 move
            if (mv.From() == C) {
                // If A=C and the A to B move flipped a tableau card
                // face up, then it changed C.
                if (IsTableau(C) && mv.NCards() == mv.FromUpCount())
                    return false;
            }
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

    // AvailableMoves() generates moves among tableau files for only two purposes:
    // to move all the face-up cards, or to uncover a card that can be moved to the 
    // foundation.  I have tried filtering out later moves that would re-cover a 
    // card that had been uncovered in that fashion.  That did not break anything, but
    // cost more time than it saved.
    // Jonathan Fry 7/12/2020
}


#endif      // GAME_HPP