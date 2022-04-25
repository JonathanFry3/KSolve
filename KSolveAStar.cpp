#include "KSolveAStar.hpp"
#include "FilteredAvailableMoves.hpp"
#include "SolutionStore.hpp"
#include "GameStateMemory.hpp"
#include <algorithm>        // for sort
#include <mutex>          	// for std::mutex, std::lock_guard
#include <shared_mutex>		// for std::shared_timed_mutex, std::shared_lock
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include <thread>
#include <atomic>

typedef std::mutex Mutex;
typedef std::shared_timed_mutex SharedMutex;
typedef std::lock_guard<Mutex> Guard;
typedef std::shared_lock<SharedMutex> SharedGuard;
typedef std::lock_guard<SharedMutex> ExclusiveGuard;

using namespace frystl;

enum {maxMoves = 512};
typedef MoveCounter<static_deque<Move,maxMoves> > MoveSequenceType;

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
    struct FringeElement {
        Mutex _mutex;
        LeafNodeStack _stack;
    };
    mf_vector<FringeElement,128> _fringe;
    SharedMutex _fringeMutex;
    std::atomic_uint _fringeSize;
    unsigned _startStackIndex;
    friend class MoveStorage;
public:
    SharedMoveStorage() 
        : _startStackIndex(-1)
        , _fringeSize(0)
    {
    }
    // Start move storage with the minimum number of moves from
    // the dealt deck before the first move.
    void Start(unsigned minMoves)
    {
        _startStackIndex = minMoves;
        _fringe.emplace_back();
        _fringe[0]._stack.push_back(-1);
        ++_fringeSize;
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
    unsigned _startSize;
    struct MovePair
    {
        Move _mv;
        std::uint32_t _offset;
        MovePair(Move mv, unsigned offset)
            : _mv(mv)
            , _offset(offset)
        {}
        bool operator< (const MovePair& rhs) const
        {
            return _offset < rhs._offset;
        }
    };
    static_vector<MovePair,128> _branches;
public:
    // Constructor.
    MoveStorage(SharedMoveStorage& shared);
    // Return a reference to the storage shared among threads
    SharedMoveStorage& Shared() const noexcept {return _shared;}
    // Push a move to the back of the current stem.
    void PushStem(Move move);
    // Push the first move of a new branch off the current stem,
    // along with the total number of moves to reach the end state
    // that move produces.
    void PushBranch(Move move, unsigned moveCount);
    // Push all the moves (stem and branch) from this trip
    // through the main loop into shared storage.
    void ShareMoves();
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
    GameStateMemory& _closedList;

    SolutionStore &_solutionStore;

    bool & _blewMemory;

    explicit WorkerState(  Game & gm, 
            SolutionStore& solution,
            SharedMoveStorage& sharedMoveStorage,
            GameStateMemory& map,
            bool blewMemory)
        : _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(map)
        , _solutionStore(solution)
        , _blewMemory(blewMemory)
        {}
    explicit WorkerState(const WorkerState& orig)
        : _game(orig._game)
        , _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _solutionStore(orig._solutionStore)
        , _blewMemory(orig._blewMemory)
        {}
            
    QMoves MakeAutoMoves() noexcept;
};


static void Worker(
        WorkerState* pMasterState);

KSolveResult KSolveAStar(
        Game& game,
        unsigned maxStates,
        unsigned nThreads)
{
    // References to these object are all shared among threads through a WorkerState copy
    SolutionStore solution;
    bool blewMemory(false);
    SharedMoveStorage sharedMoveStorage;
    GameStateMemory map(maxStates);

    WorkerState state(game,solution,sharedMoveStorage,map,blewMemory);

    const unsigned startMoves = state._game.MinimumMovesLeft();

    state._moveStorage.Shared().Start(startMoves);	// pump priming
    
    if (nThreads == 0)
        nThreads = std::thread::hardware_concurrency();

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

    KSolveResultCode outcome;
    if (blewMemory) {
        outcome = MemoryExceeded;
    } else if (solution.AnySolution()) { 
        outcome = game.TalonLookAheadLimit() < 24
                ? Solved
                : SolvedMinimal;
    } else {
        outcome = state._closedList.OverLimit()
                ? GaveUp
                : Impossible;
    }
    return KSolveResult(outcome,
                        state._closedList.size(),
                        solution.MinimumSolution());
}

void Worker(
        WorkerState* pMasterState)
{
    WorkerState state(*pMasterState);
    static int nTrips = 0;
    ++nTrips;

    try {
        // Main loop
        unsigned minMoves0;
        while ( (!state._closedList.OverLimit()
                || state._solutionStore.AnySolution())
                && !state._blewMemory
                && (minMoves0 = state._moveStorage.DequeueMoveSequence())    // <- side effect
                && minMoves0 < state._solutionStore.MinMoves()) { 

            // Restore state._game to the state it had when this move
            // sequence was enqueued.
            state._game.Deal();
            state._moveStorage.MakeSequenceMoves(state._game);

            // Make all the no-choice (stem) moves.  Returns the first choice of moves
            // (the branches from next branching node) or an empty set.
            QMoves availableMoves = state.MakeAutoMoves();

            if (availableMoves.empty()) {
                if (state._game.GameOver()) {
                    // We have a solution.  See if it is a new champion
                    state._solutionStore.CheckSolution(
                        state._moveStorage.MoveSequence(),
                        state._moveStorage.MoveSequence().MoveCount());
                }
                // else dead end
            } else {
                const unsigned movesMadeCount = 
                    state._moveStorage.MoveSequence().MoveCount();

                // Save the result of each of the possible next moves.
                for (auto mv: availableMoves){
                    state._game.MakeMove(mv);
                    const unsigned made = movesMadeCount + mv.NMoves();
                    if (state._closedList.IsShortPathToState(state._game,made)) { // <- side effect
                        const unsigned remaining = 
                            state._game.MinimumMovesLeft();
                        assert(minMoves0 <= made+remaining);
                        state._moveStorage.PushBranch(mv,made+remaining);
                    }
                    state._game.UnMakeMove(mv);
                }
            }
            // Share the moves made here
            state._moveStorage.ShareMoves();
        }
    } 
    catch(std::bad_alloc&) {
        state._blewMemory = true;
    }
    return;
}

MoveStorage::MoveStorage(SharedMoveStorage& shared)
    : _shared(shared)
    , _leafIndex(-1)
    , _startSize(0)
    {}
void MoveStorage::PushStem(Move move)
{
    _currentSequence.push_back(move);
}
void MoveStorage::PushBranch(Move mv, unsigned nMoves)
{
    assert(_shared._startStackIndex <= nMoves);
    _branches.emplace_back(mv,nMoves-_shared._startStackIndex);
}
void MoveStorage::ShareMoves()
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem nodes that led to it.
    if (_branches.size()) {
        NodeX branchIndex;      // index in _moveTree of branch
        {
            Guard rupert(_shared._moveTreeMutex);
            // Copy all the stem moves into the move tree.
            for (auto mvi = _currentSequence.begin()+_startSize;
                    mvi != _currentSequence.end();
                    ++mvi) {
                // Each stem node points to the previous node.
                NodeX ind = _shared._moveTree.size();
                _shared._moveTree.emplace_back(*mvi, _leafIndex);
                _leafIndex = ind;
            }
            // Now all the branches
            branchIndex = _shared._moveTree.size();
            for (const auto& br:_branches) {
                // Each branch node points to the last stem node.
                _shared._moveTree.emplace_back(br._mv, _leafIndex);
            }
        }
        // Update the fringe.
        auto & fringe = _shared._fringe;
        // Enlarge the fringe if needed.
        unsigned maxOffset = 
            std::max_element(_branches.cbegin(),_branches.cend())->_offset;
        if (fringe.size() <= maxOffset) {
            ExclusiveGuard freddie(_shared._fringeMutex);
            while (fringe.size() <= maxOffset)
                fringe.emplace_back();
        }
        for (const auto &br: _branches) {
            auto & elem = fringe[br._offset];
            {
                Guard clyde(elem._mutex);
                elem._stack.push_back(branchIndex++);
                ++_shared._fringeSize;
            }
        }
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
            for (offset = 0; offset < size && _shared._fringe[offset]._stack.empty(); ++offset) 
            {}
        }
        if (offset < size) {
            Guard methuselah(_shared._fringe[offset]._mutex);
            auto & stack = _shared._fringe[offset]._stack;
            if (stack.size()) {
                _leafIndex = stack.back();
                stack.pop_back();
                --_shared._fringeSize;
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
        _startSize = _currentSequence.size();
        _branches.clear();
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
    auto & moves{_moveStorage.MoveSequence()};
    while ((availableMoves = FilteredAvailableMoves(_game, moves)).size() == 1)
    {
        _moveStorage.PushStem(availableMoves[0]);
        _game.MakeMove(availableMoves[0]);
    }
    return availableMoves;
}
