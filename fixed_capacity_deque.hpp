// fixed_capacity_deque.hpp - implements fixed-capacity deque-like template class
//
// It implements (part of) the deque interface using a fixed-size array.
// The first elements added to it are placed in the middle, and it can
// expand in either direction. The capacity specified in its definition
// is the capacity in either direction.
//
// This has only enough of the vector functionality to satisfy KSolve.

#include <iterator>
#include <cassert>

template <typename T, unsigned Capacity> class fixed_capacity_deque
{
    T* _begin;
    T* _end;
    T _elem[2*Capacity+1];
public:
	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef unsigned size_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    fixed_capacity_deque()
        : _begin(_elem+Capacity)
        , _end(_elem+Capacity)
        {}
	void clear() {
        for (T&elem: *this)
            elem.~T();      // destruct all elements
        _begin = _end = _elem+Capacity;
	}
	~fixed_capacity_deque() {
		clear();
	}
	size_type size() const{
		return _end-_begin;
	}
	bool empty() const {
		return size() == 0;
	}
	template <class ... Args>
	void emplace_front(Args...args){
        assert(_elem < _begin);
		new(_begin-1) T(args...);
		_begin -= 1;
	}
	void push_front(const T& t){
		emplace_front(t);
	}
	void pop_front(){
        assert(_begin<_end);
		_begin->~T();  //destruct
        _begin += 1;
    }
	template <class ... Args>
	void emplace_back(Args...args){
        assert(_end<_elem+2*Capacity+1);
		new(_end) T(args...);
		_end += 1;
	}
	void push_back(const T& t){
		emplace_back(t);
	}
	void pop_back(){
        assert(_begin<_end);
		back().~T();  //destruct
        _end -= 1;
	}
	T& back() {
        assert(_begin<_end);
		return *(_end-1);
	}
	const T& back() const {
        assert(_begin<_end);
		return *(_end-1);
	}

	T& operator[](size_type index){
        assert(_begin+index<_end);
        return *(_begin+index);
	}
	const T& operator[](size_type index) const{
        assert(_begin+index<_end);
        return *(_begin+index);
	}
	iterator begin() {
		return _begin;
	}
	iterator end() {
		return _end;
	}
	const_iterator begin() const {
		return _begin;
	}
	const_iterator end() const{
		return _end;
	}

    class const_reverse_iterator: std::iterator<std::random_access_iterator_tag, T>
    {
        const T* _p;
        explicit const_reverse_iterator(const T* p)
            : _p(p) {}
        friend class fixed_capacity_deque;
    public:
        const T& operator*()                                        {return *_p;}
        const_reverse_iterator& operator++(/*prefix*/)              {--_p;return *this;}
        const_reverse_iterator& operator--(/*prefix*/)              {++_p;return *this;}
        bool operator==(const const_reverse_iterator& other) const  {return _p==other._p;}
        bool operator!=(const const_reverse_iterator& other) const  {return _p!=other._p;}
    };
    const_reverse_iterator crbegin() const{
        return const_reverse_iterator(_end-1);
    }
    const_reverse_iterator crend() const{
        return const_reverse_iterator(_begin-1);
    }
};