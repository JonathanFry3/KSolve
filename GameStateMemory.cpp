// GameStateMemory.cpp implements the GameStateMemory class.

#include "GameStateMemory.hpp"

GameStateMemory::GameStateMemory(unsigned maxStates)
    : _states()
    , _maxStates(maxStates)
{
    _states.reserve(maxStates);
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
