// The SolutionStore class stores the best solution found so far, if any.
// It is internally thread-protected.

#include "Game.hpp"     // Moves, MoveCount()
#include <mutex>        // for std::mutex

class SolutionStore
{
    Moves _minSolution;
    unsigned _nMoves;       // number of moves in _solution
    std::mutex _mutex;
    typedef std::lock_guard<std::mutex> Guard;
public:
    inline SolutionStore()
        : _nMoves(300)
    {}
    // Returns true iff any solution has been offered.
    inline bool AnySolution()
    {
        return _minSolution.size() != 0;
    }
    // Returns the number of moves in the minimum solution so far
    inline unsigned MinMoves() const noexcept
    {
        return _nMoves;
    }
    // Returns the sequence of Moves in the minimum solution so far
    inline const Moves& MinimumSolution() const noexcept
    {
        return _minSolution;
    }
    // Checks the argument solution against the minimum solution so far.
    // If the argument uses fewer moves, that becomes the new minimum solution.
    template <class SequenceType>
    void CheckSolution(const SequenceType& solution, unsigned nMoves)
    {
        Guard karen(_mutex);
        const unsigned n = _minSolution.size();
        if (n == 0 || nMoves < _nMoves) {
            _minSolution.assign(solution.begin(),solution.end());
            _nMoves = nMoves;
        }
    }
};