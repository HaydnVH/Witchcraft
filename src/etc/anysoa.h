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
  enum class AnyOp {
    TypeInfo,
    SizeOf,
    Destruct,
    ConstructDefault,
    ConstructCopy,
    Copy,
    Move,
    Swap,
    Hash
  };

  // Argument for the "manager" function.
  union AnyArg {
    void*                 obj;
    const std::type_info* typeInfo;
    void*                 data;
    size_t                size;
    uint64_t              hash;
  };

  // Result from the "manager" function.
  union AnyResult {
    const std::type_info* typeinfo;
    size_t size;
  };

  // The manager function. This is the real brains of type erasure.
  // op: Which operation to perform.
  // col1: The column on which we're operating on.
  // col2: The second column we're operating on, for binary operations.
  // count: The number of entries we're operating on.
  template <typename T>
  AnyResult anyManager(AnyOp op, const void* col1 = nullptr, const void* col2 = nullptr, size_t begin = 0, size_t end = 0) {
    switch (op) {
    case AnyOp::TypeInfo: return &typeid(T);
    case AnyOp::SizeOf: return sizeof(T); // TODO: empty structs should return 0.  This is easy, making it toggleable is not.
    case AnyOp::Destruct: {
      auto ptr = std::reinterpret_cast<T*>(col1);
      for (size_t i {begin}; i < end; ++i) { ptr[i].~T(); }
      return 0; }
    case AnyOp::ConstructDefault: {
      auto ptr = std::reinterpret_cast<T*>(col1);
      for (size_t i {begin}; i < end; ++i) { new (ptr + i) T(); }
      return 0; }
    case AnyOp::ConstructCopy: {
      auto lhs = std::reinterpret_cast<T*>(col1);
      auto rhs = std::reinterpret_cast<const T*>(col2);
      for (size_t i {begin}; i < end; ++i) { new (lhs + i) T(rhs[i]); }
      return 0; }
    case AnyOp::Copy: {
      auto lhs = std::reinterpret_cast<T*>(col1);
      auto rhs = std::reinterpret_cast<const T*>(col2);
      for (size_t i {begin}; i < end; ++i) { lhs[i] = rhs[i]; }
      return 0; }
    case AnyOp::Move: {
      auto lhs = std::reinterpret_cast<T*>(col1);
      auto rhs = std::reinterpret_cast<T*>(col2);
      for (size_t i {begin}; i < end; ++i) { lhs[i] = std::move(rhs[i]); }
      return 0; }
    case AnyOp::Swap: {
      auto lhs = std::reinterpret_cast<T*>(col1);
      auto rhs = std::reinterpret_cast<T*>(col2);
      for (size_t i {begin}; i < end; ++i) { std::swap(lhs[i], rhs[i]); }
      return 0; }
    case AnyOp::Hash:
      // TODO: if_constexpr to see if std::hash<T> exists, and use a backup
      // (or no-op) if it doesn't.
      return std::hash<T> {}(ptr[index]);
    }
  }
  using ManagerFunc = std::function<AnyResult(AnyOp, const void*, const void*, size_t, size_t)>;

  using AnySoaSignature = BasicTable<uint8_t, false, SIZE_MAX, size_t>;

  extern Table<size_t, ManagerFunc> managerFuncLookup_g;

  // The AnyColumnTable defines the "signature" of an AnySoa.
  // This determines how many columns there are, what's in those columns, and how wide each column has to be.
  // An AnySoa's signature cannot be changed after initialization unless 'reset' is used to totally clear its contents.
  // Currently we 'only' support up to 254 columns.  If that's not enough, idk what to say.
  class AnySoaColumnTable : public BasicTable<uint8_t, false, SIZE_MAX, size_t, ManagerFunc, void*> {
  public:
    AnySoaColumnTable() = default;
    //AnySoaColumnTable(const AnySoaSignature& sig) { for (auto hash : sig) { insert(hash, managerFuncLookup_g[sig], nullptr); }  }
    template <typename T> void addType() { insert(typeid(T).hash_code(), anyManager<T>, nullptr); }
    template <typename T> void removeType() { eraseShift(typeid(T).hash_code()); }
    friend bool operator==(const AnySoaColumnTable& lhs, AnySoaColumnTable& rhs) {
      if (lhs.size() != rhs.size()) return false;
      for (const auto& [a, b] = std::views::zip(lhs.viewColumn<0>(), rhs.viewColumn<0>())) {
        if (a != b) return false;
      } return true; }
    friend bool operator==(const AnySoaColumnTable& lhs, const AnySoaSignature& rhs) {
      if (lhs.size() != rhs.size()) return false;
      for (const auto& [a, b] = std::views::zip(lhs.viewColumn<0>(), rhs.viewColumn<0>())) {
        if (a != b) return false;
      } return true; }
    friend bool operator==(const AnySoaSignature& lhs, const AnySoaColumnTable& rhs) {
      if (lhs.size() != rhs.size()) return false;
      for (const auto& [a, b] = std::views::zip(lhs.viewColumn<0>(), rhs.viewColumn<0>())) {
        if (a != b) return false;
      } return true; }
  };

  // This class represents one complete row of data from an AnySoa.
  // This is used for moving entries from one table to another, or saving/loading a single entry to/from disk.
  class AnySoaRow {
    AnySoaSignature signature;
    std::vector<char> data;

    // Adds a "column" to this row.  Initializes it with newData.
    template <typename T> void addColumn(const T& newData) {
      if (signature.find(typeid(T).hash_code()))
        throw std::logic_error("In AnySoaRow::addColumn(): Entry of type T already present.")
      signature.addType<T>();
      if constexpr (sizeof(T) > 0) {
        size_t oldSize = data.size();
        data.resize(oldSize + sizeof(T));
        auto dst = reinterpret_cast<T*>(&data[oldSize]);
        new (dst) T(newData);
      }
    }
    // Adds a "column" to this row.  Initializes it with a default constructor.
    template <typename T> void addColumn() { addColumn<T>({}); }

    // Removes a "column" from this row.  Destructs it and shifts entries beyond it into place.
    template <typename T>
    void removeColumn() {
      if (!signature.find(typeid(T).hash_code()))
        throw std::logic_error("In AnySoaRow::removeColumn(): No entry of type T present.")
      if constexpr (sizeof(T) > 0) {
        // First we need to find the offset of the desired entry.
        size_t offset {0};
        for (auto [hash, manage] : signature.viewColumn<0, 1>()) {
          if (typeid(T).hash_code() == hash) break;
          else offset += manage(AnyOp::SizeOf);
        }
        // Destruct the desired entry.
        auto ptr {reinterpret_cast<T*>(&data[offset])}; ptr->~T();
        // Erase the data.
        data.erase(offset, offset+sizeof(T));
      }
      signature.removeType<T>();
    }
  };

} // namespace impl

class AnySoa {
  using Op = impl::AnyOp;
public:

  AnySoa() = default; // Default constructor.
  ~AnySoa() { reset(); } // Destructor.
  AnySoa(const AnySoa& rhs) { // Copy constructor.
    reserve(rhs.size_); assignSignature(rhs.getSignature());
    for (auto [col, rptr] : std::ranges::zip_view(columns_, rhs.columns_.viewColumn<2>())) {
      manageFound(*col, Op::ConstructCopy, rptr, 0, rhs.size_);
  } }
  AnySoa(AnySoa&& rhs) { swap(*this,rhs); } // Move constructor.

  AnySoa& operator=(AnySoa rhs)   { swap(*this, rhs); return *this; } // Copy assignment operator.  Uses the copy-and-swap idiom.
  AnySoa& operator=(AnySoa&& rhs) { swap(*this, rhs); return *this; } // Move assignment operator.

  friend void swap(AnySoa& lhs, AnySoa& rhs) {
    std::swap(lhs.data_, rhs.data_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.capacity_, rhs.capacity_);
    swap(lhs.columns_, rhs.columns_);
  }

  void assignSignature(const impl::AnySoaSignature& signature) {
    if (columns_.size()) throw std::logic_error("Cannot assign a signature to an AnySoa while it already has one.");
    columns_ = signature;
  }

  impl::AnyColumnTable getSignature() {
    auto sig {columns_};
    // The 'pointer offset' column of the AnyColumnTable is not part of the signature.
    // Nullify it here so we don't accidentally refer to bad memory.
    for (auto& ptr : sig.viewColumn<2>()) { ptr = nullptr; }
    return sig;
  }

  template <typename T>     bool hasType()  const { return (columns_.contains(typeid(T).hash_code())); }
  template <typename... Ts> bool hasTypes() const { return (hasType<Ts>() && ...); }

  template <typename T> auto data()       { auto found = findCol<T>(); if (!found) throw std::bad_any_cast(); return reinterpret_cast<T*>      (found.get<2>()); }
  template <typename T> auto data() const { auto found = findCol<T>(); if (!found) throw std::bad_any_cast(); return reinterpret_cast<const T*>(found.get<2>()); }

  template <typename T> auto& at(size_t index)       { boundsCheck(index); return *(data<T>() + index); }
  template <typename T> auto& at(size_t index) const { boundsCheck(index); return *(data<T>() + index); }
  template <typename T> auto  front()                { return at(0); }
  template <typename T> auto  front()          const { return at(0); }
  template <typename T> auto  back()                 { return at(size_-1); }
  template <typename T> auto  back()           const { return at(size_-1); }

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
      for (auto [manage, ptr] : columns_.viewColumns<1, 2>())
        { manage(Op::ConstructDefault, ptr, nullptr, size_, newSize); }
    }
    else if (newSize < size_) {
      for (auto [manage, ptr] : columns_.viewColumns<1, 2>())
        { manage(Op::Destruct, ptr, nullptr, newSize, size_); }
    }
    size_ = newSize;
  }
  void clear() { resize(0); }
  void reset() { realloc(0); columns_.clear(); }

  /*
  template <typename... Ts> void insert(size_t index, Ts&&... row) {
    if (index > size_) throw std::out_of_range("Index out of range.");
    if (!hasTypes<Ts...>()) throw std::bad_any_cast();
    expandIfNeeded();
    for (auto [manage, ptr] : columns_.viewColumns<1, 2>()) {
      size_t tsize = manage(Op::SizeOf);
      memmove((char*)ptr + (tsize*(index+1)), (char*)ptr + (tszie*index), size_-index);
    }
  }
  */

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
  impl::AnyColumnTable columns_;

  template <typename T> auto findCol() {  return column_.find(typeid(T).hash_code()); }

  // Gets the total number of bytes taken by one row.
  size_t rowSize() {
    size_t result {0};
    for (auto manage : columns_.viewColumn<1>()) { result += manage(Op::SizeOf); }
    return result;
  }

  void manageFound(impl::AnyColumnTable::Found found, Op op, const void* rhs = nullptr, size_t begin = 0, size_t end = 0)
    { found.get<1>()(op, found.get<2>(), rhs, begin, end); }

  void boundsCheck(size_t index) { if (index >= size_) throw std::out_of_range("Index out of range."); }

  void expandIfNeeded() { if (size_ == capacity_) reserve(std::max(capacity_ * 2, 16)); }

  void* realloc(size_t newCapacity, size_t extraBytes = 0) {
    // If there's nothing to do, bail out.
	  if (newCapacity == capacity_ || columns_.size() == 0) return nullptr;

    // For alignment purposes, we only allocate a multiple of 16 bytes.
    // We enforce a multiple of 16 elements instead of adding padding.
    newCapacity += ((newCapacity % 16) ? 16 - (newCapacity % 16) : 0);

    // If we're allocating memory...
    if (newCapacity) {
      void* newData = impl::soaAlignedMalloc(16, (newCapacity * rowSize()) + extraBytes);
      // Figure out how many rows we'll need to move.
      size_t rowsToMove = std::min(size_, newCapacity);
      if (rowsToMove) {
        // Move the data into its new position.
        void* dataCursor {newData};
        for (auto [manage, srcPtr] : columns_.viewColumns<1, 2>()) {
          size_t tsize = manage(Op::SizeOf);
          if (tsize == 0) continue;
          memmove(dataCursor, srcPtr, tsize * rowsToMove);
          dataCursor += (tsize * newCapacity);
        }
        // Clean up any leftover objects.
        if (newCapacity < size_) {
          for (auto [manage, ptr] : columns_.viewColumns<1, 2>())
            { manage(Op::Destruct, ptr, nullptr, rowsToMove, size_-rowsToMove); }
        }
        impl::soaAlignedFree(data_);
      }
      data_ = newData;
      size_ = rowsToMove;
      capacity_ = newCapacity;
      void* dataCursor {data_};
      for (auto& [manage, ptr] : columns_.viewColumns<1, 2>())
        { ptr = dataCursor; dataCursor += (manage(Op::SizeOf) * capacity_); }
    }
    // New capacity is zero, but we have old data to clean up.
    else if (data_) {
      for (auto [manage, ptr] : columns_.viewColumns<1, 2>())
        { manage(Op::Destruct, ptr, nullptr, 0, size_); }
      impl::SoaAlignedFree(data_);
      data_ = nullptr;
      size_ = 0;
      capacity_ = 0;
      columns_.clear();
      return nullptr;
    }
    return nullptr;
  }
};

}} // namespace wc::etc
#endif // WC_ ETC_ANYSOA