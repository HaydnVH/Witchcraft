#ifndef WC_ETC_SOA_H
#define WC_ETC_SOA_H

#include <algorithm>
#include <concepts>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace impl {

	// We need access to aligned allocations and deallocations,
	// but visual studio doesn't support aligned_alloc from the c11 standard.
	// This lets us have consistent behaviour.
	#ifdef _MSC_VER
  inline void* alignedMalloc(size_t alignment, size_t size) { return _aligned_malloc(size, alignment); }
  inline void  alignedFree(void* mem) { _aligned_free(mem); }
	#else
	inline void* alignedMalloc(size_t alignment, size_t size) { return aligned_alloc(alignment, size); }
  inline void  alignedFree(void* mem) { free(mem); }
	#endif

	// Gets the greatest power of 2 which is a divisor of val.
	// This is a very inefficient algorithm, so being `consteval` makes it unusable except for constexpr stuff.
	consteval size_t greatestPo2Divisor(size_t val) {
    for (size_t divisor {1ui64 << 32}; divisor > 0; divisor /= 2) {
      if (val % divisor == 0) return divisor;
		}
		return 0;
	}

	// Returns "val" rounded up to the nearest multiple of "round".
	template <typename T> constexpr T roundUpTo(T val, T round) requires (std::integral<T>) {
		return val + ((val%round) ? round-(val%round) : 0);
	}

	// Increases 'val' by 'adv', then returns the original 'val'.
	// This is to "val++" what "val+=adv" is to "++val".
	template <typename T> constexpr T postAdv(T* val, size_t adv) {
    T original {*val};
    // operator += is undefined for void*.
		// We assume that the intent is to advance a number of bytes, so we cast it to int8_t*.
		if constexpr (std::is_same_v<void*, T>) { (*(int8_t**)val) += adv; }
		else { val += adv; }
		return original;
	}

	// Destruct a range of objects in a given chunk of memory.
	template <class T> constexpr void soaDestructRange(T* data, size_t count) {
		for (size_t i{0}; i < count; ++i) { data[i].~T(); }
	}

	// Copies a range of objects from one chunk of memory to another.
	// Assumes the objects in dstData have already been constructed.
	template <class T> constexpr void soaCopyRange(T* dstData, T* srcData, size_t count) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      memcpy(dstData, srcData, sizeof(T)*count);
    } else {
     for (size_t i {0}; i < count; ++i) { dstData[i] = srcData[i]; }
		}
	}

	template <std::ranges::range R> constexpr void soaCopyRange(R&& dstRange, const R& srcRange) {
    for (auto [dstItem, srcItem] : std::ranges::zip_view(std::forward<R>(dstRange), srcRange)) {
			dstItem = srcItem;
		}
	}

	// Copies a range of objects from one chunk of memory to another.
	// Assumes the objects in dstData have not been constructed.
	template <class T> constexpr void soaCopyRangeConstruct(T* dstData, T* srcData, size_t count) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      memcpy(dstData, srcData, sizeof(T)*count);
    } else {
			for (size_t i {0}; i < count; ++i) { new (&dstData[i]) T(srcData[i]); }
		}
	}

} // namespace impl

namespace wc {
namespace etc {

template <class... ColTs>
class Soa {
protected:
  void*                 data_ {nullptr};
  size_t                size_ {0}, capacity_ {0};
  std::tuple<ColTs*...> columns_ {};

	using ThisT = Soa<ColTs...>;
	
	static constexpr size_t numColumns_c {sizeof...(ColTs)};
	static_assert(numColumns_c > 0, "Soa must have at least one column.");
	
	// The size, in bytes, of an entire row.
	static constexpr size_t rowSize_c {(sizeof(ColTs) + ...)};

	// Certain functions require an index sequence to iterate over each column.
	// This constant is a simple helper for creating said sequence.
	static constexpr auto seq_c {std::make_index_sequence<numColumns_c>{}};

	// Based on the size of the stored types, this returns the number of entries allocated to maintain alignment.
	// This value must be as small as possible to avoid allocating too much memory for small containers,
	// but must be large enough that immediately subsequent arrays can maintain alignment.
	static consteval size_t getAlignmentMultiplier(size_t alignment) {
		size_t divisor = std::min<size_t>({impl::greatestPo2Divisor(sizeof(ColTs))...});
    if (divisor >= alignment) return 1;
    else return (alignment / divisor);
	}

	static constexpr size_t align_c { getAlignmentMultiplier(16) };
	
public:
	template <int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;
	template <int N> using NthType = NthTypeOf<N, ColTs...>;

	template <size_t N> static constexpr size_t columnSize() { return sizeof(NthType<N>); }
  static constexpr size_t rowSize() { return rowSize_c; }

	Soa() = default;  // Default constructor.
	~Soa() { soaRealloc(0); } // Destructor.
	Soa(const ThisT& rhs) { copy(rhs, seq_c); }  // Copy constructor.
  Soa(ThisT&& rhs) { swap(*this, rhs); } // Move constructor.
	Soa(std::initializer_list<std::tuple<ColTs...>>&& ilist) { // Initializer list constructor.
    reserve(ilist.size());
    for (auto& entry : ilist) {
      std::apply([this](const ColTs&... row) { this->pushBack(row...);}, entry);
    }
  }
	template <std::ranges::range R>
	Soa(R&& range) { // Initialize using a range.
		// If this range has a knowable size, we can reserve memory to make our lives easier.
		if constexpr (std::ranges::sized_range<R> || std::ranges::forward_range<R>) {
      reserve(range.size());
		}
		// If this is a range of tuples....
    if constexpr (std::is_same_v<decltype(*range.begin()), std::tuple<ColTs&...>>) {
      for (const auto& entry : range) {
        std::apply([this](const ColTs&... row) { this->pushBack(row...);}, entry);
			}
		} else {
			// We're constructing with a range of one item, so we'd better only have one column.
      static_assert(sizeof...(ColTs) == 1);
			// The range's type had better be compatible with our own.
      static_assert(std::is_nothrow_convertible_v<std::ranges::range_value_t<R>, NthType<0>>);
			// Finally copy the range's data into ourselves.
      for (auto& item : range) { pushBack(item); }
		}
	}

	ThisT& operator=(ThisT rhs)   { swap(*this, rhs); return *this; } // Copy assignment operator.
  ThisT& operator=(ThisT&& rhs) { swap(*this, rhs); return *this; } // Move assignment operator.

	// Swaps the contents of two Soa's.
	friend void swap(ThisT& lhs, ThisT& rhs) {
    std::swap(lhs.data_,     rhs.data_);
		std::swap(lhs.size_,     rhs.size_);
		std::swap(lhs.capacity_, rhs.capacity_);
		std::swap(lhs.columns_,  rhs.columns_);
	}

	// Provides access to the underlying array for the indicated column.
	template <size_t N> auto const& data()       { return std::get<N>(columns_); }
	template <size_t N> auto const& data() const { return std::get<N>(columns_); }
	template <class T>  auto const& data()       { return std::get<T*>(columns_); }
	template <class T>  auto const& data() const { return std::get<T*>(columns_); }
	
	// Access the field at the indicated column and row.  Checks bounds.
	template <size_t N> auto& at(size_t index)       { boundsCheck(index); return (data<N>()[index]); }
	template <size_t N> auto& at(size_t index) const { boundsCheck(index); return (data<N>()[index]); }
	template <class T>  auto& at(size_t index)       { boundsCheck(index); return (data<T>()[index]); }
	template <class T>  auto& at(size_t index) const { boundsCheck(index); return (data<T>()[index]); }

	// Access the field at the first row of the indicated column.  Checks bounds.
	template <size_t N> auto& front()       { return at<N>(0); }
	template <size_t N> auto& front() const { return at<N>(0); }
	template <class T>  auto& front()       { return at<T>(0); }
	template <class T>  auto& front() const { return at<T>(0); }

	// Access the field at the last row of the indicated column.  Checks bounds.
	template <size_t N> auto& back()       { return at<N>(size_-1); }
	template <size_t N> auto& back() const { return at<N>(size_-1); }
	template <class T>  auto& back()       { return at<T>(size_-1); }
	template <class T>  auto& back() const { return at<T>(size_-1); }
	
	// Provides a view for a single column which can be iterated over.
	template <size_t N> auto viewColumn()       { auto* ptr{data<N>()}; return std::ranges::subrange(ptr, ptr+size_); }
	template <size_t N> auto viewColumn() const { auto* ptr{data<N>()}; return std::ranges::subrange(ptr, ptr+size_); }
	template <class T>  auto viewColumn()       { auto* ptr{data<T>()}; return std::ranges::subrange(ptr, ptr+size_); }
	template <class T>  auto viewColumn() const { auto* ptr{data<T>()}; return std::ranges::subrange(ptr, ptr+size_); }
	
	// Provides a view for multiple columns at once.
	template <size_t... Ns> auto viewColumns()       { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	template <size_t... Ns> auto viewColumns() const { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	template <class... Ts>  auto viewColumns()       { return std::ranges::zip_view(viewColumn<Ts>() ...); }
	template <class... Ts>  auto viewColumns() const { return std::ranges::zip_view(viewColumn<Ts>() ...); }
	
	// viewColumns, but you can use an index_sequence instead of `<a,b,c>`.  Neccesary for functions that want an entire row.
	template <size_t... Ns> auto viewColumns(std::index_sequence<Ns...>)       { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	template <size_t... Ns> auto viewColumns(std::index_sequence<Ns...>) const { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	
	// Iterate over the container.  This is equivalent to 'viewColumns' with every column.
	// If there's only 1 column, it uses the view of just that column.
	auto begin()       { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().begin(); else return viewColumns(seq_c).begin(); }
	auto begin() const { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().begin(); else return viewColumns(seq_c).begin(); }
	auto end()         { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().end();   else return viewColumns(seq_c).end(); }
	auto end()   const { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().end();   else return viewColumns(seq_c).end(); }
	
	// Access to the given row.
	auto operator[](size_t index)       { boundsCheck(index); return begin()+index; }
	auto operator[](size_t index) const { boundsCheck(index); return begin()+index; }
	
	bool   empty()    const { return (size_ == 0); }
	size_t size()     const { return size_; }
	size_t maxSize()  const { return SIZE_MAX; }
	size_t capacity() const { return capacity_; }
	
	void reserve(size_t newCapacity) { if (newCapacity > capacity_) soaRealloc(newCapacity); }
	void shrinkToFit()               { soaRealloc(size_); }

  void resize(size_t newSize) {
		if (newSize == size_) return;
		reserve(newSize);
		if (newSize > size_) {
      std::apply([&](ColTs*... cols) {
				((new (cols+size_) ColTs[newSize-size_] ()), ...);
			}, columns_);
		}
		else if (newSize < size_) {
      std::apply([&](ColTs*... cols) {
				(impl::soaDestructRange(cols+newSize, size_-newSize), ...);
			}, columns_);
		}
		size_ = newSize;
	}
  void clear() { resize(0); }

	void insert(size_t index, const ColTs&... row) {
		if (index > size_) throw std::out_of_range("Index out of bounds.");
		expandIfNeeded();
    std::apply([&](ColTs*... cols) {
	    (memmove(cols+index+1, cols+index, sizeof(ColTs)*(size_-index)), ...);
			((new (cols+index) ColTs(row)), ...);
		}, columns_);
		++size_;
	}

	void pushBack(const ColTs&... row) {
		expandIfNeeded();
    std::apply([&](ColTs*... cols) {
      ((new (cols + size_) ColTs(row)), ...);
	  }, columns_);
	  ++size_;
	}

	void popBack() {
		if (size_ == 0) return;
    std::apply([&](ColTs*... cols) {
			(impl::soaDestructRange(cols+size_-1, 1), ...);
		}, columns_);
		--size_;
	}
	
	void swapEntries(size_t first, size_t second) { 
		boundsCheck(first); boundsCheck(second);
    std::apply([&](ColTs*... cols) {
			((std::swap(cols[first], cols[second])), ...);
		}, columns_);
	}
  
  void eraseShift(size_t index) {
		boundsCheck(index);
    std::apply([&](ColTs*... cols) {
      (impl::soaDestructRange(cols+index, 1), ...);
			(memmove(cols+index, cols+index+1, sizeof(ColTs)*(size_-(index+1))), ...);
		}, columns_);
		--size_;
	}

  void eraseSwap(size_t index) { swapEntries(index, size_-1); popBack(); }
  void erase(size_t index) { eraseShift(index); }

	template <size_t N> auto lowerBound(const NthType<N>& goal) {
    size_t left {0}; size_t right {size_};
    while (left < right) {
			size_t middle {(left+right)/2};
			if (at<N>(middle) < goal) left = middle+1;
			else right = middle;
		} return left;
	}

	template <size_t N> auto upperBound(const NthType<N>& goal) {
    size_t left {0}; size_t right {size_};
    while (left < right) {
			size_t middle {(left+right)/2};
			if (goal < at<N>(middle)) right = middle;
			else left = middle+1;
		} return left;
	}

	// TODO: Implement sorting.
	//void sort() {
	//
	//}

	// serialize()
  // Shrinks the container to the smallest capacity that can contain its entries,
  // then returns a tuple containing the information needed to rebuild the container.
  // The tuple elements are [numRows, numBytes, dataPtr].
  std::tuple<size_t, size_t, void*> serialize() requires (std::is_trivially_copyable_v<ColTs> && ...) {
    shrinkToFit();
    return std::make_tuple(
      this->size_,
      (rowSize_c * this->capacity_),
      this->data_);
  }

  // deserialize(t)
  // Uses information returned by serialize to reconstruct the table.
  // Data pointed to by dataPtr is copied into the container.
  void deserialize(size_t numRows, size_t numBytes, void* dataPtr) requires (std::is_trivially_copyable_v<ColTs> && ...) {
    clear(); soaRealloc(numRows); this->size_ = numRows;
    memcpy(this->data_, dataPtr, numBytes);
  }
  // deserialize(t)
  // Takes a tuple (as returned by serialize) and uses it to reconstruct the
  // container. The tuple elements are [containerSize, numBytes, dataPtr]. The
  // data pointed to by the data pointer is copied into the container.
  void deserialize(std::tuple<size_t, size_t, void*> arg) requires (std::is_trivially_copyable_v<ColTs> && ...) {
    auto& [numRows, numBytes, dataPtr] = arg;
    deserialize(numRows, numBytes, dataPtr);
  }

	// Creates a new Soa which is a copy of this but with a vector added as a new column.
	template <typename T> inline auto addColumn(const std::vector<T>& col) { return implAddColumn(col, seq_c); }

	// Create a new Soa which is a column-wise merge of two Soas.
	template <typename... Ts> inline auto mergeColumns(const Soa<Ts...>& other) { return implMergeColumns(other, seq_c, other.seq_c); }

	// Returns a new Soa which contains a copy of the selected column.
	template <size_t N> auto extractColumn() {
		Soa<NthType<N>> result; result.reserve(size_);
    impl::soaCopyRangeConstruct(std::get<N>(result.columns_), std::get<N>(columns_), size_);
		result.size_ = size_;
		return result;
	}

private:
	template <typename T, size_t... Ns> auto implAddColumn(const std::vector<T>& col, std::index_sequence<Ns...>) {
    if (size_ != col.size()) throw std::logic_error("Soa::mergeColumn: Containers must be the same size.");
		Soa<ColTs..., T> result; result.reserve(size_);
    (impl::soaCopyRangeConstruct(std::get<Ns>(result.columns_), std::get<Ns>(columns_), size_), ...);
    impl::soaCopyRangeConstruct(std::get<sizeof...(Ns)>(result.columns_), col.data(), size_);
		result.size_ = size_;
		return result;
	}

	template <typename... Ts, size_t... LNs, size_t... RNs> auto implMergeColumns(const Soa<Ts...>& other, std::index_sequence<LNs...>, std::index_sequence<RNs...>) {
    if (size_ != other.size()) throw std::logic_error("Soa::mergeColumns: Containers must be the same size.");
		Soa<ColTs..., Ts...> result; result.reserve(size_);
    (impl::soaCopyRangeConstruct(std::get<LNs>(result.columns_), std::get<LNs>(columns_), size_), ...);
    (impl::soaCopyRangeConstruct(std::get<sizeof...(LNs)+RNs>(result.columns_), std::get<RNs>(other.columns_), size_), ...);
		result.size_ = size_;
		return result;
	}

	template <std::ranges::range... Rs, size_t... Ns>
	void implSeqCopyRangeConstruct(const std::tuple<Rs...>& ranges, std::index_sequence<Ns...>) {
    static_assert(sizeof...(ColTs) == sizeof...(Rs));
    resize(std::min({std::get<Ns>(ranges).size()...}));
    (impl::soaCopyRange(viewColumn<Ns>(), std::get<Ns>(ranges)), ...);
	}
	
protected:
	// Reallocates memory to just enough to fit newCapacity entries.
	void* soaRealloc(size_t newCapacity, size_t extraBytes = 0) {
		// If there's nothing to do, bail out.
		if (newCapacity == capacity_) return nullptr;

		// For alignment purposes, we only allocate a multiple of N bytes.
		// We enforce a multiple of N elements instead of adding padding.
    newCapacity = impl::roundUpTo(newCapacity, align_c);
	
		// If we are allocating memory...
		if (newCapacity) {
			void* newData = impl::alignedMalloc(16, (newCapacity * rowSize_c) + extraBytes);
			// Figure out how many rows we'll need to move.
			size_t rowsToMove = std::min(size_, newCapacity);
			if (rowsToMove) {
				void* dataCursor{newData};
				std::apply([&](ColTs*... cols) {
					// Move the data into into its new position.
					(memmove(impl::postAdv(&dataCursor, sizeof(ColTs)*newCapacity), cols, sizeof(ColTs)*rowsToMove), ...);
					// Clean up any leftover objects.
					if (newCapacity < size_) {
						(impl::soaDestructRange(cols+rowsToMove, size_-rowsToMove), ...);
					}
				}, columns_);
				impl::alignedFree(data_);
			}
			data_ = newData;
			capacity_ = newCapacity;
			size_ = rowsToMove;
      std::apply([&](ColTs*&... cols) {
	      ((cols = static_cast<ColTs*>(impl::postAdv(&newData, sizeof(ColTs)*newCapacity))), ...);
      }, columns_);
			return extraBytes ? newData : nullptr;
		}
		// New capacity is zero, but we have old data to clean up.
		else if (data_) {
      std::apply([&](ColTs*... cols) {
				(impl::soaDestructRange(cols, size_), ...);
			}, columns_);
			impl::alignedFree(data_);
			data_ = nullptr;
			capacity_ = 0;
			size_ = 0;
			columns_ = {};
			return nullptr;
		}
		return nullptr;
	}

	// Copies the contents of another Soa into this.  Overwrites the current contents.
  template <size_t... Ns> void copy(ThisT& rhs, std::index_sequence<Ns...>) {
		resize(rhs.size());
		(impl::soaCopyRange(std::get<Ns>(columns_), std::get<Ns>(rhs.columns_), rhs.size()), ...);
	}

	// Checks to see if the given index is out-of-bounds, and throws an exception if so.
	inline void boundsCheck(size_t index) const { if (index >= size_) throw std::out_of_range("Index out of bounds."); }

	// Checks to see if there's room for just one more, and if not, expands as needed.
	inline void expandIfNeeded() { if (size_ == capacity_) reserve(std::max<size_t>(size_*2, align_c)); }
};

namespace test {
	int SoaUnitTest();
} // namespace test

}} // namespace wc::etc
#endif // WC_ETC_SOA_H