template<typename T>
class Deque{
 public:
  Deque();
  Deque(const Deque<T>&);
  explicit Deque(const size_t);
  Deque(const size_t, const T&);
  Deque<T>& operator=(const Deque<T>&);
  ~Deque();
  size_t size() const;
  T& operator[](const size_t);
  const T& operator[](const size_t) const;
  T& at(const size_t);
  const T& at(const size_t) const;
  void push_back(const T&);
  void push_front(const T&);
  void pop_back();
  void pop_front();
  template<bool is_const>
  class common_iterator;
  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;
  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;
  void insert(iterator, const T&);
  void erase(iterator);

 private:
  void copy(const Deque<T>&);
  void set();
  void swap(Deque<T>&);
  void expand();
  void clear();
  static const size_t block_size_ = 32;
  static const size_t expansion_coefficient_ = 3;
  size_t first_index_;
  size_t number_of_blocks_;
  size_t size_;
  T** data_;
};

// Iterators

template <typename T>
template <bool is_const>
class Deque<T>::common_iterator {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = typename std::conditional<is_const, const T, T>::type;
  using pointer = typename std::conditional<is_const, const T*, T*>::type;
  using reference = typename std::conditional<is_const, const T&, T&>::type;
  using iterator_category = std::random_access_iterator_tag;
  using deque_type = typename std::conditional<is_const, T** const, T**>::type;

  common_iterator(deque_type deque, size_t index): deque_(deque), index_(index) {
  }
  reference operator*() const {
    return deque_[index_ / block_size_][index_ % block_size_];
  }
  pointer operator->() const {
    return deque_[index_ / block_size_] + index_ % block_size_;
  }
  common_iterator<is_const>& operator++() {
    ++index_;
    return *this;
  }
  common_iterator<is_const>& operator--() {
    --index_;
    return *this;
  }
  common_iterator<is_const>& operator++(int) {
    common_iterator<is_const> iter = *this;
    ++*this;
    return iter;
  }
  common_iterator<is_const>& operator--(int) {
    common_iterator<is_const> iter = *this;
    --*this;
    return iter;
  }
  common_iterator<is_const>& operator+=(int i) {
    index_ += i;
    return *this;
  }
  common_iterator<is_const>& operator-=(int i) {
    index_ -= i;
    return *this;
  }
  common_iterator<is_const> operator+(int i) {
    common_iterator<is_const> iter = *this;
    iter += i;
    return iter;
  }
  common_iterator<is_const> operator-(int i) {
    common_iterator<is_const> iter = *this;
    iter -= i;
    return iter;
  }
  bool operator<(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ < iter.index_;
  }
  bool operator>(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ > iter.index_;
  }
  bool operator<=(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ <= iter.index_;
  }
  bool operator>=(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ >= iter.index_;
  }
  bool operator==(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ == iter.index_;
  }
  bool operator!=(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ != iter.index_;
  }
  int operator-(const typename Deque<T>::template common_iterator<is_const> iter) const {
    return index_ - iter.index_;
  }
  size_t get_index() {
    return index_;
  }
 private:
  T** deque_;
  size_t index_;
  static const size_t block_size_ = 32;
};

// Constructors, destructor, assigning

template<typename T>
Deque<T>::Deque(): first_index_(0), number_of_blocks_(1), size_(0) {
  set();
}

template<typename T>
Deque<T>::Deque(const Deque<T>& deque) {
  copy(deque);
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& deque) {
  Deque<T> other(deque);
  swap(other);
  return *this;
}

template<typename T>
Deque<T>::Deque(const size_t size) {
  first_index_ = 0;
  number_of_blocks_ = size / block_size_ + 1;
  size_ = 0;
  set();
  for (size_t i = 0; i < size; ++i) {
    try {
      push_back(T());
    } catch (...) {
      pop_back();
      throw;
    }
  }
}

template<typename T>
Deque<T>::Deque(const size_t size, const T& value) {
  first_index_ = 0;
  number_of_blocks_ = size / block_size_ + 1;
  size_ = 0;
  set();
  for (size_t i = 0; i < size; ++i) {
    try {
      push_back(value);
    } catch (...) {
      pop_back();
      throw;
    }
  }
}

template <typename T>
Deque<T>::~Deque() {
  clear();
}

template<typename T>
size_t Deque<T>::size() const {
  return size_;
}

// Element access

template<typename T>
T& Deque<T>::operator[](const size_t index) {
  return data_[(first_index_ + index) / block_size_][(first_index_ + index) % block_size_];
}

template<typename T>
const T& Deque<T>::operator[](const size_t index) const {
  return data_[(first_index_ + index) / block_size_][(first_index_ + index) % block_size_];
}

template<typename T>
T& Deque<T>::at(const size_t index) {
  if (index >= size_) {
    throw(std::out_of_range("out_of_range"));
  }
  return data_[(first_index_ + index) / block_size_][(first_index_ + index) % block_size_];
}

template<typename T>
const T& Deque<T>::at(const size_t index) const {
  if (index >= size_) {
    throw(std::out_of_range("out_of_range"));
  }
  return data_[(first_index_ + index) / block_size_][(first_index_ + index) % block_size_];;
}

// Push, pop

template<typename T>
void Deque<T>::push_back(const T& value) {
  if (first_index_ + size_ == block_size_ * number_of_blocks_) {
    expand();
  }
  try {
    new(data_[(first_index_ + size_) / block_size_] + (first_index_ + size_) % block_size_) T(value);
    ++size_;
  } catch(...) {
    throw;
  }
}

template<typename T>
void Deque<T>::push_front(const T& value) {
  if (first_index_ == 0) {
    expand();
  }
  try {
    --first_index_;
    new(data_[first_index_ / block_size_] + first_index_ % block_size_) T(value);
    ++size_;
  } catch(...) {
    throw;
  }
}

template<typename T>
void Deque<T>::pop_back() {
  (data_[(first_index_ + size_ - 1) / block_size_] + (first_index_ + size_ - 1) % block_size_)->~T();
  --size_;
}

template<typename T>
void Deque<T>::pop_front() {
  (data_[first_index_ / block_size_] + first_index_ % block_size_)->~T();
  --size_;
  ++first_index_;
}

// Begins and ends

template<typename T>
typename Deque<T>::iterator Deque<T>::begin() {
  return Deque::iterator(data_, first_index_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const {
  return Deque::const_iterator(data_, first_index_);
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
  return Deque::const_iterator(data_, first_index_);
}

template<typename T>
typename Deque<T>::iterator Deque<T>::end() {
  return Deque::iterator(data_, first_index_ + size());
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::end() const {
  return Deque::const_iterator(data_, first_index_ + size());
}

template<typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
  return Deque::const_iterator(data_, first_index_ + size());
}

template<typename T>
typename Deque<T>::reverse_iterator Deque<T>::rbegin() {
  return std::make_reverse_iterator(end());
}

template<typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rbegin() const {
  return std::make_reverse_iterator(cend());
}

template<typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crbegin() const {
  return std::make_reverse_iterator(cend());
}

template<typename T>
typename Deque<T>::reverse_iterator Deque<T>::rend() {
  return std::make_reverse_iterator(begin());
}

template<typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rend() const {
  return std::make_reverse_iterator(cbegin());
}

template<typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crend() const {
  return std::make_reverse_iterator(cbegin());
}

// Insert and erase

template<typename T>
void Deque<T>::insert(Deque<T>::iterator iter, const T& value) {
  if (first_index_ + size_ == block_size_ * number_of_blocks_) {
    expand();
  }
  for (size_t i = first_index_ + size_; i > iter.get_index(); --i) {
    data_[i / block_size_][i % block_size_] = data_[(i - 1) / block_size_][(i - 1) % block_size_];
  }
  data_[iter.get_index() / block_size_][iter.get_index() % block_size_] = value;
  ++size_;
}

template<typename T>
void Deque<T>::erase(Deque<T>::iterator iter) {
  for (size_t i = iter.get_index(); i < first_index_ + size_ - 1; ++i) {
    data_[i / block_size_][i % block_size_] = data_[(i + 1) / block_size_][(i + 1) % block_size_];
  }
  pop_back();
}

// Helper functions

template<typename T>
void Deque<T>::set() {
  data_ = new T*[number_of_blocks_];
  for (size_t i = 0; i < number_of_blocks_; ++i) {
    data_[i] = reinterpret_cast<T*>(new uint8_t[block_size_ * sizeof(T)]);
  }
}

template<typename T>
void Deque<T>::expand() {
  size_t new_number_of_blocks = expansion_coefficient_ * number_of_blocks_;
  T** new_data = new T*[new_number_of_blocks];
  size_t j = number_of_blocks_;
  for (size_t i = 0; i < new_number_of_blocks; ++i) {
    new_data[i] = reinterpret_cast<T*>(new uint8_t[block_size_ * sizeof(T)]);
  }
  for (size_t i = 0; i < number_of_blocks_; ++i) {
    new_data[j + i] = data_[i];
  }
  delete[] data_;
  data_ = new_data;
  first_index_ += block_size_ * number_of_blocks_;
  number_of_blocks_ = new_number_of_blocks;
}

template<typename T>
void Deque<T>::swap(Deque<T>& deque) {
  std::swap(first_index_, deque.first_index_);
  std::swap(number_of_blocks_, deque.number_of_blocks_);
  std::swap(size_, deque.size_);
  std::swap(data_, deque.data_);
}

template<typename T>
void Deque<T>::copy(const Deque<T>& deque) {
  first_index_ = deque.first_index_;
  number_of_blocks_ = deque.number_of_blocks_;
  size_ = deque.size_;
  data_ = new T*[number_of_blocks_];
  for (size_t i = 0; i < number_of_blocks_; ++i) {
    data_[i] = reinterpret_cast<T*>(new uint8_t[block_size_ * sizeof(T)]);
  }
  for (size_t i = first_index_; i < first_index_ + size_; ++i) {
    data_[i / block_size_][i % block_size_] = deque.data_[i / block_size_][i % block_size_];
  }
}

template<typename T>
void Deque<T>::clear() {
  for (size_t i = first_index_; i < first_index_ + size_; ++i) {
    (data_[i / block_size_] + i % block_size_)->~T();
  }
  for (size_t i = 0; i < number_of_blocks_; ++i) {
    delete[] reinterpret_cast<uint8_t*>(data_[i]);
  }
  delete[] data_;
}
