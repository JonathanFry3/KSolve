// GameStateMemory.cpp implements the GameStateMemory class.

#include <algorithm>        // max
#include "GameStateMemory.hpp"

GameState::GameState(const Game& game) noexcept
{
    typedef std::array<uint32_t,7> TabStateT;
    TabStateT tableauState;
    const auto& tableau = game.Tableau();
    for (unsigned i = 0; i<7; ++i) {
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
            unsigned isMajor = 0;
            for (auto j = cards.end()-upCount+1;j < cards.end(); j+=1){
                isMajor = isMajor<<1 | unsigned(j->IsMajor());
            }
            const Card top = cards.Top();
            tableauState[i] = ((top.Suit()<<4 | top.Rank())<<11 | isMajor)<<4 | upCount;
        }
    }
    // Sort the tableau states because tableaus that are identical
    // except for order are considered equal
    std::sort(tableauState.begin(),tableauState.end());

    _part[0] = ((GameState::PartType(tableauState[0])<<21
                | tableauState[1])<<21) 
                | tableauState[2];
    _part[1] = ((GameState::PartType(tableauState[3])<<21
                | tableauState[4])<<21) 
                | tableauState[5];
    _part[2] = (tableauState[6]<<5) | game.StockPile().size();
    for (const auto& pile: game.Foundation()){
        _part[2] =_part[2]<<4 | pile.size();
    }
}

GameStateMemory::GameStateMemory(unsigned maxStates)
    : _states()
    , _maxStates(maxStates)
    , _size(0)
{
    _states.reserve(std::min(maxStates,MinCapacity));
}

bool GameStateMemory::IsShortPathToState(const Game& game, unsigned moveCount)
{
    const GameState state{game};
    bool valueChanged{false};
    const bool isNewKey = _states.try_emplace_l(
        state,						// key
        [&](auto& keyValuePair) {	// run behind lock when key found
            if (moveCount < keyValuePair.second) {
                keyValuePair.second = moveCount;
                valueChanged = true;
            }
        },
        moveCount 					// c'tor run behind lock when key not found
    );
    if (isNewKey) ++_size;
    return isNewKey || valueChanged;
}