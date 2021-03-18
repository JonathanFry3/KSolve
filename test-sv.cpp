// Test driver for static_vector

#include "static_vector.hpp"
#include <vector>

// A class which counts its instances
struct SelfCount {
    static int count;
    uint32_t _member;
    explicit SelfCount(int val)
        : _member(val)
        {
            count += 1;
        }
    SelfCount(const SelfCount& val)
        : _member(val._member)
        {
            count += 1; 
        }
    uint32_t operator()() const noexcept {return _member;}
    ~SelfCount() {
        assert(count);
        count -= 1;
    }
};

int SelfCount::count = 0;

int main() {

    // Constructors.
    {
        // Default Constructor, empty()
        static_vector<SelfCount,50> di50;
        assert(SelfCount::count == 0);
        assert(sizeof(di50) == 51*4);
        assert(di50.size() == 0);
        assert(di50.capacity() == 50);
        assert(di50.empty());

        // emplace_back(), size()
        for (unsigned i = 0; i < 50; i+= 1){
            di50.emplace_back(i);
            assert(di50.size() == i+1);
            assert(SelfCount::count == di50.size());
        }

        const auto & cdi50{di50};

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di50.pop_back();
            assert(SelfCount::count == di50.size());
        }
        assert(di50.size() == 30);

        // at()
        di50.at(9)() == 9;
        cdi50.at(29)() == 29;
        try {
            int k = di50.at(30)();
            assert(false);
        } 
        catch (...) {}

        // operator[](), back()
        assert(di50[7]() == 7);
        assert(cdi50[23]() == 23);
        assert(cdi50.back()() == 29);
        assert(di50.back()() == 29);

        // push_back()
        di50.push_back(SelfCount(30));
        assert(cdi50[30]() == 30);
        assert(SelfCount::count == 31);

        assert(di50.size() == 31);

        // data()
        const SelfCount& s {*(cdi50.data()+8)};
        assert(s() == 8);

        // begin(), end()
        assert(&(*(cdi50.begin()+6)) == cdi50.data()+6);
        assert((*(cdi50.begin()))() == 0);
        assert(di50.begin()+di50.size() == di50.end());

        // erase()
        assert(di50.size() == 31);
        di50.erase(di50.begin()+8);
        assert(di50.size() == 30);
        assert(di50[7]() == 7);
        assert(di50[8]() == 9);
        assert(di50[29]() == 30);

        // clear()
        di50.clear();
        assert(di50.size() == 0);
        assert(SelfCount::count == di50.size());
    }
    {
        // operator==(), operator!=()
        std::vector<int> dv;
        static_vector<unsigned,32> sv;
        for (int i = 0; i < 20; ++i){
            dv.push_back(i);
            sv.push_back(i);
        }
        assert(sv==dv);
        sv[8] = 5;
        assert(sv!=dv);
        sv[8] = 8;
        sv.pop_back();
        assert(sv!=dv);

        // operator=()
        sv = dv;
        assert(sv==dv);
    }

}