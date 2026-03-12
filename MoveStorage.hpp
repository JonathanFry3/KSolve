#include "ShareableIndexedPriorityQueue.hpp"
#include "Game.hpp"
#include "frystl/static_deque.hpp"

namespace KSolveNames {

using MoveX = uint32_t;

struct Branch
{
    MoveSpec _move;
    MoveX _prevBranchIndex{-1U};

    Branch() = default;
    Branch(const MoveSpec& mv, MoveX prevBranch) noexcept
        : _move(mv)
        , _prevBranchIndex(prevBranch)
        {}
};

class SharedMoveStorage
{
private:
    const size_t _moveTreeSizeLimit;
    std::vector<Branch> _moveTree;
    Mutex _moveTreeMutex;
    // The leaves waiting to grow new branches.  
    // Also, the task queue.
    ShareableIndexedPriorityQueue<unsigned, Branch, 512> _fringe;
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
    MoveStorage(const MoveStorage& orig) noexcept;
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
    // If the task queue is empty return 0.  Otherwise,
    // pop the next branch from the task queue, restore the 
    // game to the state it was in when that branch was pushed,
    // and return the heuristic value of that state.
    unsigned PopNextBranch(Game& game) noexcept;
    // Flush the buffer to the shared data structures
    void Flush() noexcept;
    // Return a const reference to the current move sequence in its
    // native type.
    using MoveSequenceType = MoveCounter<static_deque<MoveSpec,500>>;
    const MoveSequenceType& MoveSequence() const noexcept {return _currentSequence;}
private:
    SharedMoveStorage &_shared;

    MoveSequenceType _currentSequence;
    Branch _leaf{};	        // current sequence's starting leaf
    long _startSize{0};    // number of MoveSpecs gotten from the move tree.
    struct MovePair
    {
        MoveSpec _mv;
        uint32_t _nMoves;
        MovePair(MoveSpec mv, unsigned offset)
            : _mv(mv)
            , _nMoves(offset)
        {}
    };
    static_vector<MovePair,32> _branches{};
    void  UpdateMoveTreeBuffer() noexcept; 
    void UpdateFringeBuffer() noexcept;
    uint32_t FlushTreeBuffer() noexcept;
    void FlushFringeBuffer(uint32_t treeSize) noexcept;

    // Copy the moves in the current sequence from the move tree.
    void LoadMoveSequence() noexcept; 
    // Make all the moves in the current sequence
    void MakeSequenceMoves(Game&game) const noexcept;

    // Buffering
    struct MoveTreeElement {
        MoveSpec _move;
        uint32_t _location;      // subscript in _moveTree or offset from end of tree
        bool _isRelative;        // _location is relative to _moveTree size -1
    };
    struct FringeElement {
        MoveSpec _move;
        uint32_t _location;         // offset from end of tree at start of Flush()
        uint32_t _offset;
        bool operator<(const FringeElement & other) const noexcept
        {
            return _offset < other._offset;
        }
    };

    static const unsigned _maxBufferSize{256};
    std::vector<MoveTreeElement> _treeBuffer{};
    class FringeBufferT : public std::vector<FringeElement> 
    {
    private:    
        unsigned _minOffset{-1U};
        using Base = std::vector<FringeElement>;
    public:
        void emplace_back(MoveSpec move, uint32_t location, uint32_t offset) noexcept
        {
            if (offset < _minOffset) _minOffset = offset;
            Base::emplace_back(move, location, offset);
        }
        unsigned MinOffset() const noexcept
        {
            return _minOffset;
        }
        void clear() noexcept
        {
            _minOffset = -1U;
            Base::clear();
        }
    }   _fringeBuffer{};

    bool BuffersNearlyFull() {
        return _maxBufferSize < _treeBuffer.size()+52 
            || _maxBufferSize < _fringeBuffer.size()+20;
    }
};
}   // namespace KSolveNames