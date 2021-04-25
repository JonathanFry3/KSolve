// Test driver for mf_vector

#include "mf_vector.hpp"
#include "SelfCount.hpp"
#include <vector>
#include <list>

using namespace frystl;

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

int main() {

    // Constructors.
    {
        // fill
        {
            mf_vector<int,8> i20(17);
            assert(i20.size() == 17);
            for (int k:i20) assert(k==0);

            mf_vector<int,16> i23(17, -6);
            assert(i23.size() == 17);
            for (int k:i23) assert(k==-6);
        }
        // range
        assert(SelfCount::Count() == 0);
        std::list<int> li;
        for (int i = 0; i < 30; ++i) li.push_back(i-13);
        mf_vector<SelfCount,95> sv(li.begin(),li.end());
        assert(SelfCount::Count() == 30);
        assert(sv.size() == 30);
        for (int i = 0; i < 30; ++i) assert(sv[i]() == i-13);

        {
            // copy
            assert(SelfCount::Count() == 30);
            mf_vector<SelfCount,80> i80 (sv);
            assert(i80.size() == 30);
            assert(SelfCount::Count() == 60);
            for (int i = 0; i < 30; ++i) assert(i80[i]() == i-13);

            mf_vector<SelfCount,80> j80 (i80);
            assert(j80.size() == 30);
            assert(SelfCount::Count() == 90);
            for (int i = 0; i < 30; ++i) assert(j80[i]() == i-13);
        }
        {
            // move
            assert(SelfCount::Count() == 30);
            mf_vector<SelfCount,73> i73 (std::move(sv));
            assert(sv.size() == 30);
            assert(i73.size() == 30);
            assert(SelfCount::Count() == 30);
            for (int i = 0; i < 30; ++i) assert(i73[i]() == i-13);

            assert(SelfCount::Count() == 30);
            mf_vector<SelfCount,73> j73 (std::move(i73));
            assert(sv.size() == 30);
            assert(j73.size() == 30);
            assert(SelfCount::Count() == 30);
            for (int i = 0; i < 30; ++i) assert(j73[i]() == i-13);
        }
    }
    {
        // Default Constructor, empty()
        mf_vector<SelfCount,7> di7;
        assert(SelfCount::Count() == 0);
        assert(di7.size() == 0);
        assert(di7.empty());


        // emplace_back(), size()

        for (unsigned i = 0; i < 50; i+= 1){
            di7.emplace_back(i);
            assert(di7.size() == i+1);
            assert(SelfCount::Count() == di7.size());
        }

        const auto & cdi50{di7};

        // pop_back()
        for (unsigned i = 0; i<20; i += 1){
            di7.pop_back();
            assert(SelfCount::Count() == di7.size());
        }
        assert(di7.size() == 30);

        // at()
        assert(di7.at(9)() == 9);
        assert(cdi50.at(29)() == 29);
        try {
            int k = di7.at(30)();  // should throw std::out_of_bounds
            assert(false);
        } 
        catch (std::out_of_range) {}
        catch (...) {assert(false);}

        // operator[](), back(), front()
        assert(di7[7]() == 7);
        di7[7] = SelfCount(91);
        assert(di7[7]() == 91);
        di7[7] = SelfCount(7);
        assert(cdi50[23]() == 23);
        assert(di7.back()() == 29);
        di7.back() = SelfCount(92);
        assert(di7.back()() == 92);
        di7.back() = SelfCount(29);
        assert(di7.back()() == 29);
        assert(di7.front()() == 0);
        assert(cdi50.front()() == 0);

        // push_back()
        di7.push_back(SelfCount(30));
        assert(cdi50[30]() == 30);
        assert(SelfCount::Count() == 31);

        assert(di7.size() == 31);

        // begin(), end()
        assert((*(cdi50.begin()))() == 0);
        *(di7.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di7.begin()+8) = SelfCount(8);
        assert(di7.begin()+di7.size() == di7.end());
        assert(SelfCount::Count() == di7.size());

        // cbegin(), cend()
        assert(*(di7.cbegin()+6) == cdi50[6]);
        assert((*(di7.cbegin()))() == 0);
        *(di7.begin()+8) = SelfCount(71);
        assert(cdi50[8]() == 71);
        *(di7.begin()+8) = SelfCount(8);
        assert(di7.cbegin()+di7.size() == cdi50.end());
        assert(SelfCount::Count() == di7.size());

        // rbegin(), rend(), crbegin(), crend()
        assert(&(*(di7.crbegin()+6)) == &(*(cdi50.cend()-7)));
        assert((*(di7.crbegin()))() == 30);
        *(di7.rbegin()+8) = SelfCount(71);
        assert(cdi50[22]() == 71);
        *(di7.rbegin()+8) = SelfCount(22);
        assert(di7.crbegin()+di7.size() == cdi50.rend());
        assert(SelfCount::Count() == di7.size());
        for (int i = 0; i < 31; i++) assert(cdi50[i]() == i);

        // erase()
        assert(di7.size() == 31);
        assert(di7.erase(di7.begin()+8) == di7.begin()+8);

        assert(SelfCount::Count() == di7.size());
        assert(di7.size() == 30);
        assert(di7[7]() == 7);
        assert(di7[8]() == 9);
        assert(di7[29]() == 30);

        // emplace()
        assert((*di7.emplace(di7.begin()+8,96))() == 96);
        assert(di7[9]() == 9);
        assert(di7.size() == 31);
        assert(SelfCount::Count() == di7.size());

        // range erase()
        auto spot = di7.erase(di7.begin()+8, di7.begin()+12);
        assert(spot == di7.begin()+8);
        assert(*spot == 12);
        assert(*(spot-1) == 7);
        assert(di7.size() == 27);
        assert(SelfCount::Count() == di7.size());

        assert(di7.erase(di7.end()-7, di7.end()) == di7.end());
        assert(di7.size() == 20);
        assert(di7.back() == 23);
        assert(SelfCount::Count() == di7.size());

        // clear()
        di7.clear();
        assert(di7.size() == 0);
        assert(SelfCount::Count() == di7.size());
    }{
        // Operation on iterators
        using It = mf_vector<int,5>::const_iterator;
        mf_vector<int, 5> vec({0,1,2,3,4,5,6,7});
        It i1 = vec.cbegin()+3;
        assert(*i1 == 3);
        assert(*(i1-2) == 1);
        i1 += 1;
        assert(*i1 == 4);
        i1 -= 3;
        assert(*i1 == 1);
        assert(i1[3] == 4);

        It i2(vec.end());
        assert(i2-i1 == 7);
        assert(i2>i1);
    }{
        // assign()
        // fill type
        mf_vector<int,2> dv;
        dv.assign(6,-29);
        assert(dv.size() == 6);
        for (auto i: dv) assert(i==-29);
    }{
        // range type
        mf_vector<int,10> dv;
        std::list<unsigned> lst;
        for (unsigned i = 9; i < 9+8; ++i){
            lst.push_back(i);
        }
        dv.assign(lst.begin(),lst.end());
        assert(dv.size() == 8);
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv[i-9]==i);}

        mf_vector<int,9> dv2;
        dv2.push_back(78);
        dv2.assign(dv.begin(),dv.end());
        for (unsigned i = 9; i < 9+8; ++i) {assert(dv2[i-9]==i);}

        // initializer list type
        dv.assign({-3, 27, 12, -397});
        assert(dv.size() == 4);
        assert(dv[2] == 12);
    }{
        // assignment operators
        mf_vector<SelfCount, 50> a, b;
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
        b = {14, -293, 1200, -2, 0};
        assert(b.size() == 5);
        assert(b[3]() == -2);
    }{
        // assignment operators between vectors of different capacities
        mf_vector<SelfCount, 50> a;
        mf_vector<SelfCount, 70> b;
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
    }{
        // The many flavors of insert()

        assert(SelfCount::Count() == 0);
        mf_vector<SelfCount,99> roop;
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
            mf_vector<SelfCount,99> r2(roop);
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
            mf_vector<SelfCount,19> r2(roop);
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
        mf_vector<SelfCount, 99> v99;
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
    }
    /*
    {
        // swap() member
        assert(SelfCount::Count() == 0);
        mf_vector<SelfCount, 99> va, vc;
        mf_vector<SelfCount, 99> vb, vd;
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
        // swap() non-member overload
        assert(SelfCount::Count() == 0);
        mf_vector<SelfCount, 99> va, vb, vc, vd;
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
        swap(va,vb);
        assert(vb.size() == 57);
        assert(va.size() == 19);
        assert(SelfCount::Count() == 2*(19+57));
        assert(vd == va);
        assert(vc == vb);
    }*/{
        // comparison functions
        mf_vector<int,73> v0;
        mf_vector<int,70> v1;
        for (unsigned i = 0; i < 40; ++i){
            v0.push_back(i);
            v1.push_back(i);
        }

        assert(v0 == v1);
        assert(v0 == v0);
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