#include "MoveStorage.hpp"

namespace KSolveNames {

MoveStorage::MoveStorage(SharedMoveStorage& shared)
    : _shared(shared)
    , _leaf()
    , _startSize(0)
    {}
void MoveStorage::PushStem(MoveSpec move) noexcept
{
    _currentSequence.push_back(move);
}
void MoveStorage::PushBranch(MoveSpec mv, unsigned nMoves) noexcept
{
    assert(_shared._startStackIndex <= nMoves);
    _branches.emplace_back(mv,nMoves-_shared._startStackIndex);
}
void MoveStorage::ShareMoves()
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem nodes that led to it.
    if (_branches.size()) {
        NodeX stemEnd      // index in _moveTree of last stem MoveSpec
            = UpdateMoveTree();
        UpdateFringe(stemEnd);
    }
}

// Returns move tree index of last stem node
NodeX MoveStorage::UpdateMoveTree() noexcept
{
    NodeX stemEnd = _leaf._prevNode;
    Guard rupert(_shared._moveTreeMutex);
    {
        // Copy all the stem moves into the move tree.
        for (auto m: _currentSequence | views::drop(_startSize))
        {
            // Each stem node points to the previous node.
            _shared._moveTree.emplace_back(m, stemEnd);
            stemEnd =  _shared._moveTree.size() - 1;
        }
    }
    return stemEnd;
} 
void MoveStorage::UpdateFringe(NodeX stemEnd) noexcept
{
    auto & fringe = _shared._fringe;

    for (const auto &br: _branches) {
        fringe.Push(br._offset, MoveNode(br._mv, stemEnd));
    }
}
unsigned MoveStorage::PopNextSequenceIndex( ) noexcept
{
    unsigned result = 0;
    _leaf = MoveNode{};
    auto & fringe = _shared._fringe;

    if (fringe.Empty() && _shared._firstTime) {
        // first time 
        _shared._firstTime = false;
        return _shared._startStackIndex;
    }
    auto nextOpt = fringe.Pop();
    if (!nextOpt) return 0;
    else {
        _leaf = nextOpt->second;
        return nextOpt->first+_shared._startStackIndex;
    }
}
void MoveStorage::LoadMoveSequence() noexcept
{
    // Follow the links to recover all the moves in a sequence in reverse order.
    // Note that this operation requires no guard on _shared._moveTree only if 
    // the block vector in that structure never needs reallocation.
    _currentSequence.clear();
    if (!_leaf._move.IsDefault())
        _currentSequence.push_back(_leaf._move);
    for (NodeX node = _leaf._prevNode; 
         node != -1U; 
         node = _shared._moveTree[node]._prevNode){
        const MoveSpec &mv = _shared._moveTree[node]._move;
        _currentSequence.push_front(mv);
    }

    _startSize = _currentSequence.size();
    if (_startSize > 0) _startSize--;
    _branches.clear();
}
void MoveStorage::MakeSequenceMoves(Game&game) const noexcept
{
    for (auto & move: _currentSequence){
        game.MakeMove(move);
    }
}
}   // namespace KSolveNames
