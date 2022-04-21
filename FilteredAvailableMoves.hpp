
#include "Game.hpp"
// Return a vector of the available moves that pass the ABC_Move filter
template <class SequenceType>
QMoves FilteredAvailableMoves(const Game & game, const SequenceType& movesMade) noexcept
{
    QMoves availableMoves = game.AvailableMoves();
    for (auto i = availableMoves.begin(); i != availableMoves.end(); ) {
        if (ABC_Move(*i,movesMade)) {
            i = availableMoves.erase(i);
        } else {
            ++i;
        }
    }
    return availableMoves;
}
