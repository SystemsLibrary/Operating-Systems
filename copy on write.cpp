/*
 * copy on write.cpp
 *
 *  Created on: 12 Jun 2021
 *      Author: Grant
 */

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include <mutex>

namespace cow {

template <typename T, typename TLock = std::mutex, typename TLocker = std::lock_guard<TLock>, typename TAlloc = std::allocator<T>>
class vector
{
private:
	typedef vector<T, TLock, TLocker> TVector;
	typedef std::vector<T, TAlloc> TStorage;
	typedef std::shared_ptr<TStorage> TStoragePtr;

	TStoragePtr _storage;
	mutable TLock _lock;

	TStoragePtr copy() const
	{
		TLocker locker(_lock);
	    return _storage;
	}

	bool removeAt(typename TStorage::iterator it)
	{
	     if (it == _storage->end())
	     {
	                return false;
	     }

	     if (_storage->size() == 1) // it's single element and will remove it
	     {
	    	 _storage.reset();
	     }
	     else if (_storage.use_count() == 1) // nobody holds read-only copy of vector
	     {
	    	 _storage->erase(it);
	     }
	     else // somebody has a read-only copy
	     {
	    	 TStoragePtr newStorage = TStoragePtr(new TStorage());
	         newStorage->reserve(_storage->size());

	         if (it != _storage->begin())
	         {
	        	 newStorage->insert(newStorage->end(), _storage->begin(), it);
	         }

	         if (++it != _storage->end())
	         {
	        	 newStorage->insert(newStorage->end(), it, _storage->end());
	         }

	         _storage = newStorage;
	     }

	     return true;
	}

public:
	vector()
	{
	}

	vector(const TLock& lock)
	: _lock(lock)
	{
	}

	vector(const TVector& array)
	: _storage(array.copy()) //Makes a copy on Write
	{
	}

	void clear()
	{
		TLocker locker(_lock);
	    _storage.reset();
	}

	TVector& operator=(const TVector& _Right)
	{
	    TLocker locker(_lock);
	    _storage = _Right.copy();

	    return *this;
	}

	void push_front(const T& t)
	{
		TLocker locker(_lock);

		if (_storage.use_count() == 1)
		{
			_storage->insert(_storage->begin(), t);
		}
		else
		{
			TStoragePtr newStorage = TStoragePtr(new TStorage());

			if (!_storage)
			{
				newStorage->reserve(4);
				newStorage->push_back(t);
			}
			else
			{
				newStorage->reserve(_storage->size() + 1 + 4);
				newStorage->push_back(t);
				newStorage->insert(newStorage->end(), _storage->begin(), _storage->end());
			}

			_storage = newStorage;
		}
	}

	void push_back(const T& t)
	{
		TLocker locker(_lock);

		if (_storage.use_count() == 1)
		{
			_storage->push_back(t);
		}
		else
		{
			TStoragePtr newStorage = TStoragePtr(new TStorage());

			if (!_storage)
			{
				newStorage->reserve(4);
			}
			else
			{
				newStorage->reserve(_storage->size() + 1 + 4);
				newStorage->insert(newStorage->end(), _storage->begin(), _storage->end());
			}

			newStorage->push_back(t);

			_storage = newStorage;
		}
	}

	template< class... Args>
	void emplace_back(Args&&... args)
	{
		if (_storage.use_count() == 1)
		{
			_storage->emplace_back(std::forward<Args>(args)...);
		}
		else
		{
			TStoragePtr newStorage = TStoragePtr(new TStorage());

			if (!_storage)
			{
				newStorage->reserve(4);
			}
			else // copy everything
			{
				newStorage->reserve(_storage->size() + 1 + 4);
				newStorage->insert(newStorage->end(), _storage->begin(), _storage->end());
			}

			newStorage->emplace_back(std::forward<Args>(args)...);

			_storage = newStorage;
		}
	}

	template <typename _Pred>
	std::size_t remove(_Pred predicate)
	{
		TLocker locker(_lock);

		if (!_storage || _storage->empty())
		{
			return 0;
		}

		std::size_t count = 0;

		if (_storage.use_count() == 1) // nobody holds read-only copy of vector
		{
			for (auto it = _storage->begin(); it != _storage->end(); )
		    {
				if (!predicate(*it))
				{
					++it;
				}
		        else
		        {
		        	it = _storage->erase(it);
		            ++count;
		        }
		    }
		}
		else
		{
			TStoragePtr newStorage = TStoragePtr(new TStorage());
			newStorage->reserve(_storage->size());

			for (auto const& elem : *_storage)
			{
				if (predicate(elem))
				{
					++count;
				}
			    else
			    {
			    	newStorage->push_back(elem);
			    }
			}

			if (_storage->size() == newStorage->size())
			{
				return count;
			}

			if (newStorage->empty())
			{
				_storage.reset();
			}
			else
			{
				_storage = newStorage;
			}
		}

		return count;
	}

	template <typename _Pred>
	bool removeFirst(_Pred predicate)
	{
		TLocker locker(_lock);

	    if (!_storage || _storage->empty())
	    {
	    	return false;
	    }

	    auto it = std::find_if(_storage->begin(), _storage->end(), predicate);

	    return removeAt(it);
	}

	template <typename _Pred>
	bool removeLast(_Pred predicate)
	{
		TLocker locker(_lock);

	    if (!_storage || _storage->empty())
	    {
	    	return false;
	    }

	    auto rit = std::find_if(_storage->rbegin(), _storage->rend(), predicate);

	    if (rit == _storage->rend())
	    {
	    	return false;
	    }

	    return removeAt(--rit.base());
	}

	template <typename _Pred>
	bool exists(_Pred predicate) const
	{
		TStoragePtr storage_copy = copy();

	    if (!storage_copy || _storage->empty())
	    {
	    	return false;
	    }

	    for (auto const& elem : *storage_copy)
	    {
	    	if (predicate(elem))
	    	{
	    		return true;
	    	}
	    }

	    return false;
	}

	template <typename _Pred, typename _DefaultValue>
	T find_first(_Pred predicate, _DefaultValue default_value) const
	{
		TStoragePtr storage_copy = copy();

	    if (!storage_copy || _storage->empty())
	    {
	    	return default_value;
	    }

	    for (auto const& elem : *storage_copy)
	    {
	    	if (predicate(elem))
	    	{
	    		return elem;
	    	}
	    }

	    return default_value;
	}

	template <typename _Pred, typename _DefaultValue>
	T find_last(_Pred predicate, _DefaultValue default_value) const
	{
		TStoragePtr storage_copy = copy();

	    if (!storage_copy || _storage->empty())
	    {
	    	return default_value;
	    }

	    for (auto it = storage_copy->rbegin(); it != storage_copy->rend(); ++it)
	    {
	    	if (predicate(*it))
	    	{
	    		return *it;
	    	}
	    }

	    return default_value;
	}

	class iterator
	{
	private:
		TStoragePtr _storage;

		typename TStorage::const_iterator _it;
		typename TStorage::const_iterator _end;

	public:
		iterator() = default; // constructor for end iterator
	    iterator(const iterator & copy) = default;

	    iterator(const TStoragePtr & storage, typename TStorage::const_iterator const& begin, typename TStorage::const_iterator const& end)
	    : _storage(storage)
	    , _it(begin)
	    , _end(end)
	    {
	    }

	    iterator& operator=(iterator const & right) = default;

	    typename TStorage::const_reference operator*() const
	    {
	    	return _it.operator*();
	    }

	    typename TStorage::pointer operator->() const
	    {
	    	return _it.operator->();
	    }

	    iterator& operator++()
	    {
	    	++_it;
	        return *this;
	    }

	    iterator operator++(int)
	    {
	    	_it++;
	        return *this;
	    }

	    iterator& operator--()
	    {
	    	--_it;
	    	return *this;
	    }

	    iterator operator--(int)
	    {
	    	_it--;
	    	return *this;
	    }

	    bool operator==(iterator const & right) const
	    {
	    	if (!_storage) // this is end() iterator
	        {
	    		if (!right._storage) // right value is also end()
	    		{
	    			return true;
	    		}

	    		return right._end == right._it;
	        }
	        else // this is usual iterator
	        {
	        	if (!right._storage) // right value is end()
	        	{
	        		return _it == _end;
	        	}

	            return _it == right._it;
	        	}
	    }

	    bool operator!=(iterator const & right) const
	    {	// test for iterator inequality
	    	return (!(*this == right));
	    }
	};

	iterator begin() const
	{
		TLocker locker(_lock);
		return _storage ? iterator(_storage, _storage->begin(), _storage->end()) : iterator();
	}

	iterator end() const
	{
		return iterator();
	}

	class readonly_vector
	{
	private:
		void assign(const TStoragePtr & storage)
	    {
			_storage = !storage ? _empty_storage : storage;
	    }

	    TStoragePtr _storage;

	    static TStoragePtr _empty_storage;

	public:

	    readonly_vector() = delete;

	    readonly_vector(const TStoragePtr & storage)
	    {
	    	assign(storage);
	    }

	    readonly_vector(const readonly_vector & copy)
	    {
	    	assign(copy._storage);
	    }

	    readonly_vector& operator=(const readonly_vector& _Right)
	    {
	    	assign(_Right._storage);

	    	return *this;
	    }

	    bool empty() const
	    {
	    	return _storage->empty();
	    }

	    typename TStorage::size_type size() const
	    {
	    	return _storage->size();
	    }

	    typename TStorage::const_reference at(typename TStorage::size_type pos) const
	    {
	    	return _storage->at(pos);
	    }

	    typename TStorage::const_reference operator[](typename TStorage::size_type pos) const
	    {
	    	return _storage->operator[](pos);
	    }

	    typename TStorage::const_reference front() const
	    {
	    	return _storage->front();
	    }

	    typename TStorage::const_reference back() const
	    {
	    	return _storage->back();
	    }

	    const T* data() const
	    {
	    	return _storage->data();
	    }

	    typename TStorage::const_iterator begin() const { return _storage->begin(); }
	    typename TStorage::const_iterator cbegin() const { return _storage->cbegin(); }
	    typename TStorage::const_iterator end() const { return _storage->end(); }
	    typename TStorage::const_iterator cend() const { return _storage->cend(); }

	    typename TStorage::const_iterator rbegin() const { return _storage->rbegin(); }
	    typename TStorage::const_iterator crbegin() const { return _storage->crbegin(); }
	    typename TStorage::const_iterator rend() const { return _storage->rend(); }
	    typename TStorage::const_iterator crend() const { return _storage->crend(); }

	    operator TStorage const& () const { return *_storage; }
	};

	readonly_vector read_only_copy() const
	{
		TLocker locker(_lock);
		return readonly_vector(_storage);
	}

	TLock& lock() const
	{
		return _lock;
	}

	// This method should be called only under lock
	TStorage& data()
	{
		if (!_storage) // we need to create empty array for direct access
		{
			_storage = TStoragePtr(new TStorage());
		}

		return *_storage;
	}
};

template <typename T, typename TLock, typename TLocker, typename TAlloc>
typename vector<T, TLock, TLocker, TAlloc>::TStoragePtr vector<T, TLock, TLocker, TAlloc>::readonly_vector::_empty_storage = std::make_shared< vector<T, TLock, TLocker, TAlloc>::TStorage >();

}

struct A
{
    int value = 0;

    A(int v) : value(v) {}
};

int main()
{
	cow::vector<std::shared_ptr<A>> v1;

	cow::vector<std::shared_ptr<A>> v3 = v1; //Make a reference
	cow::vector<std::shared_ptr<A>> v2(v1); // Make a copy
	auto readonly_copy = v1.read_only_copy(); //Make a read only copy

	v1.push_back(std::make_shared<A>(1));  // v1 == { A(1) }
	v1.emplace_back(new A(2));  // v1 == { A(1), A(2) }

	for (auto v : v1)
	{
		std::cout << v->value << '\n';
	}

	v2.push_front(std::make_shared<A>(3));  // now v2 == { A(3), A(1), A(2) }, but v2 is still { A(1), A(2) }

	for (std::size_t i = 0; i < readonly_copy.size(); ++i)
	{
		std::cout << readonly_copy[i]->value << '\n';
	}

	// remove using predicate
	bool exists3 = v2.exists([](auto const& a) -> bool { return a->value == 3; });
	std::cout << "exists 3: " << exists3 << '\n';
	bool exists4 = v2.exists([](auto const& a) -> bool { return a->value == 4; });
	std::cout << "exists 4: " << exists4 << '\n';
	std::cout << '\n';

	std::size_t removed = v2.remove([](auto const& a) -> bool {
	        return a->value == 3 || a->value == 2;
	    });

	std::cout << "number of removed " << removed << '\n'; // now v2 == { A(1) }
	std::cout << "\n loop v2 copy:\n";
	for (auto v : v2)
	{
		std::cout << v->value << '\n';
	}

	std::cout << "\n loop v1 original:\n"; // v1 == { A(1), A(2) }
	for (auto v : v1)
	{
		v1.remove([](auto const& a) -> bool { return a->value == 2; });
	    v1.push_front(std::make_shared<A>(2));

	    std::cout << v->value << '\n';
	}

	std::cout << "\n after loop v1 original:\n"; // v1 == { A(2), A(1) }
	for (auto v : v1)
	{
		std::cout << v->value << '\n';
	}

	std::cout << "read-only copy v1:\n";

	for (std::size_t i = 0; i < readonly_copy.size(); ++i)
	{
		std::cout << readonly_copy[i]->value << '\n';
	}

	std::cout << "\n after loop v3 exact reference:\n"; // v1 == { A(2), A(1) }
	for (auto v : v1)
	{
		std::cout << v->value << '\n';
	}

	return 0;

	/*
	int arr1[]  = {10, 20, 15, 12};
	int n1 = sizeof(arr1)/sizeof(arr1[0]);

	char arr2[] = {1, 2, 3};
	int n2 = sizeof(arr2)/sizeof(arr2[0]);

	cout << arrMin<int, 10000>(arr1, n1) << endl;
	cout << arrMin<char, 256>(arr2, n2);

	return 0;
	*/

	/*
	int arr[5] = {1, 2, 3, 4, 5};

	Array<int> a(arr, 5);

	a.print();

	return 0;
	*/

	/*
	int a[5] = {10, 50, 30, 40, 20};
	int n = sizeof(a) / sizeof(a[0]);

	bubbleSort<int>(a, n);

	for (int i = 0; i < n; i++)
	{
		cout << a[i] << " ";
	}
	*/

	/*
	cout << myMax<int>(3, 7) << endl;
	cout << myMax<double>(3.0, 7.0) << endl;
	cout << myMax<char>('g', 'e') << endl;
	*/

	/*
	vector < string > vecOfStr {"first", "second", "third"};

	vector <string> vec;

	vec.operator = (vecOfStr);

	for (int i = 0; i < vec.size(); i++)
	{
		cout << vec.operator[](i) << endl;
	}
	cout << vec.size() << endl;
	*/

	return 0;
}
