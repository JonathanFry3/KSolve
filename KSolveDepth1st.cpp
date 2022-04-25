#include "KSolveDepth1st.hpp"
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>
#include <shared_mutex>		// for std::shared_timed_mutex, std::shared_lock
#include <atomic>
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h" 
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include "GameStateMemory.hpp"
#include "SolutionStore.hpp"
#include "FilteredAvailableMoves.hpp"

typedef std::mutex Mutex;
typedef std::shared_timed_mutex SharedMutex;
typedef std::lock_guard<Mutex> Guard;
typedef std::shared_lock<SharedMutex> SharedGuard;
typedef std::lock_guard<SharedMutex> ExclusiveGuard;

using namespace frystl;

struct Depth1stWorkerState {
    Game _game;
    MoveCounter<Moves> _currentSequence;
    // _closedList remembers the minimum move count at each game state we have
    // already visited.  If we get to that state again, we look at the current minimum
    // move count. If it is lower than the stored count, we keep our current node and store
    // its move count here.  If not, we forget the current node - we already have a
    // way to get to the same state that is at least as short.
    GameStateMemory& _closedList;
    unsigned _threadLimit;
    SolutionStore &_solutionStore;
    std::atomic_uint& _nThreads;

    bool& _blewMemory;

    explicit Depth1stWorkerState(  Game & gm, 
            unsigned threadLimit,
            std::atomic_uint& nThreads,
            SolutionStore& solution,
            GameStateMemory& map,
            bool& blewMemory)
        : _game(gm)
        , _threadLimit(threadLimit)
        , _closedList(map)
        , _solutionStore(solution)
        , _nThreads(nThreads)
        , _blewMemory(blewMemory)
        {
            _currentSequence.reserve(128);
        }
    explicit Depth1stWorkerState(const Depth1stWorkerState& orig)
        = default;
    bool MoreThreadsOK() const {return _nThreads<_threadLimit;}
    QMoves MakeAutoMoves() noexcept;
};
static void Depth1stWorker(Depth1stWorkerState* pParentState);

static void CallWorkerInNewThread(Depth1stWorkerState* pState)
{
    ++(pState->_nThreads);
    Depth1stWorkerState stateCopy(*pState);
    std::thread new_thread(Depth1stWorker, &stateCopy);
    new_thread.join();
    --(pState->_nThreads);
}
void Depth1stWorker(Depth1stWorkerState* pParentState)
{
    Depth1stWorkerState& state(*pParentState);
    auto & movesMade(state._currentSequence);

    try {
        // Main loop
        if (!state._closedList.OverLimit() 
            && movesMade.MoveCount() <= state._solutionStore.MinMoves()
            && !state._blewMemory)
        {
            unsigned nMoves = movesMade.size();
            // Make all the no-choice (stem) Moves.  Returns the first choice of Moves
            // (the branches from next branching node) or an empty set.
            QMoves availableMoves = state.MakeAutoMoves();

            const unsigned movesMadeCount = movesMade.MoveCount();

            if (availableMoves.empty()) {
                if (state._game.GameOver()) {
                    // We have a solution.  See if it is a new champion
                    state._solutionStore.CheckSolution(
                        movesMade,
                        movesMadeCount);
                }
                // Otherwise, this is a dead end.
            } else {
                // Recursively walk down each branch.
                for (auto mv: availableMoves){
                    state._game.MakeMove(mv);
                    const unsigned made = movesMadeCount + mv.NMoves();
                    if (made < state._solutionStore.MinMoves() &&
                            state._closedList.IsShortPathToState(state._game,made)) {       // <- side effect
                        movesMade.push_back(mv);
                        if (state.MoreThreadsOK())
                            CallWorkerInNewThread(pParentState);
                        else
                            Depth1stWorker(pParentState);
                        movesMade.pop_back();
                    }
                    state._game.UnMakeMove(mv);
                }
            }
            while (nMoves < movesMade.size()) {
                state._game.UnMakeMove(movesMade.back());
                movesMade.pop_back();
            }
        }
    } 
    catch(std::bad_alloc&) {
        state._blewMemory = true;
    }
    return;
}

KSolveResult KSolveDepth1st(
        Game& game,
        unsigned maxStates,
        unsigned threadLimit)
{
    SolutionStore solution;
    GameStateMemory map(maxStates);
    bool blewMemory(false);
    std::atomic_uint nThreads(1);
    if (threadLimit == 0)
        threadLimit = std::thread::hardware_concurrency();
    Depth1stWorkerState state(game,threadLimit,nThreads,solution,map,blewMemory);


    // Start a parent worker in this (main) thread
    Depth1stWorker(&state);
    KSolveResultCode outcome;
    const Moves& moves(state._solutionStore.MinimumSolution());
    if (state._blewMemory) {
        outcome = MemoryExceeded;
    } else if (moves.size()) { 
        outcome = game.TalonLookAheadLimit() < 24 
        || state._closedList.OverLimit()
                ? Solved
                : SolvedMinimal;
    } else {
        outcome = state._closedList.OverLimit() 
                ? GaveUp
                : Impossible;
    }
    return KSolveResult(outcome,state._closedList.size(),moves);
}

// Make available moves until a branching node or an empty one is encountered.
// If more than one move is available but order will make no difference
// (as when two aces are dealt face up), FilteredAvailableMoves() will
// return them one at a time.
QMoves Depth1stWorkerState::MakeAutoMoves() noexcept   
{
    QMoves availableMoves;
    auto & moves{_currentSequence};
    while ((availableMoves = FilteredAvailableMoves(_game, moves)).size() == 1)
    {
        moves.push_back(availableMoves[0]);
        _game.MakeMove(availableMoves[0]);
    }
    return availableMoves;
}
