// Template class static_vector
//
// One of these has nearly all of the API of a std::vector,
// but has a fixed capacity.  It cannot be extended past that.
// It is safe to use only where the problem limits the size needed.
//
// The implementation uses no dynamic storage; the entire vector
// resides where it is declared.
//
// The functions reserve() and shrink_to_fit() do nothing; the
// function get_allocator() is not implemented.

#ifndef STATIC_VECTOR
#define STATIC_VECTOR
#include <cstdint> 		// for uint32_t
#include <cassert>
#include <iterator>     // std::reverse_iterator
#include <algorithm>    // for std::move...(), equal(), lexicographical_compare()
#include <initializer_list>
#include <stdexcept>    // for std::out_of_range

namespace frystl {

template <class T, uint32_t Capacity>
struct static_vector{
    using this_type = static_vector<T,Capacity>;
    using value_type = T;
    using size_type = uint32_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
//
//******* Public member functions:
//
    static_vector() 						: _size(0){}
    ~static_vector()						{clear();}
    // copy constructors
    static_vector(const this_type& donor) : _size(0)
        {
            for (auto& m:donor) 
                new(data()+_size++) value_type(m);
        }
    template<unsigned C1>
    static_vector(const static_vector<T,C1>& donor) : _size(0) 
        {
            assert(donor.size() <= Capacity);
            for (auto& m:donor) 
                new(data()+_size++) value_type(m);
        }
    // move constructors
    // Constructs the new static_vector by moving all the elements of
    // the existing static_vector.  It leaves the moved-from object
    // unchanged, aside from whatever changes moving its elements
    // made.
    static_vector(this_type&& donor) : _size(0) 
        {
            for (auto& m:donor) 
                new(data()+_size++) value_type(std::move(m));
        }
    template<unsigned C1>
    static_vector(static_vector<T,C1>&& donor) : _size(0) 
        {
            assert(donor.size() <= Capacity);
            for (auto& m:donor) 
                new(data()+_size++) value_type(std::move(m));
        }
    // fill constructors
    static_vector(size_type n, const_reference value) : _size(0)
        {
            for (size_type i=0; i<n; ++i) push_back(value);
        }
    static_vector(size_type n) : static_vector(n,T()) {}
    // range constructor
    template <class InputIterator, 
                typename = std::_RequireInputIter<InputIterator> >       // TODO: not portable
    static_vector(InputIterator begin, InputIterator end) : _size(0)
        {
            for (InputIterator k=begin; k!= end; ++k) emplace_back(*k);
        }
    //
    //  Assignment functions
    void assign(size_type n, const_reference val)
                    {
                        clear();
                        while (size()<n) push_back(val);
                    }
    void assign(std::initializer_list<value_type> x)
                    {
                        assert(x.size() <= capacity());
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
    this_type& operator=(const this_type& other) noexcept
                    {
                        if (this != &other)
                            assign(other.begin(), other.end());
                        return *this;
                    }
    this_type& operator=(this_type&& other) noexcept
                    {
                        if (data() != other.data()) {
                            clear();
                            iterator p = begin();
                            for (auto & o:other)
                                new(p++) value_type(std::move(o));
                            _size = other.size();
                        }
                        return *this;
                    }
    this_type& operator=(std::initializer_list<value_type> il)
                    {
                        assign(il);
                        return *this;
                    }                        
//
//  Element access functions
//
    pointer data() noexcept                         {return reinterpret_cast<pointer>(_elem);}
    reference at(size_type i) 	                    {Verify(i<_size); return data()[i];}
    reference operator[](size_type i) 	            {assert(i<_size); return data()[i];}
    const_reference at(size_type i) const 	        {Verify(i<_size); return data()[i];}
    const_reference operator[](size_type i) const
                                                	{assert(i<_size); return data()[i];}
    const_pointer data() const noexcept             {return reinterpret_cast<const_pointer>(_elem);}
    reference back() noexcept				    	{return data()[_size-1];}
    const_reference back() const noexcept			{return data()[_size-1];}
    reference front() noexcept                      {assert(_size); return data()[0];}
    const_reference front() const noexcept          {assert(_size); return data()[0];}
//
// Iterators
//
    iterator begin() noexcept						{return data();}
    const_iterator begin() const noexcept			{return data();}
    const_iterator cbegin() const noexcept			{return data();}
    iterator end() noexcept							{return data()+_size;}
    const_iterator end() const noexcept				{return data()+_size;}
    const_iterator cend() const noexcept      		{return data()+_size;}
    reverse_iterator rbegin() noexcept              {return reverse_iterator(end());}
    const_reverse_iterator rbegin() const noexcept  {return const_reverse_iterator(end());}
    const_reverse_iterator crbegin() const noexcept {return const_reverse_iterator(cend());}
    reverse_iterator rend() noexcept                {return reverse_iterator(begin());}
    const_reverse_iterator rend() const noexcept    {return const_reverse_iterator(begin());}
    const_reverse_iterator crend()  const noexcept  {return const_reverse_iterator(cbegin());}
//
// Capacity and size
//
    constexpr std::size_t capacity() const noexcept	{return Capacity;}
    constexpr std::size_t max_size() const noexcept	{return Capacity;}
    size_type size() const noexcept				    {return _size;}
    bool empty() const	noexcept					{return _size == 0;}
    void reserve(size_type n)                       {assert(n <= capacity());}
    void shrink_to_fit()                            {}
//
//  Modifiers
//
    void pop_back()	        						{assert(_size); _size -= 1; end()->~value_type();}
    void push_back(const T& cd)     				{emplace_back(cd);}
    void push_back(T&& cd)     				        {emplace_back(std::move(cd));}
    void clear() noexcept							{while (_size) pop_back();}
    iterator erase(const_iterator position) noexcept                 
                    {
                        assert(GoodIter(position+1));
                        iterator x = const_cast<iterator>(position);
                        x->~value_type(); 
                        std::move(x+1,end(),x); 
                        _size -= 1;
                        return x;
                    }
    iterator erase (const_iterator first, const_iterator last)
                    {
                        iterator f = const_cast<iterator>(first);
                        iterator l = const_cast<iterator>(last);
                        if (first != last){
                            assert(GoodIter(first+1));
                            assert(GoodIter(last));
                            assert(first<last);
                            for (iterator it = f; it < l; ++it)
                                it->~value_type();
                            std::move(l, end(), f);
                            _size -= last-first;
                        }
                        return f;
                    }                    
    template <class... Args>
    iterator emplace (const_iterator position, Args&&... args)
                    {
                        assert(_size < Capacity);
                        assert(begin()<=position && position<=end());
                        iterator p = const_cast<iterator>(position);
                        MakeRoom(p,1);
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
    iterator insert (const_iterator position, const value_type& val)
                    {
                        iterator p = const_cast<iterator>(position);
                        return emplace(p, val);
                    }
    // move insert()	
    iterator insert (iterator position, value_type&& val)
                    {
                        assert(begin()<=position && position<=end());
                        return emplace(position, std::move(val));
                    }
    // fill insert
    iterator insert (const_iterator position, size_type n, const value_type& val)
                    {
                        assert(_size+n <= Capacity);
                        assert(begin()<=position && position<=end());
                        iterator p = const_cast<iterator>(position);
                        MakeRoom(p,n);
                        // copy val n times into newly available cells
                        for (iterator i = p; i < p+n; ++i) {
                            new(i) value_type(val);
                        }
                        _size += n;
                        return p;
                    }
    // range insert()	
    template <class InputIterator>
    iterator insert (const_iterator position, InputIterator first, InputIterator last)
                    {
                        iterator p = const_cast<iterator>(position);
                        while (first != last)
                            emplace(p++,*first++);
                        return const_cast<iterator>(position);
                    }
    // initializer list insert()
    iterator insert (const_iterator position, std::initializer_list<value_type> il)
                    {
                        size_type n = il.size();
                        assert(_size+n <= Capacity);
                        assert(begin()<=position && position<=end());
                        iterator p = const_cast<iterator>(position);
                        MakeRoom(p,n);
                        // copy il into newly available cells
                        auto j = il.begin();
                        for (iterator i = p; i < p+n; ++i,++j) {
                            new(i) value_type(*j);
                        }
                        _size += n;
                        return p;
                    }
    void resize (size_type n, const value_type& val)
                    {
                        while (n < size()) pop_back();
                        while (size() < n) push_back(val);
                    }
    void resize (size_type n)
                    {
                        resize(n, value_type());
                    }
    void swap (this_type& x)
                    {
                        std::swap(*this,x);
                    }
//
//******* Private member functions:
//
private:
    using storage_type =
        std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
    size_type _size;
    storage_type _elem[Capacity];

    static void Verify(bool cond)
        {if (!cond) throw std::out_of_range("static_vector range error");}
    // Move cells at and to the right of p to the right by n spaces.
    void MakeRoom(iterator p, size_type n)
            {
                // fill the uninitialized target cells by move construction
                size_type nu = std::min(size_type(end()-p),n);
                for (size_type i = 0; i < nu; ++i)
                    new(end()+n-nu+i) value_type(std::move(*(end()-nu+i)));
                // shift elements to previously occupied cells by assignment
                std::move_backward(p, end()-nu, end());
            }
    // returns true iff it-1 can be dereferenced.
    bool GoodIter(const const_iterator &it)
            {
                return begin() < it && it <= end();
            }
    // 
};
//
//*******  Non-member overloads
//
template <class T, unsigned C0, unsigned C1>
bool operator== (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    if (lhs.size() != rhs.size()) return false;
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
template <class T, unsigned C0, unsigned C1>
bool operator!= (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    return !(rhs == lhs);
}
template <class T, unsigned C0, unsigned C1>
bool operator<  (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),rhs.end());
}
template <class T, unsigned C0, unsigned C1>
bool operator<= (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    return !(rhs < lhs);
}
template <class T, unsigned C0, unsigned C1>
bool operator>  (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    return rhs < lhs;
}
template <class T, unsigned C0, unsigned C1>
bool operator>= (const static_vector<T,C0>& lhs, const static_vector<T,C1>& rhs)
{
    return !(lhs < rhs);
}

template <class T, unsigned C>
void swap(static_vector<T,C>& a, static_vector<T,C>& b)
{
    a.swap(b);
}
}   // namespace frystl
#endif      // ndef STATIC_VECTOR