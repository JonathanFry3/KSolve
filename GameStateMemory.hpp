// A GameStateMemory instance stores {GameState, nMoves} pairs
// so that a solver can determine whether the current game state
// has been encountered before, and if it has, was the path
// to it as short as the current path.  The object keeps the
// length of the shortest path to each state encountered so far.
//
// Instances are thread-safe.

#include "Game.hpp"                     // for Game, GameState
#include "parallel_hashmap/phmap.h"     // for parallel_flat_hash_map
#include "parallel_hashmap/phmap_base.h" 
#include <mutex>

class GameStateMemory
{
private:
    unsigned _maxStates;
    typedef phmap::parallel_flat_hash_map< 
            GameState, 								// key type
            unsigned short, 						// mapped type
            Hasher,									// hash function
            phmap::priv::hash_default_eq<GameState>,// == function
            phmap::priv::Allocator<phmap::priv::Pair<GameState,unsigned short> >, 
            7U, 									// log2(n of submaps)
            std::mutex								// mutex type
        > MapType;
    MapType _states;

public:
    // The implementation uses a hash map which will base its initial
    // capacity on the maxStates argument.  It can grow past 
    // that size, but a performance penalty will be paid.
    GameStateMemory(unsigned maxStates);
    // Returns true if the map has more states than the specified maximum
    inline bool OverLimit() const noexcept
    {
        return _states.size() > _maxStates;
    }
    // Returns true if the game argument has not been presented before
    // to this object or the moveCount argument is lower than that
    // associated with previous calls with equivalent states.
    bool IsShortPathToState(const Game& game, unsigned moveCount);
    // Returns the number of states stored
    inline unsigned size() const noexcept
    {
        return _states.size();
    }
};