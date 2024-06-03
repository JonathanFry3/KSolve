#ifndef GAME_HPP
#define GAME_HPP

/*
    This file defines the interfaces for the Klondike Solitaire card game.  It defines such
    things as a Card, a Pile, and a MoveSpec.  It defines the Game containing several piles:
    stock       the pile from which a player draws
    waste       the pile on which to lay a drawn card if it is not played elsewhere
    foundation  the four piles, one for each suit, to which one wishes to move all the cards
    tableau     the seven piles originally dealt with one card turned up.

    The talon is the stock and waste piles considered as a single entity.

    Any solvers implemented here will operate on these objects.
*/ 
#include <ranges>
#include <string>
#include <vector>
#include <array>
#include <cassert>
#include <sstream> 		// for stringstream
#include <optional>
#include <numeric>
#include <algorithm>

#include "frystl/static_vector.hpp"
namespace KSolveNames {

namespace ranges = std::ranges;
namespace views = std::views;

const unsigned CardsPerSuit {13};
const unsigned SuitsPerDeck {4};
const unsigned CardsPerDeck {CardsPerSuit*SuitsPerDeck};
const unsigned TableauSize  {7};

class Card
{
public:
    enum RankT : unsigned char
    {
        Ace = 0,
        King = 12
    };
    enum SuitT : unsigned char 
    {
        Clubs = 0,
        Diamonds,
        Spades,
        Hearts
    };

private:
    SuitT _suit:2;
    RankT _rank:6;

public:
    Card() = default;
    Card(const Card& orig) = default;

    Card(Card::SuitT suit, Card::RankT rank) : 
        _suit(suit),
        _rank(rank)
        {}

    Card(unsigned value):
        _suit(Card::SuitT(value/CardsPerSuit)),
        _rank(RankT(value%CardsPerSuit))
        {}


    Card::SuitT Suit() const noexcept	    {return _suit;}
    RankT Rank() const noexcept 	        {return _rank;}
    unsigned char IsMajor() const noexcept	{return _suit>>1;} // Hearts or spades
    unsigned char OddRed() const noexcept		// true for card that fits on stacks where odd cards are red
                                            {return (_rank&1)^(_suit&1);}
    unsigned Value() const noexcept		    {return CardsPerSuit*_suit+_rank;}
    std::string AsString() const;           // Returns a string like "ha" or "d2"
    bool Covers(Card c) const noexcept	    // can this card be moved onto c on a tableau pile?
                                            {return _rank+1 == c._rank && OddRed() == c.OddRed();}
    bool operator==(Card o) const noexcept	{return _suit==o._suit && _rank==o._rank;}
    bool operator!=(Card o) const noexcept	{return ! (o == *this);}
};
// Make a Card from a string like "ah" or "s8" or "D10" or "tc" (same as "c10").
// Ignores characters that cannot appear in a valid card string.
// Suit may come before or after rank, and letters may be in
// upper or lower case, or mixed.
std::optional<Card> CardFromString(const std::string& s) noexcept;

static_assert(sizeof(Card) == 1, "Card must be 1 byte long");

// Type to hold the cards in a pile after the deal.  None ever exceeds 24 cards.
typedef frystl::static_vector<Card,24> PileVec;
static_assert(sizeof(PileVec) <= 28, "PileVec should fit in 28 bytes");

// Type to hold a complete deck
struct CardDeck : frystl::static_vector<Card,CardsPerDeck> 
{
    CardDeck () = default;
    CardDeck (const std::vector<Card> vec) 
        : static_vector<Card,CardsPerDeck>(vec.begin(),vec.end())
    {
        assert(vec.size() == CardsPerDeck);
    }
    CardDeck(const frystl::static_vector<Card,CardsPerDeck> &v)
        : static_vector<Card,CardsPerDeck>(v)
    {
        assert(v.size() == CardsPerDeck);
    }
};

// Function to shuffle a (possible partial) deck
void Shuffle(CardDeck & deck, uint32_t seed);

// Function to generate a randomly shuffled deck
CardDeck NumberedDeal(uint32_t seed);

enum PileCodeT : unsigned char{
    Waste = 0, 
    TableauBase,  // == 1.  Must == Waste+1
    Tableau1 = TableauBase,
    Tableau2,
    Tableau3,
    Tableau4,
    Tableau5,
    Tableau6,
    Tableau7,
    Stock,       // must == TableauBase+TableauSize.  See OneMoveToShortFoundationPile
    FoundationBase, // == 9
    Foundation1C = FoundationBase,
    Foundation2D,
    Foundation3S,
    Foundation4H,
    PileCount
};
static_assert(Stock == PileCodeT(TableauBase+TableauSize));

static PileCodeT FoundationPileCode(Card::SuitT suit)
{
    unsigned suitNum = suit;
    return static_cast<PileCodeT>(FoundationBase+suitNum);
}

static bool IsTableau(PileCodeT pile) noexcept
{
    return TableauBase <= pile && pile < TableauBase+TableauSize;
}

class alignas(32) Pile : public PileVec
{
private:
    PileCodeT _code;
    unsigned char _upCount;
    bool _isTableau;
    bool _isFoundation;

public:
    Pile(PileCodeT code)
    : _code(code)
    , _upCount(0)
    , _isTableau(KSolveNames::IsTableau(code))
    , _isFoundation(FoundationBase <= code && code < FoundationBase+SuitsPerDeck)
    {}

    PileCodeT Code() const noexcept		{return _code;}
    unsigned UpCount() const noexcept		{return _upCount;}
    bool IsTableau() const noexcept			{return _isTableau;}
    bool IsFoundation() const noexcept		{return _isFoundation;}

    void SetUpCount(unsigned up) noexcept	{_upCount = up;}
    void IncrUpCount(int c) noexcept		{_upCount += c;}
    const PileVec& Cards() const noexcept	{return *this;}
    void Push(Card c) noexcept             	{push_back(c);}
    Card Pop() noexcept			            {Card r = back(); pop_back(); return r;}
    void Draw(Pile & from) noexcept			{emplace_back(std::move(from.back())); from.pop_back();}
    Card Top() const  noexcept              {return *(end()-_upCount);}
    void ClearCards() noexcept              {clear(); _upCount = 0;}
    // Take the last n cards from donor preserving order	
    void Take(Pile& donor, unsigned n) noexcept 
    {
        assert(n <= donor.size());
        for (auto p = donor.end()-n; p < donor.end(); ++p)
            emplace_back(std::move(*p));
        donor.resize(donor.size() - n);
    }
    // If n > 0, move the last n cards in other to the end of 
    // this pile reversing order. 
    // If n < 0, do the reverse.
    void Draw(Pile& other, int n) noexcept
    {
        if (n < 0) {
            while (n++) other.Draw(*this);
        } else {
            while (n--) Draw(other);
        }
    }
};

static_assert(sizeof(Pile) <= 32, "Good to make Pile fit in 32 bytes");

// Returns a string to visualize a pile for debugging.
std::string Peek(const Pile& pile);

// Directions for a move.  Game::AvailableMoves() creates these.
//
// Game::UnMakeMove() cannot infer the value of the from tableau pile's
// up count before the move (because of flips), so Game::AvailableMoves() 
// includes that in any Move from a tableau pile.
//
// Game::AvailableMoves creates Moves around the talon (the waste and
// stock piles) that must be counted as multiple moves.  The number
// of actual moves implied by a MoveSpec object is given by NMoves().
//
// A "ladder move" is a move from one tableau pile to another which
// is made for the purpose of exposing a card that can be moved to
// the tableau.  The MoveSpec for such a move makes that move and then
// moves the exposed card to the foundation. It retains the suit of 
// the exposed card because UnMakeMove() needs it.
//
// Since making this class type-safe at compile time (using std::variant) 
// requires making it larger (doubling the size of the move tree),
// it has been made type-safe at run time using asserts.
class MoveSpec
{
private:
    PileCodeT _from = PileCount;    // _from == Stock means stock MoveSpec
    PileCodeT _to = PileCount;
    unsigned char _nMoves:5 = 0;
    Card::SuitT _ladderSuit :2 = Card::Clubs; 
    bool _recycle:1 = false;
    union {
        // Non-stock MoveSpec
        struct {
            unsigned char _cardsToMove:4;
            unsigned char _fromUpCount:4;
        };
        // Stock MoveSpec
        signed char _drawCount;			// draw this many cards (may be negative)
    };
    // Construct a stock MoveSpec. Their cumulative effect is to 
    // draw 'draw' cards (may be negative) from stock to (from)
    // the waste pile. One card is then moved from the waste pile
    // to the "to" pile. Only stock Moves draw from the stock pile.
    MoveSpec(PileCodeT to, unsigned nMoves, int draw) noexcept
        : _from(Stock)
        , _to(to)
        , _nMoves(nMoves)
        , _recycle(0)
        , _drawCount(draw)
        {}
    // Construct a non-stock MoveSpec.  UnMakeMove() can't infer the count
    // of face-up cards in a tableau pile, so AvailableMoves() saves it.
    MoveSpec(PileCodeT from, PileCodeT to, unsigned n, unsigned fromUpCount) noexcept
        : _from(from)
        , _to(to)
        , _nMoves(1)
        , _recycle(0)
        , _cardsToMove(n)
        , _fromUpCount(fromUpCount)
        {
            assert(from != Stock);
        }
    friend MoveSpec StockMove(PileCodeT, unsigned, int, bool) noexcept;
    friend MoveSpec NonStockMove(PileCodeT, PileCodeT, unsigned, unsigned) noexcept;
    friend MoveSpec LadderMove(PileCodeT, PileCodeT, unsigned, unsigned, Card) noexcept;

public:
    MoveSpec() = default;
    bool IsDefault()                    {return _from == _to;}

    void SetRecycle(bool r) noexcept    {_recycle = r;}

    bool IsStockMove() const noexcept	{return _from==Stock;}
    PileCodeT From() const noexcept     {return _from;}
    PileCodeT To() const noexcept	    {return _to;}
    unsigned NCards() const noexcept	{return (_from == Stock) ? 1 : _cardsToMove;}     
    unsigned FromUpCount()const noexcept{assert(_from != Stock); return _fromUpCount;}
    unsigned NMoves() const	noexcept	{return _nMoves;}
    Card::SuitT LadderSuit() const noexcept{return _ladderSuit;}
    bool Recycle() const noexcept       {return _recycle;}
    int DrawCount() const noexcept		{assert(_from == Stock); return _drawCount;}

};
static_assert(sizeof(MoveSpec) == 4, "MoveSpec must be 4 bytes long");

inline MoveSpec StockMove(PileCodeT to, unsigned nMoves, int draw, bool recycle) noexcept
{
    MoveSpec result(to, nMoves, draw);
    result.SetRecycle(recycle);
    return result; 
}
inline MoveSpec NonStockMove(PileCodeT from, PileCodeT to, unsigned n, unsigned fromUpCount) noexcept
{
    return MoveSpec(from,to,n,fromUpCount);
}
inline MoveSpec LadderMove(PileCodeT from, PileCodeT to, unsigned n, unsigned fromUpCount, Card ladderCard) noexcept
{
    MoveSpec result{from,to,n,fromUpCount};
    result._nMoves = 2;
    result._ladderSuit = ladderCard.Suit();
    return result;
}

typedef std::vector<MoveSpec> Moves;

// Class for collecting freshly-built Moves in AvailableMoves()
class QMoves : public frystl::static_vector<MoveSpec,43>
{
public:
    void AddStockMove(PileCodeT to, unsigned nMoves, 
        int draw, bool recycle) noexcept
    {
        push_back(StockMove(to,nMoves,draw,recycle));
    }
    void AddNonStockMove(PileCodeT from, PileCodeT to, 
        unsigned n, unsigned fromUpCount) noexcept
    {
        push_back(NonStockMove(from,to,n,fromUpCount));
    }
    void AddLadderMove(PileCodeT from, PileCodeT to, 
        unsigned n, unsigned fromUpCount, Card ladderCard) noexcept
    {
        push_back(LadderMove(from,to,n,fromUpCount,ladderCard));
    }
};

// Return the number of actual moves implied by a sequence of Moves.
template <class SeqType>
unsigned MoveCount(const SeqType& moves) noexcept
{
    return std::accumulate(moves.begin(),moves.end(),0,
        [](auto acc, auto move){return acc+move.NMoves();});
}

// Return the number of stock recycles implied by a sequence of Moves.
template <class SeqType>
unsigned RecycleCount(const SeqType& moves) noexcept
{
    return ranges::count_if(moves, 
        [&](auto  p) {return p.Recycle();});
}

// Mix-in to make any sequence container an automatic move counter.
template <class Container>
class MoveCounter : public Container
{
    unsigned _nMoves;
    using Base = Container;
public:
    unsigned MoveCount() const           {return _nMoves;}

    void clear()
    {
        _nMoves = 0;
        Base::clear();
    }
    void push_front(const MoveSpec& mv)
    {
        _nMoves += mv.NMoves();
        Base::push_front(mv);
    }
    void pop_front() noexcept
    {
        _nMoves -= Base::front().NMoves();
        Base::pop_front();
    }

    void push_back(const MoveSpec& mv)
    {
        _nMoves += mv.NMoves();
        Base::push_back(mv);
    }
    void pop_back() noexcept
    {
        _nMoves -= Base::back().NMoves();
        Base::pop_back();
    }
};
// Return a string to visualize a move for debugging
std::string Peek(const MoveSpec& mv);

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
// Pile numbers are given by the enum PileCodeT.
//
// Flips of tableau cards are not counted as moves, but they
// are flagged on the move from the pile of the flip.

class XMove
{
    unsigned short _moveNum;
    PileCodeT _from;
    PileCodeT _to;
    unsigned char _nCards;
    unsigned char _flip;		// tableau flip?
public:
    XMove()
        : _nCards(0)
        {}
    XMove(    unsigned moveNum
            , PileCodeT from
            , PileCodeT to
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

// Return true if this move cannot be in a minimum solution.
template <class V>
static bool XYZ_Move(MoveSpec trial, const V& movesMade) noexcept
{
    // Consider a move at time T0 from X to Y and the next move
    // from Y, which goes from Y to Z at time Tn.  The move at Tn can
    // be skipped if the same result could have been achieved 
    // at T0 by moving the same cards directly from X to Z.

    // We are now at Tn looking back for a T0 move.  Y is our from-pile
    // and Z is our to-pile.  A candidate T0 move is one that moves
    // to our from-pile (pile Y).

    // Do those two moves move the same set of cards?.  Yes if
    // no intervening move has changed pile Y and the two moves
    // move the same number of cards.

    // Was the move from X to Z possible at T0? Yes if pile
    // Z has not changed since the cards moved at T0 were
    // lifted off pile X.  If the T0 move caused a face-down card
    // on pile Z to be turned up (i.e., X==Z, pile X is a tableau
    // pile, and the T0 move resulted in a flip of a face-down card), 
    // that changed pile Z.

    // Since nothing says X cannot equal Z, this test catches 
    // moves that exactly reverse previous moves.
    const auto Y = trial.From();
    if (Y == Stock || Y == Waste) return false; 
    const auto Z = trial.To();
    for (auto mv: views::reverse(movesMade)){ 
        if (mv.To() == Y){
            // candidate T0 move
            if (mv.From() == Z) {
                // If X=Z and the X to Y move flipped a tableau card
                // face up, then it changed Z.
                if (IsTableau(Z) && mv.NCards() == mv.FromUpCount())
                    return false;
            }
            return  mv.NCards() == trial.NCards();
        } else if (Z == mv.From() && mv.NMoves() == 2 && Y == mv.LadderSuit()+FoundationBase) {
            return true;
        } else {
            // intervening move
            if (mv.To() == Z || mv.From() == Z)
                return false;			// trial move's to-pile (Z) has changed
            if (mv.From() == Y) 
                return false;			// trial move's from-pile (Y) has changed
        }
    }
    return false;

    // UnfilteredAvailableMoves() generates moves among tableau files for only two purposes:
    // to move all the face-up cards, or to uncover a card that can be moved to the 
    // foundation.  I have tried filtering out later moves that would re-cover a 
    // card that had been uncovered in the latter fashion.  That did not break anything, but
    // cost more time than it saved.
    // Jonathan Fry 7/12/2020
}

class Game
{
public:
    using FoundationType = std::array<Pile,SuitsPerDeck>;
    using TableauType = std::array<Pile,TableauSize>;
private:
    // See the declaration of PileCodeT for the order of piles.
    Pile            _waste;
    TableauType     _tableau;
    Pile            _stock;
    FoundationType  _foundation;

    unsigned char   _drawSetting;             // number of cards to draw from stock (usually 1 or 3)
    unsigned char   _recycleLimit;            // max number of recycles allowed
    unsigned char   _recycleCount;            // n of recycles so far
    unsigned char   _kingSpaces;

    const CardDeck _deck;

    // Return true if any more empty columns are needed for kings
    bool NeedKingSpace() const noexcept {return _kingSpaces < SuitsPerDeck;}
    // Parts of UnfilteredAvailableMoves()
    void OneMoveToShortFoundationPile(QMoves & moves, unsigned minFndSize) const noexcept;
    void MovesFromTableau(QMoves & moves) const noexcept;
    bool MovesFromStock(QMoves & moves, unsigned minFndSize) const noexcept;
    void MovesFromFoundation(QMoves & moves, unsigned minFndSize) const noexcept;

    QMoves UnfilteredAvailableMoves() const noexcept;
    std::array<Pile,PileCount>& AllPiles() {
        return *reinterpret_cast<std::array<Pile,PileCount>* >(&_waste);
    }
    
public:
    Game(CardDeck deck,
         unsigned draw=1,
         unsigned recyleLimit=-1);
    Game(const Game&);
    const Pile & WastePile() const noexcept    	    {return _waste;}
    const Pile & StockPile() const noexcept    	    {return _stock;}
    const FoundationType& Foundation()const noexcept{return _foundation;}
    const TableauType& Tableau() const noexcept     {return _tableau;}
    unsigned DrawSetting() const noexcept           {return _drawSetting;}
    unsigned RecycleLimit() const noexcept          {return _recycleLimit;}
    unsigned RecycleCount() const noexcept          {return _recycleCount;}
    const std::array<Pile,PileCount>& AllPiles() const {
        return *reinterpret_cast<const std::array<Pile,PileCount>* >(&_waste);
    }

    bool CanMoveToFoundation(Card cd) const noexcept{
        return cd.Rank() == Foundation()[cd.Suit()].size();
    }

    void        Deal() noexcept;
    void        MakeMove(MoveSpec mv) noexcept;
    void        UnMakeMove(MoveSpec mv) noexcept;
    unsigned    MinimumMovesLeft() const noexcept;
    void        MakeMove(const XMove& xmv) noexcept;
    bool        IsValid(MoveSpec mv) const noexcept;
    bool        IsValid(XMove xmv) const noexcept;
    unsigned    MinFoundationPileSize() const noexcept;
    bool        GameOver() const noexcept;

    // Return a vector of the available moves that pass the XYZ_Move filter
    template <class V>
    QMoves AvailableMoves(const V& movesMade) noexcept
    {
        QMoves avail = UnfilteredAvailableMoves();
        auto newEnd = ranges::remove_if(avail,
            [&movesMade] (MoveSpec move) 
                {return XYZ_Move(move, movesMade);}).begin();
        while (avail.end() != newEnd) avail.pop_back();
        return avail;
    }

};

// Validate a solution
template <class Container>
void TestSolution(Game game, const Container& mvs)
{
		game.Deal();
		// PrintGame(game);
		for (auto mv: mvs) {
			assert(game.IsValid(mv));
			game.MakeMove(mv);
		}
		// PrintGame(game);
		assert(game.GameOver());
}

// Return a string to visualize the state of a game
std::string Peek (const Game& game);

}   // namespace KSolveNames

#endif      // GAME_HPP