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

template <typename I, typename V>
class ShareableIndexedPriorityQueue {
    //  An ShareableIndexedPriorityQueue<I,V> is a thread-safe priority queue of {I,V} pairs in
    //  ascending order by  their I values.  It is implemented as a vector of
    //  vectors indexed by the I values and containing the V values. It is
    //  efficient if the I values are all small integers. I must be an unsigned type.
    //
    //  V values sharing the same I values are returned in LIFO order.    

private:
    using StackType = mf_vector<V,1024>;
    Mutex _mutex;
    static_vector<std::pair<Mutex,StackType>,512> _stacks;
    void inline Resize(I newSize) noexcept
    {
        if (_stacks.size() < newSize) [[unlikely]]{
            Guard desposito(_mutex);
            if (_stacks.size() < newSize) [[unlikely]]
                _stacks.resize(newSize);
        }
    }

public:
    ShareableIndexedPriorityQueue() = default;
    ShareableIndexedPriorityQueue(unsigned initialCapacity)
        {_stacks.reserve(initialCapacity);}
    template <class... Args>
    void Emplace(I index, Args &&...args) noexcept
    {
        Resize(index+1);
        auto& stack = _stacks[index];
        Guard esperanto(stack.first);
        stack.second.emplace_back(std::forward<Args>(args)...);
    }
    void Push(I index, const V& value) noexcept
    {
        Emplace(index, value);
    }
    std::optional<std::pair<I,V>> 
    Pop() noexcept
    {
        for (unsigned nTries = 0; nTries < 5; ++nTries) 
        {
            unsigned size = _stacks.size();
            unsigned index;
            auto nonEmpty = [] (const auto & elem) {return !elem.second.empty();};
            for (index = 0; index < size && _stacks[index].second.empty(); ++index);

            if (index < size) {
                auto & stack = _stacks[index].second;
                Guard methuselah(_stacks[index].first);
                if (stack.size()) { 
                    auto result = std::make_pair(index,stack.back());
                    stack.pop_back();
                    return result;
                }
            }
            std::this_thread::yield();
        }
        return std::nullopt;
    }
    // Returns total size.  Not accurate when threads are making changes.
    unsigned Size() const noexcept
    {
        return std::accumulate(_stacks.begin(), _stacks.end(), 0U, 
            [&](auto accum, auto& stack){return accum + stack.second.size();});
    }
};
class SharedMoveStorage
{
private:
    size_t _moveTreeSizeLimit;
    std::vector<MoveNode> _moveTree;
    Mutex _moveTreeMutex;
    // The leaf nodes waiting to grow new branches.  
    ShareableIndexedPriorityQueue<unsigned, MoveNode> _fringe;
    unsigned _initialMinMoves {-1U};
    bool _firstTime;
    friend class MoveStorage;
public:
    // Start move storage with the minimum number of moves from
    // the dealt deck before the first move.
    void Start(size_t moveTreeSizeLimit, unsigned minMoves) noexcept
    {
        _moveTreeSizeLimit = moveTreeSizeLimit;
        _moveTree.reserve(moveTreeSizeLimit+1000);
        _initialMinMoves = minMoves;
        _firstTime = true;
    }

    unsigned FringeSize() const noexcept{
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