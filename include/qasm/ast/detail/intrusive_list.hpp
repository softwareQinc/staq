/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <iterator>
#include <memory>

namespace synthewareQ {
namespace qasm {

class decl_program;

namespace detail {

// An intrusive list is one where the pointer to the next list node is stored
// in the same structure as the node data. This is normally A Bad Thing, as it
// ties the data to the specific list implementation
template<typename T>
class intrusive_list_node {
	T* next_ = nullptr;

	void do_on_insert(const T* parent)
	{
		static_cast<T&>(*this).on_insert(parent);
	}

	template<typename U>
	friend struct intrusive_list_access;
};

template<typename T>
struct intrusive_list_access {
	template<typename U>
	static T* next(const U& obj)
	{
		static_assert(std::is_base_of<U, T>::value, "must be a base");
		return static_cast<T*>(obj.next_);
	}

	template<typename U>
	static T* next(U& obj, T* node)
	{
		static_assert(std::is_base_of<U, T>::value, "must be a base");
		obj.next_ = node;
		return static_cast<T*>(obj.next_);
	}

	template<typename U, typename V>
	static void on_insert(U& obj, const V* parent)
	{
		obj.do_on_insert(parent);
	}
};

template<typename T>
class intrusive_list_iterator {
public:
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;

	intrusive_list_iterator()
	    : current_(nullptr)
	{}

	reference operator*() const
	{
		return *current_;
	}

	pointer operator->() const
	{
		return current_;
	}

	intrusive_list_iterator& operator++()
	{
		current_ = intrusive_list_access<T>::next(*current_);
		return *this;
	}

	intrusive_list_iterator operator++(int)
	{
		auto tmp = *this;
		++(*this);
		return tmp;
	}

	friend bool operator==(const intrusive_list_iterator& rhs, const intrusive_list_iterator& lhs)
	{
		return rhs.current_ == lhs.current_;
	}

	friend bool operator!=(const intrusive_list_iterator& rhs, const intrusive_list_iterator& lhs)
	{
		return !(rhs == lhs);
	}

private:
	intrusive_list_iterator(T* ptr)
	    : current_(ptr)
	{}

	T* current_;

	template<typename U>
	friend class intrusive_list;
};

template<typename T>
class intrusive_list {
public:
	intrusive_list()
	    : size_(0)
	    , first_(nullptr)
	    , last_(nullptr)
	{}

	template<typename Dummy = T,
	         typename = typename std::enable_if<std::is_same<Dummy, decl_program>::value>::type>
	void push_back(T* obj)
	{
		push_back_impl(obj);
	}

	template<typename U, typename = typename std::enable_if<!std::is_same<T, decl_program>::value, U>::type>
	void push_back(const U* parent, T* obj)
	{
		push_back_impl(obj);
		intrusive_list_access<T>::on_insert(*last_, parent);
	}

	bool empty() const
	{
		return first_ == nullptr;
	}

	size_t size() const
	{
		return size_;
	}

	using iterator = intrusive_list_iterator<T>;
	using const_iterator = intrusive_list_iterator<const T>;

	template<typename Dummy = T,
	         typename = typename std::enable_if<std::is_same<Dummy, decl_program>::value>::type>
	void insert(iterator it, T* obj)
	{
      insert_impl(obj, it);
	}

	template<typename U, typename = typename std::enable_if<!std::is_same<T, decl_program>::value, U>::type>
	void insert(iterator it, const U* parent, T* obj)
	{
        insert_impl(obj, it);
		intrusive_list_access<T>::on_insert(*it, parent);
	}

	iterator begin()
	{
		return iterator(first_);
	}

	iterator end()
	{
		return {};
	}

	const_iterator begin() const
	{
		return const_iterator(first_);
	}

	const_iterator end() const
	{
		return {};
	}

private:
	void push_back_impl(T* obj)
	{
		if (last_ != nullptr) {
			auto ptr = intrusive_list_access<T>::next(*last_, obj);
			last_ = ptr;
		} else {
			first_ = obj;
			last_ = first_;
		}
		++size_;
	}

    void insert_impl(T* obj, iterator it)
	{
		if (it != nullptr) {
            auto tmp_ = std::next(it).operator->();
            intrusive_list_access<T>::next(*it, obj);
			intrusive_list_access<T>::next(*obj, tmp_);
		} else {
            push_back_impl(obj);
		}
		++size_;
	}

private:
	size_t size_;
	T* first_;
	T* last_;
};

} // namespace detail
} // namespace qasm
} // namespace synthewareQ
