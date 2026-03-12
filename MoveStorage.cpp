#include "MoveStorage.hpp"
#include <iostream>

namespace KSolveNames {

MoveStorage::MoveStorage(SharedMoveStorage& shared) noexcept
    : _shared(shared)
{
    _treeBuffer.reserve(_maxBufferSize);
    _fringeBuffer.reserve(_maxBufferSize);
}
MoveStorage::MoveStorage(const MoveStorage& orig) noexcept
    : _shared(orig._shared)
{
    _treeBuffer.reserve(_maxBufferSize);
    _fringeBuffer.reserve(_maxBufferSize);
}
void MoveStorage::PushStem(MoveSpec move) noexcept
{
    // This is where the program fails when XYZ_Test give false negatives.
    #ifndef NDEBUG
    if (!(_currentSequence.size() < _currentSequence.capacity())) {
        std::cerr << Peek(_currentSequence) << std::endl;
        assert(false && "XYZ_Test false negatives");
    }
    #endif
    _currentSequence.push_back(move);
}
void MoveStorage::PushBranch(MoveSpec mv, unsigned nMoves) noexcept
{
    _branches.emplace_back(mv,nMoves);
}
void MoveStorage::ShareMoves() noexcept
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem moves that led to it.
    if (_branches.size()) {
        UpdateMoveTreeBuffer();
        UpdateFringeBuffer();
        _branches.clear();
    }
}
void MoveStorage::UpdateMoveTreeBuffer() noexcept
{

    if (_currentSequence.size() > _startSize) {
        _treeBuffer.emplace_back(_currentSequence[_startSize], _leaf._prevBranchIndex, false);
        for (auto m: _currentSequence | views::drop(_startSize+1))
        {
            _treeBuffer.emplace_back(m, _treeBuffer.size()-1, true);
        }
    }

} 
void MoveStorage::UpdateFringeBuffer() noexcept
{
    unsigned backIndex = (_treeBuffer.size())
                         ? _treeBuffer.size()-1
                         : _leaf._prevBranchIndex;
    for (const auto &br: _branches) {
        _fringeBuffer.emplace_back(br._mv, backIndex, br._nMoves-_shared._initialMinMoves);
    }
}
// Flush the buffers to the shared data structures
void MoveStorage::Flush() noexcept
{
    unsigned treeSize = FlushTreeBuffer();
    FlushFringeBuffer(treeSize);
}
uint32_t MoveStorage::FlushTreeBuffer() noexcept 
{
    // Nicknames
    auto & moveTree{_shared._moveTree};
    uint32_t treeSize;
    {
        Guard Alysa(_shared._moveTreeMutex);
        treeSize = moveTree.size();
        for (auto mv: _treeBuffer) {
            uint32_t loc = mv._location;
            if (mv._isRelative) loc += treeSize;
            moveTree.emplace_back(mv._move, loc);
        }
    }
    _treeBuffer.clear();
    return treeSize;
}
void MoveStorage::FlushFringeBuffer(uint32_t treeSize)  noexcept
{
    std::sort(_fringeBuffer.begin(), _fringeBuffer.end());

    static_vector<Branch, _maxBufferSize> branches;

    for (unsigned i = 0; i < _fringeBuffer.size();){
        branches.clear();
        unsigned offset = _fringeBuffer[i]._offset;

        // In *branches*, create runs of Branches with equal
        // minimum move counts.
        do {
            auto &elem{_fringeBuffer[i]};
            branches.emplace_back(elem._move, elem._location+treeSize);
            ++i;
        } while (i < _fringeBuffer.size() && 
                _fringeBuffer[i]._offset == offset);
                
        _shared._fringe.Push(offset, branches);
    }
    _fringeBuffer.clear();
}
// If the work queue (aka fringe) is empty, return 0.
// Otherwise, pop a move sequence with the lowest available
// minimum move count, redeal the deck, make all the moves in
// that sequence to return the game to the state it was in when
// that sequence was saved, and return its minimum move count.
unsigned MoveStorage::PopNextBranch(Game& game ) noexcept
{
    if (BuffersNearlyFull()) Flush();  
    
    auto & fringe {_shared._fringe};
    auto nextLeaf = fringe.Pop();

    if (! nextLeaf)  {
        Flush();
        nextLeaf = fringe.Pop();
    }
    if (nextLeaf) {
        if (_fringeBuffer.MinOffset() < nextLeaf->first) {
            // Fringe buffer has a better next item than the fringe does.
            fringe.Emplace(nextLeaf->first, nextLeaf->second);
            Flush();
            nextLeaf = fringe.Pop();
        }
    }
    if (nextLeaf) {
        unsigned offset = nextLeaf->first;
        _leaf = nextLeaf->second;
        // Restore game to the state it had when this move
        // sequence was enqueued.
        game.Deal();
        LoadMoveSequence();
        MakeSequenceMoves(game);
        return offset +_shared._initialMinMoves;
    } else {
        return 0;     // fringe is empty
    }
}
void MoveStorage::LoadMoveSequence() noexcept
{
    // Follow the links to recover all the moves in a sequence in reverse order.
    _currentSequence.clear();
    for    (MoveX ix = _leaf._prevBranchIndex; 
            ix != -1U; 
            ix = _shared._moveTree[ix]._prevBranchIndex){
        const MoveSpec &mv = _shared._moveTree[ix]._move;
        _currentSequence.push_front(mv);
    }
    _startSize = _currentSequence.size();
    _currentSequence.push_back(_leaf._move);
}
void MoveStorage::MakeSequenceMoves(Game&game) const noexcept
{
    for (auto & move: _currentSequence){
        game.MakeMove(move);
    }
}
}   // namespace KSolveNames 

    
