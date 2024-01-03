// GameStateMemory.cpp implements the GameStateMemory class.

#include <algorithm>        // max
#include "GameStateMemory.hpp"

namespace KSolveNames {
GameState::GameState(const Game& game, unsigned moveCount) noexcept
    : _moveCount(moveCount)
{
    std::array<uint32_t,TableauSize> tableauState;
    const auto& tableau = game.Tableau();
    for (unsigned i = 0; i<TableauSize; ++i) {
        const auto& cards = tableau[i];
        const unsigned upCount = cards.UpCount();
        if (upCount == 0) {
            tableauState[i] = 0;
        } else {
            // The rules for moving to the tableau piles guarantee
            // all the face-up cards in such a pile can be identified
            // by identifying the bottom card (the first face-up card)
            // and whether each other face-up card is from 
            // a major suit (hearts or spades) or not.
            //
            // The face-up cards in a tableau pile cannot number
            // more than 12, since AvailableMoves() will never move an
            // ace there.
            unsigned isMajor = 
                std::accumulate(cards.end()-upCount+1, cards.end(), 0,
                    [](unsigned acc, Card card)
                        {return acc<<1 | card.IsMajor();});
            const Card top = cards.Top();
            tableauState[i] = ((top.Suit()<<4 | top.Rank())<<11 | isMajor)<<4 | upCount;
        }
    }
    // Sort the tableau states because tableaus that are identical
    // except for order are considered equal
    ranges::sort(tableauState);

    _part0 =    (GameState::PartType(tableauState[0])<<21
                | GameState::PartType(tableauState[1]))<<21
                | GameState::PartType(tableauState[2]);
    _part1 =    (GameState::PartType(tableauState[3])<<21
                | GameState::PartType(tableauState[4]))<<21
                | GameState::PartType(tableauState[5]);
    auto& f{game.Foundation()};
    _part2 =    ((((GameState::PartType(tableauState[6])<<5
                | game.StockPile().size())<<4
                | f[0].size())<<4 
                | f[1].size())<<4 
                | f[2].size())<<4 
                | f[3].size();
}

GameStateMemory::GameStateMemory()
    : _states()
{
    _states.reserve(MinCapacity);
}

bool GameStateMemory::IsShortPathToState(const Game& game, unsigned moveCount)
{
    const GameState newState{game,moveCount};
    bool valueChanged{false};
    bool isNewKey = _states.lazy_emplace_l(
        newState,						// key
        [&](auto& oldState) {	// run behind lock when key found
            if (moveCount < oldState._moveCount) {
                oldState._moveCount = moveCount;
                static_assert(sizeof(oldState) == 24);
                valueChanged = true;
            }
        },
        [&](const MapType::constructor& ctor) { // ... if key not found
            ctor(newState);
        }
    );
    return isNewKey || valueChanged;
}
}   // namespace KSolveNames