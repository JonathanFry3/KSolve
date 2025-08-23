#include "KSolveAStar.hpp"
#include "GameStateMemory.hpp"
#include "MoveStorage.hpp"
#include <thread>
#include <atomic>

namespace KSolveNames {

namespace ranges = std::ranges;
namespace views = ranges::views;

unsigned DefaultThreads() noexcept
{
    return std::thread::hardware_concurrency();
}

class CandidateSolution
{
private:
    Moves _sol;
    unsigned _count {-1U};
    Mutex _mutex;
public:
    const Moves & GetMoves() const noexcept
    {
        return _sol;
    }
    unsigned MoveCount() const noexcept{
        return _count;
    }
    template <class Container>
    void ReplaceIfShorter(const Container& source, unsigned count) noexcept
    {
        if (_sol.empty() || count < _count){
            Guard nikita(_mutex);
            if (_sol.empty() || count < _count){
                _sol.assign(source.begin(), source.end());
                _count = count;
            }
        }
    }
    bool IsEmpty() const noexcept {return _sol.empty();}
};

// Counts the number of times a card is higher in the stack
// than a lower card of the same suit.  Remember that the 
// stack tops are at the back.
template <class Iter>
unsigned MisorderCount(Iter begin, Iter end) noexcept
{
    unsigned  minRanks[SuitsPerDeck] {14,14,14,14};
    unsigned result = 0;
    for (auto i = begin; i != end; ++i){
        const auto rank = i->Rank();
        const auto suit = i->Suit();
        if (rank < minRanks[suit])
            minRanks[suit] = rank;
        else
            result++;
    }
    return result;
}

// Return a lower bound on the number of moves required to complete
// this game.  This function must return a result that does not 
// decrease by more than one after any single move.  The sum of 
// this result plus the number of moves made (from MoveCount())
// must never decrease when a new move is made (consistency).
// If it does, we may stop too soon.
//
// From https://en.wikipedia.org/wiki/Consistent_heuristic:
//
//		In the study of path-finding problems in artificial 
//		intelligence, a heuristic function is said to be consistent, 
//		or monotone, if its estimate is always less than or equal 
//		to the estimated distance from any neighbouring vertex to 
//		the goal, plus the cost of reaching that neighbour.
unsigned MinimumMovesLeft(const Game& game) noexcept
{
    const unsigned draw = game.DrawSetting();
    const unsigned talonCount = 
        game.WastePile().size() + game.StockPile().size();

    unsigned result = talonCount + QuotientRoundedUp(game.StockPile().size(),draw);

    if (draw == 1) {
        // This can fail the consistency test for draw setting > 1.
        result += MisorderCount(game.WastePile().begin(), game.WastePile().end());
    }

    for (const auto & tPile: game.Tableau()) {
        if (tPile.size()) {
            const auto begin = tPile.begin();
            const unsigned downCount = tPile.size() - tPile.UpCount();
            result += tPile.size() + MisorderCount(begin, begin+downCount+1);
        }
    }
    return result;
}
using AtomicUInt = std::atomic_uint;

struct WorkerState {
public:
    Game _game;
    // _moveStorage stores the portion of the move tree that has been generated.
    // Each node has a move and a reference to the node with
    // the move before it.  The leaves are indexed by the minimum number of 
    // moves possible in any finished game that might grow from that leaf.
    // _moveStorage also stores the sequence of moves we are currently working on.
    MoveStorage _moveStorage;
    // _closedList remembers the minimum move count at each game state we have
    // already visited.  If we get to that state again, we look at the current minimum
    // move count. If it is lower than the stored count, we keep our current node and store
    // its move count here.  If not, we forget the current node - we already have a
    // way to get to the same state that is at least as short.
    GameStateMemory& _closedList;
    CandidateSolution & _minSolution;
    AtomicUInt& _advances;

    explicit WorkerState(  Game & gm, 
            CandidateSolution& solution,
            SharedMoveStorage& sharedMoveStorage,
            GameStateMemory& closed,
            AtomicUInt& loopCount)
        : _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(closed)
        , _minSolution(solution)
        , _advances(loopCount)
        {}
    explicit WorkerState(const WorkerState& orig)
        : _game(orig._game)
        , _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _minSolution(orig._minSolution)
        , _advances(orig._advances)
        {}
            
    QMoves MakeAutoMoves() noexcept;
};

// Make available moves until a branching node or a childless one is
// encountered. If more than one dominant move is available
// (as when two aces are dealt face up), AvailableMoves() will
// return them one at a time.
QMoves WorkerState::MakeAutoMoves() noexcept
{
    QMoves availableMoves;
    while ((availableMoves = 
        _game.AvailableMoves(_moveStorage.MoveSequence())).size() == 1)
    {
        _moveStorage.PushStem(availableMoves[0]);
        _game.MakeMove(availableMoves[0]);
    }
    return availableMoves;
}
// *Advance()* starts from a game state and grows the tree to the next
// branching node.  It then pushes each qualifying child of that branching
// node into the work queue.
inline static void Advance(
        WorkerState& state,
        unsigned minMoves0) noexcept
{
    // Nicknames
    auto&  moveStorage  {state._moveStorage};
    auto&  game         {state._game};
    auto&  minSolution  {state._minSolution};
    auto&  closedList   {state._closedList};

    // Make all the no-choice (stem) moves.  Returns the first choice of moves
    // (the branches from next branching node) or an empty set.
    const QMoves availableMoves = state.MakeAutoMoves();

    const unsigned movesMadeCount = 
        moveStorage.MoveSequence().MoveCount();

    if (availableMoves.empty()) {
        // This could be a dead end or a win.
        if (game.GameOver()) {
            // We have a win.  See if it is a new champion
            minSolution.ReplaceIfShorter(
                moveStorage.MoveSequence(), movesMadeCount);
        }
    } else {
        // Save the result of each of the possible next moves.
        for (const auto mv: availableMoves){
            game.MakeMove(mv);
            const unsigned made = movesMadeCount + mv.NMoves();
            // The following rather convoluted logic for deciding whether or not
            // to save mv attempts to minimize time used in all situations. 
            // Both MinimumMovesLeft() and IsShortPathToState() are expensive,
            // but IsShortPathToState() is considerably the more expensive of
            // the two.  If we already have a solution to test against, we can
            // call MinimumMovesLeft() first to sometimes avoid calling
            // IsShortPathToState(). If not, the best we can do is call
            // IsShortPathToState() first to sometimes avoid calling
            // MinimumMovesLeft().
            unsigned minRemaining = -1U;
            bool pass = true;
            if (! minSolution.IsEmpty()) { 
                minRemaining = MinimumMovesLeft(game); // expensive
                pass = (made + minRemaining) < minSolution.MoveCount();
            }
            if (pass && closedList.IsShortPathToState(game, made)) { // <- side effect
                if (minRemaining == -1U) minRemaining = MinimumMovesLeft(game);
                const unsigned minMoves = made + minRemaining;
                // The following assert tests the consistency (monotonicity)
                // of MinimumMovesLeft(), our heuristic.  
                // Never remove it.
                assert(minMoves0 <= minMoves);
                moveStorage.PushBranch(mv,minMoves);
            }
            game.UnMakeMove(mv);
        }
        // Update shared data structures with the moves made here
        moveStorage.ShareMoves();
    }
}

/*************************************************************************/
/*************************** Main Loop ***********************************/
/*************************************************************************/
static void Worker(
        WorkerState* pMasterState) noexcept
{
    auto   state(*pMasterState);

    // Nicknames
    auto&  moveStorage  {state._moveStorage};
    auto&  game         {state._game};
    auto&  minSolution  {state._minSolution};

    unsigned myLoopCount{0};
    unsigned minMoves0;    
    while ( ! moveStorage.Shared().OverLimit()
            && (minMoves0 = moveStorage.PopNextBranch(game))    // <- side effect
            && minMoves0 < minSolution.MoveCount())
    {
        ++myLoopCount;
        Advance(state, minMoves0);
    }
    state._advances += myLoopCount;
    return;
}

static void RunWorkers(unsigned nThreads, WorkerState & state) noexcept
{
    // Put some work in the work queue by growing the tree from
    // the root to the first branching node.
    Advance(state, state._moveStorage.Shared().InitialMinMoves());

    if (nThreads == 0)
        nThreads = DefaultThreads();

    // Start workers in their own threads
    std::vector<std::thread> threads;
    threads.reserve(nThreads-1);
    for (unsigned t = 0; t < nThreads-1; ++t) {
        threads.emplace_back(&Worker, &state);
    }

    // Run one more worker in this (main) thread
    Worker(&state);

    for (auto& thread: threads) 
        thread.join();
    // Everybody's finished
}
/*************************************************************************/
/*************************** Entrance ************************************/
/*************************************************************************/
KSolveAStarResult KSolveAStar(
        Game& game,
        unsigned moveTreeLimit,
        unsigned nThreads) noexcept
{
    GameStateMemory closed;
    CandidateSolution solution;
    AtomicUInt loopCount{0};

    const unsigned startMoves = MinimumMovesLeft(game);
    SharedMoveStorage sharedMoveStorage(moveTreeLimit, startMoves);

    WorkerState state(game,solution,sharedMoveStorage,closed,loopCount);

    RunWorkers(nThreads, state);
    
    bool overLimit = sharedMoveStorage.OverLimit();
    KSolveAStarCode outcome;
    if (solution.GetMoves().size()) { 
        outcome = overLimit
                ? Solved
                : SolvedMinimal;
    } else {
        outcome = overLimit
                ? GaveUp
                : Impossible;
    }
    return KSolveAStarResult(
        outcome,
        solution.GetMoves(),
        state._closedList.Size(),
        sharedMoveStorage.MoveTreeSize(),
        sharedMoveStorage.FringeSize(),
        loopCount
    );
}

}   // namespace KSolveNames