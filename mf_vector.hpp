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

template <class T> class mf_vector
{
	struct loc_data
	{
		T* _block;
		unsigned _offset;
		loc_data(T*block, unsigned offset)
			: _block(block)
			, _offset(offset)
		{}
	};
	std::vector<T*> _blocks;
	size_t _size;
	static const unsigned _block_size = 
			std::max<size_t>(4096/sizeof(T), 16);
	loc_data _end;

	loc_data get_loc_data(size_t index) const {
		if (index >= _size) {
			return _end;
		} else {
			size_t which_block = index/_block_size;
			T* block = _blocks[which_block];
			unsigned offset = index%_block_size;
			return loc_data(block,offset);
		}
	}
	void alloc() {
		_end._block = reinterpret_cast<T*>(malloc(sizeof(T)*_block_size));
		_blocks.push_back(_end._block);
		_end._offset = 0;
	}
	void dealloc_back(){
		assert(_blocks.size());
		T* block = _blocks.back();
		free(block);
		_blocks.pop_back();
		if (_blocks.size())
			_end._block = _blocks.back();
		else
			_end._block = nullptr;
		_end._offset = _block_size;
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
		loc_data _location;
		explicit iterator(mf_vector* vector, size_t index)
			: _vector(vector)
			, _index(index)
			, _location(vector->get_loc_data(index))
			{}
	public:
		T& operator++(){		// prefix increment, as in ++iter;
			_location._offset += 1;
			_index += 1;
			if (_location._offset == _block_size)
				_location = _vector->get_loc_data(_index);
			return _location._block[_location._offset];
		}
		bool operator==(const iterator& other){
			return _index == other._index;
		}
		bool operator!=(const iterator& other){
			return !(*this == other);
		}
		T& operator*() {
			return _location._block[_location._offset];
		}
	};
	friend class iterator;
	mf_vector()
		: _size(0)
		, _end(nullptr,_block_size)
		{}
	~mf_vector() {
		// no provision for T requiring destuction	
		while (_blocks.size()) {
			dealloc_back();
		}
	}
	size_t size() const{
		return _size;
	}
	bool empty() const {
		return size() == 0;
	}
	void push_back(const T& t){
		if (_end._offset == _block_size)
			alloc();
		new(_end._block+_end._offset) T(t);
		_end._offset += 1;
		_size += 1;
	}
	template <class ... Args>
	void emplace_back(Args...args){
		if (_end._offset == _block_size)
			alloc();
		new(_end._block+_end._offset) T(args...);
		_end._offset += 1;
		_size += 1;
	}
	void pop_back(){
		assert(_size);
		_size -= 1;
		if (_end._offset == 1) {
			dealloc_back();
		} else {
			_end._offset -= 1;
		}
	}
	T& back() {
		assert(_end._offset > 0);
		return _end._block[_end._offset-1];
	}
	const T& back() const {
		assert(_end._offset > 0);
		return _end._block[_end._offset-1];
	}

	T& operator[](size_t index){
		loc_data d(get_loc_data(index));
		return d._block[d._offset];
	}
	iterator begin() {
		return iterator(this,0);
	}
	iterator end() {
		return iterator(this,_size);
	}
};