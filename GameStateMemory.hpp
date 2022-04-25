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

// A compact representation of the current game state.
// It is possible, although tedious, to reconstruct the
// game state from one of these and the original deck.
//
// The basic requirements for GameState are:
// 1.  Any difference in the foundation piles, the face-up cards
//     in the tableau piles, or in the stock pile length
//     should be reflected in the GameState.
// 2.  It should be quite compact, as we will usually be storing
//     millions or tens of millions of instances.
struct GameState {
    typedef std::uint_fast64_t PartType;
    std::array<PartType,3> _part;
    GameState(const Game& game) noexcept;
    bool operator==(const GameState& other) const noexcept
    {
        return _part[0] == other._part[0]
            && _part[1] == other._part[1]
            && _part[2] == other._part[2];
    }
};
struct Hasher
{
    size_t operator() (const GameState & gs) const noexcept
    {
        return 	  gs._part[0]
                ^ gs._part[1]
                ^ gs._part[2]
                ;
    }
};

class GameStateMemory
{
private:
    unsigned _maxStates;
    std::atomic_uint _size;
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

public:
    // The implementation uses a hash map which will base its initial
    // capacity on the maxStates argument.  It can grow past 
    // that size, but a performance penalty will be paid.
    GameStateMemory(unsigned maxStates);
    // Returns true if the map has more states than the specified maximum
    inline bool OverLimit() const noexcept
    {
        return size() > _maxStates;
    }
    // Returns true if the game argument has not been presented before
    // to this object or the moveCount argument is lower than that
    // associated with previous calls with equivalent states.
    bool IsShortPathToState(const Game& game, unsigned moveCount);
    // Returns the number of states stored
    inline unsigned size() const noexcept
    {
        return _size;
    }
};