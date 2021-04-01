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
#include <initializer_list>
#include <stdexcept>    // for std::out_if_range


template <class T, unsigned Capacity>
struct static_vector{
    using this_class = static_vector<T,Capacity>;
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
    // copy constructor
    static_vector(const this_class& donor) : _size(0) 
        {
            assert(donor.size() <= Capacity);
            for (auto& m:donor) emplace_back(m);
        }
    // move constructor
    // Constructs the new static_vector by moving all the elements of
    // the existing static_vector.  It leaves the moved-from object
    // unchanged, aside from whatever changes moving its elements
    // made.
    static_vector(this_class&& donor) : _size(0) 
        {
            assert(donor.size() <= Capacity);
            for (auto& m:donor) emplace_back(std::move(m));
        }
    // fill constructor
    static_vector(unsigned n) 
    : _size(0)
    {for (unsigned i=0; i<n; ++i) emplace_back();}

    // range constructor
    template <class InputIterator, 
                typename = std::_RequireInputIter<InputIterator>>       // TODO: not portable
    static_vector(InputIterator begin, InputIterator end)
    : _size(0)
    {for (InputIterator k=begin; k!= end; ++k) push_back(*k);}


    constexpr std::size_t capacity() const noexcept	{return Capacity;}
    constexpr std::size_t max_size() const noexcept	{return Capacity;}
    reference at(unsigned i) 	                    {verify(i<_size); return data()[i];}
    reference operator[](unsigned i) noexcept	    {assert(i<_size); return data()[i];}
    const_reference at(unsigned i) const 	        {verify(i<_size); return data()[i];}
    const_reference operator[](unsigned i) const noexcept
                                                	{assert(i<_size); return data()[i];}
    iterator begin() noexcept						{return data();}
    const_iterator begin() const noexcept			{return data();}
    const_iterator cbegin() const noexcept			{return data();}
    iterator end() noexcept							{return data()+_size;}
    const_iterator end() const noexcept				{return data()+_size;}
    const_iterator cend() noexcept      			{return data()+_size;}
    reverse_iterator rbegin() noexcept              {return reverse_iterator(end());}
    const_reverse_iterator rbegin() const noexcept  {return const_reverse_iterator(end());}
    const_reverse_iterator crbegin() noexcept       {return const_reverse_iterator(cend());}
    reverse_iterator rend() noexcept                {return reverse_iterator(begin());}
    const_reverse_iterator rend() const noexcept    {return const_reverse_iterator(begin());}
    const_reverse_iterator crend()  noexcept        {return const_reverse_iterator(cbegin());}
    size_type size() const noexcept				    {return _size;}
    pointer data() noexcept                         {return reinterpret_cast<pointer>(_elem);}
    const_pointer data() const noexcept             {return reinterpret_cast<const_pointer>(_elem);}
    bool empty() const	noexcept					{return _size == 0;}
    reference front() noexcept                      {assert(_size); return data()[0];}
    const_reference front() const noexcept          {assert(_size); return data()[0];}
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
    bool operator!=(const V& other) const noexcept {return !(*this==other);}                 
    void assign(size_type n, const_reference val)
                    {
                        clear();
                        for (unsigned i = 0; i < n; ++i) push_back(val);
                    }
    void assign(std::initializer_list<value_type> x)
                    {
                        clear();
                        for (auto& a:x) push_back(a);
                    }
    template <class InputIterator, 
                typename = std::_RequireInputIter<InputIterator>>       // TODO: not portable
    void assign(InputIterator begin, InputIterator end)
                    {
                        clear();
                        for (InputIterator k=begin; k!= end; ++k) push_back(*k);
                    }                        
    template <class V>
    static_vector<value_type,Capacity>& operator=(const V& other) noexcept
                    {
                        if (reinterpret_cast<const V*>(this) != &other)
                            assign(other.begin(), other.end());
                        return *this;
                    }
    static_vector<value_type,Capacity>& operator=(const this_class& other) noexcept
                    {
                        if (this != &other)
                            assign(other.begin(), other.end());
                        return *this;
                    }
    template <class V>
    static_vector<value_type,Capacity>& operator=(const V&& other) noexcept
                    {
                        if (reinterpret_cast<const V*>(this) != &other) {
                            assert(other.size()<=Capacity);
                            clear();
                            std::move(other.begin(),other.end(),begin());
                            _size = other.size();
                        }
                        return *this;
                    }
    template <class... Args>
    iterator emplace (const_iterator position, Args&&... args)
                    {
                        assert(_size < Capacity);
                        pointer p = const_cast<pointer>(position);
                        std::move_backward(p, end(), end()+1);
                        new(p) value_type(args...);
                        ++_size;
                        return p;
                    }
    template <class ... Args>
    void emplace_back(Args ... args)
                    {
                        assert(_size < Capacity);
                        pointer p{data()+_size};
                        new(p) value_type(args...);
                        ++_size;
                    }
    // single element insert()
    iterator insert (const_iterator position, const value_type& val)    {emplace(position, val);}
    // fill insert
    iterator insert (const_iterator position, size_type n, const value_type& val)
                    {
                        assert(_size+n <= Capacity);
                        iterator p = const_cast<pointer>(position);
                        std::move_backward(p, end(), end()+n);
                        for (iterator i = p; i < p+n; ++i) {
                            new(i) val;
                        }
                        _size += n;
                    }
    // range insert()	
    template <class InputIterator>
    iterator insert (const_iterator position, InputIterator first, InputIterator last);
    // move insert()	
    iterator insert (const_iterator position, value_type&& val);
    // initializer list insert()
    iterator insert (const_iterator position, initializer_list<value_type> il);

private:
    using storage_type =
        std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
    uint32_t _size;
    storage_type _elem[Capacity];

    static void verify(bool cond)
        {if (!cond) throw std::out_of_range("static_vector range error");}

    // Push the elements in [begin,end) to the back, preserving order.
    template <typename Iterator>
    void append(Iterator begin, Iterator end) noexcept	
            {
                assert(_size+(end-begin)<=Capacity);
                for (auto i=begin;i<end;i+=1){
                    push_back(*i);
                }
            }
    template <typename Iterator>
    void move_append(Iterator begin, Iterator end) noexcept	
            {
                assert(_size+(end-begin)<=Capacity);
                for (auto i=begin;i<end;i+=1){
                    push_back(std::move(*i));
                }
            }
};
#endif      // ndef STATIC_VECTOR