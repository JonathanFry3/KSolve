#include "MoveStorage.hpp"
#include <iostream>

namespace KSolveNames {

MoveStorage::MoveStorage(SharedMoveStorage& shared) noexcept
    : _shared(shared)
    , _startSize(0)
    {}
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
    _branches.emplace_back(mv,nMoves-Shared()._initialMinMoves);
}
void MoveStorage::ShareMoves() noexcept
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem moves that led to it.
    if (_branches.size()) {
        MoveX stemEnd      // index in _moveTree of last stem Branch
            = UpdateMoveTree();
        UpdateFringe(stemEnd);
        _branches.clear();
    }
}
// Returns move tree index of last stem move
MoveX MoveStorage::UpdateMoveTree() noexcept
{
    MoveX stemEnd = _leaf._prevBranchIndex;
    {
        Guard rupert(_shared._moveTreeMutex);
        // Copy all the stem moves into the move tree.
        for (auto m: _currentSequence | views::drop(_startSize))
        {
            // Each stem move points to the previous move.
            _shared._moveTree.emplace_back(m, stemEnd);
            stemEnd =  _shared._moveTree.size() - 1;
        }
    }
    return stemEnd;
} 
void MoveStorage::UpdateFringe(MoveX stemEnd) noexcept
{
    ranges::sort(_branches,ranges::greater(),&MovePair::_offset);  // descending by offset
    auto & fringe = _shared._fringe;
    for (const auto &br: _branches) {
        fringe.Emplace(br._offset, br._mv, stemEnd);
    }
}
// If the work queue (aka fringe) is empty, return 0.
// Otherwise, pop a move sequence with the lowest available
// minimum move count, redeal the deck, make all the moves in
// that sequence to return the game to the state it was in when
// that sequence was saved, and return its minimum move count.
unsigned MoveStorage::PopNextBranch(Game& game ) noexcept
{
    auto nextLeaf = _shared._fringe.Pop();
    if (nextLeaf) {
        _leaf = nextLeaf->second;
        // Restore game to the state it had when this move
        // sequence was enqueued.
        game.Deal();
        LoadMoveSequence();
        MakeSequenceMoves(game);
        return nextLeaf->first+_shared._initialMinMoves;
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
