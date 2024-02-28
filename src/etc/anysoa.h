#ifndef WC_ETC_ANYSOA_H
#define WC_ETC_ANYSOA_H

#include "table.h"
#include "anyvector.h"
#include <functional>
#include <ranges>
#include <vector>
#include <set>
#include <any>
#include <optional>

namespace wc {
namespace etc {

namespace impl {

  // Operator for the "manager" function.
  enum class TypeEraseManagerOp {
    TypeInfo,
    SizeOf,
    Divisor,
    Destruct,
    ConstructDefault,
    ConstructCopy,
    Copy,
    CopySingle,
    Move,
    Swap,
    SwapSingle,
    Hash
  };

  // Result from the "manager" function.
  union TypeEraseManagerResult {
    const std::type_info* typeinfo;
    size_t size;
  };

  // The manager function. This is the real brains of type erasure.
  // op: Which operation to perform.
  // col1: The column on which we're operating on.
  // col2: The second column we're operating on, for binary operations.
  // count: The number of entries we're operating on.
  template <typename T>
  TypeEraseManagerResult typeEraseManager(TypeEraseManagerOp op, void* col1, void* col2, size_t begin, size_t end) {
    using Op = TypeEraseManagerOp;
    switch (op) {
    case Op::TypeInfo: return &typeid(T);
    case Op::SizeOf: return sizeof(T); // TODO: empty structs should return 0.  This is easy, making it toggleable is not.
    case Op::Divisor: return impl::greatestPo2Divisor(sizeof(T));
    case Op::Destruct: {
      auto ptr = reinterpret_cast<T*>(col1);
      for (size_t i {begin}; i < end; ++i) { ptr[i].~T(); }
      return 0; }
    case Op::ConstructDefault: {
      auto ptr = reinterpret_cast<T*>(col1);
      for (size_t i {begin}; i < end; ++i) { new (ptr + i) T(); }
      return 0; }
    case Op::ConstructCopy: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<const T*>(col2);
      for (size_t i {begin}; i < end; ++i) { new (lhs + i) T(rhs[i]); }
      return 0; }
    case Op::Copy: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<const T*>(col2);
      for (size_t i {begin}; i < end; ++i) { lhs[i] = rhs[i]; }
      return 0; }
    case Op::CopySingle: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<const T*>(col2);
      *lhs = *rhs;
      return 0; }
    case Op::Move: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<T*>(col2);
      for (size_t i {begin}; i < end; ++i) { lhs[i] = std::move(rhs[i]); }
      return 0; }
    case Op::Swap: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<T*>(col2);
      for (size_t i {begin}; i < end; ++i) { std::swap(lhs[i], rhs[i]); }
      return 0; }
    case Op::SwapSingle: {
      auto lhs = reinterpret_cast<T*>(col1);
      auto rhs = reinterpret_cast<T*>(col2);
      using std::swap; swap(*lhs, *rhs);
      return 0; }
    case Op::Hash:
      // TODO: if_constexpr to see if std::hash<T> exists, and use a backup
      // (or no-op) if it doesn't.
      return std::hash<T> {}(ptr[index]);
    }
  }
  // This is the type of function pointer used for the manager function.
  // We just need the function signature, which doesn't depend on the template parameter, so 'int' could be anything.
  using TypeEraseManagerFunc = decltype(&typeEraseManager<int>);

} // namespace impl

// The TypeErasedSoaSignature is the heart of a TypeErasedSoa.  It keeps track of pointers to the manager function for each type, as well as offsets.
// This takes the form of a hashtable, where the key is the function pointer and the value is the offset.
// The offset is the number of bytes where the corresponding entry would be in a table with a capacity of 1 (ie, a single row).
// This number can be multiplied by the table's capacity in order to get the offset for the corresponding column.
class TypeErasedSoaSignature : public BasicTable<uint8_t, false, SIZE_MAX, impl::TypeEraseManagerFunc, size_t> {
  using Op = impl::TypeEraseManagerOp;
public:
  TypeErasedSoaSignature() = default;

  friend bool operator==(const TypeErasedSoaSignature& lhs, const TypeErasedSoaSignture& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (auto [lptr, rptr] : std::ranges::zip_view(lhs.viewColumn<0>(), rhs.viewColumn<0>())) {
      if (lptr != rptr) return false;
    }
    return true;
  }

  // Returns true if the signature has the indicated type, false otherwise.
  template <typename T> bool hasType() const {
    return (this->find(&impl::typeEraseManager<T>))
  }

  // Adds a type to this signature.
  // Throws an exception of the signature already contains this type.
  template <typename T> void addType() {
    if (this->find(&impl::typeEraseManager<T>))
      { throw std::logic_error("In TypeErasedSoaSignature::addType(): Entry of type T already present."); }
    this->insert(&impl::typeEraseManager<T>, this->size() ? (this->template back<1>()) + (this->template back<0>()(Op::SizeOf, nullptr, nullptr, 0, 0)) : 0);
  }

  // Removes a type from this signature.
  // Throws an exception if the signature does not contain this type.
  template <typename T> void removeType() {
    if (!this->find(&impl::typeEraseManager<T>))
      { throw std::logic_error("In TypeErasedSoaSignature::removeType(): No entry of type T present."); }
    // Correct the offset for entries after the erased type. 
    for (size_t i {this->eraseShift(&impl::typeEraseManager<T>)}; i < this->size(); ++i) {
      this->template at<1>(i) -= sizeof(T);
    }
  }

  // Add multiple types to this signature.
  template <typename... Ts> void addTypes() { (addType<T>, ...); }

  // Removes multiple types from this signature.
  template <typename... Ts> void removeTypes() { (removeType<T>, ...); }
};

// A TypeErasedSoaRow contains the data and type information for a single row of a TypeErasedSoa.
// It's used for serialization of rows and transferring rows from one table to another.
class TypeErasedSoaRow {
public:
  TypeErasedSoaRow() = default;
  TypeErasedSoaRow(const TypeErasedSoaRow&) = default;
  TypeErasedSoaRow(TypeErasedSoaRow&&) = default;
  TypeErasedSoaRow& operator=(const TypeErasedSoaRow&) = default;
  TypeErasedSoaRow& operator=(TypeErasedSoaRow&&) = default;
  template <typename... Ts> TypeErasedSoaRow(Ts&&... args) {
    ((addColumn(std::forward<Ts>(args))), ...);
  }
  template <typename... Ts> TypeErasedSoaRow(std::tuple<Ts...>&& args) {
    std::apply([this](Ts&&... args) { TypeErasedSoaRow(args...); }, args);
  }

  TypeErasedSoaSignature signature;
  std::vector<char> data;

  // Adds a column of data to this row.
  template <typename T> addColumn(const T& newData) {
    signature.addType<T>();
    if constexpr (sizeof(T) > 0) {
      size_t oldsize {data.size()};
      data.resize(oldsize + sizeof(T));
      auto dst {reinterpret_cast<T*>(&data[oldsize])};
      new (dst) T(newData);
    }
  }

  // Adds a default-constructed column to this row.
  template <typename T> addColumn() {
    signature.addType<T>();
    if constexpr (sizeof(T) > 0) {
      size_t oldsize {data.size()};
      data.resize(oldsize + sizeof(T));
      auto dst = reinterpret_cast<T*>(&data[oldsize]);
      new (dst) T();
    }
  }

  // Removes a column of data from this row.
  template <typename T> void removeColumn() {
    if constexpr (sizeof(T) > 0) {
      auto found = signature.find(&impl::typeEraseManager<T>);
      if (found) {
        size_t offset = found.get<1>();
        auto ptr {reinterpret_cast<T*>(&data[offset])};
        ptr->~T();
        data.erase(offset, offset+sizeof(T));
      }
    }
    signature.eraseShift(&impl::typeEraseManager<T>);
  }
};

class TypeErasedSoa {
  using Op = impl::TypeEraseManagerOp;
public:

  TypeErasedSoa() = default; // Default constructor.
  ~TypeErasedSoa() { reset(); } // Destructor.
  TypeErasedSoa(const TypeErasedSoa& rhs) { // Copy constructor.
    reserve(rhs.size_); assignSignature(rhs.getSignature());
    for (auto [col, rptr] : std::ranges::zip_view(columns_, rhs.columns_.viewColumn<2>())) {
      manageFound(*col, Op::ConstructCopy, rptr, 0, rhs.size_);
  } }
  TypeErasedSoa(TypeErasedSoa&& rhs) { swap(*this,rhs); } // Move constructor.

  TypeErasedSoa& operator=(TypeErasedSoa rhs)   { swap(*this, rhs); return *this; } // Copy assignment operator.  Uses the copy-and-swap idiom.
  TypeErasedSoa& operator=(TypeErasedSoa&& rhs) { swap(*this, rhs); return *this; } // Move assignment operator.

  friend void swap(TypeErasedSoa& lhs, TypeErasedSoa& rhs) {
    std::swap(lhs.data_, rhs.data_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.capacity_, rhs.capacity_);
    swap(lhs.columns_, rhs.columns_);
  }

  void assignSignature(const impl::TypeErasedSoaSignature& signature) {
    if (size_ || columns_.size()) throw std::logic_error("Cannot assign a signature to an TypeErasedSoa while it already has one.");
    columns_ = signature;
  }

  const TypeErasedSoaSignature& getSignature() {
    return columns_;
  }

  template <typename T>     bool hasType()  const { return (columns_.contains(&anyManager<T>)); }
  template <typename... Ts> bool hasTypes() const { return (hasType<Ts>() && ...); }

  template <typename T> auto* data()       { auto found {findCol<T>()}; if (!found) throw std::bad_any_cast(); return reinterpret_cast<T*>      (static_cast<char*>(data_)+(found.get<1>()*capacity_)); }
  template <typename T> auto* data() const { auto found {findCol<T>()}; if (!found) throw std::bad_any_cast(); return reinterpret_cast<const T*>(static_cast<char*>(data_)+(found.get<1>()*capacity_)); }

  template <typename T> auto& at(size_t index)       { boundsCheck(index); return *(data<T>() + index); }
  template <typename T> auto& at(size_t index) const { boundsCheck(index); return *(data<T>() + index); }
  template <typename T> auto& front()                { return at(0); }
  template <typename T> auto& front()          const { return at(0); }
  template <typename T> auto& back()                 { return at(size_-1); }
  template <typename T> auto& back()           const { return at(size_-1); }

  template <typename T>     auto viewColumn()        { auto* ptr = data<T>(); return std::ranges::subrange(ptr, ptr+size_); }
  template <typename T>     auto viewColumn()  const { auto* ptr = data<T>(); return std::ranges::subrange(ptr, ptr+size_); }
  template <typename... Ts> auto viewColumns()       { return std::views::zip(viewColumn<Ts>()...); }
  template <typename... Ts> auto viewColumns() const { return std::views::zip(viewColumn<Ts>()...); }

  void reserve(size_t newCapacity) { if (newCapacity > capacity_) realloc(newCapacity); }
  void shrinkToFit() { realloc(size_); }
  void resize(size_t newSize) {
    if (newSize == size_) return;
    if (newSize > size_) {
      reserve(newSize);
      for (auto [manage, offset] : columns_.viewColumns<1, 2>()) {
        auto ptr = static_cast<char*>(data_) + (offset * capacity_);
        manage(Op::ConstructDefault, ptr, nullptr, size_, newSize);
      }
    }
    else if (newSize < size_) {
      for (auto [manage, offset] : columns_.viewColumns<1, 2>()) {
        auto ptr = static_cast<char*>(data_) + (offset * capacity_);
        manage(Op::Destruct, ptr, nullptr, newSize, size_);
      }
    }
    size_ = newSize;
  }
  void clear() { resize(0); }
  void reset() { realloc(0); columns_.clear(); }

  void insert(size_t index, const TypeErasedSoaRow& row) {
    if (index > size_) throw std::out_of_range("Index out of range.");
    if (size_ == 0) columns_ = row.signature;
    else if (columns_ != row.signature) throw std::bad_any_cast();
    expandIfNeeded();
    for (auto [manage, offset] : columns_) {
      auto   ptr   = static_cast<char*>(data_) + (offset * column_);
      size_t tsize = manage(Op::SizeOf);
      // Move data forward by one spot, if needed.
      if (index < size_) { memmove(ptr+(tsize*(index+1)), ptr+(tsize*index), (size_-index)*tsize); }
      // Copy data from "row" into the container.
      manage(Op::CopySingle, ptr+(tsize*index), &row.data[offset]);
    }
    ++size_;
  }
  void pushBack(const TypeErasedSoaRow& row) { insert(size_, row); }

  void eraseShift(size_t index) {
    boundsCheck(index);
    for (auto [manage, offset] : columns_) {
      auto   ptr   = static_cast<char*>(data_) + (offset * column_);
      size_t tsize = manage(Op::SizeOf);
      // Destruct the entry at index.
      manage(Op::Destruct, ptr, nullptr, index, index+1);
      // Move data backwards by one spot, if needed.
      if (index < (size_-1)) { memmove(ptr+(tsize*index), ptr+(tsize*(index+1)), (size_-(index+1))*tsize); }
    }
    --size_;
  }

  void eraseSwap(size_t index) {
    boundsCheck(index);
    for (auto [manage, offset] : columns_) {
      auto   ptr   = static_cast<char*>(data_) + (offset * column_);
      size_t tsize = manage(Op::SizeOf);
      manage(Op::SwapSingle, ptr+(tsize*index), ptr+(tsize*(size_-1)));
      manage(Op::Destruct, ptr, nullptr, size_-1, size_);
    }
    --size_;
  }

  void popBack() { eraseShift(size_-1); }

  TypeErasedSoaRow getRow(size_t index) {
    boundsCheck(index);
    TypeErasedSoaRow result; result.signature = columns_;
    for (auto [manage, offset] : columns_) {
      auto   ptr   = static_cast<char*>(data_) + (offset * column_);
      size_t tsize = manage(Op::SizeOf);
      manage(Op::CopySingle, &result.data[offset], ptr+(tsize*index));
    }
    return result;
  }

private:

  // Where the data actually lives.
  void* data_ {nullptr};
  // The number of rows in the container.
  size_t size_ {0};
  // How many rows the container could hold without allocating more memory.
  size_t capacity_ {0};
  // This table stores information about the columns of the container.
  // The table uses a key of type size_t, which is obtained by calling
  // typeid(T).hash_code(). The first item column of the table is a pointer to
  // the manager function.  This function handles all type-specific actions. The
  // second item column is an offset pointer; where the data for *this* column
  // begins.
  impl::TypeErasedSoaSignature columns_;

  template <typename T> auto findCol() {  return column_.find(&impl::anyManager<T>); }

  // Gets the total number of bytes taken by one row.
  size_t rowSize() {
    size_t result {0};
    for (auto manage : columns_.viewColumn<0>()) { result += manage(Op::SizeOf, nullptr, nullptr, 0, 0); }
    return result;
  }

  void manageFound(impl::TypeErasedSoaSignature::Found found, Op op, const void* rhs = nullptr, size_t begin = 0, size_t end = 0) {
    auto manage = found.get<0>();
    auto ptr    = static_cast<char*>(data_) + (found.get<1>() * capacity_);
    manage(op, ptr, rhs, begin, end);
  }

  void boundsCheck(size_t index) { if (index >= size_) throw std::out_of_range("Index out of range."); }

  void expandIfNeeded() { if (size_ == capacity_) reserve(std::max(capacity_ * 2, 16)); }

  void* realloc(size_t newCapacity, size_t extraBytes = 0) {
    // If there's nothing to do, bail out.
    if (newCapacity == capacity_ || columns_.size() == 0) return nullptr;

    // Calculate alignment.
    size_t align {16};
    for (auto [manage] : columns_.viewColumn<0>()) {
      size_t divisor = manage(Op::Divisor, nullptr, nullptr, 0, 0);
      if (divisor > 0)
        align = std::min(align, divisor);
    }
    align = (align >= 16) ? 1 : (16 / align);

    // For alignment purposes, we only allocate a multiple of N bytes.
    // We enforce a multiple of N elements instead of adding padding.
    newCapacity = impl::roundUpTo(newCapacity, align);

    // If we're allocating memory...
    if (newCapacity) {
      void* newData = impl::alignedMalloc(16, (newCapacity * rowSize()) + extraBytes);
      // Figure out how many rows we'll need to move.
      size_t rowsToMove = std::min(size_, newCapacity);
      if (rowsToMove) {
        // Move the data into its new position.
        void* dataCursor {newData};
        for (auto [manage, offset] : columns_) {
          size_t tsize = manage(Op::SizeOf, nullptr, nullptr, 0, 0);
          if (tsize == 0) continue;
          auto srcptr = static_cast<const char*>(data_) + (offset * capacity_);
          memmove(dataCursor, srcPtr, tsize * rowsToMove);
          (char*)dataCursor += (tsize * newCapacity);
        }
        // Clean up any leftover objects.
        if (newCapacity < size_) {
          for (auto [manage, offset] : columns_) {
            auto ptr = static_cast<char*>(data_) + (offset * capacity_);
            manage(Op::Destruct, ptr, nullptr, rowsToMove, size_-rowsToMove);
          }
        }
        impl::alignedFree(data_);
      }
      data_ = newData;
      size_ = rowsToMove;
      capacity_ = newCapacity;
    }
    // New capacity is zero, but we have old data to clean up.
    else if (data_) {
      for (auto [manage, offset] : columns_) {
        auto ptr = static_cast<char*>(data_) + (offset * capacity_);
        manage(Op::Destruct, ptr, nullptr, 0, size_);
      }
      impl::alignedFree(data_);
      data_ = nullptr;
      size_ = 0;
      capacity_ = 0;
      return nullptr;
    }
    return nullptr;
  }
};

}} // namespace wc::etc
#endif // WC_ ETC_ANYSOA