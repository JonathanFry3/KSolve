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

#include <iterator>
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
            : _begin(Data() + Capacity - 1), _end(Data() + Capacity - 1)
        {
        }
        // fill c'tor with explicit value
        static_deque(size_type count, const_reference value)
            : _begin(Data() + Capacity - 1 - count / 2), _end(_begin + count)
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
            : _begin(Data()+Capacity-1-donor.size()/2)
            , _end(_begin)
        {
            for (auto &m : donor)
                emplace_back(m);
        }
        template <unsigned C1>
        static_deque(const static_deque<value_type, C1> &donor)
            : _begin(Data()+Capacity-1-donor.size()/2)
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
            : _begin(Data()+Capacity-1-donor.size()/2)
            , _end(_begin)
        {
            for (auto &m : donor)
                emplace_back(std::move(m));
        }
        template <unsigned C1>
        static_deque(static_deque<value_type, C1> &&donor)
            : _begin(Data()+Capacity-1-donor.size()/2)
            , _end(_begin)
        {
            assert(donor.size() <= _trueCap);
            for (auto &m : donor)
                emplace_back(std::move(m));
        }
        ~static_deque() noexcept
        {
            clear();
        }
        void clear() noexcept
        {
            for (reference elem : *this)
                elem.~value_type(); // destruct all elements
            _begin = _end = Data() + Capacity - 1;
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
        void emplace_front(Args... args) noexcept
        {
            assert(Data() < _begin);
            new (_begin - 1) value_type(args...);
            _begin -= 1;
        }
        void push_front(const_reference t) noexcept
        {
            emplace_front(t);
        }
        void pop_front() noexcept
        {
            assert(_begin < _end);
            _begin += 1;
            (_begin - 1)->~value_type(); //destruct
        }
        template <class... Args>
        void emplace_back(Args... args)
        {
            assert(_end < Data() + _trueCap);
            new (_end) value_type(args...);
            _end += 1;
        }
        void push_back(const reference t) noexcept
        {
            emplace_back(t);
        }
        void pop_back() noexcept
        {
            assert(_begin < _end);
            back().~value_type(); //destruct
            _end -= 1;
        }
        reference back() noexcept
        {
            assert(_begin < _end);
            return *(_end - 1);
        }
        const_reference back() const noexcept
        {
            assert(_begin < _end);
            return *(_end - 1);
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

    private:
        static constexpr unsigned _trueCap{2 * (Capacity-1) + 1};
        using storage_type =
            std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
        value_type *_begin;
        value_type *_end;
        storage_type _elem[_trueCap];

        pointer Data()
        {
            return reinterpret_cast<pointer>(_elem);
        }
        const_pointer Data() const
        {
            return reinterpret_cast<const_pointer>(_elem);
        }
    };
}