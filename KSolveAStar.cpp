#include "KSolveAStar.hpp"
#include "GameStateMemory.hpp"
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>
#include <atomic>

namespace KSolveNames {

namespace ranges = std::ranges;
namespace views = ranges::views;

unsigned DefaultThreads()
{
    return std::thread::hardware_concurrency();
}

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
    void Start(size_t moveTreeSizeLimit, unsigned minMoves) noexcept
    {
        _moveTreeSizeLimit = moveTreeSizeLimit;
        _moveTree.reserve(moveTreeSizeLimit+1000);
        _startStackIndex = minMoves;
        _fringe.emplace_back();
        _fringe[0]._stack.push_back(-1);
    }

    FringeSizeType MaxFringeElementSize() const noexcept{
        return ranges::max_element(_fringe,{},
            [](const auto& f){return f._stack.MaxSize();})
            ->_stack.MaxSize();
    }

    unsigned MoveCount() const noexcept{
        return _moveTree.size();
    }
    bool OverLimit() const noexcept{
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

    NodeX UpdateMoveTree() noexcept; // Returns move tree index of first branch
    void UpdateFringe(NodeX branchIndex) noexcept;
public:
    // Constructor.
    MoveStorage(SharedMoveStorage& shared);
    // Return a reference to the storage shared among threads
    SharedMoveStorage& Shared() const noexcept {return _shared;}
    // Push a move to the back of the current stem.
    void PushStem(Move move) noexcept;
    // Push the first move of a new branch off the current stem,
    // along with the total number of moves to reach the end state
    // that move produces.
    void PushBranch(Move move, unsigned moveCount) noexcept;
    // Push all the moves (stem and branch) from this trip
    // through the main loop into shared storage.
    void ShareMoves();
    // Identify a move sequence with the lowest available minimum move count, 
    // return its minimum move count or, if no more sequences are available.
    // return 0. Remove that sequence from the open queue and make it current.
    unsigned PopNextSequenceIndex() noexcept;
    // Copy the moves in the current sequence from the move tree.
    void LoadMoveSequence() noexcept; 
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

    static bool k_blewMemory;

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
bool WorkerState::k_blewMemory(false);

static void Worker(
        WorkerState* pMasterState);

static void RunWorkers(unsigned nThreads, WorkerState & state)
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
        unsigned nThreads)
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
    if (state.k_blewMemory) {
        outcome = MemoryExceeded;
    } else if (solution.GetMoves().size()) { 
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
        sharedMoveStorage.MoveCount(),
        sharedMoveStorage.MaxFringeElementSize());
    ;
}

/*************************************************************************/
/*************************** Main Loop ***********************************/
/*************************************************************************/
static void Worker(
        WorkerState* pMasterState)
{
    WorkerState         state(*pMasterState);

    // Nicknames
    MoveStorage&        moveStorage {state._moveStorage};
    Game&               game {state._game};
    CandidateSolution&  minSolution {state._minSolution};
    GameStateMemory&    closedList{state._closedList};

    try {
        unsigned minMoves0;
        while ( !moveStorage.Shared().OverLimit()
                && !state.k_blewMemory
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
                    unsigned minRemaining = -1U;
                    bool pass = true;
                    if (!minSolution.IsEmpty()) { // rare condition
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
void MoveStorage::PushStem(Move move) noexcept
{
    _currentSequence.push_back(move);
}
void MoveStorage::PushBranch(Move mv, unsigned nMoves) noexcept
{
    assert(_shared._startStackIndex <= nMoves);
    _branches.emplace_back(mv,nMoves-_shared._startStackIndex);
}
void MoveStorage::ShareMoves()
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem nodes that led to it.
    if (_branches.size()) {
        NodeX branch0Index      // index in _moveTree of first branch
            = UpdateMoveTree();
        UpdateFringe(branch0Index);
    }
}

// Returns move tree index of first branch
MoveStorage::NodeX MoveStorage::UpdateMoveTree() noexcept
{
    NodeX stemEnd = _leafIndex;
    Guard rupert(_shared._moveTreeMutex);
    // Copy all the stem moves (if any) into the move tree.
    for (auto m: _currentSequence | views::drop(_startSize))
    {
        // Each stem node points to the previous node.
        _shared._moveTree.emplace_back(m, stemEnd);
        stemEnd =  _shared._moveTree.size() - 1;
    }
    // Now all the branches
    NodeX branch0Index = _shared._moveTree.size();
    for (const auto& br:_branches) {
        // Each branch node points to the last stem node.
        _shared._moveTree.emplace_back(br._mv, stemEnd);
    }
    return branch0Index;
} 
void MoveStorage::UpdateFringe(MoveStorage::NodeX branchIndex) noexcept
{
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

        Guard clyde(elem._mutex);
        elem._stack.push_back(branchIndex++);
    }
}
unsigned MoveStorage::PopNextSequenceIndex( ) noexcept
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
        auto & fringe = _shared._fringe;
        size = fringe.size();
        // Set offset to the index of the first non-empty leaf node stack
        // or size if all are empty.
        auto nonEmpty = [] (const auto & elem) {return !elem._stack.empty();};
        offset = ranges::find_if(fringe, nonEmpty) - fringe.begin();

        if (offset < size) {
            auto & stack = fringe[offset]._stack;
            {
                Guard methuselah(fringe[offset]._mutex);
                if (stack.size()) {
                    _leafIndex = stack.back();
                    stack.pop_back();
                    result = offset+_shared._startStackIndex;
                }
            }
        } 
        if (result == 0) {
            std::this_thread::yield();
        }
    }
    return result;
}
void MoveStorage::LoadMoveSequence() noexcept
{
    // Follow the links to recover all the moves in a sequence in reverse order.
    // Note that this operation requires no guard on _shared._moveTree only if 
    // the block vector in that structure never needs reallocation.
    _currentSequence.clear();
    for (NodeX node = _leafIndex; 
         node != -1U; 
         node = _shared._moveTree[node]._prevNode){
        const Move &mv = _shared._moveTree[node]._move;
        _currentSequence.push_front(mv);
    }

    _startSize = _currentSequence.size();
    _branches.clear();
}
void MoveStorage::MakeSequenceMoves(Game&game) const noexcept
{
    for (auto & move: _currentSequence){
        game.MakeMove(move);
    }
}

// Make available moves until a branching node or an childless one is encountered.
// If more than one move is available but order will make no difference
// (as when two aces are dealt face up), AvalaibleMoves() will
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

}   // namespace KSolveNames