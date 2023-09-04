#include "KSolveAStar.hpp"
#include "GameStateMemory.hpp"
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>

typedef std::mutex Mutex;
typedef std::lock_guard<Mutex> Guard;

using namespace frystl;

enum {maxMoves = 500};
typedef MoveCounter<static_deque<Move,maxMoves> > MoveSequenceType;

// Mix-in to measure max size
template <class VectorType>
class MaxSizeCollector : public VectorType
{
public:
    using size_type = typename VectorType::size_type;
    MaxSizeCollector() = default;

    size_type MaxSize() const {
        return std::max(VectorType::size(),_maxSize);
    }
    void pop_back()
    {
        Remember();
        VectorType::pop_back();
    }
private:
    size_type _maxSize {0};
    void Remember()
    {
        _maxSize = std::max(VectorType::size(),_maxSize);
    }
};

struct SharedMoveStorage
{
private:
    size_t _moveTreeSizeLimit;
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
    mf_vector<MoveNode,2*1024> _moveTree;
    Mutex _moveTreeMutex;
    // Stack of indexes to leaf nodes in _moveTree
    using LeafNodeStack  = MaxSizeCollector<mf_vector<NodeX,2*1024> >;
    using FringeSizeType = MaxSizeCollector<mf_vector<NodeX,2*1024> >::size_type;
    // The leaf nodes waiting to grow new branches.  Each LeafNodeStack
    // stores nodes with the same minimum number of moves in any
    // completed game that can grow from them.  MoveStorage uses it
    // to implement a priority queue ordered by the minimum move count.
    struct FringeElement {
        Mutex _mutex;
        LeafNodeStack _stack;
    };
    static_vector<FringeElement,256> _fringe;
    Mutex _fringeMutex;
    unsigned _startStackIndex {-1U};
    friend class MoveStorage;
public:
    // Start move storage with the minimum number of moves from
    // the dealt deck before the first move.
    void Start(size_t moveTreeSizeLimit, unsigned minMoves)
    {
        _moveTreeSizeLimit = moveTreeSizeLimit;
        _moveTree.reserve(moveTreeSizeLimit+1000);
        _startStackIndex = minMoves;
        _fringe.emplace_back();
        _fringe[0]._stack.push_back(-1);
    }
    FringeSizeType MaxFringeElementSize() const{
        FringeSizeType result = 0;
        for (const auto & f: _fringe) {
            result = std::max(result, f._stack.MaxSize());
        }
        return result;
    }
    unsigned MoveCount() const{
        return _moveTree.size();
    }
    bool OverLimit() const{
        return _moveTree.size() > _moveTreeSizeLimit;
    }
};
class MoveStorage
{
private:
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
    // Return a const reference to the current move sequence in its
    // native type.
    const MoveSequenceType& MoveSequence() const noexcept {return _currentSequence;}
};

class CandidateSolution
{
private:
    Moves _sol;
    unsigned _count {-1U};
    Mutex _mutex;
public:
    const Moves & GetMoves() const
    {
        return _sol;
    }
    unsigned MoveCount() const{
        return _count;
    }
    template <class Container>
    void ReplaceIfShorter(const Container& source, unsigned count)
    {
        Guard nikita(_mutex);
        if (_sol.empty() || count < _count){
            _sol.assign(source.begin(), source.end());
            _count = count;
        }
    }
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

    static bool k_blewMemory;

    explicit WorkerState(  Game & gm, 
            CandidateSolution& solution,
            SharedMoveStorage& sharedMoveStorage,
            GameStateMemory& map)
        : _game(gm)
        , _moveStorage(sharedMoveStorage)
        , _closedList(map)
        , _minSolution(solution)
        {
            k_blewMemory = false;
        }
    explicit WorkerState(const WorkerState& orig)
        : _game(orig._game)
        , _moveStorage(orig._moveStorage.Shared())
        , _closedList(orig._closedList)
        , _minSolution(orig._minSolution)
        {}
            
    QMoves MakeAutoMoves() noexcept;
    void CheckForMinSolution();
    QMoves FilteredAvailableMoves()noexcept;
};
bool WorkerState::k_blewMemory(false);

static void Worker(
        WorkerState* pMasterState);
/*************************************************************************/
/*************************** Entrance ************************************/
/*************************************************************************/
KSolveAStarResult KSolveAStar(
        Game& game,
        unsigned moveTreeLimit,
        unsigned nThreads)
{
    SharedMoveStorage sharedMoveStorage;
    GameStateMemory map;
    CandidateSolution solution;
    WorkerState state(game,solution,sharedMoveStorage,map);

    const unsigned startMoves = state._game.MinimumMovesLeft();

    state._moveStorage.Shared().Start(moveTreeLimit,startMoves);	// pump priming
    
    if (nThreads == 0)
        nThreads = 2*std::thread::hardware_concurrency();

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
    
    KSolveAStarCode outcome;
    if (state.k_blewMemory) {
        outcome = MemoryExceeded;
    } else if (solution.GetMoves().size()) { 
        outcome = game.TalonLookAheadLimit() < 24 
                  || sharedMoveStorage.OverLimit()
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
        sharedMoveStorage.MoveCount(),
        sharedMoveStorage.MaxFringeElementSize());
    ;
}

void Worker(
        WorkerState* pMasterState)
{
    WorkerState state(*pMasterState);

    try {
        // Main loop
        unsigned minMoves0;
        while ( !state._moveStorage.Shared().OverLimit()
                && !state.k_blewMemory
                && (minMoves0 = state._moveStorage.DequeueMoveSequence())    // <- side effect
                && minMoves0 < state._minSolution.MoveCount()) { 

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
                    state.CheckForMinSolution();
                }
            } else {
                const unsigned movesMadeCount = 
                    state._moveStorage.MoveSequence().MoveCount();

                // Save the result of each of the possible next moves.
                for (auto mv: availableMoves){
                    state._game.MakeMove(mv);
                    const unsigned made = movesMadeCount + mv.NMoves();
                    unsigned minRemaining = -1U;
                    bool pass = true;
                    if (state._minSolution.MoveCount() != -1U) { // rare copndition
                        minRemaining = state._game.MinimumMovesLeft(); // expensive
                        pass = (made + minRemaining) < state._minSolution.MoveCount();
                    }
                    if (pass && state._closedList.IsShortPathToState(state._game, made)) { // <- side effect
                        if (minRemaining == -1U) minRemaining = state._game.MinimumMovesLeft();
                        const unsigned minMoves = made + minRemaining;
                        // The following assert tests the consistency of
                        // Game::MinimumMovesLeft(), our heuristic.  
                        // Never remove it.
                        assert(minMoves0 <= minMoves);
                        state._moveStorage.PushBranch(mv,minMoves);
                    }
                    state._game.UnMakeMove(mv);
                }
                // Share the moves made here
                state._moveStorage.ShareMoves();
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
        NodeX stemEnd = _leafIndex;
        NodeX branchIndex;      // index in _moveTree of branch
        {
            Guard rupert(_shared._moveTreeMutex);
            // Copy all the stem moves into the move tree.
            for (auto mvi = _currentSequence.begin()+_startSize;
                    mvi != _currentSequence.end();
                    ++mvi) {
                // Each stem node points to the previous node.
                _shared._moveTree.emplace_back(*mvi, stemEnd);
                stemEnd =  _shared._moveTree.size() - 1;
            }
            // Now all the branches
            branchIndex = _shared._moveTree.size();
            for (const auto& br:_branches) {
                // Each branch node points to the last stem node.
                _shared._moveTree.emplace_back(br._mv, stemEnd);
            }
        }
        // Update the fringe.
        auto & fringe = _shared._fringe;
        // Enlarge the fringe if needed.
        unsigned maxOffset = 
            std::max_element(_branches.cbegin(),_branches.cend())->_offset;
        if (fringe.size() <= maxOffset) {
            Guard freddie(_shared._fringeMutex);
            if (fringe.size() <= maxOffset)
                fringe.resize(maxOffset+1);
        }
        for (const auto &br: _branches) {
            auto & elem = fringe[br._offset];
            {
                Guard clyde(elem._mutex);
                elem._stack.push_back(branchIndex++);
            }
        }
    }
}
unsigned MoveStorage::DequeueMoveSequence() noexcept
{
    unsigned offset;
    unsigned size;
    unsigned result = 0;
    _leafIndex = -1;
    // Find the first non-empty leaf node stack, pop its top into _leafIndex.
    //
    // It's not quite that simple with more than one thread, but that's the idea.
    // When we don't have a lock on it, any of the stacks may become empty or non-empty.
    for (unsigned nTries = 0; result == 0 && nTries < 5; ++nTries) 
    {
        size = _shared._fringe.size();
        for (offset = 0; offset < size && _shared._fringe[offset]._stack.empty(); ++offset) {}

        if (offset < size) {
            Guard methuselah(_shared._fringe[offset]._mutex);
            auto & stack = _shared._fringe[offset]._stack;
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

// Make available moves until a branching node or an empty one is encountered.
// If more than one move is available but order will make no difference
// (as when two aces are dealt face up), FilteredAvailableMoves() will
// return them one at a time.
QMoves WorkerState::MakeAutoMoves() noexcept
{
    QMoves availableMoves;
    while ((availableMoves = FilteredAvailableMoves()).size() == 1)
    {
        _moveStorage.PushStem(availableMoves[0]);
        _game.MakeMove(availableMoves[0]);
    }
    return availableMoves;
}

// Return a vector of the available moves that pass the XYZ_Move filter
QMoves WorkerState::FilteredAvailableMoves() noexcept
{
    QMoves avail = _game.AvailableMoves();
    const auto& movesMade{_moveStorage.MoveSequence()};
    auto newEnd = std::remove_if(
        avail.begin(), 
        avail.end(),
        [&movesMade] (Move& move) 
            {return XYZ_Move(move, movesMade);});
    while (avail.end() != newEnd) avail.pop_back();
    return avail;
}

// A solution has been found.  If it's the first, or shorter than
// the current champion, we have a new champion
void WorkerState::CheckForMinSolution() {
    const unsigned nmv = _moveStorage.MoveSequence().MoveCount();
    _minSolution.ReplaceIfShorter(_moveStorage.MoveSequence(), nmv);
}
