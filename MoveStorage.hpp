#include "Game.hpp"
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>           // for std::this_thread::yield()

namespace KSolveNames {

using NodeX = std::uint32_t;
using Mutex = std::mutex;
using Guard = std::lock_guard<Mutex>;

// A ShareableIndexedPriorityQueue<I,V> is a thread-safe priority queue of
// {I,V} pairs in ascending order by their I values (approximately).  It is
// implemented as a vector indexed by the I values of stacks of V values.
// I must be an unsigned integer type.
// It is efficient only if the I values are all small integers.
//
// Pairs sharing the same I values are returned in LIFO order. 
template <typename I, typename V, unsigned Sz>
class ShareableIndexedPriorityQueue {
private:
    using StackT = mf_vector<V,1024>;
    struct ProtectedStackT {
        Mutex _mutex;
        StackT _stack;
    };
    
    Mutex _mutex;
    static_vector<ProtectedStackT, Sz>_stacks;

    void inline UpsizeTo(I newSize) noexcept
    {
        if (_stacks.size() < newSize) {
            Guard desposito(_mutex);
            if (_stacks.size() < newSize)
                _stacks.resize(newSize);
        }
    }

public:
    template <class... Args>
    void Emplace(I index, Args &&...args) noexcept
    {
        UpsizeTo(index+1);
        auto& pStack = _stacks[index];
        Guard esperanto(pStack._mutex);
        pStack._stack.emplace_back(std::forward<Args>(args)...);
    }
    void Push(I index, const V& value) noexcept
    {
        Emplace(index, value);
    }
    std::optional<std::pair<I,V>> Pop() noexcept
    {
        // Something like the Uncertainty Principle applies here: in a multithreaded
        // environment, since a stack may become empty or non-empty 
        // at any instant, which one is the first non-empty one may depend on 
        // which thread is looking and exactly when. It is thus impossible to
        // be certain what the correct return value is without stopping the running
        // of other threads. No attempt is made here to
        // eliminate that problem. In this application, it does no harm.   
        std::optional<std::pair<I,V>> result;
        for (unsigned nTries = 0; !result && nTries < 5; ++nTries) 
        {
            auto nonEmpty = [] (const ProtectedStackT & elem) 
                {return !elem._stack.empty();};
            unsigned index = ranges::find_if(_stacks,nonEmpty) - _stacks.begin();
            unsigned size = _stacks.size();

            if (index < size) {
                StackT & stack = _stacks[index]._stack;
                Guard methuselah(_stacks[index]._mutex);
                if (stack.size()) {
                    result = std::make_pair(index,stack.back());
                    stack.pop_back();
                }
            }
            if (!result) std::this_thread::yield();
        }
        return result;
    }
    // Returns total size.  Approximate if threads are making changes.
    unsigned Size() const noexcept
    {
        unsigned result{0};
        for (auto& prStack: _stacks) {result += prStack._stack.size();}
        return result;
    }
};

struct MoveNode
{
    MoveSpec _move;
    NodeX _prevNodeIndex{-1U};

    MoveNode() = default;
    MoveNode(const MoveSpec& mv, NodeX prevNode) noexcept
        : _move(mv)
        , _prevNodeIndex(prevNode)
        {}
};

class SharedMoveStorage
{
private:
    const size_t _moveTreeSizeLimit;
    std::vector<MoveNode> _moveTree;
    Mutex _moveTreeMutex;
    // The leaf nodes waiting to grow new branches.  
    // Also, the task queue.
    ShareableIndexedPriorityQueue<unsigned, MoveNode, 512> _fringe;
    const unsigned _initialMinMoves;
    friend class MoveStorage;
public:
    SharedMoveStorage(size_t moveTreeSizeLimit, unsigned minMoves) noexcept
        : _moveTreeSizeLimit(moveTreeSizeLimit)
        , _initialMinMoves(minMoves)
    {
        _moveTree.reserve(moveTreeSizeLimit+1000);
    }
    unsigned InitialMinMoves() const noexcept {
        return _initialMinMoves;
    }
    unsigned FringeSize() const noexcept{
        return _fringe.Size();
    }
    unsigned MoveTreeSize() const noexcept{
        return _moveTree.size();
    }
    bool OverLimit() const noexcept{
        return _moveTree.size() > _moveTreeSizeLimit;
    }
};

class MoveStorage
{
public:
    MoveStorage(SharedMoveStorage& shared) noexcept;
    // Return a reference to the storage shared among threads
    SharedMoveStorage& Shared() const noexcept {return _shared;}
    // Push a move to the back of the current stem.
    void PushStem(MoveSpec move) noexcept;
    // Push the first move of a new branch off the current stem,
    // along with the heuristic value associated with that move,
    // i.e. its the minimum move count.
    void PushBranch(MoveSpec move, unsigned moveCount) noexcept;
    // Push all the moves (stem and branch) from this trip
    // through the main loop into shared storage.
    void ShareMoves() noexcept;
    // If the work queue (aka fringe) is empty, return 0.
    // Otherwise, pop a move sequence with the lowest available
    // minimum move count, redeal the deck, make all the moves in
    // that sequence to return the game to the state it was in when
    // that sequence was saved, and return its minimum move count.
    unsigned PopNextMoveSequence(Game& game) noexcept;
    // Return a const reference to the current move sequence in its
    // native type.
    using MoveSequenceType = MoveCounter<static_deque<MoveSpec,500>>;
    const MoveSequenceType& MoveSequence() const noexcept {return _currentSequence;}
private:
    SharedMoveStorage &_shared;
    MoveSequenceType _currentSequence;
    MoveNode _leafNode{};	// current sequence's starting leaf node 
    unsigned _startSize{0}; // number of MoveSpecs gotten from the move tree.
    struct MovePair
    {
        MoveSpec _mv;
        std::uint32_t _offset;
        MovePair(MoveSpec mv, unsigned offset)
            : _mv(mv)
            , _offset(offset)
        {}
    };
    static_vector<MovePair,32> _branches;

    NodeX UpdateMoveTree() noexcept; // Returns move tree index of last stem node
    void UpdateFringe(NodeX branchIndex) noexcept;
    // Copy the moves in the current sequence from the move tree.
    void LoadMoveSequence() noexcept; 
    // Make all the moves in the current sequence
    void MakeSequenceMoves(Game&game) const noexcept;
};
}   // namespace KSolveNames