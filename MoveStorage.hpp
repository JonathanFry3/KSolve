#include "Game.hpp"
#include "frystl/mf_vector.hpp"
#include "frystl/static_deque.hpp"
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>

namespace KSolveNames {

using namespace frystl;

using NodeX = std::uint32_t;

using Mutex = std::mutex;
typedef std::lock_guard<Mutex> Guard;

struct MoveNode
{
    MoveSpec _move;
    NodeX _prevNode{-1U};

    MoveNode() = default;
    MoveNode(const MoveSpec& mv, NodeX prevNode)
        : _move(mv)
        , _prevNode(prevNode)
        {}
};

using I = NodeX;
using V = MoveNode;

class ShareableIndexedQueue {
    //  An ShareableIndexedQueue<I,V> is a thread-safe ordered queue of {I,T} pairs in
    //  ascending order by  their I values.  It is implemented as a vector of
    //  vectors indexed by the I values and containing the V values. It is
    //  efficient if the I values are all small integers. I must be an unsigned type.
    //
    //  V values sharing the same I values are returned in LIFO order.    

private:
    using StackType = mf_vector<V,1024>;
    Mutex _mutex;
    mf_vector<std::pair<Mutex,StackType>,1024> _stacks;

public:
    ShareableIndexedQueue() = default;
    ShareableIndexedQueue(unsigned initialCapacity)
        {_stacks.reserve(initialCapacity);}
    void Push(I index, const V& value)
    {
        if (_stacks.size() < index+1) {
            Guard desposito(_mutex);
            if (_stacks.size() < index+1)
                _stacks.resize(index+1);
        }
        auto& stack = _stacks[index];
        Guard esperanto(stack.first);
        stack.second.push_back(value);
    }
    std::optional<std::pair<I,V>> Pop()
    {
        for (unsigned nTries = 0; nTries < 5; ++nTries) 
        {
            unsigned size = _stacks.size();
            auto nonEmpty = [] (const auto & elem) {return !elem.second.empty();};
            unsigned offset = std::find_if(_stacks.begin(), _stacks.end(), nonEmpty) - _stacks.begin();

            if (offset < size) {
                auto & stack = _stacks[offset].second;
                Guard methuselah(_stacks[offset].first);
                if (stack.size()) {
                    auto result = std::make_pair(offset,stack.back());
                    stack.pop_back();
                    return result;
                }
            }
            std::this_thread::yield();
        }
        return std::nullopt;
    }
    // Returns total size.  Not accurate when threads are still making changes.
    unsigned Size() const
    {
        return std::accumulate(_stacks.begin(), _stacks.end(), 0, 
            [&](auto accum, auto& stack){return accum + stack.second.size();});
    }
    bool Empty() const 
    {
        return _stacks.empty();
    }
};

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
        _maxSize = MaxSize();
        VectorType::pop_back();
    }
private:
    size_type _maxSize {0};
};

class SharedMoveStorage
{
private:
    size_t _moveTreeSizeLimit;
    std::vector<MoveNode> _moveTree;
    Mutex _moveTreeMutex;
    // Stack of indexes to leaf nodes in _moveTree
    using LeafNodeStack  = MaxSizeCollector<mf_vector<MoveNode,1024> >;
    using FringeSizeType = LeafNodeStack::size_type;
    // The leaf nodes waiting to grow new branches.  Each LeafNodeStack
    // stores nodes with the same minimum number of moves in any
    // completed game that can grow from them.  MoveStorage uses it
    // to implement a priority queue ordered by the minimum move count.
    struct FringeElement {
        Mutex _mutex;
        LeafNodeStack _stack;
    };
    ShareableIndexedQueue _fringe;
    Mutex _fringeMutex;
    unsigned _startStackIndex {-1U};
    bool _firstTime;
    friend class MoveStorage;
public:
    // Start move storage with the minimum number of moves from
    // the dealt deck before the first move.
    void Start(size_t moveTreeSizeLimit, unsigned minMoves) noexcept
    {
        _moveTreeSizeLimit = moveTreeSizeLimit;
        _moveTree.reserve(moveTreeSizeLimit+1000);
        _startStackIndex = minMoves;
        _firstTime = true;
    }

    FringeSizeType FringeSize() const noexcept{
        return _fringe.Size();
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
public:
    // Constructor.
    MoveStorage(SharedMoveStorage& shared);
    // Return a reference to the storage shared among threads
    SharedMoveStorage& Shared() const noexcept {return _shared;}
    // Push a move to the back of the current stem.
    void PushStem(MoveSpec move) noexcept;
    // Push the first move of a new branch off the current stem,
    // along with the total number of moves to reach the end state
    // that move produces.
    void PushBranch(MoveSpec move, unsigned moveCount) noexcept;
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
    enum {maxMoves = 500};
    typedef MoveCounter<static_deque<MoveSpec,maxMoves> > MoveSequenceType;
    const MoveSequenceType& MoveSequence() const noexcept {return _currentSequence;}
private:
//    typedef SharedMoveStorage::NodeX NodeX;
    typedef SharedMoveStorage::LeafNodeStack LeafNodeStack;
    SharedMoveStorage &_shared;
    MoveSequenceType _currentSequence;
    MoveNode _leaf;			// current sequence's leaf node 
    unsigned _startSize;
    struct MovePair
    {
        MoveSpec _mv;
        std::uint32_t _offset;
        MovePair(MoveSpec mv, unsigned offset)
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
};
}   // namespace KSolveNames