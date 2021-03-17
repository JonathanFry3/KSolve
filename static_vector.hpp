// Template class static_vector
//
// One of these has much of the API of a std::vector,
// but has a fixed capacity.  It cannot be extended past that.
// It is safe to use only where the problem limits the size needed.

#ifndef STATIC_VECTOR
#define STATIC_VECTOR
#include <cstdint> 		// for uint32_t
#include <cassert>
#include <iterator>     // std::reverse_iterator, std::distance
#include <algorithm>    // for std::move(), std::copy()


template <class T, unsigned Capacity>
struct static_vector{
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_vector() 						: _size(0){}
    ~static_vector()						{clear();}
    template <class V>
        static_vector(const V& donor) 
        : _size(0)
        {append(donor.begin(),donor.end());}
    std::size_t capacity() const noexcept			{return Capacity;}
    reference at(unsigned i) 	                    {verify(i<_size); return data()[i];}
    reference operator[](unsigned i) noexcept	    {assert(i<_size); return data()[i];}
    const_reference at(unsigned i) const 	        {verify(i<_size); return data()[i];}
    const_reference operator[](unsigned i) const noexcept
                                                	{assert(i<_size); return data()[i];}
    iterator begin() noexcept						{return data();}
    const_iterator begin() const noexcept			{return data();}
    size_type size() const noexcept				    {return _size;}
    pointer data() noexcept                         {return reinterpret_cast<pointer>(_elem);}
    const_pointer data() const noexcept             {return reinterpret_cast<const_pointer>(_elem);}
    bool empty() const	noexcept					{return _size == 0;}
    iterator end() noexcept							{return data()+_size;}
    const_iterator end() const noexcept				{return data()+_size;}
    reference back() noexcept				    	{return data()[_size-1];}
    const_reference back() const noexcept			{return data()[_size-1];}
    void pop_back()	noexcept						{assert(_size); _size -= 1; end()->~value_type();}
    void push_back(const T& cd)	noexcept			{emplace_back(cd);}
    void clear() noexcept							{while (_size) pop_back();}
    void erase(iterator x) noexcept                 
                    {x->~value_type(); std::move(x+1,end(),x); _size -= 1;}
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
    static_vector<value_type,Capacity>& operator=(const V& other) noexcept
                    {
                        assert(other.size()<=Capacity);
                        clear();
                        std::copy(other.begin(),other.end(),begin());
                        _size = other.size();
                        return *this;
                    }
    template <class V>
    static_vector<value_type,Capacity>& operator=(const V&& other) noexcept
                    {
                        assert(other.size()<=Capacity);
                        clear();
                        std::move(other.begin(),other.end(),begin());
                        _size = other.size();
                        return *this;
                    }
    template <class ... Args>
    void emplace_back(Args ... args)
                    {
                        assert(_size < Capacity);
                        pointer p{data()+_size};
                        new(p) value_type(args...);
                        _size += 1;
                    }
    // Functions not part of the std::vector API

    // Push the elements in [begin,end) to the back, preserving order.
    template <typename Iterator>
    void append(Iterator begin, Iterator end) noexcept	
                    {
                        assert(_size+(end-begin)<=Capacity);
                        for (auto i=begin;i<end;i+=1){
                            data()[_size] = *i;
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
private:
    using storage_type =
        std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;

    static void verify(bool cond){if (!cond) throw std::out_of_range("static_vector range error");}

    uint32_t _size;
    storage_type _elem[Capacity];
public:
};
#endif      // ndef STATIC_VECTOR