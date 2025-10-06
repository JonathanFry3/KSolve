#include "frystl/mf_vector.hpp"
#include "frystl/static_vector.hpp"
#include <ranges>
#include <optional>
#include <mutex>          	// for std::mutex, std::lock_guard
#include <thread>           // for std::this_thread::yield()

namespace KSolveNames {

using namespace frystl;    
using Mutex = std::mutex;
using Guard = std::lock_guard<Mutex>;
namespace ranges = std::ranges;
namespace views = std::views;

// A ShareableIndexedPriorityQueue<I,V> is a thread-safe priority queue of
// {I,V} pairs in ascending order by their I values (approximately).  It is
// implemented as a vector indexed by the I values of stacks of V values.
// I must be an unsigned integer type.
// It is efficient only if the I values are all small integers.
//
// Pairs sharing the same I values are returned in LIFO order. 
template <typename I, typename V, unsigned Sz>
class ShareableIndexedPriorityQueue {
private:
    using StackT = mf_vector<V,1024>;
    struct ProtectedStackT {
        Mutex _mutex;
        StackT _stack;
    };
    
    Mutex _mutex;
    static_vector<ProtectedStackT, Sz>_stacks;

    void inline UpsizeTo(I newSize) noexcept
    {
        if (_stacks.size() < newSize) {
            Guard desposito(_mutex);
            if (_stacks.size() < newSize)
                _stacks.resize(newSize);
        }
    }

public:
    template <class... Args>
    void Emplace(I index, Args &&...args) noexcept
    {
        UpsizeTo(index+1);
        auto& pStack = _stacks[index];
        Guard esperanto(pStack._mutex);
        pStack._stack.emplace_back(std::forward<Args>(args)...);
    }
    void Push(I index, const V& value) noexcept
    {
        Emplace(index, value);
    }
    std::optional<std::pair<I,V>> Pop() noexcept
    {
        // Something like the Uncertainty Principle applies here: in a multithreaded
        // environment, since a stack may become empty or non-empty 
        // at any instant, which one is the first non-empty one may depend on 
        // which thread is looking and exactly when. It is thus impossible to
        // be certain what the correct return value is without stopping the running
        // of other threads. No attempt is made here to
        // eliminate that problem. In this application, it does no harm.   
        std::optional<std::pair<I,V>> result;
        for (unsigned nTries = 0; !result && nTries < 5; ++nTries) 
        {
            auto nonEmpty = [] (const ProtectedStackT & elem) 
                {return !elem._stack.empty();};
            unsigned index = ranges::find_if(_stacks,nonEmpty) - _stacks.begin();
            unsigned size = _stacks.size();

            if (index < size) {
                StackT & stack = _stacks[index]._stack;
                Guard methuselah(_stacks[index]._mutex);
                if (stack.size()) {
                    result = std::make_pair(index,stack.back());
                    stack.pop_back();
                }
            }
            if (!result) std::this_thread::yield();
        }
        return result;
    }
    // Returns total size.  Approximate if threads are making changes.
    unsigned Size() const noexcept
    {
        unsigned result{0};
        for (auto& prStack: _stacks) {result += prStack._stack.size();}
        return result;
    }
};
}   // namespace KSolveNames