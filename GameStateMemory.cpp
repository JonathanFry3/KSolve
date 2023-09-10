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

    struct {
        uint64_t _p0;
        uint64_t _p1;
        union {
            uint64_t _p2:48;
            unsigned short _p2_shorts[3];
        };
    } p;
    p._p0  =    uint64_t(tableauState[0])<<42
                | uint64_t(tableauState[1])<<21
                | uint64_t(tableauState[2]);
    p._p1  =    uint64_t(tableauState[3])<<42
                | uint64_t(tableauState[4])<<21
                | uint64_t(tableauState[5]);
    auto& f{game.Foundation()};
    p._p2  =    uint64_t(tableauState[6])<<21
                | game.StockPile().size()<<16
                | f[0].size()<<12 
                | f[1].size()<<8 
                | f[2].size()<<4 
                | f[3].size();
    memcpy(_shorts.data(),&p._p0,16);
    memcpy(_shorts.data()+8,&p._p2_shorts,6);
}

GameStateMemory::GameStateMemory()
    : _states()
{
    _states.reserve(MinCapacity);
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
    return isNewKey || valueChanged;
}