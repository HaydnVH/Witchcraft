/* soa.hpp
 * Struct-Of-Arrays template class
 * by Haydn V. Harach
 * Created October 2019
 * Modified February 2023
 *
 * Implements a Struct-Of-Arrays container class to store and manage a series
 * of contiguous arrays which are stored back-to-back in memory.
 * The interface is designed to be similar to that of std::vector.
 */

#ifndef HVH_TOOLS_STRUCTOFARRAYS_H
#define HVH_TOOLS_STRUCTOFARRAYS_H

#include <algorithm>  // For std::min
#include <climits>
#include <cstdint>
#include <cstring>  // For memcpy and memmove
#include <functional>
#include <tuple>
#include <vector>

/******************************************************************************
 * Aligned Malloc/Free
 *****************************************************************************/
#include <cstdlib>

namespace hvh {

  namespace impl {

// We need access to aligned allocations and deallocations,
// but visual studio doesn't support aligned_alloc from the c11 standard.
// This define lets us have consistent behaviour.
#ifdef _MSC_VER
  #define SoaAlignedMalloc(alignment, size) _aligned_malloc(size, alignment)
  #define SoaAlignedFree(mem)               _aligned_free(mem)
#else
  #define SoaAlignedMalloc(alignment, size) aligned_alloc(alignment, size)
  #define SoaAlignedFree(mem)               free(mem)
#endif

    // This is the base case for the recursive class.
    // It contains basic size info for the container,
    // but mainly does nothing so the recursive calls can stop.
    template <typename... Ts>
    class SoaBase {
    public:
      inline size_t constexpr sizePerEntry() const { return 0; }
      inline void        nullify() {}
      inline void        constructRange(size_t, size_t) {}
      inline void        destructRange(size_t, size_t) {}
      inline void        divyBuffer(void*) {}
      inline void        pushBack() {}
      inline void        emplaceBack() {}
      inline void        emplaceBackDefault() {}
      inline void        popBack() {}
      inline void        insert(size_t) {}
      inline void        emplace(size_t) {}
      inline void        emplaceDefault(size_t) {}
      inline void        eraseSwap(size_t) {}
      inline void        eraseShift(size_t) {}
      friend inline void swap(SoaBase<Ts...>& lhs, SoaBase<Ts...>& rhs) {
        std::swap(lhs.size_, rhs.size_);
        std::swap(lhs.capacity_, rhs.capacity_);
      }
      inline void copy(const SoaBase<Ts...>& other) { size_ = other.size_; }
      inline void swapEntries(size_t, size_t) {}
      inline std::tuple<> makeRowTuple(size_t) const { return std::tuple<>(); }

    protected:
      SoaBase() {}
      size_t size_     = 0;
      size_t capacity_ = 0;
    };

    template <typename FT, typename... RTs>
    class SoaBase<FT, RTs...>: public SoaBase<RTs...> {

      // This allows us to get the type of a given column.
      template <size_t, typename>
      struct ElemTypeHolder;

      // Template specialization to fetch the actual type (base case).
      template <typename T, typename... Ts>
      struct ElemTypeHolder<0, SoaBase<T, Ts...>> {
        typedef T type;
      };

      // Template specialization to fetch the actual type (recursive case).
      template <size_t K, typename T, typename... Ts>
      struct ElemTypeHolder<K, SoaBase<T, Ts...>> {
        typedef typename ElemTypeHolder<K - 1, SoaBase<Ts...>>::type type;
      };

    public:
      // data<K>()
      // Gets a constant reference to the pointer to the Kth array.
      // Elements of the array may be modified, but the array itself cannot.
      // As a reference to the internal array, the result will remain valid even
      // after a reallocation.
      template <size_t K>
      typename std::enable_if<K == 0, FT* const&>::type inline data() {
        return data_;
      }

      // data<K>()
      // Gets a constant reference to the pointer to the Kth array.
      // Elements of the array may be modified, but the array itself cannot.
      // As a reference to the internal array, the result will remain valid even
      // after a reallocation.
      template <size_t K>
      typename std::enable_if<K != 0,
                              typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::
                                  type* const&>::type inline data() {
        SoaBase<RTs...>& base = *this;
        return base.template data<K - 1>();
      }

      // data<K>() const
      // Gets a constant reference to a constant pointer to the Kth array.
      // Neither the array nor its elements can be modified.
      // As a reference to the internal array, the result will remain valid even
      // after a reallocation.
      template <size_t K>
      typename std::enable_if<K == 0, const FT* const&>::type inline data()
          const {
        return data_;
      }

      // data<K>() const
      // Gets a constant reference to a constant pointer to the Kth array.
      // Neither the array nor its elements can be modified.
      // As a reference to the internal array, the result will remain valid even
      // after a reallocation.
      template <size_t K>
      typename std::enable_if<
          K != 0, const typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::
                      type* const&>::type inline data() const {
        SoaBase<RTs...>& base = *this;
        return base.template data<K - 1>();
      }

      // at<K>(i)
      // Gets a reference to the ith item of the Kth array.
      // Does not perform bounds checking; do not use with an out-of-bounds
      // index!
      template <size_t K>
      typename std::enable_if<K == 0, FT&>::type inline at(size_t index) {
        return data_[index];
      }

      // at<K>(i)
      // Gets a reference to the ith item of the Kth array.
      // Does not perform bounds checking; do not use with an out-of-bounds
      // index!
      template <size_t K>
      typename std::enable_if<
          K != 0, typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::type&>::
          type inline at(size_t index) {
        SoaBase<RTs...>& base = *this;
        return base.template at<K - 1>(index);
      }

      // at<K>(i) const
      // Gets a constant reference to the ith item of the Kth array.
      // Does not perform bounds checking; do not use with an out-of-bounds
      // index!
      template <size_t K>
      typename std::enable_if<K == 0, const FT&>::type inline at(
          size_t index) const {
        return data_[index];
      }

      // at<K>(i) const
      // Gets a constant reference to the ith item of the Kth array.
      // Does not perform bounds checking; do not use with an out-of-bounds
      // index!
      template <size_t K>
      typename std::enable_if<K != 0, const typename ElemTypeHolder<
                                          K, SoaBase<FT, RTs...>>::type&>::
          type inline at(size_t index) const {
        SoaBase<RTs...>& base = *this;
        return base.template at<K - 1>(index);
      }

      // front<K>()
      // Gets a reference to the item at the front of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K == 0, FT&>::type inline front() {
        return data_[0];
      }

      // front<K>()
      // Gets a reference to the item at the front of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<
          K != 0, typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::type&>::
          type inline front() {
        SoaBase<RTs...>& base = *this;
        return base.template front<K - 1>();
      }

      // front<K>() const
      // Gets a const reference to the item at the front of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K == 0, const FT&>::type inline front() const {
        return data_[0];
      }

      // front<K>() const
      // Gets a constant reference to the item at the front of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K != 0, const typename ElemTypeHolder<
                                          K, SoaBase<FT, RTs...>>::type&>::
          type inline front() const {
        SoaBase<RTs...>& base = *this;
        return base.template front<K - 1>();
      }

      // back<K>()
      // Gets a reference to the item at the back of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K == 0, FT&>::type inline back() {
        return data_[this->size_ - 1];
      }

      // back<K>()
      // Gets a reference to the item at the back of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<
          K != 0, typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::type&>::
          type inline back() {
        SoaBase<RTs...>& base = *this;
        return base.template back<K - 1>();
      }

      // back<K>() const
      // Gets a constant reference to the item at the back of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K == 0, const FT&>::type inline back() const {
        return data_[this->size_ - 1];
      }

      // back<K>() const
      // Gets a constant reference to the item at the back of the Kth array.
      // Does not perform bounds checking; do not call on an empty container!
      template <size_t K>
      typename std::enable_if<K != 0, const typename ElemTypeHolder<
                                          K, SoaBase<FT, RTs...>>::type&>::
          type inline back() const {
        SoaBase<RTs...>& base = *this;
        return base.template back<K - 1>();
      }

      // lower_bound<K>(goal)
      // Performs a binary search looking for 'goal' in sorted array 'K'.
      // If 'goal' is in sorted array 'K', returns the index of leftmost element
      // which equals 'goal'. Otherwise, returns the number of items which are
      // less than 'goal'. This behaviour is similar to std::lower_bound.
      // Complexity: O(logn).
      template <size_t K>
      typename std::enable_if<K == 0, size_t>::type
          lowerBound(const FT& goal) const {
        size_t left  = 0;
        size_t right = this->size_;
        while (left < right) {
          size_t middle = (left + right) / 2;
          if (data_[middle] < goal) left = middle + 1;
          else
            right = middle;
        }
        return left;
      }

      // lower_bound_row<K>(goal_row)
      // Performs a binary search looking for the Kth entry in 'goal_row' in
      // sorted array 'K'. This behaves like lower_bound, but letting the user
      // give an entire row of data instead of just the key.  Entries in the row
      // other than the Kth are ignored. Complexity: O(logn).
      template <size_t K>
      typename std::enable_if<K == 0, size_t>::type
          lowerBoundRow(const FT& goal, const RTs&... rest) const {
        size_t left  = 0;
        size_t right = this->size_;
        while (left < right) {
          size_t middle = (left + right) / 2;
          if (data_[middle] < goal) left = middle + 1;
          else
            right = middle;
        }
        return left;
      }

      // lower_bound<K>(goal)
      // Performs a binary search looking for 'goal' in sorted array 'K'.
      // If 'goal' is in the array, returns the index of leftmost element which
      // equals 'goal'. Otherwise, returns the number of items which are less
      // than 'goal'. This behaviour is similar to std::lower_bound. Complexity:
      // O(logn).
      template <size_t K>
      typename std::enable_if<K != 0, size_t>::type inline lowerBound(
          const typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::type& goal) {
        SoaBase<RTs...>& base = *this;
        return base.template lowerBound<K - 1>(goal);
      }
      // lower_bound_row<K>(goal_row)
      // Performs a binary search looking for the Kth entry in 'goal_row' in
      // sorted array 'K'. This behaves like lower_bound, but letting the user
      // give an entire row of data instead of just the key.  Entries in the row
      // other than the Kth are ignored. Complexity: O(logn).
      template <size_t K>
      typename std::enable_if<K != 0, size_t>::type inline lowerBoundRow(
          const FT& first, const RTs&... rest) {
        SoaBase<RTs...>& base = *this;
        return base.template lowerBound<K - 1>(rest...);
      }

      // upper_bound<K>(goal)
      // Performs a binary search looking for 'goal' in sorted array 'K'.
      // If 'goal' is in the array, returns the index of the leftmost element
      // which is greather than 'goal'. Otherwise, returns the number of items
      // which are less than 'goal'. This behaviour is similar to
      // std::upper_bound, and is useful if one array in a container is treated
      // as the key for a binary search table. Complexity: O(logn).
      template <size_t K>
      typename std::enable_if<K == 0, size_t>::type
          upperBound(const FT& goal) const {
        size_t left  = 0;
        size_t right = this->size_;
        while (left < right) {
          size_t middle = (left + right) / 2;
          if (goal < data_[middle]) right = middle;
          else
            left = middle + 1;
        }
        return left;
      }
      // upper_bound<K>(goal)
      // Performs a binary search looking for 'goal' in sorted array 'K'.
      // If 'goal' is in the array, returns the index of the leftmost element
      // which is greather than 'goal'. Otherwise, returns the number of items
      // which are less than 'goal'. This behaviour is similar to
      // std::upper_bound, and is useful if one array in a container is treated
      // as the key for a binary search table. Complexity: O(logn).
      template <size_t K>
      typename std::enable_if<K != 0, size_t>::type inline upperBound(
          const typename ElemTypeHolder<K, SoaBase<FT, RTs...>>::type& goal) {
        SoaBase<RTs...>& base = *this;
        return base.template upperBound<K - 1>(goal);
      }

      ///////////////////////////////////////////////////////////////////////////
      // "protected" methods.
      // Not actually protected; child classes need to ban them manually.
      // This is because of the wonky way in which child classes can't access
      // protected members in instances of their base class.
      ///////////////////////////////////////////////////////////////////////////

      // size_per_entry gives the total sizeof() of an entire row of data.
      inline constexpr size_t sizePerEntry() const {
        const SoaBase<RTs...>& base = *this;
        return sizeof(FT) + base.sizePerEntry();
      }

      // nullify sets the data pointer of every column to nullptr.
      inline void nullify() {
        SoaBase<RTs...>& base = *this;
        data_                 = nullptr;
        base.nullify();
      }

      // construct_range calls the default constructor on a range of entries.
      inline void constructRange(size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) { new (&data_[i]) FT(); }
        SoaBase<RTs...>& base = *this;
        base.constructRange(begin, end);
      }

      // construct_range calls the copy constructor on a range of entries.
      inline void constructRange(size_t begin, size_t end, const FT& initval,
                                 const RTs&... restvals) {
        for (size_t i = begin; i < end; ++i) { new (&data_[i]) FT(initval); }
        SoaBase<RTs...>& base = *this;
        base.constructRange(begin, end, restvals...);
      }

      // destruct_range calls the destructor on a range of entries.
      inline void destructRange(size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) { data_[i].~FT(); }
        SoaBase<RTs...>& base = *this;
        base.destructRange(begin, end);
      }

      // divy_buffer splits a big buffer of memory into a series of column
      // arrays. This also copies existing data into the new memory buffer.
      inline void divyBuffer(void* newmem) {
        if (data_) {
          memcpy(newmem, data_,
                 sizeof(FT) * std::min(this->size_, this->capacity_));
        }
        data_                 = (FT*)newmem;
        SoaBase<RTs...>& base = *this;
        base.divyBuffer(((FT*)newmem) + this->capacity_);
      }

      // push_back copies a row onto the back of the container.
      template <typename FirstType = FT, typename... RestTypes>
      typename std::enable_if<std::is_copy_constructible<FirstType>::value,
                              void>::type inline pushBack(const FirstType&
                                                              first,
                                                          RestTypes&&... rest) {
        new (&data_[this->size_]) FT(first);
        SoaBase<RTs...>& base = *this;
        base.pushBack(rest...);
      }

      // push_back moves a row onto the back of the container.
      template <typename FirstType = FT, typename... RestTypes>
      typename std::enable_if<std::is_move_constructible<FirstType>::value,
                              void>::type inline pushBack(FirstType&& first,
                                                          RestTypes&&... rest) {
        new (&data_[this->size_]) FT(std::move(first));
        SoaBase<RTs...>& base = *this;
        base.pushBack(rest...);
      }

      // emplace_back using a single argument to copy-construct the object.
      template <typename FirstType, typename... RestTypes>
      typename std::enable_if<
          std::is_copy_constructible<FirstType>::value,
          void>::type inline emplaceBack(const FirstType& first,
                                         RestTypes&&... rest) {
        new (&data_[this->size_]) FT(first);
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(rest...);
      }

      // emplace_back using a single argument to move-construct the object.
      template <typename FirstType, typename... RestTypes>
      typename std::enable_if<
          std::is_move_constructible<FirstType>::value,
          void>::type inline emplaceBack(FirstType&& first,
                                         RestTypes&&... rest) {
        new (&data_[this->size_]) FT(std::move(first));
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(rest...);
      }

      // emplace_back using no arguments to construct the object.
      template <typename... RestTypes>
      inline void emplaceBack(decltype(std::ignore), const RestTypes&... rest) {
        new (&data_[this->size_]) FT();
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(rest...);
      }

      // emplace_back using multiple arguments to construct the object.
      template <typename... FirstTypes, typename... RestTypes>
      inline void emplaceBack(const std::tuple<FirstTypes...>& first,
                              const RestTypes&... rest) {
        std::apply(
            [this](const FirstTypes&... args) {
              new (&data_[this->size_]) FT(args...);
            },
            first);
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(rest...);
      }

      // emplace_back using all default constructors.
      inline void emplaceBackDefault() {
        new (&data_[this->size_]) FT();
        SoaBase<RTs...>& base = *this;
        base.emplaceBackDefault();
      }

      // inserts (copies) a row at the specified index, moving later entries
      // back by one.
      template <typename FirstType = FT, typename... RestTypes>
      typename std::enable_if<std::is_copy_constructible<FirstType>::value,
                              void>::type inline insert(size_t location,
                                                        const FirstType& first,
                                                        RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT(first);
        SoaBase<RTs...>& base = *this;
        base.insert(location, rest...);
      }

      // inserts (moves) a row at the specified index, moving later entries back
      // by one.
      template <typename FirstType = FT, typename... RestTypes>
      typename std::enable_if<std::is_move_constructible<FirstType>::value,
                              void>::type inline insert(size_t      location,
                                                        FirstType&& first,
                                                        RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT(std::move(first));
        SoaBase<RTs...>& base = *this;
        base.insert(location, rest...);
      }

      // emplace using a single argument to copy-construct the object.
      template <typename FirstType, typename... RestTypes>
      typename std::enable_if<std::is_copy_constructible<FirstType>::value,
                              void>::type inline emplace(size_t location,
                                                         const FirstType& first,
                                                         RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT(first);
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(location, rest...);
      }

      // emplace using a single argument to move-construct the object.
      template <typename FirstType, typename... RestTypes>
      typename std::enable_if<std::is_move_constructible<FirstType>::value,
                              void>::type inline emplace(size_t      location,
                                                         FirstType&& first,
                                                         RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT(first);
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(location, rest...);
      }

      // emplace using no arguments to construct the object.
      template <typename... RestTypes>
      inline void emplace(size_t location, decltype(std::ignore),
                          RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT();
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(location, rest...);
      }

      // emplace using multiple arguments to construct the object.
      template <typename... FirstTypes, typename... RestTypes>
      inline void emplace(size_t                           location,
                          const std::tuple<FirstTypes...>& first,
                          RestTypes&&... rest) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        std::apply(
            [this, location](const FirstTypes&... args) {
              new (&data_[location]) FT(args...);
            },
            first);
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(location, rest...);
      }

      // emplace using all default constructors.
      inline void emplaceDefault(size_t location) {
        memmove(data_ + (location + 1), data_ + location,
                sizeof(FT) * (this->size_ - location));
        new (&data_[location]) FT();
        SoaBase<RTs...>& base = *this;
        base.emplaceBack(location);
      }

      // pop_back removes the last row in the container.
      inline void popBack() {
        data_[this->size_ - 1].~FT();
        SoaBase<RTs...>& base = *this;
        base.popBack();
      }

      // erase_swap swaps the given row with the back of the container, then
      // removes the last row.
      inline void eraseSwap(size_t location) {
        using std::swap;
        swap(data_[location], data_[this->size_ - 1]);
        data_[this->size_ - 1].~FT();
        SoaBase<RTs...>& base = *this;
        base.eraseSwap(location);
      }

      // erase_shift removes the given row, and move all further rows forward by
      // one.
      inline void eraseShift(size_t location) {
        data_[location].~FT();
        memmove(data_ + location, data_ + (location + 1),
                sizeof(FT) * (this->size_ - location));
        SoaBase<RTs...>& base = *this;
        base.eraseShift(location);
      }

      // swaps two containers.
      friend inline void swap(SoaBase<FT, RTs...>& lhs,
                              SoaBase<FT, RTs...>& rhs) {
        using std::swap;
        swap(lhs.data_, rhs.data_);
        SoaBase<RTs...>& lhsbase = lhs;
        SoaBase<RTs...>& rhsbase = rhs;
        swap(lhsbase, rhsbase);
      }

      // performs a deep copy.
      inline void copy(const SoaBase<FT, RTs...>& other) {
        memcpy(data_, other.data_, sizeof(FT) * other.size_);
        SoaBase<RTs...>&       lhs = *this;
        const SoaBase<RTs...>& rhs = other;
        lhs.copy(rhs);
      }

      // swaps the position of two indicated rows.
      inline void swapEntries(size_t first, size_t second) {
        using std::swap;
        swap(data_[first], data_[second]);
        SoaBase<RTs...>& base = *this;
        base.swapEntries(first, second);
      }

      // Creates a tuple of references representing a whole row.
      inline std::tuple<FT&, RTs&...> makeRowTuple(size_t row) {
        SoaBase<RTs...>& base = *this;
        return std::tuple_cat(
            std::make_tuple(std::reference_wrapper<FT>(data_[row])),
            base.makeRowTuple(row));
      }

      // Creates tuple of const references representing a whole row.
      inline std::tuple<const FT&, const RTs&...>
          makeRowTuple(size_t row) const {
        const SoaBase<RTs...>& base = *this;
        return std::tuple_cat(
            std::make_tuple(std::reference_wrapper<FT>(data_[row])),
            base.makeRowTuple(row));
      }

    protected:
      SoaBase() {}
      FT* data_ = nullptr;
    };

  }  // namespace impl

  template <typename... Ts>
  class Soa: public impl::SoaBase<Ts...> {
  public:
    // Soa()
    // Default constructor for a Struct-Of-Arrays object.
    // Initial size and capacity will be 0.
    // Complexity: O(1).
    Soa() {}
    // Soa(initsize)
    // Constructs a Struct-Of-Arrays object.
    // Initial size is set to the input,
    // and initial capacity will be enough to hold that many items.
    // Calls default constructors for each new item.
    // Complexity: O(n).
    Soa(size_t initSize) { resize(initSize); }
    // Soa(initsize, args...)
    // Constructs a Struct-Of-Arrays object.
    // Initial size is set to the input,
    // and initial capacity will be enough to hold that many items.
    // Calls copy-constructors for each new item using 'args...'.
    // Complexity: O(n).
    Soa(size_t initSize, const Ts&... initVals) {
      resize(initSize, initVals...);
    }
    // Soa({...})
    // Constructs a Struct-Of-Arrays using a list of tuples.
    // Copies the items from the initializer list into ourselves.
    // Complexity: O(n).
    Soa(const std::initializer_list<std::tuple<Ts...>>& initList) {
      reserve(initList.size());
      for (auto& entry : initList) {
        std::apply([this](const Ts&... args) { this->pushBack(args...); },
                   entry);
      }
    }
    // Soa(&& rhs)
    // Move constructor for Struct-Of-Arrays.
    // Moves the contents from the rhs struct-of-arrays into ourselves.
    // Complexity: O(1).
    Soa(Soa<Ts...>&& other) { swap(other); }
    // Soa(const& rhs)
    // Copy constructor for Struct-Of-Arrays.
    // Copies the contents from the rhs struct-of-arrays into ourselves.
    // Complexity: O(n).
    Soa(const Soa<Ts...>& other) {
      reserve(other.size());
      impl::SoaBase<Ts...>& base      = *this;
      impl::SoaBase<Ts...>& otherBase = other;
      base.copy(otherBase);
    }
    // operator = (&& rhs)
    // Move-assignment operator for Struct-Of-Arrays.
    // Moves the contents from the rhs struct-of-arrays into ourselves.
    // Complexity: O(1).
    Soa<Ts...>& operator=(Soa<Ts...>&& other) {
      swap(other);
      return *this;
    }
    // operator = (const& rhs)
    // Copy-assignment operator for Struct-Of-Arrays.
    // Copies the contents from the rhs struct-of-arrays into ourselves.
    // Complexity: O(n).
    Soa<Ts...>& operator=(Soa<Ts...> other) {
      swap(other);
      return *this;
    }
    // ~Soa()
    // Destructor for Struct-Of-Arrays.
    // Destructs all stored elements and frees held memory.
    // Complexity: O(n).
    ~Soa() {
      impl::SoaBase<Ts...>& base = *this;
      base.destructRange(0, this->size_);
      void* oldmem = this->template data<0>();
      if (oldmem) SoaAlignedFree(oldmem);
    }

    // swap(& rhs)
    // Swaps the contents of this container with the other container.
    // Complexity: O(1).
    friend inline void swap(Soa<Ts...>& lhs, Soa<Ts...>& rhs) {
      impl::SoaBase<Ts...>& lhsBase = lhs;
      impl::SoaBase<Ts...>& rhsBase = rhs;
      swap(lhsBase, rhsBase);
    }

    // clear()
    // Clears and destructs all held items.
    // Does not change capacity.
    // Complexity: O(n).
    inline void clear() {
      impl::SoaBase<Ts...>& base = *this;
      base.destructRange(0, this->size_);
      this->size_ = 0;
    }

    // reserve(n)
    // Ensures that the container has enough space to hold n items.
    // If n is greater than the current capacity, this triggers a memory
    // re-allocation. Returns false if a memory allocation error occurs, or true
    // otherwise. Complexity: O(n).
    bool reserve(size_t newSize) {
      // For alignment, we must have a multiple of 16 items.
      if (newSize % 16 != 0) newSize += 16 - (newSize % 16);

      // We need at least 16 elements.
      if (newSize == 0) newSize = 16;

      // We can't shrink the actual memory.
      if (newSize <= this->capacity_) return true;

      // Remember the old memory so we can free it.
      impl::SoaBase<Ts...>& base   = *this;
      void*                 oldmem = this->template data<0>();

      // Allocate new memory.
      void* alloc_result = SoaAlignedMalloc(16, base.sizePerEntry() * newSize);
      if (!alloc_result) return false;

      // Copy the old data into the new memory.
      this->capacity_ = newSize;
      base.divyBuffer(alloc_result);

      // Free the old memory.
      if (oldmem) SoaAlignedFree(oldmem);
      return true;
    }

    // shrinkToFit()
    // Shrinks the capacity to the smallest amount that can hold all
    // currently-held items. If the capacity is reduced, this triggers a memory
    // re-allocation. Returns false is a memory allocation error occurs, true
    // otherwise. Complexity: O(n).
    bool shrinkToFit() {
      // For alignment, we must have a multiple of 16 items.
      size_t newSize = this->size_;
      if (newSize % 16 != 0) newSize += 16 - (newSize % 16);

      // If the container is already as small as it can be, bail out now.
      if (newSize == this->capacity_) return true;

      // Remember the old memory so we can free it.
      impl::SoaBase<Ts...>& base   = *this;
      void*                 oldmem = this->template data<0>();

      if (newSize > 0) {
        // Allocate new memory.
        void* alloc_result =
            SoaAlignedMalloc(16, base.sizePerEntry() * newSize);
        if (!alloc_result) return false;

        // Copy the old data into the new memory.
        this->capacity_ = newSize;
        base.divyBuffer(alloc_result);
      } else {
        base.nullify();
        this->capacity_ = 0;
      }

      // Free the old memory.
      if (oldmem) SoaAlignedFree(oldmem);
      return true;
    }

    // resize(n)
    // Resizes the container to contain exactly n items.
    // If n is greater than the current capacity, reserve(n) is called.
    // If n is greater than the current number of items, the new items are
    // default-constructed. If n is less than the current number of items, the
    // lost items are destructed. Returns false if a memory allocation failure
    // occurs in reserve, true otherwise. Complexity: O(n).
    inline bool resize(size_t newsize) {
      impl::SoaBase<Ts...>& base = *this;
      if (newsize > this->size_) {
        if (newsize > this->capacity_) {
          if (!reserve(newsize)) return false;
        }
        base.constructRange(this->size_, newsize);
        this->size_ = newsize;
      } else if (newsize < this->size_) {
        base.destructRange(newsize, this->size_);
        this->size_ = newsize;
      }
      return true;
    }

    // resize(n, args...)
    // Resizes the container to contain exactly n items.
    // If n is greater than the current capacity, reserve(n) is called.
    // If n is greater than the current number of items, the new items are
    // copy-constructed using 'args...'. If n is less than the current number of
    // items, the lost items are destructed. Returns false if a memory
    // allocation failure occurs in reserve, true otherwise. Complexity: O(n).
    inline bool resize(size_t newsize, const Ts&... initvals) {
      impl::SoaBase<Ts...>& base = *this;
      if (newsize > this->size_) {
        if (newsize > this->capacity_) {
          if (!reserve(newsize)) return false;
        }
        base.constructRange(this->size_, newsize, initvals...);
      } else if (newsize < this->size_) {
        base.destructRange(newsize, this->size_);
        this->size_ = newsize;
      }
      return true;
    }

    // pushBack(args...)
    // Increases the size of the container by 1 and places the given args at the
    // back of each array. If we're out of space, reserve is called to expand
    // the size of the buffer. Returns false if a memory allocation occurs in
    // reserve, true otherwise. Complexity: O(1) unless reserve is called, then
    // O(n).
    template <typename... EntryTypes>
    inline bool pushBack(EntryTypes&&... args) {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      impl::SoaBase<Ts...>& base = *this;
      base.pushBack(args...);
      ++this->size_;
      return true;
    }

    // exmplaceBack(args...)
    // Increases the size of the container by 1 and constructs the new items at
    // the back of each array using 'args...'. If we're out of space, reserve is
    // called to expand the size of the buffer. Returns false if a memory
    // allocation occurs in reserve, true otherwise. You can pass std::ignore as
    // an argument to default-construct the corresponding element, or an
    // std::tuple to initialize the corresponding element with multiple
    // arguments. Complexity: O(1) unless reserve is called, then O(n).
    template <typename... CTypes>
    inline bool emplaceBack(CTypes&&... args) {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      impl::SoaBase<Ts...>& base = *this;
      base.emplaceBack(args...);
      ++this->size_;
      return true;
    }

    // exmplaceBack()
    // Increases the size of the container by 1 and constructs the new items at
    // the back of each array using default constructors. If we're out of space,
    // reserve is called to expand the size of the buffer. Returns false if a
    // memory allocation occurs in reserve, true otherwise. Complexity: O(1)
    // unless reserve is called, then O(n).
    inline bool emplaceBack() {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      impl::SoaBase<Ts...>& base = *this;
      base.emplaceBackDefault();
      ++this->size_;
      return true;
    }

    // insert(where, args...)
    // Increases the size of the container by 1 and inserts 'args...' into the
    // arrays at location 'where'. If we're out of space, reserve is called to
    // expand the size of the buffer. Returns false if a memory allocation error
    // occurs in reserve or if 'where' is out of bounds, true otherwise.
    // Complexity: O(n).
    template <typename... Args>
    inline bool insert(size_t where, Args&&... args) {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      if (where > this->size_) return false;
      impl::SoaBase<Ts...>& base = *this;
      base.insert(where, args...);
      ++this->size_;
      return true;
    }

    // emplace(where, args...)
    // Increases the size of the container by 1 and inserts newly-constructed
    // objects using 'args...' into the arrays at location 'where'. If we're out
    // of space, reserve is called to expand the size of the buffer. Returns
    // false if a memory allocation error occurs in reserve or if 'where' is out
    // of bounds, true otherwise. You can pass std::ignore as an argument to
    // default-construct the corresponding element, or an std::tuple to
    // initialize the corresponding element with multiple arguments. Complexity:
    // O(n).
    template <typename... CTypes>
    inline bool emplace(size_t where, CTypes&&... args) {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      if (where > this->size_) return false;
      impl::SoaBase<Ts...>& base = *this;
      base.emplace(where, args...);
      ++this->size_;
      return true;
    }

    // emplace(where)
    // Increases the size of the container by 1 and inserts newly-constructed
    // objects using default constructors into the arrays at location 'where'.
    // If we're out of space, reserve is called to expand the size of the
    // buffer. Returns false if a memory allocation error occurs in reserve or
    // if 'where' is out of bounds, true otherwise. Complexity: O(n).
    inline bool emplace(size_t where) {
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2)) return false;
      }
      if (where > this->size_) return false;
      impl::SoaBase<Ts...>& base = *this;
      base.emplaceDefault(where);
      ++this->size_;
      return true;
    }

    // popBack()
    // Reduces the size of the container by 1 and destructs the items at the
    // back of the arrays. Complexity: O(1).
    inline void popBack() {
      if (this->size_ == 0) return;
      impl::SoaBase<Ts...>& base = *this;
      base.popBack();
      --this->size_;
    }

    // eraseSwap(where)
    // Swaps the items at 'where' in each array with the back of the container,
    // then destructs the rear of the container and recuces the size by 1.
    // Complexity: O(1).
    inline void eraseSwap(size_t where) {
      if (where >= this->size_) return;
      impl::SoaBase<Ts...>& base = *this;
      base.eraseSwap(where);
      --this->size_;
    }

    // eraseShift(where)
    // Destructs the item in each array at 'where', then moves each item after
    // it 1 space forward. Maintains the ordering of a sorted container.
    // Complexity: O(n).
    inline void eraseShift(size_t where) {
      if (where >= this->size_) return;
      impl::SoaBase<Ts...>& base = *this;
      base.eraseShift(where);
      --this->size_;
    }

    // swapEntries(first, second)
    // Swaps the 'first' and 'second' entries in each array.
    // Complexity: O(1).
    inline void swapEntries(size_t first, size_t second) {
      if (first >= this->size_ || second >= this->size_) return;
      impl::SoaBase<Ts...>& base = *this;
      base.swapEntries(first, second);
    }

    // empty()
    // Returns true if the container is empty, false otherwise.
    inline bool empty() const { return (this->size_ == 0); }
    // size()
    // Returns the number of items in the container.
    inline size_t size() const { return this->size_; }
    // maxSize()
    // Returns the maximum number of items that this container could
    // theoretically hold. Does not account for running out of memory.
    inline constexpr size_t maxSize() const { return SIZE_MAX; }
    // capacity()
    // Returns the number of items that this container could hold before needing
    // to reserve additional memory.
    inline size_t capacity() const { return this->capacity_; }

    // serialize()
    // Shrinks the container to the smallest capacity that can contain its
    // entries, then returns a pointer to the raw data buffer that stores the
    // container's data. num_bytes is filled with the number of bytes in that
    // buffer. This function should be used in tandem with 'deserialize' to save
    // and load a container to disk.
    void* serialize(size_t& num_bytes) {
      shrinkToFit();
      num_bytes = this->sizePerEntry() * this->capacity_;
      return this->template data<0>();
    }

    // deserialize(n)
    // Reserves just enough space for n elements,
    // then returns a pointer to the place in memory where the container's data
    // should be copied/read into. Immediately after calling deserialize, the
    // user MUST fill the buffer with EXACTLY the data returned from serialize!
    // num_bytes is filled with the number of bytes in that buffer.
    // This function should be used in tandem with 'serialize' to save and load
    // a container to disk.
    void* deserialize(size_t num_elements, size_t& num_bytes) {
      reserve(num_elements);
      num_bytes   = this->sizePerEntry() * this->capacity_;
      this->size_ = num_elements;
      return this->template data<0>();
    }

    // sort<K>()
    // Sorts the entries in the table according to the Kth array.
    // Returns the exact number of swaps performed while sorting the data.
    // Complexity: O(nlogn).
    template <size_t K>
    size_t sort() {
      return quicksort(this->template data<K>(), 0, this->size_ - 1);
    }

    // Iterators
    // These iterators allow you to iterate over each row of the container.
    // As this is as struct-of-arrays and not an array-of-structs, you should
    // generally not do this. It can be useful for debugging I guess.

    struct Iterator {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = std::tuple<Ts&...>;

      Iterator(Soa& container, size_t row): container_(container), row_(row) {}

      Iterator& operator*() { return *this; }
      Iterator* operator->() { return this; }

      Iterator& operator++() {
        row_++;
        return *this;
      }
      Iterator operator++(int) {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      friend bool operator==(const Iterator& a, const Iterator& b) {
        return a.row_ == b.row_;
      }
      friend bool operator!=(const Iterator& a, const Iterator& b) {
        return a.row_ != b.row_;
      }

      /// Gets the row index that this iterator points to.
      size_t index() const { return row_; }
      /// Gets a tuple containing the row that this iterator points to.
      value_type row() const { return container_.makeRowTuple(row_); }
      /// Gets the item at the Kth column of the row this iterator points to.
      template <size_t K>
      auto& get() {
        return container_.template at<K>(row_);
      }

    private:
      Soa&   container_;
      size_t row_;
    };

    Iterator begin() { return Iterator(*this, 0); }
    Iterator end() { return Iterator(*this, this->size_); }

    struct ConstIterator {
      using iterator_category = std::forward_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = std::tuple<const Ts&...>;

      ConstIterator(const Soa& container, size_t row):
          container_(container), row_(row) {}

      const ConstIterator& operator*() const { return *this; }
      const ConstIterator* operator->() const { return this; }

      ConstIterator& operator++() {
        row_++;
        return *this;
      }
      ConstIterator operator++(int) {
        ConstIterator tmp = *this;
        ++(*this);
        return tmp;
      }

      friend bool operator==(const ConstIterator& a, const ConstIterator& b) {
        return a.row_ == b.row_;
      }
      friend bool operator!=(const ConstIterator& a, const ConstIterator& b) {
        return a.row_ != b.row_;
      }

      /// Gets the row index that this iterator points to.
      size_t index() const { return row_; }
      /// Gets a tuple containing the row that this iterator points to.
      value_type row() const { return container_.makeRowTuple(row_); }
      /// Gets the item at the Kth column of the row this iterator points to.
      template <size_t K>
      const auto& get() const {
        return container_.template at<K>(row_);
      }

    private:
      const Soa& container_;
      size_t     row_;
    };

    ConstIterator begin() const { return ConstIterator(*this, 0); }
    ConstIterator end() const { return ConstIterator(*this, this->size_); }

  protected:
    // Ban access to certain parent methods.
    using impl::SoaBase<Ts...>::nullify;
    using impl::SoaBase<Ts...>::divyBuffer;
    using impl::SoaBase<Ts...>::constructRange;
    using impl::SoaBase<Ts...>::destructRange;
    using impl::SoaBase<Ts...>::copy;
    using impl::SoaBase<Ts...>::emplaceBackDefault;
    using impl::SoaBase<Ts...>::emplaceDefault;

    template <typename T>
    size_t quicksort(T* arr, size_t low, size_t high) {
      size_t numSwaps = 0;
      // Create a stack and initialize the top.
      std::vector<size_t> stack((high - low) + 2);
      // size_t stack[(high - low) + 1];
      int top = -1;
      // Push initial values of low and high to the stack.
      stack[++top] = low;
      stack[++top] = high;
      // Keep popping from the stack while it's not empty.
      while (top >= 0) {
        // pop h and l
        size_t h = stack[top--];
        size_t l = stack[top--];
        // Set pivot element at its correct position.
        size_t p = partition(arr, l, h, numSwaps);
        // If there are elements on the left side, push left to the stack.
        if (p > l + 1) {
          stack[++top] = l;
          stack[++top] = p - 1;
        }
        // If there are elements on the right side, push right to the stack.
        if (p + 1 < h) {
          stack[++top] = p + 1;
          stack[++top] = h;
        }
      }
      return numSwaps;
    }
    template <typename T>
    size_t partition(T* arr, size_t low, size_t high, size_t& numSwaps) {
      T*     pivot = &arr[high];
      size_t i     = low;
      for (size_t j = low; j < high; ++j) {
        if (arr[j] < *pivot) {
          swapEntries(i, j);
          ++numSwaps;
          ++i;
        }
      }
      swapEntries(i, high);
      ++numSwaps;
      return i;
    }
  };

}  // namespace hvh

#endif  // HVH_TOOLKIT_STRUCTOFARRAYS_H