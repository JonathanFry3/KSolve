// Various mutex variations

#include <atomic>
#include <mutex>

// A spinlock submitted to StackOverflow by João Neto Aug 27, 2018 
class Spinlock {
    std::atomic_flag _lock = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (_lock.test_and_set(std::memory_order_acquire)) continue;
    }
    void unlock() {
        _lock.clear(std::memory_order_release);
    }
};

// A compound lock. It first spins for a bit, then uses a mutex.
// Interesting idea.  This implementation will work (if it works at all)
// only on a single-threaded environment :(
class CompoundLock
{
    std::atomic_flag _spinLock = ATOMIC_FLAG_INIT;
    std::mutex _mutex;
    bool _usingMutex = false;
    const unsigned _spinLimit = 500;
public:
    CompoundLock(unsigned spinLimit)
        : _spinLimit(spinLimit)
        {}
    CompoundLock(const CompoundLock&) = delete;
    CompoundLock(const CompoundLock&&) = delete;
    ~CompoundLock() {unlock();}

    void lock() {
        unsigned spins = 0;
        if (!_usingMutex) {
            while (spins++ < _spinLimit 
                && _spinLock.test_and_set(std::memory_order_acquire)) {}
        }
        if (_usingMutex || spins == _spinLimit) {
            _usingMutex = true;
            _mutex.lock();
        }
    }
    void unlock()
    {
        if (_usingMutex) _mutex.unlock();
        else _spinLock.clear(std::memory_order_release);
        _usingMutex = false;
    }
};