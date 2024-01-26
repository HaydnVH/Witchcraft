#ifndef WC_ETC_SOA_H
#define WC_ETC_SOA_H

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace impl {

	// We need access to aligned allocations and deallocations,
	// but visual studio doesn't support aligned_alloc from the c11 standard.
	// This define lets us have consistent behaviour.
	#ifdef _MSC_VER
  inline void* soaAlignedMalloc(size_t alignment, size_t size) { return _aligned_malloc(size, alignment); }
  inline void  soaAlignedFree(void* mem) { _aligned_free(mem); }
	#else
	inline void* soaAlignedMalloc(size_t alignment, size_t size) { return aligned_alloc(alignment, size); }
  inline void  soaAlignedFree(void* mem) { free(mem); }
	#endif

	// Increases 'val' by 'adv', then returns the original 'val'.
	// This is to "val++" what "val+=adv" is to "++val".
	template <class T> constexpr T soaPostAdv(T* val, size_t adv) {
    T original {*val};
		if constexpr (std::is_pointer_v<T>) { (*(char**)val) += adv; }
		else { val += adv; }
		return original;
	}
	
	// Default-construct a range of object in a given chunk of memory.
	template <class T> constexpr void soaConstructRangeDefault(T* data, size_t count) {
		new (data) T[count] ();
	}
	
	// Destruct a range of objects in a given chunk of memory.
	template <class T> constexpr void soaDestructRange(T* data, size_t count) {
		for (size_t i{0}; i < count; ++i) { data[i].~T(); }
	}
	
	// Moves a range of objects from one chunk of memory to another.
	template <class T> constexpr void soaMoveRange(T* dstData, T* srcData, size_t count) {
		//for (size_t i{0}; i < count; ++i) { dstData[i] = std::move(srcData[i]); }
    memmove(dstData, srcData, sizeof(T)*count);
	}

	// Copies a range of objects from one chunk of memory to another.
	// Assumes the objects in dstData have already been constructed.
	template <class T> constexpr void soaCopyRange(T* dstData, T* srcData, size_t count) {
    for (size_t i {0}; i < count; ++i) { dstData[i] = srcData[i]; }
	}

	template <std::ranges::range R> constexpr void soaCopyRange(R& dstRange, const R& srcRange) {
    for (auto& [dstItem, srcItem] : std::ranges::zip_view(dstRange, srcRange)) {
			dstItem = srcItem;
		}
	}

	// Copies a range of objects from one chunk of memory to another.
	// Assumes the objects in dstData have not been constructed.
	template <class T> constexpr void soaCopyRangeConstruct(T* dstData, T* srcData, size_t count) {
    for (size_t i {0}; i < count; ++i) { new (&dstData[i]) T(srcData[i]); }
	}

	template <std::ranges::range R> constexpr void soaCopyRangeConstruct(R& dstRange, const R& srcRange) {
    for (auto& [dstItem, srcItem] : std::ranges::zip_view(dstRange, srcRange)) {
      new (&dstItem) R::value_type(srcItem);
		}
	}

	// Swaps a pair of entries within the same chunk of memory.
	template <class T> constexpr void soaSwapEntries(T* data, size_t first, size_t second) {
    std::swap(data[first], data[second]);
	}

} // namespace impl

namespace wc {
namespace etc {

template <class... ColTs>
class Soa {
protected:
	using ThisT = Soa<ColTs...>;
	
	static constexpr size_t numColumns_s {sizeof...(ColTs)};
	static_assert(sizeof...(ColTs) > 0, "Soa must have at least one column.");
	
	// The size, in bytes, of an entire row.
	static constexpr size_t rowSize_s {(sizeof(ColTs) + ...)};

	// Certain functions require an index sequence to iterate over each column.
	// This constant is a simple helper for creating said sequence.
	static constexpr auto seq {std::make_index_sequence<numColumns_s>{}};
	
public:
	template <int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;
	template <int N> using NthType = NthTypeOf<N, ColTs...>;

	template <size_t N> static constexpr size_t columnSize() { return sizeof(NthType<N>); }
  static constexpr size_t rowSize() { return rowSize_s; }

	Soa() = default;  // Default constructor.
	~Soa() { realloc(0); } // Destructor.
	Soa(const ThisT& rhs) { copy(rhs); }  // Copy constructor.
  Soa(ThisT&& rhs) { swap(*this, rhs); } // Move constructor.
	Soa(const std::initializer_list<std::tuple<ColTs...>>& ilist) { // Initializer list constructor.
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
    if constexpr (std::is_same_v<R::value_type, std::tuple<ColTs&...>>) {
      for (auto& entry : range) {
        std::apply([this](const ColTs&... row) { this->pushBack(row...);}, entry);
			}
		} else {
			// We're constructing with a range of one item, so we'd better only have one column.
      static_assert(sizeof...(ColTs) == 1);
			// The range's type had better be compatible with our own.
      static_assert(std::is_nothrow_convertible_v<R::value_type, NthType<0>>);
			// Finally copy the range's data into ourselves.
      for (auto& item : range) { pushBack(item); }
		}
	}
	template <std::ranges::range... Rs>
	Soa(std::tuple<Rs...>&& ranges) { // Initialize using a tuple of ranges.
    static_assert(sizeof...(ColTs) == sizeof...(Rs));
    //(impl::soaCopyRangeConstruct(viewColumn<Ns>(), std::get<Ns>(ranges)), ...);
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
	template <size_t... Ns> auto viewColumns(std::index_sequence<Ns...> = seq)       { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	template <size_t... Ns> auto viewColumns(std::index_sequence<Ns...> = seq) const { return std::ranges::zip_view(viewColumn<Ns>() ...); }
	
	// Iterate over the container.  This is equivalent to 'viewColumns' with every column.
	// If there's only 1 column, it uses the view of just that column.
	auto begin()       { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().begin(); else return viewColumns(seq).begin(); }
	auto begin() const { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().begin(); else return viewColumns(seq).begin(); }
	auto end()         { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().end();   else return viewColumns(seq).end(); }
	auto end()   const { if constexpr (sizeof...(ColTs) == 1) return viewColumn<0>().end();   else return viewColumns(seq).end(); }

	// Creates a tuple consisting of the selected column views.  A "struct of arrays", if you will.
	template <size_t... Ns> auto tupleOfViews()       { return std::make_tuple(viewColumn<Ns>() ...); }
	template <size_t... Ns> auto tupleOfViews() const { return std::make_tuple(viewColumn<Ns>() ...); }
	template <class... Ts>  auto tupleOfViews()       { return std::make_tuple(viewColumn<Ts>() ...); }
	template <class... Ts>  auto tupleOfViews() const { return std::make_tuple(viewColumn<Ts>() ...); }
	
	// Access to the given row.
	auto operator[](size_t index)       { boundsCheck(index); return begin()+index; }
	auto operator[](size_t index) const { boundsCheck(index); return begin()+index; }
	
	bool   empty()    const { return (size_ == 0); }
	size_t size()     const { return size_; }
	size_t maxSize()  const { return SIZE_MAX; }
	size_t capacity() const { return capacity_; }
	
	void reserve(size_t newCapacity) { if (newCapacity > capacity_) realloc(newCapacity); }
	void shrinkToFit()               { realloc(size_); }
  void resize(size_t newSize)      { Helper(*this, seq).resize(newSize); }
  void clear()                     { resize(0); }

	template <typename... Ts> void insert(size_t index, const Ts&... row)        { Helper(*this, seq).insert(index, row...); }
  template <typename... Ts> void pushBack(const Ts&... row)                    { Helper(*this, seq).pushBack(row...); }
	
	void swapEntries(size_t first, size_t second) { Helper(*this, seq).swapEntries(first, second); }
  void popBack()                                { Helper(*this, seq).popBack(); }
  void eraseShift(size_t index)                 { Helper(*this, seq).eraseShift(index); }
  void eraseSwap(size_t index)                  { swapEntries(index, size_-1); popBack(); }
  void erase(size_t index)                      { eraseShift(index); }

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
  // The tuple elements are [containerSize, numBytes, dataPtr].
	template <typename = typename std::enable_if<std::is_trivially_copyable_v<ColTs> && ...>::type>
  std::tuple<size_t, size_t, void*> serialize() {
    shrinkToFit();
    return std::make_tuple(
      this->size_,
      (rowSize_s * this->capacity_),
      this->data_);
  }

  // deserialize(t)
  // Uses information returned by serialize to reconstruct the table.
  // Data pointed to by dataPtr is copied into the container.
	template <typename = typename std::enable_if<std::is_trivially_copyable_v<ColTs> && ...>::type>
  void deserialize(size_t containerSize, size_t numBytes, void* dataPtr) {
    clear(); realloc(containerSize);
    memcpy(this->data_, dataPtr, numBytes);
  }
  // deserialize(t)
  // Takes a tuple (as returned by serialize) and uses it to reconstruct the
  // container. The tuple elements are [containerSize, numBytes, dataPtr]. The
  // data pointed to by the data pointer is copied into the container.
	template <typename = typename std::enable_if<std::is_trivially_copyable_v<ColTs> && ...>::type>
  void deserialize(std::tuple<size_t, size_t, void*> arg) {
    auto& [containerSize, numBytes, dataPtr] = arg;
    deserialize(containerSize, numBytes, dataPtr);
  }

	// Creates a new Soa which is a copy of this but with a vector added as a new column.
	template <typename T> inline auto addColumn(const std::vector<T>& col) { return implAddColumn(col, seq); }

	// Create a new Soa which is a column-wise merge of two Soas.
	template <typename... Ts> inline auto mergeColumns(const Soa<Ts...>& other) { return implMergeColumns(other, seq, other.seq); }

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
	
protected:
	void* data_{nullptr};
	size_t size_{0}, capacity_{0};
	std::tuple<ColTs*...> columns_{};

	template <size_t... Ns> class Helper {
    ThisT& soa;
  public:
    Helper(ThisT& container, std::index_sequence<Ns...>): soa(container) {}

		// This is the only function allowed to allocate or free memory.
		// If extraBytes is nonzero, that many additional bytes of memory are allocated in addition to the space needed by the container.
		// This memory is located after the container, and a pointer to it is returned from this function.
		// If extraBytes is zero, nullptr is returned. 
    void* realloc(size_t newCapacity, size_t extraBytes = 0) {
			// If there's nothing to do, bail out.
			if (newCapacity == soa.capacity_) return nullptr;

			// For alignment purposes, we only allocate a multiple of 16 bytes.
			// We enforce a multiple of 16 elements instead of adding padding.
      newCapacity += ((newCapacity%16) ? 16-(newCapacity%16) : 0);
		
			// If we are allocating memory...
			if (newCapacity) {
				void* newData = impl::soaAlignedMalloc(16, (newCapacity * rowSize_s) + extraBytes);
				// Figure out how many rows we'll need to move.
				size_t rowsToMove = std::min(soa.size_, newCapacity);
				if (rowsToMove) {
					// Move the data into into its new position.
					void* dataCursor{newData};
					(memmove(impl::soaPostAdv(&dataCursor, sizeof(NthType<Ns>)*newCapacity), std::get<Ns>(soa.columns_), sizeof(NthType<Ns>)*rowsToMove), ...);
					// Clean up any leftover objects.
					if (newCapacity < soa.size_) {
						(impl::soaDestructRange(std::get<Ns>(soa.columns_)+rowsToMove, soa.size_-rowsToMove), ...);
					}
					impl::soaAlignedFree(soa.data_);
				}
				soa.data_ = newData;
				soa.capacity_ = newCapacity;
				soa.size_ = rowsToMove;
        ((std::get<Ns>(soa.columns_) = static_cast<NthType<Ns>*>(impl::soaPostAdv(&newData, sizeof(NthType<Ns>)*newCapacity))), ...);
				return extraBytes ? newData : nullptr;
			}
			// New capacity is zero, but we have old data to clean up.
			else if (soa.data_) {
				(impl::soaDestructRange(std::get<Ns>(soa.columns_), soa.size_), ...);
				impl::soaAlignedFree(soa.data_);
				soa.data_ = nullptr;
				soa.capacity_ = 0;
				soa.size_ = 0;
				soa.columns_ = {};
				return nullptr;
			}
			return nullptr;
		}

		template <typename... Ts>
		void insert(size_t index, Ts&... row) {
			if (index > soa.size_) throw std::out_of_range("Index out of bounds.");
			soa.expandIfNeeded();
      (memmove(std::get<Ns>(soa.columns_)+index+1, std::get<Ns>(soa.columns_)+index, sizeof(NthType<Ns>)*(soa.size_-index)), ...);
			((new (std::get<Ns>(soa.columns_)+index) NthType<Ns>(std::forward<NthTypeOf<Ns, Ts...>>(row))), ...);
			++soa.size_;
		}

		template <typename... Ts>
		void pushBack(Ts&&... row) {
      soa.expandIfNeeded();
      ((new (std::get<Ns>(soa.columns_)+soa.size_) NthType<Ns>(std::forward<NthTypeOf<Ns, Ts...>>(row))), ...);
			++soa.size_;
    }

		void eraseShift(size_t index) {
			soa.boundsCheck(index);
      (impl::soaDestructRange(std::get<Ns>(soa.columns_)+index, 1), ...);
			(memmove(std::get<Ns>(soa.columns_)+index, std::get<Ns>(soa.columns_)+index+1, sizeof(NthType<Ns>)*(soa.size_-(index+1))), ...);
			--soa.size_;
		}

		void popBack() {
      if (soa.size_ == 0) return;
			(impl::soaDestructRange(std::get<Ns>(soa.columns_)+soa.size_-1, 1), ...);
			--soa.size_;
		}

		void resize(size_t newSize) {
			if (newSize == soa.size_) return;
			soa.reserve(newSize);
			if (newSize > soa.size_) {
				((new (std::get<Ns>(soa.columns_)+soa.size_) (NthType<Ns>[newSize-soa.size_]) ()), ...);
			}
			else if (newSize < soa.size_) {
				(impl::soaDestructRange(std::get<Ns>(soa.columns_)+newSize, soa.size_-newSize), ...);
			}
			soa.size_ = newSize;
		}

		// Copy the contents of 'rhs' into 'this'.
		void copy(const ThisT& rhs) {
			soa.resize(rhs.size());
			(impl::soaCopyRange(std::get<Ns>(soa.columns_), std::get<Ns>(rhs.soa.columns_), rhs.size()), ...);
		}

		// Swaps the position of two rows.
		void swapEntries(size_t first, size_t second) {
			soa.boundsCheck(first); soa.boundsCheck(second);
      (impl::soaSwapEntries(std::get<Ns>(soa.columns_), first, second), ...);
		}
	};

	// Reallocates memory to just enough to fit newCapacity entries.
	void* realloc(size_t newCapacity, size_t extraBytes = 0) { return Helper(*this, seq).realloc(newCapacity, extraBytes); }

	// Copies the contents of another Soa into this.  Overwrites the current contents.
	void copy(ThisT& rhs) { Helper(*this, seq).copy(rhs); }

	// Checks to see if the given index is out-of-bounds, and throws an exception if so.
	inline void boundsCheck(size_t index) const { if (index >= size_) throw std::out_of_range("Index out of bounds."); }

	// Checks to see if there's room for just one more, and if not, expands as needed.
	inline void expandIfNeeded() { if (size_ == capacity_) reserve(std::max<size_t>(size_*2, 16)); }
};

namespace test {
	int SoaUnitTest();
} // namespace test

}} // namespace wc::etc
#endif // WC_ETC_SOA_H