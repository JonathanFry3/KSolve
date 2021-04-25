// mf_vector.hpp - implements a malloc-friendly vector-like template class
//
// mf_vector<T> allocates blocks of memory whose default size is the larger
// of 16*sizeof(T) or 4096 bytes.  It keeps track of them in a std::vector.
// Think of it a a vector implemented with the storage technique of a deque.
// Since it does not have to support changes at the front, it may be faster
// than a deque.
//
// In a std::vector, looping though members using [] is about as fast
// as using an iterator.  With an mf_vector (or a std::deque),
// an iterator is faster, especially so if sizeof(T) is not a power of 2.
//
// Data Races: emplace_back() and push_back() modify the container.  If
// reallocation happens in the vector that tracks the storage blocks,
// all elements of that vector are modified.  Otherwise, no existing element
// is accessed and concurrently accessing or modifying them is safe.
//
// Pointers and References: if erase() or insert() is (implemented and)
// used, pointers and references to elements after the target will be
// invalid. Otherwise, elements remain in the same place in memory,
// so pointers and references remain valid.
//
// The functions reserve(), shrink_to_fit(), capacity(), and data()
// are not implemented.
#ifndef MF_VECTOR
#define MF_VECTOR

#include <utility>   // max
#include <cassert>   // assert
#include <stdexcept> // std::out_of_range
#include <iterator>  // std::reverse_iterator
#include <vector>
#include <initializer_list>

namespace frystl
{
    template <
        class T,
        unsigned BlockSize // Number of T elements per block.
                           // Powers of 2 are faster.
        = std::max<unsigned>(4096 / sizeof(T), 16)>
    class mf_vector
    {
    public:
        typedef const T *const_pointer;
        typedef T value_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T *pointer;
        typedef size_t size_type;

    private:
        struct Locater
        {
            pointer _block;
            unsigned _offset;
            Locater(pointer block, unsigned offset) noexcept
                : _block(block), _offset(offset)
            {
            }
        };

    public:
        template <typename ValueType>
        struct Iterator
        {
            using iterator_category = std::random_access_iterator_tag;
            using value_type = ValueType;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;
            Iterator() = delete;
            Iterator(const Iterator &) = default;
            // implicit conversion operator iterator -> const_iterator
            operator Iterator<ValueType const>()
            {
                return Iterator<ValueType const>(_vector, _index, _locater, _location);
            }
            Iterator operator++() noexcept  // prefix increment, as in ++iter
            {
                _index += 1;
                _locater._offset += 1;
                if (_locater._offset == _blockSize)
                {
                    _locater = _vector->GetLocater(_index);
                    _location = _locater._block + _locater._offset;
                }
                else
                {
                    _location += 1;
                }
                return *this;
            }
            Iterator operator++(int i) noexcept  // postfix increment, as in iter++
            {
                Iterator result = *this;
                _index += 1;
                _locater._offset += 1;
                if (_locater._offset == _blockSize)
                {
                    _locater = _vector->GetLocater(_index);
                    _location = _locater._block + _locater._offset;
                }
                else
                {
                    _location += 1;
                }
                return result;
            }
            Iterator operator--() noexcept  // prefix decrement, as in --iter;
            { 
                _index -= 1;
                if (_locater._offset == 0)
                {
                    _locater = _vector->GetLocater(_index);
                    _location = _locater._block + _locater._offset;
                }
                else
                {
                    _locater._offset -= 1;
                    _location -= 1;
                }
                return *this;
            }
            Iterator operator--(int i) noexcept // postfix decrement, as in iter--;
            { 
                Iterator result = *this;
                _index -= 1;
                if (_locater._offset == 0)
                {
                    _locater = _vector->GetLocater(_index);
                    _location = _locater._block + _locater._offset;
                }
                else
                {
                    _locater._offset -= 1;
                    _location -= 1;
                }
                return result;
            }
            bool operator==(const Iterator &other) const noexcept
            {
                return _index == other._index;
            }
            bool operator!=(const Iterator &other) const noexcept
            {
                return !(*this == other);
            }
            bool operator<(const Iterator &other) const noexcept
            {
                return _index < other._index;
            }
            bool operator<=(const Iterator &other) const noexcept
            {
                return !(other < *this);
            }
            bool operator>(const Iterator &other) const noexcept
            {
                return other < *this;
            }
            bool operator>=(const Iterator &other) const noexcept
            {
                return !(other < *this);
            }
            int operator-(const Iterator &o) const noexcept
            {
                return _index - o._index;
            }
            reference operator*() const noexcept
            {
                assert(_index < _vector->size());
                return *_location;
            }
            pointer operator->() const noexcept
            {
                return _location;
            }
            Iterator operator+(std::ptrdiff_t i) const noexcept
            {
                return Iterator(_vector, _index + i);
            }
            Iterator operator-(std::ptrdiff_t i) const noexcept
            {
                return Iterator(_vector, _index - i);
            }
            Iterator operator+=(std::ptrdiff_t i) noexcept
            {
                if (i == 1)
                    ++(*this);
                else if (i == -1)
                    --(*this);
                else
                    *this = Iterator(_vector, _index + i);
                return *this;
            }
            Iterator operator-=(std::ptrdiff_t i) noexcept
            {
                assert(_index > 0);
                if (i == 1)
                    --(*this);
                else if (i == -1)
                    ++(*this);
                else
                    *this = Iterator(_vector, _index - i);
                return *this;
            }
            reference operator[](std::ptrdiff_t i) const noexcept
            {
                std::ptrdiff_t j = j + _index;
                assert(0 <= j && j < _vector->size());
                return *Iterator(_vector, j);
            }

        private:
            friend class mf_vector<T, BlockSize>;
            mf_vector<T, BlockSize> *_vector;
            pointer _location;
            size_type _index;
            Locater _locater;
            explicit Iterator(mf_vector *vector, size_type index) noexcept
                : _vector(vector), _index(index), _locater(vector->GetLocater(index)), _location(_locater._block + _locater._offset)
            {
            }
            explicit Iterator(
                mf_vector *const vector,
                size_type index,
                Locater &locater,
                pointer location) noexcept
                : _vector(vector), _index(index), _locater(locater), _location(location)
            {
            }
        };
        using const_iterator = Iterator<T const>;
        friend const_iterator;

        using iterator = Iterator<T>;
        friend iterator;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // Constructors
        mf_vector() // default c'tor
            : _size(0), _end(nullptr, _blockSize)
        {
            _blocks.reserve(_blockSize);
        }
        // Fill constructors
        explicit mf_vector(size_type count, const_reference value)
            : mf_vector()
        {
            for (size_type i = 0; i < count; ++i)
                push_back(value);
        }
        explicit mf_vector(size_type count)
            : mf_vector(count, T())
        {
        }
        // Range constructors
        template <class InputIt,
                  typename = std::_RequireInputIter<InputIt>> // TODO: not portable
        mf_vector(InputIt first, InputIt last)
            : mf_vector()
        {
            for (InputIt i = first; i != last; ++i)
                push_back(*i);
        }
        // Copy Constructors
        mf_vector(const mf_vector &other)
            : mf_vector()
        {
            for (auto &value : other)
                push_back(value);
        }
        template <unsigned B1>
        mf_vector(const mf_vector<T, B1> &other)
            : mf_vector()
        {
            for (auto &value : other)
                push_back(value);
        }
        // Move constructors
        mf_vector(mf_vector &&other)
            : mf_vector()
        {
            for (auto &&value : other)
                MovePushBack(std::move(value));
        }
        template <size_type B1>
        mf_vector(mf_vector<T, B1> &&other)
            : mf_vector()
        {
            for (auto &&value : other)
                MovePushBack(std::move(value));
        }
        // initializer list constructor
        mf_vector(std::initializer_list<value_type> il)
            : mf_vector()
        {
            for (auto &value : il)
                push_back(value);
        }
        void clear() noexcept
        {
            for (auto &m : *this)
                m.~T(); // destruct all
            _size = 0;
            Shrink(); // free memory
        }
        ~mf_vector() noexcept
        {
            clear();
        }
        size_type size() const noexcept
        {
            return _size;
        }
        bool empty() const noexcept
        {
            return size() == 0;
        }
        template <class... Args>
        void emplace_back(Args... args)
        {
            Grow(_size + 1);
            new (_end._block + _end._offset - 1) value_type(args...);
            _size += 1;
        }
        template <class... Args>
        iterator emplace(const_iterator position, Args... args)
        {
            iterator pos = MakeIterator(position);
            MakeRoom(pos, 1);
            new (pos._location) T(args...);
            _size += 1;
            return pos;
        }
        void push_back(const_reference t)
        {
            emplace_back(t);
        }
        void pop_back() noexcept
        {
            assert(_size);
            back().~T(); //destruct
            _size -= 1;
            if (_end._offset == 1)
            {
                Shrink();
            }
            else
            {
                _end._offset -= 1;
            }
        }
        reference back() noexcept
        {
            assert(_end._offset > 0);
            assert(_end._block != nullptr);
            return _end._block[_end._offset - 1];
        }
        const_reference back() const noexcept
        {
            assert(_end._offset > 0);
            assert(_end._block != nullptr);
            return _end._block[_end._offset - 1];
        }
        reference front() noexcept
        {
            assert(_size);
            return (*this)[0];
        }
        const_reference front() const noexcept
        {
            assert(_size);
            return (*this)[0];
        }

        reference at(size_type index)
        {
            Verify(index < _size);
            Locater d(GetLocater(index));
            return d._block[d._offset];
        }
        const_reference at(size_type index) const
        {
            Verify(index < _size);
            Locater d(GetLocater(index));
            return d._block[d._offset];
        }
        reference operator[](size_type index) noexcept
        {
            assert(index < _size);
            Locater d(GetLocater(index));
            return d._block[d._offset];
        }
        const_reference operator[](size_type index) const noexcept
        {
            assert(index < _size);
            Locater d(GetLocater(index));
            return d._block[d._offset];
        }
        iterator erase(const_iterator first, const_iterator last) noexcept
        {
            iterator f = MakeIterator(first);
            assert(GoodRange(first, last));
            if (first < last)
            {
                iterator l = MakeIterator(last);
                for (iterator it = f; it < l; ++it)
                    it->~value_type();
                std::move(l, end(), f);
                _size -= last - first;
                Shrink();
            }
            return f;
        }
        iterator erase(const_iterator position) noexcept
        {
            return erase(position, position + 1);
        }
        //
        //  Assignment functions
        void assign(size_type n, const_reference val)
        {
            clear();
            while (size() < n)
                push_back(val);
        }
        void assign(std::initializer_list<value_type> x)
        {
            clear();
            for (auto &a : x)
                push_back(a);
        }
        template <class InputIterator,
                  typename = std::_RequireInputIter<InputIterator>> // TODO: not portable
        void assign(InputIterator begin, InputIterator end)
        {
            clear();
            for (InputIterator k = begin; k != end; ++k)
                push_back(*k);
        }
        mf_vector &operator=(const mf_vector &other) noexcept
        {
            if (this != &other)
                assign(other.begin(), other.end());
            return *this;
        }
        mf_vector &operator=(mf_vector &&other) noexcept
        {
            if (this != &other)
            {
                clear();
                Grow(other.size());
                iterator p = begin();
                for (auto &o : other)
                    new (p++._location) value_type(std::move(o));
                _size = other.size();
            }
            return *this;
        }
        mf_vector &operator=(std::initializer_list<value_type> il)
        {
            assign(il);
            return *this;
        }

        // move insert()
        iterator insert(iterator position, value_type &&val)
        {
            assert(GoodIter(position));
            return emplace(position, std::move(val));
        }
        // fill insert()
        iterator insert(const_iterator position, size_type n, const value_type &val)
        {
            assert(GoodIter(position));
            iterator p = MakeIterator(position);
            MakeRoom(p, n);
            // copy val n times into newly available cells
            for (iterator i = p; i < p + n; ++i)
            {
                new (i._location) value_type(val);
            }
            _size += n;
            return p;
        }
        // Range insert()
        template <class InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last)
        {
            iterator p = MakeIterator(position);
            while (first != last)
                emplace(p++, *first++);
            return MakeIterator(position);
        }
        // initializer list insert()
        iterator insert(const_iterator position, std::initializer_list<value_type> il)
        {
            size_type n = il.size();
            assert(GoodIter(position));
            iterator p = MakeIterator(position);
            MakeRoom(p, n);
            // copy il into newly available cells
            auto j = il.begin();
            for (iterator i = p; i < p + n; ++i, ++j)
            {
                new (i._location) value_type(*j);
            }
            _size += n;
            return p;
        }
        void resize(size_type n, const value_type &val)
        {
            while (n < size())
                pop_back();
            while (size() < n)
                push_back(val);
        }
        void resize(size_type n)
        {
            resize(n, value_type());
        }
        iterator begin() noexcept
        {
            return iterator(this, 0);
        }
        iterator end() noexcept
        {
            return iterator(this, _size);
        }
        const_iterator begin() const noexcept
        {
            return const_iterator(const_cast<mf_vector *>(this), 0);
        }
        const_iterator end() const noexcept
        {
            return const_iterator(const_cast<mf_vector *>(this), _size);
        }
        const_iterator cbegin() const noexcept
        {
            return const_iterator(const_cast<mf_vector *>(this), 0);
        }
        const_iterator cend() const noexcept
        {
            return const_iterator(const_cast<mf_vector *>(this), _size);
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }
        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }

    private:
        std::vector<pointer> _blocks;
        size_type _size;
        static const unsigned _blockSize = BlockSize;
        // Invariant: on exit from any public function,
        // 0 < end._offset <= _blockSize
        Locater _end;

        Locater GetLocater(size_type index) const noexcept
        {
            assert(index <= _size);
            if (index == _size)
            {
                return _end;
            }
            else
            {
                size_type which_block = index / _blockSize;
                pointer const block = _blocks[which_block];
                const unsigned offset = index % _blockSize;
                return Locater(block, offset);
            }
        }
        // Grow capacity to newSize.  Adjust _end for new size.  Don't touch _size.
        void Grow(size_type newSize)
        {
            using storage_type =
                std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
            size_type newBlocks = (newSize + _blockSize - 1) / _blockSize;
            while (newBlocks < _blocks.size())
            {
                _end._block = reinterpret_cast<pointer>(new storage_type[_blockSize]);
                _blocks.push_back(_end._block);
            }
            _end._offset = newSize % _blockSize;
        }
        // Release any no-longer-needed storage blocks.  Adjust _end to new _size;
        // Expects _size already reflects the value(s) being erased.
        void Shrink() noexcept
        {
            size_type cap = _blockSize * _blocks.size();
            while (_size + _blockSize <= cap)
            {
                free(_blocks.back());
                _blocks.pop_back();
                cap -= _blockSize;
            }
            if (_blocks.size())
                _end._block = _blocks.back();
            else
                _end._block = nullptr;
            _end._offset = (_blockSize + _size - 1) % _blockSize + 1;
            assert((_size + _blockSize - 1) / _blockSize == _blocks.size());
        }
        // Make n spaces available starting at pos.  Shift
        // all elements at and after pos right by n spaces.
        // Adjusts _end but not _size.
        void MakeRoom(iterator pos, size_type n)
        {
            iterator oldEnd = end();
            Grow(_size + n);
            std::move_backward(pos, oldEnd, end());
        }
        void MovePushBack(T &&value)
        {
            Grow(_size + 1);
            new (_end._block + _end._offset - 1) T(std::move(value));
            _size += 1;
        }
        static void Verify(bool cond)
        {
            if (!cond)
                throw std::out_of_range("mf_vector range error");
        }
        iterator MakeIterator(const const_iterator &ci) noexcept
        {
            return iterator(ci._vector, ci._index,
                            const_cast<Locater &>(ci._locater), const_cast<pointer>(ci._location));
        }
        bool GoodIter(const const_iterator &it)
        {
            return it._vector == this && it._index <= _size;
        }
        bool Dereferenceable(const const_iterator &it)
        {
            return it._vector == this && it._index < _size;
        }
        bool GoodRange(const_iterator &first, const_iterator &last)
        {
            return Dereferenceable(first) && GoodIter(last) && first._index <= last._index;
        }
    }; // template class mf_vector
    //
    //*******  Non-member overloads
    //
    template <class T, unsigned C0, unsigned C1>
    bool operator==(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    template <class T, unsigned C0, unsigned C1>
    bool operator!=(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        return !(rhs == lhs);
    }
    template <class T, unsigned C0, unsigned C1>
    bool operator<(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    template <class T, unsigned C0, unsigned C1>
    bool operator<=(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        return !(rhs < lhs);
    }
    template <class T, unsigned C0, unsigned C1>
    bool operator>(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        return rhs < lhs;
    }
    template <class T, unsigned C0, unsigned C1>
    bool operator>=(const mf_vector<T, C0> &lhs, const mf_vector<T, C1> &rhs)
    {
        return !(lhs < rhs);
    }
}; // namespace frystl
#endif      // ndef MF_VECTOR