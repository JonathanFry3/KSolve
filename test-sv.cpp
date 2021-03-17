// Test driver for static_vector

#include "static_vector.hpp"

// A class which counts its instances
struct SelfCount {
    static int count;
    uint32_t _member;
    explicit SelfCount(int val)
        : _member(val)
        {
            count += 1;
        }
    ~SelfCount() {
        assert(count);
        count -= 1;
    }
};

int SelfCount::count = 0;

int main() {

    // Constructors.
    {
        // Default Constructor
        static_vector<SelfCount,50> di50;
        assert(SelfCount::count == 0);
        assert(sizeof(di50) == 51*4);
        assert(di50.size() == 0);
        assert(di50.capacity() == 50);

        // emplace_back(), size()
        for (unsigned i = 0; i < 50; i+= 1){
            di50.emplace_back(i);
            assert(di50.size() == i+1);
            assert(SelfCount::count == di50.size());
        }

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di50.pop_back();
            assert(SelfCount::count == di50.size());
        }
        assert(di50.size() == 30);

        // clear()
        di50.clear();
        assert(di50.size() == 0);
        assert(SelfCount::count == di50.size());
    }

}