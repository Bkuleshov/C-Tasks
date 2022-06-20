#include <iostream>
#include <cstddef>

template<size_t N>
class StackStorage {
 private:
  char data_[N];
  size_t align_ = 0;
 public:
  StackStorage() = default;

  void* allocate(size_t n, size_t align) {
    if (align_ % align != 0) {
      align_ += align - align_ % align;
    }
    char* block = data_ + align_;
    align_ += n;
    return block;
  }
};

template<typename T, size_t N>
class StackAllocator {
 private:
  StackStorage<N>* storage_;

 public:
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using const_pointer = const T*;
  using const_reference = const T&;

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  StackAllocator() = default;

  StackAllocator(const StackAllocator<T, N>& alloc): storage_(alloc.get_storage()) {}

  StackAllocator& operator=(const StackAllocator<T, N>& alloc) {
    storage_ = alloc.get_storage();
    return *this;
  }

  StackAllocator(StackStorage<N>& storage): storage_(&storage) {}

  StackStorage<N>* get_storage() const {
    return storage_;
  }

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& alloc): storage_(alloc.get_storage()) {}

  pointer allocate(size_t n) {
    return reinterpret_cast<T*>(storage_->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T*, size_t) {}

  template <typename U>
  bool operator==(const StackAllocator<U, N>& alloc) const {
    return storage_ == alloc.get_storage();
  }

  template<typename U>
  bool operator!=(const StackAllocator<U, N>& alloc) const {
    return !(*this == alloc);
  }

};

template<typename T, typename alloc_type = std::allocator<T> >
class List {
 private:
  template <bool is_const>
  class common_iterator;
  struct BaseNode {
    BaseNode() = default;
    BaseNode* prev;
    BaseNode* next;
  };
  struct Node: public BaseNode {
    T value;
    Node() = default;
    Node(const T& value): value(value) {}
  };
  BaseNode* end_;
  size_t size_;
  using node_alloc_type = typename std::allocator_traits<alloc_type>::template rebind_alloc<Node>;
  using basenode_alloc_type = typename std::allocator_traits<alloc_type>::template rebind_alloc<BaseNode>;
  using alloc_traits = typename std::allocator_traits<node_alloc_type>;
  using base_alloc_traits = typename std::allocator_traits<basenode_alloc_type>;
  node_alloc_type allocator_;
  basenode_alloc_type base_allocator_;

 public:
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  explicit List(const alloc_type& allocator = alloc_type()) {
    base_allocator_ = allocator;
    allocator_ = alloc_traits::select_on_container_copy_construction(allocator);
    end_ = base_alloc_traits::allocate(base_allocator_, 1);
    end_->prev = end_->next = end_;
    size_ = 0;
  }

  List(size_t n, const alloc_type& allocator = alloc_type()): List(allocator) {
    try {
      for (size_t i = 0; i < n; ++i) {
        Node* new_node = alloc_traits::allocate(allocator_, 1);
        alloc_traits::construct(allocator_, new_node);
        new_node->next = end_;
        end_->prev->next = new_node;
        new_node->prev = end_->prev;
        end_->prev->next = new_node;
        end_->prev = new_node;
        ++size_;
      }
    } catch(...) {
      while (size() > 0) {
        pop_back();
      }
    }
  }

  explicit List(size_t n, const T& value, const alloc_type& allocator = alloc_type()): List(allocator) {
    for (size_t i = 0; i < n; ++i) {
      push_back(value);
    }
  }

  List(const List& other): List(alloc_traits::select_on_container_copy_construction(other.allocator_)) {
    for (auto i = other.begin(); i != other.end(); ++i) {
      push_back(*i);
    }
  }

  List& operator=(const List& other) {
    if (alloc_traits::propagate_on_container_copy_assignment::value) {
      allocator_ = other.allocator_;
    }
    List copy(allocator_);
    for (auto i = other.begin(); i != other.end(); ++i) {
      copy.push_back(*i);
    }
    std::swap(end_, copy.end_);
    std::swap(size_, copy.size_);
    return *this;
  }

  ~List() {
    while (size() > 0) {
      pop_back();
    }
    base_alloc_traits::destroy(base_allocator_, end_);
    base_alloc_traits::deallocate(base_allocator_, end_, 1);
  }

  //size, get_allocator

  alloc_type get_allocator() const {
    return allocator_;
  }

  size_t size() const {
    return size_;
  }

  //push and pop

  void push_back(const T& value) {
    try {
      Node* new_node = alloc_traits::allocate(allocator_, 1);
      alloc_traits::construct(allocator_, new_node, value);
      new_node->prev = end_->prev;
      new_node->next = end_;
      end_->prev = end_->prev->next = new_node;
      ++size_;
    } catch (...) {
      pop_back();
      throw;
    }
  }

  void push_front(const T& value) {
    insert(begin(), value);
  }

  void pop_back() {
    erase(--end());
  }

  void pop_front() {
    erase(begin());
  }

  //begins and ends

  iterator begin() {
    return iterator(end_->next);
  }

  const_iterator begin() const {
    return const_iterator(end_->next);
  }

  const_iterator cbegin() const {
    return const_iterator(end_->next);
  }

  iterator end() {
    return iterator(end_);
  }

  const_iterator end() const {
    return const_iterator(end_);
  }

  const_iterator cend() const {
    return const_iterator(end_);
  }

  reverse_iterator rbegin() {
    return std::make_reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const {
    return std::make_reverse_iterator(cend());
  }

  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }

  reverse_iterator rend() {
    return std::make_reverse_iterator(begin());
  }

  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(cbegin());
  }

  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  //insert and erase

  void insert(const_iterator it, const T& value) {
    try{
      ++size_;
      Node* new_node = alloc_traits::allocate(allocator_, 1);
      alloc_traits::construct(allocator_, new_node, value);
      new_node->next = it.node_;
      new_node->prev = it.node_->prev;
      it.node_->prev = it.node_->prev->next = new_node;
    } catch(...) {
      erase(it);
      throw;
    }
  }

  void erase(const_iterator it) {
    it.node_->prev->next = it.node_->next;
    it.node_->next->prev = it.node_->prev;
    alloc_traits::destroy(allocator_, reinterpret_cast<Node*>(it.node_));
    alloc_traits::deallocate(allocator_, reinterpret_cast<Node*>(it.node_), 1);
    --size_;
  }

 private:
  template<bool is_const>
  class common_iterator {
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename std::conditional<is_const, const T, T>::type;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;
    using iterator_category = std::bidirectional_iterator_tag;

    common_iterator(BaseNode* node): node_(node) {}

    reference operator*() const {
      return static_cast<Node*>(node_)->value;
    }
    pointer operator->() const {
      return &static_cast<Node*>(node_)->value;
    }
    common_iterator& operator=(const common_iterator &iter) {
      node_ = iter.node_;
      return *this;
    }
    common_iterator& operator++() {
      node_ = node_->next;
      return *this;
    }
    common_iterator& operator--() {
      node_ = node_->prev;
      return *this;
    }
    common_iterator operator++(int) {
      common_iterator iter = *this;
      ++*this;
      return iter;
    }
    common_iterator operator--(int) {
      common_iterator iter = *this;
      --*this;
      return iter;
    }
    bool operator==(const common_iterator& iter) {
      return node_ == iter.node_;
    }
    bool operator!=(const common_iterator& iter) {
      return !(*this == iter);
    }

    operator common_iterator<true>() const {
      return common_iterator<true>(node_);
    }

    friend List;
   private:
    BaseNode* node_;
  };
};
