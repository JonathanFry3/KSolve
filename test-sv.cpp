// Test driver for static_vector

#include "static_vector.hpp"
#include <vector>
#include <list>

// A class which simulates owning a resource (or not) and
// counts the instances that own the resource.
struct SelfCount {
    static int count;
    int32_t _member;
    bool _owns;
    explicit SelfCount(int val)
        : _member(val), _owns(true)
        {
            count += 1;
        }
    SelfCount(const SelfCount& val)
        : _member(val._member), _owns(true)
        {
            count += 1; 
        }
    SelfCount(SelfCount&& val)
        : _member(val._member), _owns(true)
        {
            _owns = val._owns;
            val._owns = false;
        }
    SelfCount & operator=(SelfCount&& right)
    {
        count -= _owns;
        _member = right._member;
        _owns = right._owns;
        right._owns = false;
        return *this;
    }
    uint32_t operator()() const noexcept {return _member;}
    ~SelfCount() {
        count -= _owns;
        _owns = false;
        assert(count >= 0);
    } 
};

int SelfCount::count = 0;

int main() {

    // Constructors.
    {
        // fill
        static_vector<int,20> i20(17);
        assert(i20.size() == 17);
        for (int k:i20) assert(k==0);

        // range
        assert(SelfCount::count == 0);
        std::list<SelfCount> li;
        for (int i = 0; i < 30; ++i) li.emplace_back(i-13);
        assert(SelfCount::count == 30);
        static_vector<SelfCount,95> sv(li.begin(),li.end());
        assert(SelfCount::count == 60);
        assert(sv.size() == 30);
        for (int i = 0; i < 30; ++i) assert(sv[i]() == i-13);

        {
            // copy
            assert(SelfCount::count == 60);
            static_vector<SelfCount,95> i95 (sv);
            assert(i95.size() == 30);
            assert(SelfCount::count == 90);
            for (int i = 0; i < 30; ++i) assert(i95[i]() == i-13);
        }
        {
            // move
            assert(SelfCount::count == 60);
            static_vector<SelfCount,95> i95 (std::move(sv));
            assert(sv.size() == 30);
            assert(i95.size() == 30);
            assert(SelfCount::count == 60);
            for (int i = 0; i < 30; ++i) assert(i95[i]() == i-13);
        }

    }
    {
        // Default Constructor, empty()
        static_vector<SelfCount,50> di50;
        assert(SelfCount::count == 0);
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
        assert(di50.at(9)() == 9);
        assert(cdi50.at(29)() == 29);
        try {
            int k = di50.at(30)();  // should throw std::out_of_bounds
            assert(false);
        } 
        catch (std::out_of_range) {}
        catch (...) {assert(false);}

        // operator[](), back(), front()
        assert(di50[7]() == 7);
        di50[7] = SelfCount(91);
        assert(di50[7]() == 91);
        di50[7] = SelfCount(7);
        assert(cdi50[23]() == 23);
        assert(di50.back()() == 29);
        di50.back() = SelfCount(92);
        assert(di50.back()() == 92);
        di50.back() = SelfCount(29);
        assert(di50.back()() == 29);
        assert(di50.front()() == 0);
        assert(cdi50.front()() == 0);

        // push_back()
        di50.push_back(SelfCount(30));
        assert(cdi50[30]() == 30);
        assert(SelfCount::count == 31);

        assert(di50.size() == 31);

        // data()
        const SelfCount& s {*(cdi50.data()+8)};
        assert(s() == 8);

        // begin(), end()
        assert(&(*(di50.begin()+6)) == cdi50.data()+6);
        assert((*(cdi50.begin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.begin()+di50.size() == di50.end());
        assert(SelfCount::count == di50.size());

        // begin(), end()
        assert(&(*(di50.begin()+6)) == cdi50.data()+6);
        assert((*(cdi50.begin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.begin()+di50.size() == di50.end());
        assert(SelfCount::count == di50.size());

        // cbegin(), cend()
        assert(&(*(di50.cbegin()+6)) == cdi50.data()+6);
        assert((*(di50.cbegin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.cbegin()+di50.size() == di50.cend());
        assert(SelfCount::count == di50.size());


        // rbegin(), rend(), crbegin(), crend()
        assert(&(*(di50.crbegin()+6)) == cdi50.end()-7);
        assert((*(di50.crbegin()))() == 30);
        *(di50.rbegin()+8) = SelfCount(71);
        assert(cdi50[22]() == 71);
        *(di50.rbegin()+8) = SelfCount(22);
        assert(di50.crbegin()+di50.size() == di50.crend());
        assert(SelfCount::count == di50.size());
        for (int i = 0; i < 31; i++) assert(cdi50[i]() == i);

        // erase()
        assert(di50.size() == 31);
        di50.erase(di50.begin()+8);
        assert(SelfCount::count == di50.size());
        assert(di50.size() == 30);
        assert(di50[7]() == 7);
        assert(di50[8]() == 9);
        assert(di50[29]() == 30);

        // emplace()
        assert((*di50.emplace(di50.begin()+8,96))() == 96);
        assert(di50[9]() == 9);
        assert(SelfCount::count == di50.size());

        // clear()
        di50.clear();
        assert(di50.size() == 0);
        assert(SelfCount::count == di50.size());
    }{
        // assign()
        // fill type
        std::vector<int> dv;
        dv.assign(6,-29);
        assert(dv.size() == 6);
        for (auto i: dv) assert(i==-29);
    }{
        // range type
        static_vector<int,10> dv;
        std::list<unsigned> lst;
        for (unsigned i = 9; i < 9+8; ++i){
            lst.push_back(i);
        }
        dv.assign(lst.begin(),lst.end());
        assert(dv.size() == 8);
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv[i-9]==i);}

        static_vector<int,9> dv2;
        dv2.push_back(78);
        dv2.assign(dv.begin(),dv.end());
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv2[i-9]==i);}

        // initializer list type
        dv.assign({-3, 27, 12, -397});
        assert(dv.size() == 4);
        assert(dv[2] == 12);
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

        // copy operator=()
        sv = dv;
        assert(sv==dv);
        assert(sv.size() == 20);
        sv = sv;
        assert(sv.size() == 20);
        assert(sv == dv);

        // move operator=()
        sv = std::move(dv);
        assert(sv.size() == 20);
        assert(sv == dv);
        sv = std::move(sv);
        assert(sv.size() == 20);
        assert(sv == dv);
    }

}