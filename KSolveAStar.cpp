#include "KSolveAStar.hpp"
#include "GameStateMemory.hpp"
#include "MoveStorage.hpp"
#include <thread>

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

    explicit WorkerState(  Game & gm, 
            CandidateSolution& solution,
            SharedMoveStorage& sharedMoveStorage,
            GameStateMemory& closed)
        : _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(closed)
        , _minSolution(solution)
        {}
    explicit WorkerState(const WorkerState& orig)
        : _game(orig._game)
        , _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _minSolution(orig._minSolution)
        {}
            
    QMoves MakeAutoMoves() noexcept;
};

// Make available moves until a branching node or an childless one is
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

/*************************************************************************/
/*************************** Main Loop ***********************************/
/*************************************************************************/
static void Worker(
        WorkerState* pMasterState) noexcept
{
    WorkerState         state(*pMasterState);

    // Nicknames
    MoveStorage&        moveStorage {state._moveStorage};
    Game&               game {state._game};
    CandidateSolution&  minSolution {state._minSolution};
    GameStateMemory&    closedList{state._closedList};

    unsigned minMoves0;
    while ( !moveStorage.Shared().OverLimit()
            && (minMoves0 = moveStorage.PopNextSequenceIndex())    // <- side effect
            && minMoves0 < minSolution.MoveCount()) { 

        // Restore game to the state it had when this move
        // sequence was enqueued.
        game.Deal();
        moveStorage.LoadMoveSequence();
        moveStorage.MakeSequenceMoves(game);

        // Make all the no-choice (stem) moves.  Returns the first choice of moves
        // (the branches from next branching node) or an empty set.
        QMoves availableMoves = state.MakeAutoMoves();

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
            for (auto mv: availableMoves){
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
                if (!minSolution.IsEmpty()) { 
                    minRemaining = game.MinimumMovesLeft(); // expensive
                    pass = (made + minRemaining) < minSolution.MoveCount();
                }
                if (pass && closedList.IsShortPathToState(game, made)) { // <- side effect
                    if (minRemaining == -1U) minRemaining = game.MinimumMovesLeft();
                    const unsigned minMoves = made + minRemaining;
                    // The following assert tests the consistency of
                    // Game::MinimumMovesLeft(), our heuristic.  
                    // Never remove it.
                    assert(minMoves0 <= minMoves);
                    moveStorage.PushBranch(mv,minMoves);
                }
                game.UnMakeMove(mv);
            }
            // Share the moves made here
            moveStorage.ShareMoves();
        }
    } 
    return;
}

static void RunWorkers(unsigned nThreads, WorkerState & state) noexcept
{
    if (nThreads == 0)
        nThreads = DefaultThreads();

    // Start workers in their own threads
    std::vector<std::thread> threads;
    threads.reserve(nThreads-1);
    for (unsigned t = 0; t < nThreads-1; ++t) {
        threads.emplace_back(&Worker, &state);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
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
    SharedMoveStorage sharedMoveStorage;
    GameStateMemory closed;
    CandidateSolution solution;
    WorkerState state(game,solution,sharedMoveStorage,closed);

    const unsigned startMoves = state._game.MinimumMovesLeft();

    // Prime the pump
    state._moveStorage.Shared().Start(moveTreeLimit,startMoves);
    
    RunWorkers(nThreads, state);
    
    KSolveAStarCode outcome;
    if (solution.GetMoves().size()) { 
        outcome = sharedMoveStorage.OverLimit()
                ? Solved
                : SolvedMinimal;
    } else {
        outcome = sharedMoveStorage.OverLimit()
                ? GaveUp
                : Impossible;
    }
    return KSolveAStarResult(
        outcome,
        solution.GetMoves(),
        state._closedList.Size(),
        sharedMoveStorage.MoveTreeSize(),
        sharedMoveStorage.FringeSize());
    ;
}

}   // namespace KSolveNames