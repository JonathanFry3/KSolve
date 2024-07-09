#include "MoveStorage.hpp"
#include <thread>

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
void MoveStorage::EndIteration()
{
    // If _branches is empty, a dead end has been reached.  There
    // is no need to store any stem nodes that led to it.
    if (_branches.size()) {
        auto stemEnd      // index in _moveTree of last stem MoveSpec
            = UpdateMoveTree();
        UpdateFringe(stemEnd);
    }
    auto & fringe = _shared._fringe;
    auto& el  {fringe[_threadOffset]};
    Guard tracie(el._mutex);
    ++el._threadCount;
}

// Returns move tree index of last stem node
MoveStorage::NodeX MoveStorage::UpdateMoveTree() noexcept
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
        elem._stack.emplace_back(br._mv,stemEnd);
    }
}
unsigned MoveStorage::PopNextSequenceIndex( ) noexcept
{
    unsigned size;
    unsigned result = 0;
    _leaf = MoveNode{};
    auto & fringe = _shared._fringe;

    if (fringe.empty() && _shared._firstTime) {
        // first time 
        _shared._firstTime = false;
        return _shared._startStackIndex;
    }
    // Find the first non-empty leaf node stack, pop its top into _leaf.
    //
    // It's not quite that simple with more than one thread, but that's the idea.
    // When we don't have a lock on it, any of the stacks may become empty or non-empty.
    for (unsigned nTries = 0; result == 0 && nTries < 5; ++nTries) 
    {
        size = fringe.size();
        // Set _threadOffset to the index of the first non-empty leaf node stack
        // or size if all are empty.
        auto nonEmpty = [] (const auto & elem) {return !elem._stack.empty();};
        _threadOffset = ranges::find_if(fringe, nonEmpty) - fringe.begin();

        if (_threadOffset < size) {
            auto & stack = fringe[_threadOffset]._stack;
            {
                Guard methuselah(fringe[_threadOffset]._mutex);
                if (stack.size()) {
                    _leaf = stack.back();
                    stack.pop_back();
                    result = _threadOffset+_shared._startStackIndex;
                    ++fringe[_threadOffset]._threadCount;
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
