// static_deque.hpp - implements fixed-capacity deque-like template class
//
// It implements (part of) the deque interface using a fixed-size array.
// The first elements added to it are placed in the middle, and it can
// expand in either direction. The capacity specified in its definition
// is the capacity in either direction.
//
// This has only enough of the deque functionality to satisfy KSolve.

#include <iterator>
#include <cassert>

template <typename value_type, unsigned Capacity> 
class static_deque
{
    static constexpr unsigned _trueCap{2*Capacity+1};
    value_type* _begin;
    value_type* _end;
    value_type _elem[_trueCap];
public:
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = uint32_t;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static_deque() noexcept
        : _begin(_elem+Capacity)
        , _end(_elem+Capacity)
        {}
    void clear() noexcept {
        for (reference elem: *this)
            elem.~value_type();      // destruct all elements
        _begin = _end = _elem+Capacity;
    }
    ~static_deque() noexcept{
        clear();
    }
    size_type size() const noexcept{
        return _end-_begin;
    }
    bool empty() const noexcept{
        return _begin == _end;
    }
    template <class ... Args>
    void emplace_front(Args...args) noexcept{
        assert(_elem < _begin);
        new(_begin-1) value_type(args...);
        _begin -= 1;
    }
    void push_front(const_reference  t) noexcept{
        emplace_front(t);
    }
    void pop_front() noexcept{
        assert(_begin<_end);
        _begin += 1;
        (_begin-1)->~value_type();  //destruct
    }
    template <class ... Args>
    void emplace_back(Args...args){
        assert(_end<_elem+_trueCap);
        new(_end) value_type(args...);
        _end += 1;
    }
    void push_back(const reference  t) noexcept{
        emplace_back(t);
    }
    void pop_back() noexcept{
        assert(_begin<_end);
        back().~value_type();  //destruct
        _end -= 1;
    }
    reference back() noexcept{
        assert(_begin<_end);
        return *(_end-1);
    }
    const_reference back() const noexcept{
        assert(_begin<_end);
        return *(_end-1);
    }

    reference operator[](size_type index)noexcept{
        assert(_begin+index<_end);
        return *(_begin+index);
    }
    const_reference operator[](size_type index) const noexcept{
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
    reverse_iterator rbegin() noexcept{
        return reverse_iterator(_end-1);
    }
    reverse_iterator rend() noexcept{
        return reverse_iterator(_begin-1);
    }
    const_reverse_iterator crbegin() const noexcept{
        return const_reverse_iterator(_end-1);
    }
    const_reverse_iterator crend() const noexcept{
        return const_reverse_iterator(_begin-1);
    }
};