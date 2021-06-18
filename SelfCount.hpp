#ifndef SELF_COUNT_HPP
#define SELF_COUNT_HPP

#include <cstdint>      // int32_t
#include <cassert>      // assert()

// A class which simulates owning a resource (or not) and
// counts the instances that own the resource.

struct SelfCount {
    SelfCount()
        : _member(0), _owns(true)
        {
            IncrCount(1);
        }
    SelfCount(int val)
        : _member(val), _owns(true)
        {
            IncrCount(1);
        }
    SelfCount(const SelfCount& val)
        : _member(val._member), _owns(true)
        {
            IncrCount(1); 
        }
    SelfCount(SelfCount&& val)
        : _member(val._member)
        , _owns(val._owns)
        {
            val._owns = false;
        }
    SelfCount & operator=(SelfCount&& right)
    {
        if (this != &right) {
            IncrCount(-_owns); 
            _member = right._member;
            _owns = right._owns;
            right._owns = false;
        }
        return *this;
    }
    SelfCount & operator=(const SelfCount& right)
    {
        assert(false);
    }
    bool operator==(const SelfCount& right) const 
    {
        return _member == right._member && _owns == right._owns;
    }
    bool operator!=(const SelfCount& right) const 
    {
        return ! operator==(right);
    }
    uint32_t operator()() const noexcept {return _member;}
    
    ~SelfCount() {
        IncrCount(-_owns); 
        _owns = false;
        assert(count >= 0);
    }
    static int Count()
    {
        return count;
    }
private:
    int32_t _member;
    bool _owns;
    static int count;
    void IncrCount(int i) 
    {
        count += i;
    }
};

int SelfCount::count = 0;
#endif // ndef SELF_COUNT_HPP