/*
    Implements Game.hpp.
*/

#include "Game.hpp"
#include <cassert>
#include <algorithm>		// shuffle
#include <random>

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
    return std::string(1,suits[_suit]) + ranks[_rank];
}

// Make a Card from a string like "ah" or "s8".
// Returns true if it succeeds.
std::pair<bool,Card> Card::FromString(const std::string& s0) noexcept  
{
    const std::string s1 = Filtered(LowerCase(s0),suits+ranks+"10");
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

CardDeck NumberedDeal(uint32_t seed)
{
    // Create and seed a random number generator
    std::mt19937 engine;
    engine.seed(seed);

    // Create a new sorted pack of cards
    CardDeck deck;
    for (unsigned i = 0; i < 52; ++i){
        deck.emplace_back(i);
    }
    // Randomly shuffle the deck
    std::shuffle(deck.begin(),deck.end(),engine);
    return deck;
}

static void SetAllPiles(Game& game)
{
    auto & allPiles = game.AllPiles();
    allPiles[Waste] = &game.WastePile();
    allPiles[Stock] = &game.StockPile();
    for (int ip = 0; ip < 4; ip+=1) allPiles[FoundationBase+ip] = &game.Foundation()[ip];
    for (int ip = 0; ip < 7; ip+=1) allPiles[TableauBase+ip] = &game.Tableau()[ip];
}

Game::Game(CardDeck deck,unsigned draw,unsigned talonLookAheadLimit,unsigned recycleLimit)
    : _deck(deck)
    , _waste(Waste)
    , _stock(Stock)
    , _drawSetting(draw)
    , _talonLookAheadLimit(talonLookAheadLimit)
    , _recycleLimit(recycleLimit)
    , _tableau{Tableau1,Tableau2,Tableau3,Tableau4,Tableau5,Tableau6,Tableau7}
    , _foundation{Foundation1C,Foundation2D,Foundation3S,Foundation4H}
{
    SetAllPiles(*this);
    Deal();
}
Game::Game(const Game& orig)
    : _deck(orig._deck)
    , _waste(orig._waste)
    , _stock(orig._stock)
    , _drawSetting(orig._drawSetting)
    , _talonLookAheadLimit(orig._talonLookAheadLimit)
    , _recycleLimit(orig._recycleLimit)
    , _recycleCount(orig._recycleCount)
    , _kingSpaces(orig._kingSpaces)
    , _tableau(orig._tableau)
    , _foundation(orig._foundation)
    {
        SetAllPiles(*this);
    }

// Deal the cards for Klondike Solitaire.
void Game::Deal()
{
    _kingSpaces = 0;
    _recycleCount = 0;

    for (auto pile: _allPiles)	{
        pile->ClearCards();
    }
    unsigned ideck = 0;
    for (unsigned i = 0; i<7; i+=1) {
        for (unsigned icd = i; icd < 7; icd+=1)	{
            _tableau[icd].Push(_deck[ideck]);
            ideck += 1;
        }
        _tableau[i].SetUpCount(1);      // turn up the top card
        _kingSpaces += _tableau[i][0].Rank() == King;	// count kings at base
    }
    for (unsigned ic = 51; ic >= 28; ic-=1)
        _stock.Push(_deck[ic]);
}

void Game::MakeMove(Move mv) noexcept
{
    const auto to = mv.To();
    Pile& toPile = *_allPiles[to];
    if (mv.IsTalonMove()) {
        _waste.Draw(_stock,mv.DrawCount());
        toPile.Push(_waste.Pop());
        toPile.IncrUpCount(1);
        if (mv.Recycle()) _recycleCount += 1;
    } else {
        const auto n = mv.NCards();
        Pile& fromPile = *_allPiles[mv.From()];
        toPile.Take(fromPile, n);
        // For tableau piles, UpCount counts face-up cards.  
        // For other piles, it is undefined.
        toPile.IncrUpCount(n);
        fromPile.IncrUpCount(-n);
        if (fromPile.empty())
            _kingSpaces += fromPile.IsTableau(); // count newly cleared columns
        else {
            if (fromPile.UpCount() == 0){
                fromPile.SetUpCount(1);    // flip the top card
            }
        }
    }
}

void  Game::UnMakeMove(Move mv) noexcept
{
    const auto to = mv.To();
    Pile & toPile = *_allPiles[to];
    if (mv.IsTalonMove()) {
        _waste.Push(toPile.Pop());
        toPile.IncrUpCount(-1);
        _stock.Draw(_waste,mv.DrawCount());
        if (mv.Recycle()) _recycleCount -= 1;
    } else {
        const auto n = mv.NCards();
        Pile & fromPile = *_allPiles[mv.From()];
        if (fromPile.IsTableau()) {
            _kingSpaces -= fromPile.empty();  // uncount newly cleared columns
            fromPile.SetUpCount(mv.FromUpCount());
        }
        fromPile.Take(toPile, n);
        toPile.IncrUpCount(-n);
    }
}

void Game::MakeMove(const XMove & xmv) noexcept
{
    const auto from = xmv.From();
    const auto to = xmv.To();
    const unsigned n = xmv.NCards();
    Pile& toPile = *_allPiles[to];
    Pile& fromPile = *_allPiles[from];
    if (from == Stock || to == Stock)
        toPile.Draw(fromPile, n);
    else
        toPile.Take(fromPile, n);
    if (fromPile.empty() && fromPile.IsTableau())
        _kingSpaces += 1;
    toPile.IncrUpCount(n);
    fromPile.IncrUpCount(-n);
    if (xmv.Flip()){
        fromPile.SetUpCount(1);    // flip the top card
    }
}

bool Game::GameOver() const noexcept
{
    for (auto & f: _foundation) {
        if (f.size() < 13) return false;
    }
    return true;
}

// Return the height of the shortest foundation pile
unsigned Game::MinFoundationPileSize() const noexcept
{
    const auto& fnd = _foundation;
    unsigned result = fnd[0].size();
    for (int ifnd = 1; ifnd < 4; ifnd+=1) {
        const unsigned sz = fnd[ifnd].size();
        if (sz < result) { 
            result = sz;
        }
    }
    return result;
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
    for (int iPile = Waste; iPile<TableauBase+7 && result.empty(); iPile+=1) {
        const Pile &pile = *allPiles[iPile] ;
        if (pile.size()) {
            const Card& card = pile.back();
            const unsigned suit = card.Suit();
            if (card.Rank() <= minFoundationSize+1) {
                if (fnd[suit].size() == card.Rank()) {
                    const unsigned up = (iPile == Waste) ? 0 : pile.UpCount();
                    result.emplace_back(iPile,FoundationBase+suit,1,up);
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

    TalonFuture() {};
    TalonFuture(const Card& card, unsigned nMoves, int draw)
        : _card(card)
        , _nMoves(nMoves)
        , _drawCount(draw)
        {}
};

// Class to simulate draws and recycles of the talon and
// return the top card of the simulated waste pile.
class TalonSim{
    const PileVec& _waste;
    const PileVec& _stock;
    unsigned _wSize;
    unsigned _sSize;
public:
    TalonSim(const Game& game)
        : _waste(game.WastePile())
        , _stock(game.StockPile())
        , _wSize(game.WastePile().size())
        , _sSize(game.StockPile().size())
        {}
    unsigned WasteSize() const
    {
        return _wSize;
    }
    unsigned StockSize() const
    {
        return _sSize;
    }
    void Cycle()
    {
        _sSize += _wSize;
        _wSize = 0;
    }
    void Draw(unsigned n)
    {
        _wSize += n;
        _sSize -= n;
    }
    Card TopCard() const
    {
        if (_wSize <= _waste.size()){
            return _waste[_wSize-1];
        } else {
            return *(_stock.end()-(_wSize-_waste.size()) );
        }
    }
};

// Return a vector of all the cards that can be played
// from the talon (the stock and waste piles), along
// with the number of moves required to reach each one
// and the number of cards that must be drawn (or undrawn)
// to reach each one.
//
// Enforces the limit on recycles
typedef frystl::static_vector<TalonFuture,24> TalonFutureVec;
static TalonFutureVec TalonCards(const Game & game)
{
    TalonFutureVec result;
    const unsigned talonSize = game.WastePile().size() + game.StockPile().size();
    if (talonSize == 0) return result;

    TalonSim talon(game);
    const unsigned originalWasteSize = talon.WasteSize();
    const unsigned drawSetting = game.DrawSetting();
    unsigned nMoves = 0;
    unsigned nRecycles = 0;

    do {
        if (talon.WasteSize()) {
            result.emplace_back(talon.TopCard(), nMoves, 
                talon.WasteSize()-originalWasteSize);
        }	
        if (talon.StockSize()) {
            // Draw from the stock pile
            nMoves += 1;
            talon.Draw( std::min<unsigned>(drawSetting,talon.StockSize()));
        } else {
            if (game.RecycleCount() < game.RecycleLimit()){
                // Recycle the waste pile
                nRecycles += 1;
                talon.Cycle();
            } else {
                nRecycles = 3;      // kluge to end loop
            }
        }
    } while (talon.WasteSize() != originalWasteSize && nRecycles < 2);
    return result;
}

// Push a talon move onto a sequence.
// This is to visually distinguish talon Move construction from
// non-talon Move construction in AvailableMoves().
static inline void PushTalonMove(const TalonFuture& f, unsigned pileNum, bool recycle, QMoves& qm)
{
    qm.emplace_back(pileNum, f._nMoves+1, f._drawCount);
    qm.back().SetRecycle(recycle);
}

// Return true if any more empty columns are needed for kings
bool Game::NeedKingSpace() const noexcept
{
    return _kingSpaces < 4;
}

// If any short-foundation moves exist, returns one of those.
// Otherwise, returns a list of moves that are legal and not
// known to be wasted.  Rather than generate individual draws from
// stock to waste, it generates Move objects that represent one or more
// draws and that expose a playable top waste card and then play that card.
QMoves Game::AvailableMoves() noexcept
{
    QMoves result;

    const unsigned minFoundationSize = MinFoundationPileSize();
    if (minFoundationSize == 13) return result;		// game over
    result = ShortFoundationMove(*this,minFoundationSize);
    if (result.size()) return result;

    // Look for moves from tableau piles.
    for (const Pile& fromPile: _tableau) {
        // skip empty from piles
        if (fromPile.empty()) continue;

        const Card fromTip = fromPile.back();
        const Card fromBase = fromPile.Top();
        const auto upCount = fromPile.UpCount();

        // look for moves from tableau to foundation
        const Pile& foundation = _foundation[fromTip.Suit()];
        if (foundation.size() == fromTip.Rank()) {
            result.emplace_back(fromPile.Code(),foundation.Code(),1,upCount);
        }

        // Look for moves between tableau piles.  These may involve
        // moving multiple cards.
        bool kingMoved = false;     // prevents moving the same king twice
        for (const Pile& toPile: _tableau) {
            if (&fromPile == &toPile) continue;

            if (toPile.empty()) { 
                if (!kingMoved 
                        && fromBase.Rank() == King 
                        && fromPile.size() > upCount) {
                    // toPile is empty, a king sits atop fromPile's face-up
                    // cards, and it is covering at least one face-down card.
                    result.emplace_back(fromPile.Code(),toPile.Code(),upCount,upCount);
                    kingMoved = true;
                }
            } else {
                // Other moves follow the opposite-color-and-next-lower-rank rule.
                // We move from one tableau pile to another only to 
                // (a) move all the face-up cards on the from pile to 
                //		(1) expose a face-down card, or
                //		(2) make an empty column, or
                // (b) expose a card on the from pile that can be moved to a foundation
                // 	   pile.
                const Card cardToCover = toPile.back();
                const auto& fromCds = fromPile;
                // See whether any of the from pile's up cards can be moved
                // to the to pile.
                const unsigned toRank = cardToCover.Rank();
                if (fromTip.Rank() < toRank && toRank <= fromBase.Rank()+1U
                        && (fromTip.OddRed() == cardToCover.OddRed())){
                    // Some face-up card in the from pile covers the top card
                    // in the to pile, so a move is possible.
                    const unsigned moveCount = cardToCover.Rank() - fromTip.Rank();
                    assert(moveCount <= upCount);
                    if (moveCount==upCount && (upCount<fromPile.size() || NeedKingSpace())){
                        // This move will expose a face-down card or
                        // clear a column that's needed for a king.
                        // Move all the face-up cards on the from pile.
                        assert(fromBase.Covers(cardToCover));
                        result.emplace_back(fromPile.Code(),toPile.Code(),upCount,upCount);
                    } else if (moveCount < upCount || upCount < fromPile.size()) {
                        const Card exposed = *(fromCds.end()-moveCount-1);
                        if (exposed.Rank() == _foundation[exposed.Suit()].size()){
                            // This move will expose a card that can be moved to 
                            // its foundation pile.
                            assert((fromCds.end()-moveCount)->Covers(cardToCover));
                            result.emplace_back(fromPile.Code(),toPile.Code(),moveCount,upCount);
                        }
                    }
                }
            }
        }
    }
    // Look for move from waste to tableau or foundation, including moves that become available 
    // after one or more draws.  
    const TalonFutureVec talon(TalonCards(*this));
    for (TalonFuture talonCard : talon){

        // Stop generating talon moves if they require too many moves
        // and there are alternative moves.  The ungenerated moves will get
        // their chances later if we get that far before we find a minimum,
        // although that may require an extra move or more.
        if (result.size() > 1 && talonCard._nMoves > _talonLookAheadLimit) break;

        const unsigned cardSuit = talonCard._card.Suit();
        const unsigned cardRank = talonCard._card.Rank();
        bool recycle = talonCard._drawCount != int(talonCard._nMoves*DrawSetting());
        if (cardRank == _foundation[cardSuit].size()) {
            const unsigned pileNo = FoundationBase+cardSuit;
            PushTalonMove(talonCard, pileNo, recycle, result);
            if (cardRank <= minFoundationSize+1){
                if (_drawSetting == 1) {
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
            if ((tPile.size() > 0)) {
                if (talonCard._card.Covers(tPile.back())) {
                    PushTalonMove(talonCard, tPile.Code(), recycle, result);
                }
            } else if (cardRank == King) {
                PushTalonMove(talonCard, tPile.Code(), recycle, result);
                break;  // move that king to just one empty pile
            }
        }
    }

    // Look for moves from foundation piles to tableau piles,
    // even though they rarely appear in optimal solutions.
    for (const Pile& fPile: _foundation) {
        if (fPile.size() > minFoundationSize+1) {  
            const Card& top = fPile.back();
            for (const Pile& tPile: _tableau) {
                if (tPile.size() > 0) {
                    if (top.Covers(tPile.back())) {
                        result.emplace_back(fPile.Code(),tPile.Code(),1,0);
                    }
                } else {
                    if (top.Rank() == King) {
                        result.emplace_back(fPile.Code(),tPile.Code(),1,0);
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
    unsigned  mins[4] {14,14,14,14};
    unsigned result = 0;
    for (auto i = begin; i != end; i+=1){
        const unsigned rank = i->Rank();
        const unsigned suit = i->Suit();
        if (rank < mins[suit])
            mins[suit] = rank;
        else
            result += 1;
    }
    return result;
}

// Return a lower bound on the number of moves required to complete
// this game.  This function must return a result that does not 
// decrease by more than one after any single move.  The sum of 
// this result plus the number of moves made (from MoveCount())
// must never decrease when a new move is made (consistency).
// If it does, we won't know when to stop.
//
// From https://en.wikipedia.org/wiki/Consistent_heuristic:
//
//		In the study of path-finding problems in artificial 
//		intelligence, a heuristic function is said to be consistent, 
//		or monotone, if its estimate is always less than or equal 
//		to the estimated distance from any neighbouring vertex to 
//		the goal, plus the cost of reaching that neighbour.
unsigned Game::MinimumMovesLeft() const noexcept
{
    const unsigned draw = DrawSetting();
    const unsigned talonCount = _waste.size() + _stock.size();

    unsigned result = talonCount + QuotientRoundedUp(_stock.size(),draw);

    if (draw == 1) {
        // This can fail the consistency test for draw setting > 1.
        result += MisorderCount(_waste.begin(), _waste.end());
    }

    for (const Pile & tpile: _tableau) {
        if (tpile.size()) {
            const auto begin = tpile.begin();
            const unsigned downCount = tpile.size() - tpile.UpCount();
            result += tpile.size() + MisorderCount(begin, begin+downCount+1);
        }
    }
    return result;
}

// Return true if the cards in a sequence are in non-descending order by rank.
template <class Iter>
bool NonDescending(Iter begin, Iter end) noexcept
{
    if (end < begin+2) return true;
    for (auto cd = begin+1; cd < end; ++cd) {
        if ((cd-1)->Rank() > cd->Rank()) return false;
    }
    return true;
}

// Return true if the cards in a Pile are in non-descending order when
// scanned from back to front.
static bool NonDescending(const Pile & pile)
{
    auto& cards = pile.Cards();
    return NonDescending(cards.rbegin(),cards.rend());
}

// Return true only if it is clear that the game can be completed in the number of
// moves returned by MinimumMovesLeft().  False negatives are allowed.
bool Game::MinMoveSeqExists() const noexcept
{
    if (!NonDescending(_waste)) return false;
    for (auto& p: _tableau) {
        auto cds = p.Cards();
        if (p.UpCount() < p.size() && 
            !NonDescending(cds.rbegin()+p.UpCount()-1, cds.rend())) 
            return false;
    }
    return IsStockReady();
}

// Returns true if we can tell that the talon cards can all be
// moved to the foundation without recycling or moving any
// cards onto the tableau.  
// Assumes waste pile is in nondescending order.
bool Game::IsStockReady() const noexcept
{
    if (_stock.size() < 2) {
        return true;
    } else if (_drawSetting == 1) {
        return NonDescending(_stock.begin(),_stock.end());
    } else {
        // Test whether code to play from the talon in minimum moves
        // will work with a non-trivial stock pile and
        // _drawSetting > 1.  
        PileVec wst(_waste.Cards());
        PileVec stk(_stock.Cards());
        unsigned lastRank = Ace;
        while (!stk.empty()) {
            int draw = std::min(DrawSetting(),stk.size());
            unsigned peek = (stk.end()-draw)->Rank();
            // Check for first draw card too low
            if (peek < lastRank) return false;
            // Play waste cards until stack card is lower 
            // than next waste card.
            while (!wst.empty() && wst.back().Rank() <= peek) {
                lastRank = wst.back().Rank();
                wst.pop_back();
            }
            // check order of next draw
            if (!NonDescending(stk.end()-draw, stk.end()))
                return false;
            // check for draw bracketing top waste card
            if (!wst.empty() && stk.back().Rank() > wst.back().Rank()) 
                return false;
            // draw from stock
            for (int i = 0; i < draw; ++i)
            {
                wst.push_back(stk.back());
                stk.pop_back();
            }
        }
        return true;
    }
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
        const unsigned from = mv.From();
        const unsigned to = mv.To();
        
        if (!mv.IsTalonMove()){
            unsigned n = mv.NCards();
            bool flip = false;
            if (IsTableau(from)) {
                assert(totalCount[from-TableauBase] >= n);
                assert(upCount[from-TableauBase] >= n);
                totalCount[from-TableauBase] -= n;
                upCount[from-TableauBase] -= n;
                if (totalCount[from-TableauBase] && !upCount[from-TableauBase]){
                    flip = true;
                    upCount[from-TableauBase] = 1;
                }
            }
            if (IsTableau(to)) {
                totalCount[to-TableauBase] += n;
                upCount[to-TableauBase] += n;
            }
            mvnum += 1;
            result.emplace_back(mvnum,from, to, n,flip);
            if (mv.From() == Waste){
                assert (wasteSize >= 1);
                wasteSize -= 1;
            }
        } else {
            assert(stockSize+wasteSize > 0);
            unsigned nTalonMoves = mv.NMoves()-1;
            const unsigned stockMovesLeft = QuotientRoundedUp(stockSize,draw);
            if (nTalonMoves > stockMovesLeft && stockSize) {
                // Draw all remaining cards from stock
                mvnum += 1;
                result.emplace_back(mvnum,Stock,Waste,stockSize,false);
                mvnum += stockMovesLeft-1;
                wasteSize += stockSize;
                stockSize = 0;
                nTalonMoves -= stockMovesLeft;
            }
            if (nTalonMoves > 0) {
                mvnum += 1;
                if (stockSize == 0) {
                    // Recycle the waste pile
                    result.emplace_back(mvnum,Waste,Stock,wasteSize,false);
                    stockSize = wasteSize;
                    wasteSize = 0;
                }
                const unsigned nMoved = std::min<unsigned>(stockSize,nTalonMoves*draw);
                result.emplace_back(mvnum,Stock,Waste,nMoved,false);
                assert (stockSize >= nMoved && wasteSize+nMoved <= 24);
                assert(stockSize >= nMoved);
                stockSize -= nMoved;
                wasteSize += nMoved;
                assert(wasteSize <= 24);
                mvnum += nTalonMoves-1;
            }
            mvnum += 1;
            result.emplace_back(mvnum,Waste,to,1,false);
            assert(wasteSize >= 1);
            wasteSize -= 1;
            if (IsTableau(to)) {
                totalCount[to-TableauBase] += 1;
                upCount[to-TableauBase] += 1;
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
    for (auto ip = pile.begin(); ip < pile.end(); ip+=1)
    {
        char sep(' ');
        if (pile.IsTableau())
        {
            if (ip == pile.end()-pile.UpCount()) {sep = '|';}
        }
        outStr << sep << ip->AsString();
    }
    return outStr.str();
}

std::string Peek(const Move & mv)
{
    std::stringstream outStr;
    if (mv.IsTalonMove()){
        outStr << "+" << mv.NMoves() << "d" << mv.DrawCount();
        if (mv.Recycle()) {
            outStr << "c";
        }
        outStr << ">" << PileNames[mv.To()];
    } else {
        outStr << PileNames[mv.From()] << ">" << PileNames[mv.To()];
        unsigned n = mv.NCards();
        if (n != 1) outStr << "x" << n;
        if (mv.FromUpCount()) outStr << "u" << mv.FromUpCount();
    }
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

typedef std::array<uint32_t,7> TabStateT;

GameState::GameState(const Game& game) noexcept
{
    TabStateT tableauState;
    const auto& tableau = game.Tableau();
    for (unsigned i = 0; i<7; i+=1) {
        const auto& pile = tableau[i];
        const unsigned upCount = pile.UpCount();
        if (upCount == 0) {
            tableauState[i] = 0;
        } else {
            const auto& cards = pile;
            unsigned isMajor = 0;
            for (auto j = cards.end()-upCount+1;j < cards.end(); j+=1){
                isMajor = isMajor<<1 | unsigned(j->IsMajor());
            }
            const Card top = pile.Top();
            tableauState[i] = ((isMajor<<4 | upCount)<<4 | top.Rank())<<2 | top.Suit();
        }
    }
    std::sort(tableauState.begin(),tableauState.end());

    _part[0] = ((GameState::PartType(tableauState[0])<<21
                | tableauState[1])<<21) 
                | tableauState[2];
    _part[1] = ((GameState::PartType(tableauState[3])<<21
                | tableauState[4])<<21) 
                | tableauState[5];
    _part[2] = (tableauState[6]<<5) | game.StockPile().size();
    for (const auto& pile: game.Foundation()){
        _part[2] =_part[2]<<4 | pile.size();
    }
}
