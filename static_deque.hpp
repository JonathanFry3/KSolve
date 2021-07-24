// static_deque.hpp - implements fixed-capacity deque-like template class
//
// It implements (part of) the deque interface using a fixed-size array.
// The first elements added to it are placed in the middle, and it can
// expand in either direction. The capacity specified in its definition
// is the capacity in either direction, so if its capacity is x and one
// element has been added (either way). x-1 more elements may be pushed
// to the front and x-1 elements may be pushed to the back.
//
// Fill constructors and initializer list constructors center the
// elements within the available space.  Range constructors fill
// from the center toward the back. After the default constructor
// is executed, the next insertion will be to the center cell.

#include <iterator>  // std::reverse_iterator
#include <algorithm> // for std::move...(), equal(), lexicographical_compare()
#include <initializer_list>
#include <stdexcept> // for std::out_of_range
#include <cassert>

namespace frystl
{
    template <typename value_type, unsigned Capacity>
    struct static_deque
    {
        using this_type = static_deque<value_type,Capacity>;
        using reference = value_type &;
        using const_reference = const value_type &;
        using size_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // default c'tor
        static_deque() noexcept
            : _begin(FirstSpace() + Capacity - 1), _end(FirstSpace() + Capacity - 1)
        {
        }
        // fill c'tor with explicit value
        static_deque(size_type count, const_reference value)
            : _begin(FirstSpace() + Capacity - 1 - count / 2), _end(_begin + count)
        {
            assert(count <= 2 * Capacity - 1);
            for (pointer p = _begin; p < _end; ++p)
                new (p) value_type(value);
        }
        // fill c'tor with default value
        static_deque(size_type count)
            : static_deque(count, value_type())
        {
        }
        // range c'tor
        template <class InputIterator,
                  typename = std::_RequireInputIter<InputIterator>> // TODO: not portable
        static_deque(InputIterator begin, InputIterator end)
            : static_deque()
        {
            for (InputIterator k = begin; k != end; ++k)
                emplace_back(*k);
        }
        // copy constructors
        static_deque(const this_type &donor)
            : _begin(Centered(donor.size()))
            , _end(_begin)
        {
            for (auto &m : donor)
                emplace_back(m);
        }
        template <unsigned C1>
        static_deque(const static_deque<value_type, C1> &donor)
            : _begin(FirstSpace()+Capacity-1-donor.size()/2)
            , _end(_begin)
        {
            assert(donor.size() <= _trueCap);
            for (auto &m : donor)
                emplace_back(m);
        }
        // move constructors
        // Constructs the new static_deque by moving all the elements of
        // the existing static_deque.  It leaves the moved-from object
        // unchanged, aside from whatever changes moving its elements
        // made.
        static_deque(this_type &&donor)
            : _begin(Centered(donor.size()))
            , _end(_begin)
        {
            for (auto &m : donor)
                emplace_back(std::move(m));
        }
        template <unsigned C1>
        static_deque(static_deque<value_type, C1> &&donor)
            : _begin(Centered(donor.size()))
            , _end(_begin)
        {
            assert(donor.size() <= _trueCap);
            for (auto &m : donor)
                emplace_back(std::move(m));
        }
        // initializer list constructor
        static_deque(std::initializer_list<value_type> il)
            : _begin(Centered(il.size()))
            , _end(_begin)
        {
            assert(il.size() <= _trueCap);
            for (auto &value : il)
                emplace_back(value);
        }
        ~static_deque() noexcept
        {
            clear();
        }
        void clear() noexcept
        {
            for (reference elem : *this)
                elem.~value_type(); // destruct all elements
            _begin = _end = FirstSpace() + Capacity - 1;
        }
        size_type size() const noexcept
        {
            return _end - _begin;
        }
        bool empty() const noexcept
        {
            return _begin == _end;
        }
        template <class... Args>
        void emplace_front(Args... args)
        {
            assert(FirstSpace() < _begin);
            new (_begin - 1) value_type(args...);
            _begin -= 1;
        }
        void push_front(const_reference t)
        {
            emplace_front(t);
        }
        void pop_front()
        {
            assert(_begin < _end);
            _begin += 1;
            (_begin - 1)->~value_type(); //destruct
        }
        template <class... Args>
        void emplace_back(Args... args)
        {
            assert(_end < FirstSpace() + _trueCap);
            new (_end) value_type(args...);
            _end += 1;
        }
        void push_back(const_reference t) noexcept
        {
            emplace_back(t);
        }
        void pop_back() noexcept
        {
            assert(_begin < _end);
            back().~value_type(); //destruct
            _end -= 1;
        }

        reference operator[](size_type index) noexcept
        {
            assert(_begin + index < _end);
            return *(_begin + index);
        }
        const_reference operator[](size_type index) const noexcept
        {
            assert(_begin + index < _end);
            return *(_begin + index);
        }

        pointer data() noexcept 
        { 
            return _begin; 
        }

        const_pointer data() const noexcept
        { 
            return _begin; 
        }

        reference at(size_type index)
        {
            Verify(_begin + index < _end);
            return *(_begin + index);
        }
        const_reference at(size_type index) const
        {
            Verify(_begin + index < _end);
            return *(_begin + index);
        }
        reference front() noexcept
        {
            assert(_begin < _end);
            return *_begin;
        }
        const_reference front() const noexcept
        {
            assert(_begin < _end);
            return *_begin;
        }
        reference back() noexcept
        {
            assert(_begin < _end);
            return *(_end-1);
        }
        const_reference back() const noexcept
        {
            assert(_begin < _end);
            return *(_end-1);
        }
        template <class... Args>
        iterator emplace(const_iterator pos, Args...args)
        {
            bool atEdge = pos==cbegin() || pos==cend();
            iterator p = MakeRoom(const_cast<iterator>(pos),1);
            if (atEdge)
                new(p) value_type(args...);
            else 
                (*p) = std::move(value_type(args...));
            return p;
        }
        //
        //  Assignment functions
        void assign(size_type n, const_reference val)
        {
            assert(n <= _trueCap);
            clear(); 
            _begin = _end = Centered(n);
            while (size() < n)
                push_back(val);
        }
        void assign(std::initializer_list<value_type> x)
        {
            assert(x.size() <= _trueCap);
            clear();
            _begin = _end = Centered(x.size());
            for (auto &a : x)
                push_back(a);
        }
        template <class Iter,
                  typename = std::_RequireInputIter<Iter>> // TODO: not portable
        void assign(Iter begin, Iter end)
        {
            clear();
            Center(begin,end,
                typename std::iterator_traits<Iter>::iterator_category());
            for (Iter k = begin; k != end; ++k) {
                assert(_end < FirstSpace()+_trueCap);
                push_back(*k);
            }
        }
        iterator begin() noexcept
        {
            return _begin;
        }
        iterator end()
        {
            return _end;
        }
        const_iterator begin() const noexcept
        {
            return _begin;
        }
        const_iterator end() const noexcept
        {
            return _end;
        }
        const_iterator cbegin() noexcept
        {
            return _begin;
        }
        const_iterator cend() noexcept
        {
            return _end;
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(_end);
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(_begin);
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(_end);
        }
        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(_begin);
        }
        iterator erase(const_iterator first, const_iterator last)
        {
            const iterator f = const_cast<iterator>(first);
            const iterator l = const_cast<iterator>(last);
            iterator result;
            if (first != last)
            {
                assert(GoodIter(first + 1));
                assert(GoodIter(last));
                assert(first < last);
                unsigned nToErase = last-first;
                for (iterator it = f; it < l; ++it)
                    it->~value_type();
                if (first-_begin < _end-last) {
                    // Move the elements before first
                    std::move_backward(_begin,f,l);
                    _begin += nToErase;
                    result = l;
                } else {
                    // Move the elements at and after last
                    std::move(l, end(), f);
                    _end -= nToErase;
                    result = f;
                }
            }
            return result;
        }
        iterator erase(const_iterator position) noexcept
        {
            return erase(position, position+1);
        }

    private:
        static constexpr unsigned _trueCap{2 * (Capacity-1) + 1};
        using storage_type =
            std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
        pointer _begin;
        pointer _end;
        storage_type _elem[_trueCap];

        pointer FirstSpace()
        {
            return reinterpret_cast<pointer>(_elem);
        }
        const_pointer Data() const
        {
            return reinterpret_cast<const_pointer>(_elem);
        }
        static void Verify(bool cond)
        {
            if (!cond)
                throw std::out_of_range("static_deque range error");
        }
        // returns true iff it-1 can be dereferenced.
        bool GoodIter(const const_iterator &it)
        {
            return begin() < it && it <= end();
        }
        // Slide cells at and behind p to the back by n spaces.
        // Return an iterator pointing to the first cleared cell (p).
        // Update _end.
        iterator MakeRoomAfter(iterator p, size_type n)
        {
            assert(end()+n <= FirstSpace()+_trueCap);
            iterator src = end();
            iterator tgt = src+n;
            // Fill the uninitialized target cells by move construction
            while(p<src && end()<tgt)
                new (--tgt) value_type(std::move(*(--src)));       
            // Shift elements to previously occupied cells by assignment
            std::move_backward(p, src, tgt);
            _end += n;
            return p;
        }
        // Slide cells before p to the front by n spaces.
        // Return an iterator pointing to the first cleared cell (p-n).
        // Update _begin.
        iterator MakeRoomBefore(iterator p, size_type n)
        {
            iterator src = begin();
            iterator tgt = src-n;
            assert(FirstSpace() <= tgt);
            // fill the uninitialized target cells by move construction
            while (src < p && tgt < begin())
                new (tgt++) value_type(std::move(*(src++)));
            // shift elements to previously occupied cells by move assignment
            std::move(src,p,tgt);
            _begin -= n;
            return p-n;
        }
        // Slide cells toward the front or back to make room for n elements
        // before p.  Choose the faster direction. Update _begin or _end.
        // Return an iterator pointing to the first cleared space. 
        iterator MakeRoom(iterator p, size_type n)
        {
            if (end()-p < p-begin())
                return MakeRoomAfter(p, n);
            else
                return MakeRoomBefore(p, n);
        }
        // Return a pointer to the front end of a range of n cells centered
        // in the space.
        pointer Centered(unsigned n)
        {
            return FirstSpace()+Capacity-1-n/2;
        }
        template <class RAIter>
        void Center(RAIter begin, RAIter end,std::random_access_iterator_tag)
        {
            assert(end-begin <= _trueCap);
            _begin = _end = Centered(end-begin);
        }
        template <class InpIter>
        void Center(InpIter begin, InpIter end,std::input_iterator_tag)
        {
            // do nothing
        }
    };
}