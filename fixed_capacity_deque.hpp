// fixed_capacity_deque.hpp - implements fixed-capacity deque-like template class
//
// It implements (part of) the deque interface using a fixed-size array.
// The first elements added to it are placed in the middle, and it can
// expand in either direction. The capacity specified in its definition
// is the capacity in either direction.
//
// This has only enough of the deque functionality to satisfy KSolve.

#include <iterator>
#include <cassert>

template <typename T, unsigned Capacity> class fixed_capacity_deque
{
    static constexpr unsigned _trueCap{2*Capacity+1};
    T* _begin;
    T* _end;
    T _elem[_trueCap];
public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef unsigned size_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    fixed_capacity_deque() noexcept
        : _begin(_elem+Capacity)
        , _end(_elem+Capacity)
        {}
    void clear() noexcept {
        for (T&elem: *this)
            elem.~T();      // destruct all elements
        _begin = _end = _elem+Capacity;
    }
    ~fixed_capacity_deque() noexcept{
        clear();
    }
    size_type size() const noexcept{
        return _end-_begin;
    }
    bool empty() const noexcept{
        return _begin==_end;
    }
    template <class ... Args>
    void emplace_front(Args...args) noexcept{
        assert(_elem < _begin);
        new(_begin-1) T(args...);
        _begin -= 1;
    }
    void push_front(const T& t) noexcept{
        emplace_front(t);
    }
    void pop_front() noexcept{
        assert(_begin<_end);
        _begin += 1;
        (_begin-1)->~T();  //destruct
    }
    template <class ... Args>
    void emplace_back(Args...args){
        assert(_end<_elem+_trueCap);
        new(_end) T(args...);
        _end += 1;
    }
    void push_back(const T& t) noexcept{
        emplace_back(t);
    }
    void pop_back() noexcept{
        assert(_begin<_end);
        back().~T();  //destruct
        _end -= 1;
    }
    T& back() noexcept{
        assert(_begin<_end);
        return *(_end-1);
    }
    const T& back() const noexcept{
        assert(_begin<_end);
        return *(_end-1);
    }

    T& operator[](size_type index)noexcept{
        assert(_begin+index<_end);
        return *(_begin+index);
    }
    const T& operator[](size_type index) const noexcept{
        assert(_begin+index<_end);
        return *(_begin+index);
    }
    iterator begin() noexcept{
        return _begin;
    }
    iterator end() {
        return _end;
    }
    const_iterator begin() const noexcept{
        return _begin;
    }
    const_iterator end() const noexcept{
        return _end;
    }

    class const_reverse_iterator: public std::iterator<std::random_access_iterator_tag, T>
    {
        const T* _p;
        explicit const_reverse_iterator(const T* p) noexcept
            : _p(p) {}
        friend class fixed_capacity_deque;
    public:
        const T& operator*() noexcept                               {return *_p;}
        const_reverse_iterator& operator++(/*prefix*/) noexcept     {--_p;return *this;}
        const_reverse_iterator& operator--(/*prefix*/) noexcept     {++_p;return *this;}
        bool operator==(const const_reverse_iterator& other) const noexcept {return _p==other._p;}
        bool operator!=(const const_reverse_iterator& other) const noexcept {return _p!=other._p;}
    };
    const_reverse_iterator crbegin() const noexcept{
        return const_reverse_iterator(_end-1);
    }
    const_reverse_iterator crend() const noexcept{
        return const_reverse_iterator(_begin-1);
    }
};