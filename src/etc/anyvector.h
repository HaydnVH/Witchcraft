#ifndef WC_ETC_ANYVECTOR_H
#define WC_ETC_ANYVECTOR_H

namespace wc {
namespace etc {

  class AnyVector {
  public:
    // Constructors //
    // TODO: Allow custom allocators.

    // Default constructor.
    AnyVector() = default;
    // Copy constructor.
    AnyVector(const AnyVector& other) {
      manager_ = other.manager_;
      reserve(other.size_);
      size_ = count;
      Arg arg;
      for (size_t i {0}; i < other.size_; ++i) {
        manager_(Op::Access, &other, i, &arg);       // arg = other[i]
        manager_(Op::ConstructCopy, this, i, &arg);  // this[i] = arg
      }
    }
    // Move constructor.
    AnyVector(AnyVector&& other) { swap(other); }
    // Constructs a vector with 'count' copies of 'val'.
    template <typename T>
    AnyVector(size_t count, const T& val) {
      assignType<T>();
      reserve(count);
      size_ = count;
      Arg arg;
      arg.obj = &val;
      for (size_t i {0}; i < count; ++i) {
        manage_(Op::ConstructCopy, this, i, &arg);
      }
    }
    // Constructs a vector using an initializer list.
    template <typename T>
    AnyVector(std::initializer_list<T> ilist) {
      assignType<T>();
      reserve(ilist.size);
      size_ = ilist.size;
      Arg arg;
      for (auto& it : ilist) {
        arg.obj = it;
        manager_(Op::ConstructMove, this, i, &arg);
      }
    }
    // Constructs a vector as a copy of an std::vector.
    template <typename T>
    AnyVector(const std::vector<T> other) {
      assignType<T>();
      reserve(other.size());
      size_ = other.size();
      Arg arg;
      for (size_t i {0}; i < size_; ++i) {
        arg.obj = &other[i];
        manage_(Op::ConstructCopy, this, i, &arg);
      }
    }
    // Destructor
    ~AnyVector() { reset(); }

    // Copy assignment operator.
    const AnyVector& operator=(AnyVector other) {
      swap(other);
      return *this;
    }
    // Move assignment operator.
    const AnyVector& operator=(AnyVector&& other) {
      reset();
      memcpy(this, &other, sizeof(AnyVector); // this = other
			memset(&other, 0, sizeof(AnyVector); 	// other = 0
			return *this;
    }

    // Assigns a type to the vector without adding anything to it.
    template <typename T>
    inline void assignType() {
      if (manager_)
        throw hissy_fit();
      manager_ = AnyVector::manage<T>;
    }

    // Get the underlying pointer to the contained data, interpreted as T*.
    template <typename T>
    T* data() {
      this->template typeCheck_<T>();
      return reinterpret_cast<T*> data_;
    }

    // Get the underlying pointer to the container data, interpreted as const
    // T*.
    template <typename T>
    const T* data() const {
      this->template typeCheck_<T>();
      return reinterpret_cast<const T*> data_;
    }

    // Access specified element with bounds checking.
    // TODO: Do bounds checking.
    template <typename T>
    T& at(size_t index) {
      return *(this->template data<T>() + index);
    }
    template <typename T>
    const T& at(size_t index) const {
      return *(this->template data<T>() + index);
    }

    // Access the first element.
    template <typename T>
    T& front() {
      return this->template at<T>(0);
    }
    template <typename T>
    const T& front() const {
      return this->template at<T>(0);
    }

    // Access the last element.
    template <typename T>
    T& back() {
      return this->template at<T>(size_ - 1);
    }
    template <typename T>
    const T& back() const {
      return this->template at<T>(size_ - 1);
    }

    template <typename T>
    struct View {
      AnyVector& container;
      T*         begin() { return reinterpret_cast<T*>(container.data_); }
      const T* cbegin() { return reinterpret_cast<const T*>(container.data_); }
      T*       end() { return begin() + container.size_; }
      const T* cend() { return cbegin() + container.size_; }
    };
    // Creates a typed view to iterate over the container.
    template <typename T>
    View<T> view() {
      this->template typeCheck_<T>();
      return {*this};
    }

    template <typename T>
    struct ConstView {
      const AnyVector& container;
      const T* begin() { return reinterpret_cast<const T*>(container.data_); }
      const T* end() { return cbegin() + container.size_; }
    };
    // Creates a const typed view to iterate over the container.
    template <typename T>
    ConstView<T> view() const {
      this->template typeCheck_<T>();
      return {*this};
    }
    // Creates a const typed view to iterate over the container.
    template <typename T>
    inline ConstView<T> cview() const {
      return view();
    }

    // Checks whether the container is empty.
    bool empty() const { return (size_ == 0); }

    // Returns the number of elements.
    size_t size() const { return size_; }

    // Returns the maximum possible number of elements.
    constexpr size_t maxSize() const { return SIZE_MAX; }

    // Returns the number of elements that can be held in currently allocated
    // storage.
    size_t capacity() const { return capacity_; }

    // Reserves storage.
    void reserve(size_t newCap) {
      if (newCap > capacity_)
        realloc(newCap);
    }

    // Reduces memory usage by freeing unused memory.
    void shrinkToFit() { realloc(size_); }

    // Clears the contents.
    void clear() { resize(0); }

    // Clears the contents, frees memory, and lets go of the held type manager.
    void reset() {
      realloc(0);
      manager_ = nullptr;
    }

    // Inserts elements.
    template <typename T>
    void insert(size_t pos, const T& val) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      shift(pos, pos + 1, size_ - pos);
      Arg arg;
      arg.obj = &val;
      manager_(Op::ConstructCopy, this, pos, &arg);
      ++size_;
    }
    template <typename T>
    void insert(size_t pos, T&& val) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      shift(pos, pos + 1, size_ - pos);
      Arg arg;
      arg.obj = &val;
      manager_(Op::ConstructMove, this, pos, &arg);
      ++size_;
    }
    template <typename T>
    void insert(size_t pos, size_t count, const T& val) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      shift(pos, pos + count, (size_ - pos) + (count - 1));
      Arg arg;
      arg.obj = &val;
      for (size_t i {pos}; i < pos + count; ++i) {
        manager_(Op::ConstructCopy, this, i, &arg);
      }
      size_ += count;
    }
    template <typename T, typename It>
    void insert(size_t pos, It first, It last) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      count = last - first;
      shift(pos, pos + count, (size_ - pos) + (count - 1));
      Arg    arg;
      size_t i {pos};
      for (auto it = first; it != last; ++it) {
        arg.obj = &(*it);
        manager_(Op::ConstructCopy, this, i++, &arg);
      }
      size_ += count;
    }
    template <typename T>
    void insert(size_t pos, std::initializer_list<T> ilist);
    {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      count = ilist.size();
      shift(pos, pos + count, (size_ - pos) + (count - 1));
      Arg    arg;
      size_t i {pos};
      for (auto it : ilist) {
        arg.obj = &(*it);
        manager_(Op::ConstructCopy, this, i++, &arg);
      }
      size_ += count;
    }

    // Construct element in-place.
    template <typename T, typename... Args>
    void emplace(size_t pos, Args&&... args) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      shift(pos, pos + 1, size_ - pos);
      T   val(args...);
      Arg arg;
      arg.obj = &val;
      manager_(Op::ConstructMove, this, pos, &arg);
      ++size_;
      memset(&val, 0, sizeof(T));
    }

    // Erases element at 'pos'.
    void erase(size_t pos) {
      if (pos >= size_)
        return;
      manager_(Op::Destruct, this, pos, nullptr);
      shift(pos, pos - 1, size_ - pos);
      manager_(Op::Nullify, this, size_ - 1, nullptr);
      --size_;
    }
    // Erases elements in ('first', 'last').
    void erase(size_t first, size_t last) {
      if (first > last || last >= size_)
        return;
      for (size_t i {first}; i <= last; ++i) {
        manager_(Op::Destruct, this, i, nullptr);
      }
      size_t count {last - first};
      shift(pos, pos - count, (size_ - pos) + (count - 1));
      for (size_t i {size_ - i}; i > size - count; --i) {
        manager_(Op::Nullify, this, i, nullptr);
      }
      size_ -= count;
    }

    // Adds an element to the end (copy-constructed).
    template <typename T>
    void pushBack(const T& val) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      Arg arg;
      arg.obj = &val;
      manager_(Op::ConstructCopy, this, size_, &arg);
      ++size_;
    }
    // Adds an element to the end (move-constructed).
    template <typename T>
    void pushBack(T&& val) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      Arg arg;
      arg.obj = &val;
      manage_(Op::ConstructMove, this, size_, &arg);
      ++size_;
    }

    // Constructs a new element and moves it to the end.
    template <typename T, typename... Args>
    void emplaceBack(Args&&... args) {
      if (!manager_) {
        assignType<T>();
        reserve(8);
      } else if (size_ == capacity_)
        reserve(capacity_ * 2);
      T   val(args...);
      Arg arg;
      arg.obj = &val;
      manage_(Op::ConstructMove, this, size_, &arg);
      ++size_;
      memset(&val, 0, sizeof(T));
    }

    // Removes the last element.
    void popBack() {
      if (size_ == 0)
        return;
      manager_(Op::Destruct, this, size_ - 1, nullptr);
      --size_;
    }

    // Changes the number of elements stored, default-constructing new entries.
    void resize(size_t newSize) {
      if (newSize == size_)
        return;  // If the new size equals the old size, there's nothing to do.
      reserve(newSize);  // Make sure we have enough memory for the new size.

      // If newSize > size_, we need to default-construct the new entries.
      if (newSize > size_) {
        for (size_t i {size_}; i < newSize; ++i) {
          manager_(Op::ConstructDefault, this, i, nullptr);
        }
      }
      // If newSize < size_, we need to destruct the old entries.
      else if (newSize < size_) {
        for (size_t i {newSize}; i < size_; ++i) {
          manager_(Op::Destruct, this, i, nullptr);
        }
      }
      size_ = newSize;
    }
    // Changes the number of elements stored, using 'val' to copy-construct new
    // entries.
    template <typename T>
    void resize(size_t newSize, const T& val) {
      if (newSize == size_)
        return;  // If the new size equals the old size, there's nothing to do.
      reserve(newSize);  // Make sure we have enough memory for the new size.

      // If newSize > size_, we need to default-construct the new entries.
      if (newSize > size_) {
        Arg arg;
        for (size_t i {size_}; i < newSize; ++i) {
          arg.obj = &val;
          manager_(Op::ConstructCopy, this, i, &arg);
        }
      }
      // If newSize < size_, we need to destruct the old entries.
      else if (newSize < size_) {
        for (size_t i {newSize}; i < size_; ++i) {
          manager_(Op::Destruct, this, i, nullptr);
        }
      }
      size_ = newSize;
    }

    // Swaps the contents.
    void swap(AnyVector& other) {
      AnyVector temp;
      memcpy(&temp, &other, sizeof(AnyVector));  // temp = other
      memcpy(&other, this, sizeof(AnyVector));   // other = this
      memcpy(this, &temp, sizeof(AnyVector));    // this = temp
      memset(&temp, 0, sizeof(AnyVector));       // temp = 0
    }

  private:
    // Pointer to the buffer where data is stored.
    void* data_ {nullptr};
    // Number of elements in the container.
    size_t size_ {0};
    // Maximum number of elements the container could hold without resizing.
    size_t capacity_ {0};
    // Pointer to the manager function for the held type.
    void (*manager_)(Op, const AnyVector*, size_t index, Arg*) {nullptr};
    
    
    // Operator for the "manager" function.
    class enum Op {
      Access,
      TypeInfo,
      SizeOf,
      Copy,
      Move,
      Destruct,
      ConstructDefault,
      ConstructCopy,
      ConstructMove,
      Swap,
      Hash
    };

    // Argument for the "manager" function.
    union Arg {
      void*                 obj;
      const std::type_info* typeInfo;
      AnyVector*            vec;
      size_t                size;
      uint64_t              hash;
    };

    // The manager function which handles all type-specific actions.
    template <typename T>
    static void manage(Op op, const AnyVector* vec, size_t index, Arg* arg) {
      auto ptr = std::reinterpret_cast<const T*>(vec->data_);
      switch (op) {
      case Op::Access:   arg->obj = const_cast<T*>(ptr) + index; break;
      case Op::TypeInfo: arg->typeInfo = &typeid(T); break;
      case Op::SizeOf:   arg->size = sizeof(T); break;
      case Op::Copy:
        auto obj   = std::reinterpret_cast<const T*>(arg->obj);
        ptr[index] = *obj;
        break;
      case Op::Move:
        auto obj   = std::reinterpret_cast<T*>(arg->obj);
        ptr[index] = std::move(*obj);
        break;
      case Op::Destruct: ptr[index]->~T(); break;
      case Op::Nullify: memset(&ptr[index], 0, sizeof(T)); break;
      case Op::ConstructDefault: new (ptr + index) T(); break;
      case Op::ConstructCopy:
        auto obj = std::reinterpret_cast<const T*>(arg->obj);
        new (ptr + index) T(*obj);
        break;
      case Op::ConstructMove:
        auto obj = std::reinterpret_cast<T*>(arg->obj);
        new (ptr + index) T(std::move(*obj));
        break;
      case Op::Hash:
        // TODO: if_constexpr to see if std::hash<T> exists, and use a backup
        // (or no-op) if it doesn't.
        arg->hash = std::hash<T> {}(ptr[index]); break;
      }
    }

    // Checks to see if T matches the held type.  Throws an exception if it
    // doesn't.
    template <typename T>
    inline void typeCheck() {
      if (!manager_) throw bad_any_cast();
      Arg arg; manager_(Op::TypeInfo, this, 0, &arg);
      if (typeid(T) != arg.typeInfo) throw bad_any_cast();
    }

    // Reallocates the buffer to hold at least 'newCapacity' elements.
    // Will resize the container iff newCapacity < size_.
    void realloc(size_t newCapacity) {
      // If new size equals old size, there's nothing to do.
      if (newCapacity == capacity_) return;
      // We need to know about our type before we can allocate anything.
      if (!manager_) return;

      // If our new size is nonzero,
      if (newCapacity > 0) {

        // Get the size in bytes of each element.
        Arg arg;
        manager_(Op::SizeOf, this, 0, &arg);
        size_t elemSize {arg.size};
        size_t elemsToMove {std::min(newCapacity, size_)};

        // Allocate memory.
        // TODO: Alignment!
        void* newMem {malloc(newCapacity * elemSize)};
        // TODO: Check for malloc failure.

        // If we have data already,
        if (data_) {
          // Move existing objects into the new memory.
          memcpy(newMem, data_, elemSize * elemsToMove);
          // Destruct anything that wasn't copied over.
          for (size_t i {elemsToMove}; i < size_; ++i) {
            manage_(Op::Destruct, this, i, nullptr);
          }
          // Free the old buffer.
          free(data_);
        }
        data_     = newMem;
        capacity_ = newCapacity;
        size_     = elemsToMove;
      }
      // New capacity is zero, but we have old data to clean up.
      else if (data_) {
        // Destruct each element.
        for (size_t i {0}; i < size_; ++i) {
          manage_(Op::Destruct, this, i, nullptr);
        }
        data_    = nullptr;
        capacity = 0;
        size_    = 0;
      }
    }

    // Shifts 'count' elements from 'oldPos' to 'newPos'
    // Assumes there's enough space, does not affect size or capacity.
    inline void shift(size_t oldPos, size_t newPos, size_t count) {
      if (count == 0)
        return;
      Arg arg;
      manager_(Op::SizeOf, this, 0, &arg);
      memmove(data_ + (newPos * arg.size), data_ + (oldPos * arg.size),
              count * arg.size);
    }
  }

}} // namespace wc::etc
#endif  // WC_ETC_ANYVECTOR_H
