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
    // Make it bi-directional to support modification
    // This isn't a great solution
    T* prev_ = nullptr;
    

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
    static T* get_next(const U& obj)
    {
      static_assert(std::is_base_of<U, T>::value, "must be a base");
      return static_cast<T*>(obj.next_);
    }

    template<typename U>
    static T* set_next(U& obj, T* node)
    {
      static_assert(std::is_base_of<U, T>::value, "must be a base");
      obj.next_ = node;
      return static_cast<T*>(obj.next_);
    }

    template<typename U>
    static T* get_prev(const U& obj)
    {
      static_assert(std::is_base_of<U, T>::value, "must be a base");
      return static_cast<T*>(obj.prev_);
    }

    template<typename U>
    static T* set_prev(U& obj, T* node)
    {
      static_assert(std::is_base_of<U, T>::value, "must be a base");
      obj.prev_ = node;
      return static_cast<T*>(obj.prev_);
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
    using iterator_category = std::bidirectional_iterator_tag;

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
      current_ = intrusive_list_access<T>::get_next(*current_);
      return *this;
    }

    intrusive_list_iterator operator++(int)
    {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    intrusive_list_iterator& operator--()
    {
      current_ = intrusive_list_access<T>::get_prev(*current_);
      return *this;
    }

    intrusive_list_iterator operator--(int)
    {
      auto tmp = *this;
      --(*this);
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

    // Shallow assignment operator
    intrusive_list<T>& operator=(const intrusive_list<T>& xs)
    {
	  size_ = xs.size_;
      first_ = xs.first_;
      last_ = xs.last_;
      return *this;
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

    // Iterators
    iterator begin()
    {
      return iterator(first_);
    }

    iterator end()
    {
      return iterator(nullptr);
    }

    const_iterator begin() const
    {
      return const_iterator(first_);
    }

    const_iterator end() const
    {
      return const_iterator(nullptr);
    }

    // Modifiers -- follows <list> conventions mostly
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

    // Inserts directly before
    // It is unchanged
    // Return iterator points to inserted element
    template<typename Dummy = T,
             typename = typename std::enable_if<std::is_same<Dummy, decl_program>::value>::type>
    iterator insert(iterator it, T* obj)
    {
      return insert_impl(it, obj);
    }

    template<typename U, typename = typename std::enable_if<!std::is_same<T, decl_program>::value, U>::type>
    iterator insert(iterator it, const U* parent, T* obj)
    {
      it = insert_impl(it, obj);
      intrusive_list_access<T>::on_insert(*obj, parent);
      return it;
    }

    // Inserts entire list directly before
    // It is unchanged
    // Return iterator points to beginning of inserted list
    template<typename Dummy = T,
             typename = typename std::enable_if<std::is_same<Dummy, decl_program>::value>::type>
    iterator splice(iterator it, intrusive_list<T>& xs)
    {
      return splice_impl(it, xs);
    }

    template<typename U, typename = typename std::enable_if<!std::is_same<T, decl_program>::value, U>::type>
    iterator splice(iterator it, const U* parent, intrusive_list<T>& xs)
    {
      for (auto& obj : xs) intrusive_list_access<T>::on_insert(obj, parent);
      return splice_impl(it, xs);
    }

    // Replaces an item
    // It is invalidated
    // Return iterator points to new element
    template<typename Dummy = T,
             typename = typename std::enable_if<std::is_same<Dummy, decl_program>::value>::type>
    iterator assign(iterator it, T* obj)
    {
      return assign_impl(it, obj);
    }

    template<typename U, typename = typename std::enable_if<!std::is_same<T, decl_program>::value, U>::type>
    iterator assign(iterator it, const U* parent, T* obj)
    {
      it = assign_impl(it, obj);
      intrusive_list_access<T>::on_insert(*obj, parent);
      return it;
    }

    // Replaces an item
    // It is invalidated
    // Return iterator points to element directly after removed item
    iterator erase(iterator it)
    {
      return erase_impl(it);
    }

  private:
    void push_back_impl(T* obj)
    {

      if (empty()) {
        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, nullptr);

        first_ = obj;
      } else {
        intrusive_list_access<T>::set_next(*last_, obj);

        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, last_);
      }

      last_ = obj;
      size_++;
    }

    iterator insert_impl(iterator it, T* obj)
    {
      if (empty()) {
        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, nullptr);

        first_ = obj;
        last_ = obj;
      } else if (it == end()) {
        // last is not null
        intrusive_list_access<T>::set_next(*last_, obj);

        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, last_);

        last_ = obj;
      } else if (it.current_ == first_) {
        // first is not null
        intrusive_list_access<T>::set_next(*obj, first_);
        intrusive_list_access<T>::set_prev(*obj, nullptr);

        intrusive_list_access<T>::set_prev(*first_, obj);
        first_ = obj;
      } else {
        // it->prev is not null
        auto prev_ = intrusive_list_access<T>::get_prev(*it);

        // Forward pointers
        intrusive_list_access<T>::set_next(*prev_, obj);
        intrusive_list_access<T>::set_next(*obj, it.current_);

        // Backward pointers
        intrusive_list_access<T>::set_prev(*it, obj);
        intrusive_list_access<T>::set_prev(*obj, prev_);
      }

      size_++;
      return iterator(obj);
	}

    iterator splice_impl(iterator it, intrusive_list<T>& xs)
    {
      if (xs.empty()) {
        return it;
      }
      
      if (empty()) {
        first_ = xs.first_;
        last_ = xs.last_;
      } else if (it == end()) {
        // last & xs.first are not null
        intrusive_list_access<T>::set_next(*last_, xs.first_);
        intrusive_list_access<T>::set_prev(*(xs.first_), last_);
        last_ = xs.last_;
      } else if (it.current_ == first_) {
        // first & xs.last are not null
        intrusive_list_access<T>::set_next(*(xs.last_), first_);
        intrusive_list_access<T>::set_prev(*first_, xs.last_);
        first_ = xs.first_;
      } else {
        // it->prev, as.first & as.last are not null
        auto prev_ = intrusive_list_access<T>::get_prev(*it);

        // Forward pointers
        intrusive_list_access<T>::set_next(*prev_, xs.first_);
        intrusive_list_access<T>::set_next(*(xs.last_), it.current_);

        // Backward pointers
        intrusive_list_access<T>::set_prev(*it, xs.last_);
        intrusive_list_access<T>::set_prev(*(xs.first_), prev_);
      }

      size_ += xs.size_;
      return iterator(xs.first_);
	}

    iterator assign_impl(iterator it, T* obj)
    {
      if (empty()) {
        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, nullptr);

        first_ = obj;
        last_ = obj;
      } else if (it == end()) {
        // last is not nullptr
        intrusive_list_access<T>::set_next(*last_, obj);

        intrusive_list_access<T>::set_next(*obj, nullptr);
        intrusive_list_access<T>::set_prev(*obj, last_);

        last_ = obj;
      } else {
        auto prev_ = intrusive_list_access<T>::get_prev(*it);
        auto next_ = intrusive_list_access<T>::get_next(*it);

        // Forward and back pointers for inserted object
        intrusive_list_access<T>::set_next(*obj, next_);
        intrusive_list_access<T>::set_prev(*obj, prev_);

        if (next_ != nullptr) { // Back pointer for next_
         intrusive_list_access<T>::set_prev(*next_, obj);
        } else { // Inserting at end of list
          last_ = obj;
        }

        if (prev_ != nullptr) { // Back forward for prev_
         intrusive_list_access<T>::set_next(*prev_, obj);
        } else { // Inserting at beginning of list
          first_ = obj;
        }
      }

      return iterator(obj);
	}

    // Note: sets 'it' to the next element
    iterator erase_impl(iterator it)
    {
      if (it == end()) {
        return end();
      } else {
        auto prev_ = intrusive_list_access<T>::get_prev(*it);
        auto next_ = intrusive_list_access<T>::get_next(*it);

        if (next_ != nullptr) { // Back pointer for next_
         intrusive_list_access<T>::set_prev(*next_, prev_);
        } else {
          last_ = prev_;
        }

        if (prev_ != nullptr) { // Back forward for prev_
         intrusive_list_access<T>::set_next(*prev_, next_);
        } else {
         first_ = next_;
        }

        --size_;
        return iterator(next_);
      }
	}

  private:
	size_t size_;
	T* first_;
	T* last_;
  };

} // namespace detail
} // namespace qasm
} // namespace synthewareQ
