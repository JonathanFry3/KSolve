#include "KSolveAStar.hpp"
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include <shared_mutex>		// for std::shared_mutex, std::shared_lock
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h" 
#include "mf_vector.hpp"
#include "fixed_capacity_deque.hpp"
#include <thread>

typedef std::mutex Mutex;
typedef std::shared_timed_mutex SharedMutex;
typedef std::lock_guard<Mutex> Guard;
typedef std::shared_lock<SharedMutex> SharedGuard;
typedef std::lock_guard<SharedMutex> ExclusiveGuard;

enum {maxMoves = 512};
typedef fixed_capacity_deque<Move,maxMoves> MoveSequenceType;

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
    mf_vector<Mutex> _fringeStackMutexes;
    unsigned _startStackIndex;
    bool _firstTime;
    friend class MoveStorage;
public:
    SharedMoveStorage() 
        : _firstTime(true)
        , _startStackIndex(-1)
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
        : _minSolution(solution)
        , _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(map)
        , _maxStates(maxStates)
        {
            _closedList.reserve(maxStates);
            _minSolution.clear();
            k_minSolutionCount = -1;
            k_blewMemory = false;
        }
    explicit WorkerState(const WorkerState& orig)
        : _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _game(orig._game)
        , _minSolution(orig._minSolution)
        , _maxStates(orig._maxStates)
        {}
            
    QMoves MakeAutoMoves() noexcept;
    void CheckForMinSolution();
    bool IsShortPathToState(unsigned minMoveCount);
    bool SkippableMove(Move mv)noexcept;
    QMoves FilteredAvailableMoves()noexcept;
};
unsigned WorkerState::k_minSolutionCount(-1);
Mutex WorkerState::k_minSolutionMutex;
bool WorkerState::k_blewMemory(false);

static void KSolveWorker(
        WorkerState* pMasterState);

KSolveAStarResult KSolveAStar(
        Game& game,
        unsigned maxStates,
        unsigned nthreads)
{
    Moves solution;
    SharedMoveStorage sharedMoveStorage;
    WorkerState::MapType map;
    WorkerState state(game,solution,sharedMoveStorage,map,maxStates);

    const unsigned startMoves = state._game.MinimumMovesLeft();

    state._moveStorage.EnqueueMoveSequence(startMoves);	// pump priming
    
    if (nthreads == 0)
        nthreads = std::thread::hardware_concurrency();

    // Start workers in their own threads
    std::vector<std::thread> threads;
    threads.reserve(nthreads-1);
    for (unsigned ithread = 0; ithread < nthreads-1; ++ithread) {
        threads.emplace_back(&KSolveWorker, &state);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    // Run one more worker in this (main) thread
    KSolveWorker(&state);

    for (auto& thread: threads) 
        thread.join();
    // Everybody's finished

    KSolveAStarCode outcome;
    if (state.k_blewMemory) {
        outcome = MemoryExceeded;
    } else if (state._minSolution.size()) {
        outcome = game.TalonLookAheadLimit() < 24
                ? Solved
                : SolvedMinimal;
    } else {
        outcome = state._closedList.size() >= maxStates 
                ? GaveUp
                : Impossible;
    }
    return KSolveAStarResult(outcome,state._closedList.size(),solution);
}

void KSolveWorker(
        WorkerState* pMasterState)
{
    WorkerState state(*pMasterState);

    try {
        // Main loop
        unsigned minMoves0;
        while ( (state._closedList.size() < state._maxStates || state.k_minSolutionCount > 0)
                && !state.k_blewMemory
                && (minMoves0 = state._moveStorage.DequeueMoveSequence())    // <- side effect
                && minMoves0 < state.k_minSolutionCount) { 

            // Restore state._game to the state it had when this move
            // sequence was enqueued.
            state._game.Deal();
            state._moveStorage.MakeSequenceMoves(state._game);

            QMoves availableMoves = state.MakeAutoMoves();

            if (availableMoves.size() == 0 && state._game.GameOver()) {
                // We have a solution.  See if it is a new champion
                state.CheckForMinSolution();
                // See if it the final winner.  It nearly always is.
                if (minMoves0 == state.k_minSolutionCount)
                    break;
            }
            
            const unsigned movesMadeCount = MoveCount(state._moveStorage.MoveSequence());

            // Save the result of each of the possible next moves.
            for (auto mv: availableMoves){
                state._game.MakeMove(mv);
                const unsigned made = movesMadeCount + mv.NMoves();
                const unsigned remaining = state._game.MinimumMovesLeft();
                assert(minMoves0 <= made+remaining);
                if (state.IsShortPathToState(made)) {
                    state._moveStorage.Push(mv);
                    state._moveStorage.EnqueueMoveSequence(made+remaining); 
                    state._moveStorage.Pop();
                }
                state._game.UnMakeMove(mv);
            }
        }
    } 
    catch(std::bad_alloc) {
        state.k_blewMemory = true;
    }
    return;
}
MoveStorage::MoveStorage(SharedMoveStorage& shared)
    : _leafIndex(-1)
    , _shared(shared)
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
    for (nTries = 0; result == 0 && nTries < 5; nTries+=1) {
        {
            SharedGuard marilyn(_shared._fringeMutex);
            size = _shared._fringe.size();
            for (offset = 0; offset < size && _shared._fringe[offset].empty(); offset += 1) {}
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
        _currentSequence.clear();
        for (NodeX node = _leafIndex; node != -1; node = _shared._moveTree[node]._prevNode){
            const Move mv = _shared._moveTree[node]._move;
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

// Return a vector of the available moves that pass the SkippableMove filter
QMoves WorkerState::FilteredAvailableMoves() noexcept
{
    QMoves availableMoves = _game.AvailableMoves();
    for (auto i = availableMoves.begin(); i < availableMoves.end(); ){
        if (SkippableMove(*i)) {
            availableMoves.erase(i);
        } else {
            i += 1;
        }
    }
    return availableMoves;
}


// Return true if this move cannot be in a minimum solution.
bool WorkerState::SkippableMove(Move trial) noexcept
{
    // Consider a move at time T0 from A to B and the next move
    // from B, which goes to C at time Tn.  The move at Tn is
    // skippable if the same result could have been achieved 
    // at T0 by moving the same cards directly from A to C.

    // We are now at Tn looking back for a T0 move.  B is our from pile
    // and C is our to pile.  A candidate T0 move is one that moves
    // to our from pile (pile B).

    // Do those two moves move the same set of cards?.  Yes if
    // no intervening move has changed pile B and the two moves
    // move the same number of cards.

    // Was the move from A to C possible at T0? Yes if neither that move
    // nor an intervening move has changed pile C.

    // Since nothing says A cannot equal C, this test catches 
    // moves that exactly reverse previous moves.
    const auto B = trial.From();
    if (B == Stock || B == Waste) return false; 
    const auto C = trial.To();
    const auto &movesMade = _moveStorage.MoveSequence();
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

    // If you find another profitable way to filter available moves, you could
    // call this one ABC_Move. 
    //
    // AvailableMoves() generates moves among tableau files for only two purposes:
    // to move all the face-up cards, or to expose a card that can be moved to the 
    // foundation.  I have tried filtering out later moves that would re-cover a 
    // card that had been exposed in that fashion.  That did not break anything, but
    // cost more time than it saved.
    // Jonathan Fry 7/12/2020
}

// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void WorkerState::CheckForMinSolution() {
    const unsigned nmv = MoveCount(_moveStorage.MoveSequence());
    {
        Guard karen(k_minSolutionMutex);
        const unsigned x = _minSolution.size();
        if (x == 0 || nmv < k_minSolutionCount) {
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
