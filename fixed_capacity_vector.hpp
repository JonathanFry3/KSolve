// Template class fixed_capacity_vector
//
// One of these has much of the API of a std::vector,
// but has a fixed capacity.  It cannot be extended past that.
// It is safe to use only where the problem limits the size needed.

#ifndef FIXED_CAPACITY_VECTOR
#define FIXED_CAPACITY_VECTOR
#include <cstdint> 		// for uint_fast32_t, uint_fast64_t
#include <cassert>

template <class T, unsigned Capacity>
class fixed_capacity_vector{
    uint_fast32_t _size;
    T _elem[Capacity];
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    fixed_capacity_vector() 						: _size(0){}
    ~fixed_capacity_vector()						{clear();}
    template <class V>
        fixed_capacity_vector(const V& donor) 
        : _size(0)
        {append(donor.begin(),donor.end());}
    std::size_t capacity() const noexcept			{return Capacity;}
    T & operator[](unsigned i) noexcept				{assert(i<_size); return _elem[i];}
    const T& operator[](unsigned i) const noexcept	{assert(i<_size); return _elem[i];}
    iterator begin() noexcept						{return _elem;}
    const_iterator begin() const noexcept			{return _elem;}
    std::size_t size() const noexcept				{return _size;}
    bool empty() const	noexcept					{return _size == 0;}
    iterator end() noexcept							{return _elem+_size;}
    const_iterator end() const noexcept				{return _elem+_size;}
    T & back() noexcept								{return _elem[_size-1];}
    const T& back() const noexcept					{return _elem[_size-1];}
    void pop_back()	noexcept						{assert(_size); _size -= 1; _elem[_size].~T();}
    void push_back(const T& cd)	noexcept			{emplace_back(cd);}
    void clear() noexcept							{while (_size) pop_back();}
    void erase(iterator x) noexcept
                    {x->~T();for (iterator y = x+1; y < end(); ++y) *(y-1) = *y; _size-=1;}
    template <class V>
    bool operator==(const V& other) const noexcept
                    {	
                        if (_size != other.size()) return false;
                        auto iv = other.begin();
                        for(auto ic=begin();ic!=end();ic+=1,iv+=1){
                            if (*ic != *iv) return false;
                        }
                        return true;
                    }
    template <class V>
    fixed_capacity_vector<T,Capacity>& operator=(const V& other) noexcept
                    {
                        assert(other.size()<=Capacity);
                        clear();
                        for (const auto & m: other) emplace_back(m);
                        return *this;
                    }
    template <class ... Args>
    void emplace_back(Args ... args) noexcept
                    {
                        assert(_size < Capacity);
                        new(&(*end())) T(args...);
                        _size += 1;
                    }
    // Functions not part of the std::vector API

    // Push the elements in [begin,end) to the back, preserving order.
    template <typename Iterator>
    void append(Iterator begin, Iterator end) noexcept	
                    {
                        assert(_size+(end-begin)<=Capacity);
                        for (auto i=begin;i<end;i+=1){
                            _elem[_size]=*i;
                            _size+=1;
                        }
                    }
    // Move the last n elements from the argument vector to this, preserving order.
    template <typename V>
    void take_back(V& donor, unsigned n) noexcept
                    {
                        assert(donor.size() >= n);
                        append(donor.end()-n, donor.end());
                        donor._size -= n;
                    }
};
#endif      // ndef FIXED_CAPACITY_VECTOR