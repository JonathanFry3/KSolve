// mf_vector.hpp - implements a malloc-friendly vector-like template class
//
// mf_vector<T> allocates blocks of memory whose size is the larger
// of 16*sizeof(T) or 4096 bytes.  It keeps track of them in a std::vector.
// Think of it a a vector implemented with the storage technique of a deque.
// Since it does not have to support changes at the front, it is faster
// than a deque.
//
// In a std::vector, looping though members using [] is about as fast
// as using an iterator.  With an mf_vector (or a std::deque), 
// an iterator is faster.
//
// This has only enough of the vector functionality to satisfy KSolve.

#include <vector>
#include <utility>	// max
#include <iterator>
#include <cassert>
#include <cstdlib>	// malloc, free
#include <new>		// bad_alloc

template <class T> class mf_vector
{
	struct Locater
	{
		T* _block;
		unsigned _offset;
		Locater(T*block, unsigned offset)
			: _block(block)
			, _offset(offset)
		{}
	};
	std::vector<T*> _blocks;
	size_t _size;
	static const unsigned _blockSize = 
			std::max<size_t>(4096/sizeof(T), 16);
	// Invariant: on exit from any public function,
	// 0 < end._offset <= _blockSize
	Locater _end;

	Locater GetLocater(size_t index) const {
		assert(index <= _size);
		if (index == _size) {
			return _end;
		} else {
			size_t which_block = index/_blockSize;
			T* block = _blocks[which_block];
			unsigned offset = index%_blockSize;
			return Locater(block,offset);
		}
	}
	void AllocBack() {
		_end._block = reinterpret_cast<T*>(malloc(sizeof(T)*_blockSize));
		if (!_end._block) throw std::bad_alloc();
		_end._offset = 0;
		_blocks.push_back(_end._block);
	}
	void DeallocBack(){
		assert(_blocks.size());
		T* block = _blocks.back();
		free(block);
		_blocks.pop_back();
		if (_blocks.size())
			_end._block = _blocks.back();
		else
			_end._block = nullptr;
		_end._offset = _blockSize;
	}
public:
	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef size_t size_type;
	class iterator: std::iterator<std::random_access_iterator_tag, T> 
	{
		friend class mf_vector<T>;
		mf_vector<T>* _vector;
		size_t _index;
		Locater _locater;
		T* _location;
		explicit iterator(mf_vector* vector, size_t index)
			: _vector(vector)
			, _index(index)
			, _locater(vector->GetLocater(index))
			, _location(_locater._block+_locater._offset)
			{}
	public:
		T& operator++(){		// prefix increment, as in ++iter;
			_index += 1;
			_locater._offset += 1;
			if (_locater._offset == _blockSize){
				_locater = _vector->GetLocater(_index);
				_location = _locater._block;
			} else {
				_location += 1;
			}
			return *_location;
		}
		bool operator==(const iterator& other){
			return _location == other._location;
		}
		bool operator!=(const iterator& other){
			return !(*this == other);
		}
		T& operator*() {
			return *_location;
		}
	};
	friend class iterator;
	mf_vector()
		: _size(0)
		, _end(nullptr,_blockSize)
		{}
	void clear() {
		for (auto&m:*this) m.~T();	//destruct all
		while (_blocks.size()) {
			DeallocBack();
		}
		_size = 0;
	}
	~mf_vector() {
		clear();
	}
	size_t size() const{
		return _size;
	}
	bool empty() const {
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
	void push_back(const T& t){
		emplace_back(t);
	}
	void pop_back(){
		assert(_size);
		back().~T();  //destruct
		_size -= 1;
		if (_end._offset == 1) {
			DeallocBack();
		} else {
			_end._offset -= 1;
		}
	}
	T& back() {
		assert(_end._offset > 0);
		assert(_end._block!=nullptr);
		return _end._block[_end._offset-1];
	}
	const T& back() const {
		assert(_end._offset > 0);
		assert(_end._block!=nullptr);
		return _end._block[_end._offset-1];
	}

	T& operator[](size_t index){
		assert(index < _size);
		Locater d(GetLocater(index));
		return d._block[d._offset];
	}
	const T& operator[](size_t index) const{
		assert(index < _size);
		Locater d(GetLocater(index));
		return d._block[d._offset];
	}
	iterator begin() {
		return iterator(this,0);
	}
	iterator end() {
		return iterator(this,_size);
	}
};