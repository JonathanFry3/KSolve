#include "KSolveAStar.hpp"
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include <shared_mutex>		// for std::shared_timed_mutex, std::shared_lock
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h" 
#include "mf_vector.hpp"
#include "static_deque.hpp"
#include <thread>

typedef std::mutex Mutex;
typedef std::shared_timed_mutex SharedMutex;
typedef std::lock_guard<Mutex> Guard;
typedef std::shared_lock<SharedMutex> SharedGuard;
typedef std::lock_guard<SharedMutex> ExclusiveGuard;

using namespace frystl;

enum {maxMoves = 512};
typedef static_deque<Move,maxMoves> MoveSequenceType;

class SharedMoveStorage
{
    typedef std::uint32_t NodeX;
    struct MoveNode
    {
        Move _move;
        NodeX _prevNode;

        MoveNode(const Move& mv, NodeX prevNode)
            : _move(mv)
            , _prevNode(prevNode)
            {}
    };
    mf_vector<MoveNode,16*1024> _moveTree;
    Mutex _moveTreeMutex;
    // Stack of indexes to leaf nodes in _moveTree
    typedef mf_vector<NodeX> LeafNodeStack;
    // The leaf nodes waiting to grow new branches.  Each LeafNodeStack
    // stores nodes with the same minimum number of moves in any
    // completed game that can grow from them.  MoveStorage uses it
    // to implement a priority queue ordered by the minimum move count.
    mf_vector<LeafNodeStack,128> _fringe;
    SharedMutex _fringeMutex;
    mf_vector<Mutex,128> _fringeStackMutexes;
    unsigned _startStackIndex;
    bool _firstTime;
    friend class MoveStorage;
public:
    SharedMoveStorage() 
        : _startStackIndex(-1)
        , _firstTime(true)
    {
    }
};
class MoveStorage
{
    typedef SharedMoveStorage::NodeX NodeX;
    typedef SharedMoveStorage::MoveNode MoveNode;
    typedef SharedMoveStorage::LeafNodeStack LeafNodeStack;
    SharedMoveStorage &_shared;
    MoveSequenceType _currentSequence;
    NodeX _leafIndex;			// index of current sequence's leaf node in _moveTree
public:
    // Constructor.
    MoveStorage(SharedMoveStorage& shared);
    // Return a reference to the storage shared among threads
    SharedMoveStorage& Shared() const noexcept {return _shared;}
    // Push the given move to the back of the current sequence of moves.
    void Push(Move move);
    // Remove the last move from the current move sequence.
    void Pop() noexcept;
    // File the current move sequence under the given index number.
    // Calls after the first may not use an index less than the first.
    // Expects index > 0.
    void EnqueueMoveSequence(unsigned index);
    // Fetch a move sequence with the lowest available index, make it
    // the current move sequence and remove it from the queue.  
    // Return its index number, or zero if no more sequences are available.
    unsigned DequeueMoveSequence() noexcept; 
    // Make all the moves in the current sequence
    void MakeSequenceMoves(Game&game) const noexcept;
    // Return the current move sequence in a vector.
    Moves MovesVector() const;
    // Return a const reference to the current move sequence in its
    // native type.
    const MoveSequenceType& MoveSequence() const noexcept {return _currentSequence;}
};

struct WorkerState {
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
    typedef phmap::parallel_flat_hash_map< 
            GameState, 								// key type
            unsigned short, 						// mapped type
            Hasher,									// hash function
            phmap::priv::hash_default_eq<GameState>,// == function
            phmap::priv::Allocator<phmap::priv::Pair<GameState,unsigned short> >, 
            4U, 									// log2(n of submaps)
            Mutex									// mutex type
        > MapType;
    MapType& _closedList;
    unsigned _maxStates;

    Moves & _minSolution;
    static unsigned k_minSolutionCount;
    static Mutex k_minSolutionMutex;

    static bool k_blewMemory;

    explicit WorkerState(  Game & gm, 
            Moves& solution,
            SharedMoveStorage& sharedMoveStorage,
            MapType& map,
            unsigned maxStates)
        : _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(map)
        , _maxStates(maxStates)
        , _minSolution(solution)
        {
            _closedList.reserve(maxStates);
            _minSolution.clear();
            k_minSolutionCount = -1;
            k_blewMemory = false;
        }
    explicit WorkerState(const WorkerState& orig)
        : _game(orig._game)
        , _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _maxStates(orig._maxStates)
        , _minSolution(orig._minSolution)
        {}
            
    QMoves MakeAutoMoves() noexcept;
    void CheckForMinSolution();
    bool IsShortPathToState(unsigned minMoveCount);
    QMoves FilteredAvailableMoves()noexcept;
    void CompleteSolution();
private:
    void PlayTopOnMatch(unsigned rank, Pile& pile);
};
unsigned WorkerState::k_minSolutionCount(-1);
Mutex WorkerState::k_minSolutionMutex;
bool WorkerState::k_blewMemory(false);

static void Worker(
        WorkerState* pMasterState);

KSolveAStarResult KSolveAStar(
        Game& game,
        unsigned maxStates,
        unsigned nThreads)
{
    Moves solution;
    SharedMoveStorage sharedMoveStorage;
    WorkerState::MapType map;
    WorkerState state(game,solution,sharedMoveStorage,map,maxStates);

    const unsigned startMoves = state._game.MinimumMovesLeft();

    state._moveStorage.EnqueueMoveSequence(startMoves);	// pump priming
    
    if (nThreads == 0)
        nThreads = std::thread::hardware_concurrency();

    // Start workers in their own threads
    std::vector<std::thread> threads;
    threads.reserve(nThreads-1);
    for (unsigned t = 0; t < nThreads-1; ++t) {
        threads.emplace_back(&Worker, &state);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // Run one more worker in this (main) thread
    Worker(&state);

    for (auto& thread: threads) 
        thread.join();
    // Everybody's finished

    KSolveAStarCode outcome;
    if (state.k_blewMemory) {
        outcome = MemoryExceeded;
    } else if (solution.size()) { 
        outcome = game.TalonLookAheadLimit() < 24
                ? Solved
                : SolvedMinimal;
        state.CompleteSolution();
    } else {
        outcome = state._closedList.size() >= maxStates 
                ? GaveUp
                : Impossible;
    }
    return KSolveAStarResult(outcome,state._closedList.size(),solution);
}

void Worker(
        WorkerState* pMasterState)
{
    WorkerState state(*pMasterState);

    try {
        // Main loop
        unsigned minMoves0;
        while ( (state._closedList.size() < state._maxStates 
                || state.k_minSolutionCount != -1U)
                && !state.k_blewMemory
                && (minMoves0 = state._moveStorage.DequeueMoveSequence())    // <- side effect
                && minMoves0 < state.k_minSolutionCount) { 

            // Restore state._game to the state it had when this move
            // sequence was enqueued.
            state._game.Deal();
            state._moveStorage.MakeSequenceMoves(state._game);

            // Make all the no-choice moves.  Returns the first choice of moves
            // (the branches from next branching node) or an empty set.
            QMoves availableMoves = state.MakeAutoMoves();

            if (state._game.MinMoveSeqExists()) {
                // We have a solution.  See if it is a new champion
                state.CheckForMinSolution();
            } else if (!availableMoves.empty()) {
                const unsigned movesMadeCount = 
                    MoveCount(state._moveStorage.MoveSequence());

                // Save the result of each of the possible next moves.
                for (auto mv: availableMoves){
                    state._game.MakeMove(mv);
                    const unsigned made = movesMadeCount + mv.NMoves();
                    const unsigned remaining = 
                        state._game.MinimumMovesLeft();
                    assert(minMoves0 <= made+remaining);
                    if (state.IsShortPathToState(made)) {       // <- side effect
                        state._moveStorage.Push(mv);
                        state._moveStorage.EnqueueMoveSequence(made+remaining); 
                        state._moveStorage.Pop();
                    }
                    state._game.UnMakeMove(mv);
                }
            }
        }
    } 
    catch(std::bad_alloc&) {
        state.k_blewMemory = true;
    }
    return;
}

MoveStorage::MoveStorage(SharedMoveStorage& shared)
    : _shared(shared)
    , _leafIndex(-1)
    {}
void MoveStorage::Push(Move move)
{
    _currentSequence.push_back(move);
    NodeX ind;
    {
        Guard rupert(_shared._moveTreeMutex);
        ind = _shared._moveTree.size();
        _shared._moveTree.emplace_back(move, _leafIndex);
    }
    _leafIndex = ind;
}
void MoveStorage::Pop() noexcept
{
    _currentSequence.pop_back();
    _leafIndex = _shared._moveTree[_leafIndex]._prevNode;
}
void MoveStorage::EnqueueMoveSequence(unsigned index)
{
    if (_shared._firstTime) {
        _shared._firstTime = false;
        _shared._startStackIndex = index;
    }
    assert(_shared._startStackIndex <= index);
    const unsigned offset = index - _shared._startStackIndex;
    if (!(offset < _shared._fringe.size())) {
        // Grow the fringe as needed.
        ExclusiveGuard freddie(_shared._fringeMutex);
        while (!(offset < _shared._fringe.size())){
            _shared._fringeStackMutexes.emplace_back();
            _shared._fringe.emplace_back();
        }
    }
    assert(offset < _shared._fringe.size());
    {
        Guard clyde(_shared._fringeStackMutexes[offset]);
        _shared._fringe[offset].push_back(_leafIndex);
    }
}
unsigned MoveStorage::DequeueMoveSequence() noexcept
{
    unsigned offset;
    unsigned size;
    unsigned nTries;
    unsigned result = 0;
    _leafIndex = -1;
    // Find the first non-empty leaf node stack, pop its top into _leafIndex.
    //
    // It's not quite that simple with more than one thread, but that's the idea.
    // When we don't have a lock on it, any of the stacks may become empty or non-empty.
    for (nTries = 0; result == 0 && nTries < 5; ++nTries) {
        {
            SharedGuard marilyn(_shared._fringeMutex);
            size = _shared._fringe.size();
            for (offset = 0; offset < size && _shared._fringe[offset].empty(); ++offset) {}
        }
        if (offset < size) {
            Guard methuselah(_shared._fringeStackMutexes[offset]);
            auto & stack = _shared._fringe[offset];
            if (stack.size()) {
                _leafIndex = stack.back();
                stack.pop_back();
                result = offset+_shared._startStackIndex;
            }
        } 
        if (result == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    if (result) {
        // Follow the links to recover all of its preceding moves in reverse order.
        // Note that this operation requires no guard on _shared._moveTree only if 
        // the block vector in that structure never needs reallocation.
        _currentSequence.clear();
        for (NodeX node = _leafIndex; node != -1U; node = _shared._moveTree[node]._prevNode){
            const Move &mv = _shared._moveTree[node]._move;
            _currentSequence.push_front(mv);
        }
    }
    return result;
}
void MoveStorage::MakeSequenceMoves(Game&game) const noexcept
{
    for (auto & move: _currentSequence){
        game.MakeMove(move);
    }
}
Moves MoveStorage::MovesVector() const
{
    Moves result(_currentSequence.begin(), _currentSequence.end());
    return result;
}

// Make available moves until a branching node or an empty one is encountered.
// If more than one move is available but order will make no difference
// (as when two aces are dealt face up), FilteredAvailableMoves() will
// return them one at a time.
QMoves WorkerState::MakeAutoMoves() noexcept
{
    QMoves availableMoves;
    while ((availableMoves = FilteredAvailableMoves()).size() == 1)
    {
        _moveStorage.Push(availableMoves[0]);
        _game.MakeMove(availableMoves[0]);
    }
    return availableMoves;
}

// Return a vector of the available moves that pass the ABC_Move filter
QMoves WorkerState::FilteredAvailableMoves() noexcept
{
    QMoves availableMoves = _game.AvailableMoves();
    const auto& movesMade{_moveStorage.MoveSequence()};
    for (auto i = availableMoves.begin(); i < availableMoves.end(); ){
        if (ABC_Move(*i,movesMade)) {
            availableMoves.erase(i);
        } else {
            ++i;
        }
    }
    return availableMoves;
}

// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void WorkerState::CheckForMinSolution() {
    const unsigned nmv = MoveCount(_moveStorage.MoveSequence())
                         + _game.MinimumMovesLeft();
    {
        Guard karen(k_minSolutionMutex);
        if (nmv < k_minSolutionCount) {
            _minSolution = _moveStorage.MovesVector();
            k_minSolutionCount = nmv;
        }
    }
}

// Returns true if the current move sequence is the shortest path found
// so far to the current game state.
bool WorkerState::IsShortPathToState(unsigned moveCount)
{
    const GameState state{_game};
    bool valueChanged{false};
    const bool isNewKey = _closedList.try_emplace_l(
        state,						// key
        [&](auto& mappedValue) {	// run behind lock when key found
            valueChanged = moveCount < mappedValue;
            if (valueChanged) 
                mappedValue = moveCount;
        },
        moveCount 					// c'tor run behind lock when key not found
    );
    return isNewKey || valueChanged;
}

// The minimum solution is often left incomplete, as it can be 
// shown that a solution exists in k_minSolutionCount moves that
// this function can execute.  This version will finish the game
// iff the stock, waste, and all tableau piles are in nondescending
// order by rank, from back to front.
void WorkerState::CompleteSolution()
{
    _game.Deal();
    for (auto move: _minSolution) {
        _game.MakeMove(move);
    }
    auto & wst = _game.WastePile();
    auto & stk = _game.StockPile();
    auto & stCds = stk.Cards();
    auto drawSet = _game.DrawSetting();
    for (unsigned rank = _game.MinFoundationPileSize(); 
         rank <= King; ++rank) {
        int draw;
        unsigned wstTop = (wst.Empty()) ? King+1 : wst.Back().Rank();
        while ((draw = std::min(drawSet,stk.Size())) > 0
                && (stk.Cards().end()-draw)->Rank() < wstTop)
        {
            _minSolution.emplace_back(Waste, 1, draw);
            _game.MakeMove(_minSolution.back());
            wstTop = wst.Back().Rank();
        }
        PlayTopOnMatch(rank, wst);
        for (auto & pile: _game.Tableau()) {
            PlayTopOnMatch(rank, pile);
        }
    }
    assert(_game.GameOver());
    assert(k_minSolutionCount == MoveCount(_minSolution));
}

// If the top card in a pile has the given rank, add a move
// to _minSolution that moves that card to its foundation pile, 
// and make that move, which pops that top card.
void WorkerState::PlayTopOnMatch(const unsigned rank, Pile& pile)
{
    assert(pile.Code() != Stock);
    while (!pile.Empty()){
        const Card top = pile.Back();
        assert(rank <= top.Rank());
        if (top.Rank() != rank) return;
        const Pile& toPile = _game.Foundation()[top.Suit()];
        assert(toPile.Size() == rank);
        _minSolution.emplace_back(pile.Code(),toPile.Code(),1,pile.UpCount());
        _game.MakeMove(_minSolution.back());
    }
}
