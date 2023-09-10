// A GameStateMemory instance stores {GameState, nMoves} pairs
// so that a solver can determine whether the current game state
// has been encountered before, and if it has, was the path
// to it as short as the current path.  The object keeps the
// length of the shortest path to each state encountered so far.
//
// Instances are thread-safe.

#include "Game.hpp"                     // for Game
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h" 
#include <mutex>
#include <atomic>
#include <algorithm>                    // for equal

// A compact representation of the current game state.
//
// For game play purposes, two tableaus that are identical except
// that one or more piles are in different spots are considered
// equal.  Two game states are defined as equal here if their
// foundation piles and stock and waste piles are the same and
// their tableaus are equal except for order of piles.
//
// The basic requirements for GameState are:
// 1.  Any difference in between game states as defined above
//     must be reflected in the corresponding GameState objects.
//     A GameState is a perfect hash of the game state given
//     that equivalence relation.
// 2.  It should be quite compact, as we will usually be storing
//     millions or tens of millions of instances.
struct GameState {
    std::array<unsigned short,11> _shorts;
    GameState(const Game& game) noexcept;
    bool operator==(const GameState& other) const noexcept
    {
        return std::equal(_shorts.begin(), _shorts.end(), other._shorts.begin());
    }
};
struct Hasher
{
    size_t operator() (const GameState & gs) const noexcept
    {
        size_t result = (gs._shorts[0]<<8) | (gs._shorts[1]<<4) | (gs._shorts[2]);
        for (unsigned i = 3; i < 11; ++i)
            result = (result << 5) | gs._shorts[i];
        return result;
    }
};

class GameStateMemory
{
private:
    typedef phmap::parallel_flat_hash_map< 
            GameState, 								// key type
            unsigned short, 						// mapped type
            Hasher,									// hash function
            phmap::priv::hash_default_eq<GameState>,// == function
            phmap::priv::Allocator<phmap::priv::Pair<GameState,unsigned short> >, 
            8U, 									// log2(n of submaps)
            std::mutex								// mutex type
        > MapType;
    MapType _states;

    // Starting minimum capacity for hash map
    const unsigned MinCapacity = 4096*1024;

public:
    GameStateMemory();
    // Returns true if no equal Game argument has been presented before
    // to this object or the moveCount argument is lower than that
    // associated with previous calls with equal states.
    bool IsShortPathToState(const Game& game, unsigned moveCount);
    // Returns the number of states stored.  
    size_t Size() {return _states.size();}
};