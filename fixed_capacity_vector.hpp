// Template class fixed_capacity_vector
//
// One of these has much of the API of a std::fixed_capacity_vector,
// but has a fixed capacity.  It cannot be extended past that.
// It is safe to use only where the problem limits the size needed,
// as it does not check for overfilling.  It is designed for speed,
// so it does not check subscripts.

template <class T, unsigned Capacity>
class fixed_capacity_vector{
	uint32_t _size;
	T _elem[Capacity];
public:
	fixed_capacity_vector() : _size(0){}
	T & operator[](unsigned i)						{return _elem[i];}
	const T& operator[](unsigned i) const			{return _elem[i];}
	T* begin()										{return _elem;}
	const T* begin() const							{return _elem;}
	size_t size() const								{return _size;}
	T* end()										{return _elem+_size;}
	const T* end() const							{return _elem+_size;}
	T & back()										{return _elem[_size-1];}
	const T& back() const							{return _elem[_size-1];}
	void pop_back()									{_size -= 1;}
	void pop_back(unsigned n)						{_size -= n;}
	void push_back(const T& cd)						{_elem[_size] = cd; _size += 1;}
	void clear()									{_size = 0;}
	void append(const T* begin, const T* end)	
					{for (auto i=begin;i<end;++i){_elem[_size++]=*i;}}
	bool operator==(const fixed_capacity_vector<T,Capacity>& other) const
					{	
						if (_size != other._size) return false;
						for(unsigned i = 0; i < _size; ++i){
							if ((*this)[i] != other[i]) return false;
						}
						return true;
					}
	fixed_capacity_vector<T,Capacity>& operator=(const fixed_capacity_vector& other) 
					{
						std::copy(other._elem,other._elem+other._size,_elem);
						_size = other._size;
						return *this;
					}
};