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

#include <vector>
#include <utility>	// max
#include <iterator>
#include <cassert>
#include <cstdlib>	// malloc, free
#include <new>		// bad_alloc

namespace frystl {
template <
    class T, 
    size_t BlockSize			// Number of T elements per block.
                                // Powers of 2 are faster.
        =std::max<size_t>(4096/sizeof(T), 16)
    > 
class mf_vector
{
public:
    typedef const T* const_pointer;
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* pointer;
    typedef size_t size_type;
    using this_type = mf_vector<T,BlockSize>;
private:
    struct Locater
    {
        pointer _block;
        unsigned _offset;
        Locater(pointer block, unsigned offset) noexcept
            : _block(block)
            , _offset(offset)
        {}
    };
    std::vector<pointer> _blocks;
    size_t _size;
    static const unsigned _blockSize = BlockSize;
    // Invariant: on exit from any public function,
    // 0 < end._offset <= _blockSize
    Locater _end;

    Locater GetLocater(size_t index) const noexcept{
        assert(index <= _size);
        if (index == _size) {
            return _end;
        } else {
            size_t which_block = index/_blockSize;
            pointer const block = _blocks[which_block];
            const unsigned offset = index%_blockSize;
            return Locater(block,offset);
        }
    }
    void AllocBack() {
        _end._block = reinterpret_cast<pointer>(malloc(sizeof(T)*_blockSize));
        if (!_end._block) throw std::bad_alloc();
        _end._offset = 0;
        _blocks.push_back(_end._block);
    }
    void DeallocBack() noexcept {
        assert(_blocks.size());
        free(_blocks.back());
        _blocks.pop_back();
        if (_blocks.size())
            _end._block = _blocks.back();
        else
            _end._block = nullptr;
        _end._offset = _blockSize;
    }
public:

    class iterator: public std::iterator<std::random_access_iterator_tag, T> 
    {
        friend class mf_vector<T,BlockSize>;
        mf_vector<T,BlockSize>* _vector;
        size_t _index;
        Locater _locater;
        pointer _location;
        explicit iterator(mf_vector* vector, size_t index) noexcept
            : _vector(vector)
            , _index(index)
            , _locater(vector->GetLocater(index))
            , _location(_locater._block+_locater._offset)
            {}
    public:
        iterator operator++() noexcept {		// prefix increment, as in ++iter;
            _index += 1;
            _locater._offset += 1;
            if (_locater._offset == _blockSize){
                _locater = _vector->GetLocater(_index);
                _location = _locater._block+_locater._offset;
            } else {
                _location += 1;
            }
            return *this;
        }
        iterator operator--() noexcept {		// prefix decrement, as in --iter;
            _index -= 1;
            if (_locater._offset == 0){
                _locater = _vector->GetLocater(_index);
                _location = _locater._block+_locater._offset;
            } else {
                _locater._offset -= 1;
                _location -= 1;
            }
            return *this;
        }
        bool operator==(const iterator& other) noexcept {
            return _index == other._index;
        }
        bool operator!=(const iterator& other) noexcept {
            return !(*this == other);
        }
        bool operator<(const iterator& other) noexcept {
            return _index < other._index;
        }
        int operator-(const iterator& o) noexcept {
            return _index - o._index;
        }
        reference operator*() noexcept {
            return *_location;
        }
        pointer operator->() noexcept {
            return _location;
        }
        iterator operator+(std::ptrdiff_t i) noexcept {
            return iterator(_vector, _index+i);
        }
        iterator operator-(std::ptrdiff_t i) noexcept {
            return iterator(_vector, _index-i);
        }
    };
    friend class iterator;

    class const_iterator: public std::iterator<std::random_access_iterator_tag, T> 
    {
        friend class mf_vector<T,BlockSize>;
        const mf_vector<T,BlockSize>* _vector;
        size_t _index;
        Locater _locater;
        const_pointer _location;
        explicit const_iterator(const mf_vector* vector, size_t index) noexcept
            : _vector(vector)
            , _index(index)
            , _locater(vector->GetLocater(index))
            , _location(_locater._block+_locater._offset)
            {}
    public:
        const_iterator operator++() noexcept {		// prefix increment, as in ++iter;
            _index += 1;
            _locater._offset += 1;
            if (_locater._offset == _blockSize){
                _locater = _vector->GetLocater(_index);
                _location = _locater._block+_locater._offset;
            } else {
                _location += 1;
            }
            return *this;
        }
        const_iterator operator--() noexcept {		// prefix decrement, as in --iter;
            _index -= 1;
            if (_locater._offset == 0){
                _locater = _vector->GetLocater(_index);
                _location = _locater._block+_locater._offset;
            } else {
                _locater._offset -= 1;
                _location -= 1;
            }
            return *this;
        }
        bool operator==(const const_iterator& other) noexcept {
            return _index == other._index;
        }
        bool operator!=(const const_iterator& other) noexcept {
            return !(*this == other);
        }
        bool operator<(const const_iterator& other) noexcept {
            return _index < other._index;
        }
        int operator-(const const_iterator& o) noexcept {
            return _index - o._index;
        }
        const_reference operator*() noexcept {
            return *_location;
        }
        const_pointer operator->() noexcept {
            return _location;
        }
        const_iterator operator+(std::ptrdiff_t i) noexcept {
            return const_iterator(_vector, _index+i);
        }
        const_iterator operator-(std::ptrdiff_t i) noexcept {
            return const_iterator(_vector, _index-i);
        }
    };
    friend class iterator;

    // Constructors
    mf_vector()             // default c'tor
        : _size(0)
        , _end(nullptr,_blockSize)
        {
            _blocks.reserve(_blockSize);
        }
    // Fill constructors
    explicit mf_vector( size_type count, const_reference value ) 
        : mf_vector()
        {
            for (size_type i = 0; i < count; ++i)
                push_back(value);
        }
    explicit mf_vector( size_type count ) 
        : mf_vector(count, T())
        {}
    // Range constructors
    template< class InputIt , 
                typename = std::_RequireInputIter<InputIt> >        // TODO: not portable
    mf_vector( InputIt first, InputIt last)
        : mf_vector()
        {
            for (InputIt i = first; i != last; ++i)
                push_back(*i);
        }
    // Copy Constructors
    mf_vector(const this_type& other)                               // TODO: don't know why we need this
        : mf_vector()
        {
            for (auto& value: other) 
                push_back(value);
        }
    template<size_type B1>
    mf_vector(const mf_vector<T,B1>& other)
        : mf_vector()
        {
            for (auto& value: other) 
                push_back(value);
        }
    void clear() noexcept {
        for (auto&m:*this) m.~T();	//destruct all
        while (_blocks.size()) {
            DeallocBack();
        }
        _size = 0;
    }
    ~mf_vector() noexcept {
        clear();
    }
    size_t size() const noexcept{
        return _size;
    }
    bool empty() const noexcept{
        return size() == 0;
    }
    template <class ... Args>
    void emplace_back(Args...args){
        if (_end._offset == _blockSize)
            AllocBack();
        new(_end._block+_end._offset) T(args...);
        _end._offset += 1;
        _size += 1;
    }
    void push_back(const_reference t){
        emplace_back(t);
    }
    void pop_back() noexcept{
        assert(_size);
        back().~T();  //destruct
        _size -= 1;
        if (_end._offset == 1) {
            DeallocBack();
        } else {
            _end._offset -= 1;
        }
    }
    reference back() noexcept {
        assert(_end._offset > 0);
        assert(_end._block!=nullptr);
        return _end._block[_end._offset-1];
    }
    const_reference back() const noexcept {
        assert(_end._offset > 0);
        assert(_end._block!=nullptr);
        return _end._block[_end._offset-1];
    }
    reference front() noexcept {
        assert(_size);
        return (*this)[0];
    }
    const_reference front() const noexcept {
        assert(_size);
        return (*this)[0];
    }

    reference operator[](size_t index) noexcept {
        assert(index < _size);
        Locater d(GetLocater(index));
        return d._block[d._offset];
    }
    const_reference operator[](size_t index) const noexcept{
        assert(index < _size);
        Locater d(GetLocater(index));
        return d._block[d._offset];
    }
    iterator begin() noexcept {
        return iterator(this,0);
    }
    iterator end() noexcept {
        return iterator(this,_size);
    }
    const_iterator begin() const noexcept {
        return const_iterator(this,0);
    }
    const_iterator end() const noexcept {
        return const_iterator(this,_size);
    }
};  // template class mf_vector
}   // namespace frystl