// Test driver for static_vector

#include "static_vector.hpp"
#include <vector>
#include <list>

// A class which simulates owning a resource (or not) and
// counts the instances that own the resource.
struct SelfCount {
    int32_t _member;
    bool _owns;
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
    static int count;
    void IncrCount(int i) 
    {
        count += i;
    }
};

// Test fill insert.
// Assumes vec is a static_vector of type SelfCount
// such that vec[i]() == i for all vec[i].
template <class C>
static void TestFillInsert(C vec, unsigned iat, unsigned n)
{
    unsigned count0 = SelfCount::Count();
    unsigned size = vec.size();
    auto spot = vec.begin() + iat;
    vec.insert(spot,n,SelfCount(843));
    assert(vec.size() == size+n);
    assert(SelfCount::Count() == count0+n);
    assert(vec[iat-1]() == iat-1);
    assert(vec[iat]() == 843);
    assert(vec[iat+n-1]() == 843);
    if (iat < size) {
        assert(vec[iat+n]() == iat);
        assert(vec[size+n-1]()== size-1);
    }
}

int SelfCount::count = 0;

int main() {

    // Constructors.
    {
        // fill
        static_vector<int,20> i20(17);
        assert(i20.size() == 17);
        for (int k:i20) assert(k==0);

        // range
        assert(SelfCount::Count() == 0);
        std::list<int> li;
        for (int i = 0; i < 30; ++i) li.push_back(i-13);
        static_vector<SelfCount,95> sv(li.begin(),li.end());
        assert(SelfCount::Count() == 30);
        assert(sv.size() == 30);
        for (int i = 0; i < 30; ++i) assert(sv[i]() == i-13);

        {
            // copy
            assert(SelfCount::Count() == 30);
            static_vector<SelfCount,95> i95 (sv);
            assert(i95.size() == 30);
            assert(SelfCount::Count() == 60);
            for (int i = 0; i < 30; ++i) assert(i95[i]() == i-13);
        }
        {
            // move
            assert(SelfCount::Count() == 30);
            static_vector<SelfCount,95> i95 (std::move(sv));
            assert(sv.size() == 30);
            assert(i95.size() == 30);
            assert(SelfCount::Count() == 30);
            for (int i = 0; i < 30; ++i) assert(i95[i]() == i-13);
        }

    }
    {
        // Default Constructor, empty()
        static_vector<SelfCount,50> di50;
        assert(SelfCount::Count() == 0);
        assert(di50.size() == 0);
        assert(di50.capacity() == 50);
        assert(di50.empty());


        // emplace_back(), size()
        for (unsigned i = 0; i < 50; i+= 1){
            di50.emplace_back(i);
            assert(di50.size() == i+1);
            assert(SelfCount::Count() == di50.size());
        }

        const auto & cdi50{di50};

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di50.pop_back();
            assert(SelfCount::Count() == di50.size());
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
        assert(SelfCount::Count() == 31);

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
        assert(SelfCount::Count() == di50.size());

        // cbegin(), cend()
        assert(&(*(di50.cbegin()+6)) == cdi50.data()+6);
        assert((*(di50.cbegin()))() == 0);
        *(di50.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di50.begin()+8) = SelfCount(8);
        assert(di50.cbegin()+di50.size() == di50.cend());
        assert(SelfCount::Count() == di50.size());

        // rbegin(), rend(), crbegin(), crend()
        assert(&(*(di50.crbegin()+6)) == cdi50.end()-7);
        assert((*(di50.crbegin()))() == 30);
        *(di50.rbegin()+8) = SelfCount(71);
        assert(cdi50[22]() == 71);
        *(di50.rbegin()+8) = SelfCount(22);
        assert(di50.crbegin()+di50.size() == di50.crend());
        assert(SelfCount::Count() == di50.size());
        for (int i = 0; i < 31; i++) assert(cdi50[i]() == i);

        // erase()
        assert(di50.size() == 31);
        assert(di50.erase(di50.begin()+8) == di50.begin()+8);

        assert(SelfCount::Count() == di50.size());
        assert(di50.size() == 30);
        assert(di50[7]() == 7);
        assert(di50[8]() == 9);
        assert(di50[29]() == 30);

        // emplace()
        assert((*di50.emplace(di50.begin()+8,96))() == 96);
        assert(di50[9]() == 9);
        assert(di50.size() == 31);
        assert(SelfCount::Count() == di50.size());

        // range erase()
        auto spot = di50.erase(di50.begin()+8, di50.begin()+12);
        assert(spot == di50.begin()+8);
        assert(*spot == 12);
        assert(*(spot-1) == 7);
        assert(di50.size() == 27);
        assert(SelfCount::Count() == di50.size());

        assert(di50.erase(di50.end()-7, di50.end()) == di50.end());
        assert(di50.size() == 20);
        assert(di50.back() == 23);
        assert(SelfCount::Count() == di50.size());

        // clear()
        di50.clear();
        assert(di50.size() == 0);
        assert(SelfCount::Count() == di50.size());
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
        // assignment operators
        static_vector<SelfCount, 50> a, b;
        assert(SelfCount::Count() == 0);
        for (unsigned i = 0; i<20; ++i)
            a.emplace_back(i);
        assert(SelfCount::Count() == 20);

        // copy operator=()
        b = a;
        assert(a==b);
        assert(b.size() == 20);
        assert(SelfCount::Count() == 40);

        a = a;
        assert(a.size() == 20);
        assert(a == b);
        assert(SelfCount::Count() == 40);

        // move operator=()
        b = std::move(a);
        assert(b.size() == 20);
        assert(SelfCount::Count() == 20);
        assert(a != b);

        a = b;
        assert(SelfCount::Count() == 40);

        b = std::move(b);
        assert(SelfCount::Count() == 40);
        assert(b.size() == 20);
        assert(a == b);

        // initializer_list operator=()
        auto& ref = b = {14, -293, 1200, -2, 0};
        assert(b.size() == 5);
        assert(ref[3]() ==-2);
        assert(ref == b);
    }{
        // The many flavors of insert()

        assert(SelfCount::Count() == 0);
        static_vector<SelfCount,99> roop;
        for (unsigned i = 0; i < 47; ++i)
            roop.emplace_back(i);

        // Move insert()
        assert(SelfCount::Count() == 47);
        auto spot = roop.begin()+9;
        roop.insert(spot,SelfCount(71));
        assert(roop.size() == 48);
        assert(SelfCount::Count() == 48);
        assert(roop[8]() == 8);
        assert(roop[9]() == 71);
        assert(roop[10]() == 9);
        assert(roop[47]() == 46);
        roop.erase(spot);

        // Fill insert()
        assert(roop.size() == 47);
        assert(SelfCount::Count() == 47);
        TestFillInsert(roop,19,13);
        TestFillInsert(roop,43,13);
        TestFillInsert(roop,roop.size(),13);

        {
            // Range insert()
            std::list<int> intList;
            for (int i = 0; i < 9; ++i) {
                intList.push_back(i+173);
            }
            static_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::Count() == 47*2);
            r2.insert(r2.begin()+31, intList.begin(), intList.end());
            assert(r2.size() == 47+9);
            assert(SelfCount::Count() == 2*47+9);
            assert(r2[30]() == 30);
            assert(r2[31+4]() == 4+173);
            assert(r2[31+9]() == 31);
        }
        assert(SelfCount::Count() == 47);
        {
            // Initializer list insert()
            static_vector<SelfCount,99> r2(roop);
            assert(r2.size() == 47);
            assert(SelfCount::Count() == 47*2);
            using Z = SelfCount;
            r2.insert(r2.begin()+31, {Z(-72),Z(0),Z(274),Z(-34245)});
            assert(r2.size() == 47+4);
            assert(SelfCount::Count() == 2*47+4);
            assert(r2[30]() == 30);
            assert(r2[30+3]() == 274);
            assert(r2[31+4]() == 31);
        }
        assert(SelfCount::Count() == 47);
    }

    {
        // resize()
        assert(SelfCount::Count() == 0);
        static_vector<SelfCount, 99> v99;
        for (int i = 0; i < 73; ++i)
            v99.emplace_back(i);
        assert(v99.size() == 73);
        assert(SelfCount::Count() == 73);
        v99.resize(78,SelfCount(-823));
        assert(v99.size() == 78);
        assert(SelfCount::Count() == 78);
        assert(v99[72]() == 72);
        assert(v99[73]() == -823);
        assert(v99[77]() == -823);
        v99.resize(49);
        assert(v99.size() == 49);
        assert(SelfCount::Count() == 49);
        assert(v99[48]() == 48);
        v99.resize(56);
        assert(v99.size() == 56);
        assert(SelfCount::Count() == 56);
        assert(v99[55]() == 0);
    }{
        // swap()
        assert(SelfCount::Count() == 0);
        static_vector<SelfCount, 99> va, vb, vc, vd;
        for (int i = 0; i < 57; ++i){
            va.emplace_back(i);
            if (i < 19) vb.emplace_back(i+300);
        }
        vc = va;
        vd = vb;
        assert(va.size() == 57);
        assert(vb.size() == 19);
        assert(SelfCount::Count() == 2*(19+57));
        assert(vc == va);
        assert(vd == vb);
        va.swap(vb);
        assert(vb.size() == 57);
        assert(va.size() == 19);
        assert(SelfCount::Count() == 2*(19+57));
        assert(vd == va);
        assert(vc == vb);
    }{
        // comparison functions
        static_vector<int,73> v0;
        static_vector<int,70> v1;
        for (unsigned i = 0; i < 40; ++i){
            v0.push_back(i);
            v1.push_back(i);
        }

        assert(v0 == v1);
        assert(v1 == v0);
        assert(!(v0 < v1));

        v1.pop_back();
        assert(v1 < v0);
        assert(v1 <= v0);
        assert(v0 > v1);
        assert(v0 >= v1);
        assert(v1 != v0);

        v1[16] = 235;
        assert(v0 < v1);
        assert(v0 != v1);
    }
}