/******************************************************************************
 * table.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2019 - present
 * Last modified January 2024
 * ---------------------------------------------------------------------------
 * A Hash Table implementation utilizing a Struct-Of-Arrays.
 * By storing a lightweight hashmap alongside a struct-of-arrays, memory
 * efficiency is improved compared to a traditional hash table.
 * Any number of items per entry can be stored, and multiple entries with
 * the same key can be present in the table simultaneously.
 * As with Soa, and unlike a traditional hashtable, entries are stored
 * contiguously in memory for fast iteration.
 *****************************************************************************/
#ifndef WC_ETC_TABLE_H
#define WC_ETC_TABLE_H

#include "soa.h"

namespace wc {
namespace etc {

// Template parameters:
// - IndexT: The type used internally for indexing.  Should be an unsigned integer type.
//     Maximum entries is defined by the size of this type, but larger indices means more memory overhead.
// - AllowMulti: If true, multiple entries with the same key are allowed to exist in the table.
//     "find()" and the like still only find the first entry.  Use "findEach()" and the like to view each entry with the given key.
// - SortColumn: Which column according to which the container should stay sorted.
//     If this is SIZE_MAX, the table will not be sorted.
//     If this feature is used, insert and erase become O(n) operations as a shift and rehash is performed with every operation.
// - KeyT: The type of the table key.  Must be hashable using std::hash.
// - ItemTs...: The column types.  Can be empty (in which case the table acts like a set instead of a map).
template <typename IndexT, bool AllowMulti, size_t SortColumn, typename KeyT, typename... ItemTs>
class BasicTable : public Soa<KeyT, ItemTs...> {
	using ThisT = BasicTable<IndexT, AllowMulti, SortColumn, KeyT, ItemTs...>;
	using BaseT = Soa<KeyT, ItemTs...>;
	static_assert(std::is_integral_v<IndexT>, "IndexT must be an integer.");
	static_assert(SortColumn == SIZE_MAX || SortColumn <= (sizeof...(ItemTs)+1), "SortColumn index out of bounds.");
public:

  BasicTable() = default; // Default constructor.
  ~BasicTable() { realloc(0); } // Destructor.
  BasicTable(const ThisT& rhs) { // Copy constructor.
    reserve(rhs.size());
    BaseT& lhsBase = *this;
    const BaseT& rhsBase = rhs;
    lhsBase.copy(rhsBase);
    rehash();
  }
  BasicTable(ThisT&& rhs) { swap(*this, rhs); }  // Move constructor.
  BasicTable(const std::initializer_list<std::tuple<KeyT, ItemTs...>>& ilist) { // Initializer list constructor.
    reserve(ilist.size());
    for (auto& entry : ilist) {
      std::apply([this](const KeyT& key, const ItemTs&... items) { this->insert(key, items...);}, entry);
		} 
	}
	template <std::ranges::range R>
	BasicTable(R&& range) { // Range constructor.
    reserve(range.size());
		for (auto& entry : range) {
      std::apply([this](const KeyT& key, const ItemTs&... items) { this->insert(key, items...);}, entry);
		}
	}
  
  ThisT& operator=(ThisT rhs)   { swap(*this, rhs); return *this; } // Copy-assignment operator.
  ThisT& operator=(ThisT&& rhs) { swap(*this, rhs); return *this; } // Move-assignment operator.

  // Swaps the contents of two tables.
  friend inline void swap(ThisT& lhs, ThisT& rhs) {
    std::swap(lhs.hashMap_, rhs.hashMap_);
    std::swap(lhs.hashCap_, rhs.hashCap_);
    BaseT::swap(lhs, rhs);
  }

  // Clears the contents of the table without freeing its memory.
  void clear() { if (this->size_ > 0) {
    memset(hashMap_, INDEXNUL, sizeof(IndexT) * hashCap_);
    BaseT& base {*this};
    base.clear();
  }}

	// Provides an interface similar to std::optional for accessing a found element.
	template <typename TableT> class Found {
		TableT& tbl; size_t idx;
	public:
		Found(TableT& table, size_t index): tbl(table), idx(index) {}
		
		bool hasValue() const { return idx < tbl.size(); }
		operator bool() const { hasValue(); }
		
		auto value() { return tbl[idx]; }
		auto operator*() { return value(); }
		
		template <size_t N> auto get() { return tbl.at<N>(idx); }
		template <class T>  auto get() { return tbl.at<T>(idx); }
		template <size_t N, class T> T getOr(const T& defaultVal) { if (hasValue()) return tbl.at<N>(idx); else return defaultVal; }
		template <class T>           T getOr(const T& defaultVal) { if (hasValue()) return tbl.at<T>(idx); else return defaultVal; }
	};
	
	auto find(const KeyT& key)       { size_t cur{hashCurInit(key)}; return Found<ThisT>      (*this, findGoal<>(key, cur)); }
	auto find(const KeyT& key) const { size_t cur{hashCurInit(key)}; return Found<const ThisT>(*this, findGoal<>(key, cur)); }
	
	// Provides a view-like interface for accessing each instance of a found element.
	template <typename TableT> class FindEachView {
		TableT& tbl; const KeyT& key;
	public:
		FindEachView(TableT& table, const KeyT& key): tbl(table), key(key) {}
		
		class Iterator {
			const FindEachView& view; size_t cur; size_t idx;
		public:
			Iterator(const FindEachView& view, size_t cur): view(view), cur(cur), idx(view.tbl.findGoal<>(view.key, cur)) {}
			
			bool operator==(const Iterator& rhs) const { return (idx == rhs.idx); }
			Iterator& operator++()   { idx = view.tbl.findNextGoal(view.key, cur); return *this; }
			Iterator operator++(int) { Iterator result{*this}; ++*this; return result; }
			
			bool hasValue() { return idx < view.tbl.size(); }
			operator bool() { return hasValue(); }
			
			auto value() { return view.tbl[idx]; }
			auto operator*() { return Found<TableT>(view.tbl, idx); }
			
			template <size_t N> auto get() { return view.tbl.at<N>(idx); }
			template <class T>  auto get() { return view.tbl.at<T>(idx); }
			template <size_t N, class T> T getOr(const T& defaultVal) { if (hasValue()) return view.tbl.at<N>(idx); else return defaultVal; }
			template <class T>           T getOr(const T& defaultVal) { if (hasValue()) return view.tbl.at<T>(idx); else return defaultVal; }
		};
		
		auto begin() { return Iterator(*this, tbl.hashCurInit(key)); }
		auto end()   { return Iterator(*this, SIZE_MAX); }
	};
	
  // Finds each entry with the given key.  Returns a view that can be iterated over.
	template <typename = typename std::enable_if<AllowMulti>::type> FindEachView<ThisT>       findEach(const KeyT& key)       { return {*this, key}; }
  // Finds each entry with the given key.  Returns a view that can be iterated over.
	template <typename = typename std::enable_if<AllowMulti>::type> FindEachView<const ThisT> findEach(const KeyT& key) const { return {*this, key}; }
	
  // Returns true if the table contains at least one entry with the given key, false otherwise.
	bool   contains(const KeyT& key) const { size_t cur{hashCurInit(key)}; return (findGoal<>(key, cur) != SIZE_MAX); }
	// Returns the number of entries with the given key.
  size_t count   (const KeyT& key) const {
    if constexpr (AllowMulti) { size_t result{0}; for (auto _: findEach(key)) { ++result; } return result; }
    else return contains(key) ? 1 : 0;
  }
  size_t maxSize() { return (size_t)INDEXMAX; }
	
  // Ensures there's enough space for at least 'newCapacity' entries.
	void reserve(size_t newCapacity) { if (newCapacity > this->capacity_) realloc(newCapacity); }

  // Frees excess space if capacity is greater than size.
	void shrinkToFit() { realloc(this->size_); }
	
  // Insert an entry into the table.
  // O(n) if the table is sorted, O(1) otherwise.
	template <class... Ts> void insert(const KeyT& key, Ts&&... items) {
		expandIfNeeded();
		if constexpr (SortColumn == SIZE_MAX) {
			size_t cursor{hashCurInit(key)}; size_t index{findNextEmpty(key, cursor)};
			if (index != SIZE_MAX) {
				this->pushBack(key, items...);
				hashMap_[cursor] = index;
			}
		} else {
			if constexpr (!AllowMulti) { if (contains(key)) return; }
			size_t index = this->lower_bound<SortColumn>(std::get<SortColumn>(std::forward_as_tuple(key, items...)));
			this->insert(index, key, items...);
			rehash();
		}
	}
	
  // Erase the entry with the given key.  If multiple entries have this key, the first is erased.
  // O(n) if the table is sorted, O(1) otherwise.
	void erase(const KeyT& key) {
		size_t cursor {hashCurInit(key)}; size_t index{findGoal<>(key, cursor)};
		if (index != SIZE_MAX) { eraseFound(cursor, index); }
	}

	// Erase the entry with the given key and maintains ordering. O(n).
	void eraseShift(const KeyT& key) {
    size_t cursor {hashCurInit(key)}; size_t index{findGoal<>(key, cursor)};
    if (index != SIZE_MAX) { eraseFoundShift(index); }
	}
	
  // Erase each entry with the given key.
  // O(n) if the table is sorted, O(1) otherwise.
	template <typename = typename std::enable_if<AllowMulti>::type>
  void eraseEach(const KeyT& key) {
		size_t cursor{hashCurInit(key)}; size_t index{findGoal<>(key, cursor)};
		while (index != SIZE_MAX) {
			eraseFound(cursor, index);
			if constexpr (SortColumn == SIZE_MAX) { index = findNextGoal(key, cursor); }
			else { cursor = hashCurInit(key); index = findGoal<>(key, cursor); }
		}
	}

  // Swaps two entries.
	// O(1).
  template <typename = typename std::enable_if<SortColumn == SIZE_MAX>::type>
  void swapEntries(size_t first, size_t second) {
    // Swap the rows.
    BaseT& base {*this}; this->swapEntries(first, second);
    // Search for the hashmap entry for what is now in 'first' position.
    size_t hashCursor {hashCurInit(this->template at<0>(first))};
    size_t firstHashPos {SIZE_MAX};
    while (true) {
      IndexT index {hashMap_[hashCursor]};
      if (index == (IndexT)first) { firstHashPos = hashCursor; break; }
      if (index == INDEXNUL) { throw std::logic_error("Table::swapEntries failed to repair hash."); }
      hashInc(hashCursor);
    }
    // Search for the hashmap entry for what is now in 'second' position.
    hashCursor = hashCurInit(this->template at<0>(second));
    size_t secondHashPos {SIZE_MAX};
    while (true) {
      IndexT index {hashMap_[hashCursor]};
      if (index == (IndexT)second) { secondHashPos = hashCursor; break; }
      if (index == INDEXNUL) { throw std::logic_error("Table::swapEntries failed to repair hash."); }
      hashInc(hashCursor);
    }
    // Swap the hashmap entries.
    hashMap_[firstHashPos] =  (IndexT)second;
    hashMap_[secondHashPos] = (IndexT)first;
  }

  // sort<K>()
  // Sorts the entries in the table according to the Kth array, then does a
  // rehash. Potentially useful if the data needs to be sorted for some reason
  // other than searching. Returns the exact number of swaps performed while
  // sorting the data. Complexity: O(nlogn).
  //template <size_t K, typename = typename std::enable_if<SortColumn == SIZE_MAX>::type>
  //void sort() {
  //  BaseT& base {*this}; base.sort<K>();
  //  rehash();
  //}

  // serialize()
  // Shrinks the container to the smallest capacity that can contain its entries,
  // then returns a tuple containing the information needed to rebuild the container.
  // The tuple elements are [containerSize, numBytes, dataPtr].
  std::tuple<size_t, size_t, void*> serialize() {
    shrinkToFit();
    return std::make_tuple(
      this->size_,
      (BaseT::rowSize_s * this->capacity_) + (sizeof(IndexT) * hashCap_+1),
      this->data_);
  }

  // deserialize(containerSize, numBytes, dataPtr)
  // Uses information returned by serialize to reconstruct the table.
  // Data pointed to by dataPtr is copied into the container.
  void deserialize(size_t containerSize, size_t numBytes, void* dataPtr) {
    clear(); realloc(containerSize);
    memcpy(this->data_, dataPtr, numBytes);
  }
  // deserialize(t)
  // Takes a tuple (as returned by serialize) and uses it to reconstruct the
  // container. The tuple elements are [containerSize, numBytes, dataPtr]. The
  // data pointed to by the data pointer is copied into the container.
  void deserialize(std::tuple<size_t, size_t, void*> arg) {
    auto& [containerSize, numBytes, dataPtr] = arg;
    deserialize(containerSize, numBytes, dataPtr);
  }

	//template <size_t N> auto extractColumn() {
	//	BasicTable<IndexT, AllowMulti, SortColumn, BaseT::NthType<N>> result; result.reserve(size_);
  //  impl::SoaCopyRangeConstruct(std::get<N>(result.columns_), std::get<N>(columns_), size_);
	//	result.size_ = size_; result.rehash();
	//	return result;
	//}

private:
  // The actual hashtable part of the hashtable.
  // This array consists of indices that point into the underlying Soa.
	IndexT* hashMap_ {nullptr};
  // The current size of the hashMap.
	size_t  hashCap_ {0};
	
	static constexpr size_t INDEXNUL {std::numeric_limits<IndexT>::max()};
	static constexpr size_t INDEXDEL {INDEXNUL-1};
	static constexpr size_t INDEXMAX {INDEXNUL-2};
	
  // If the container can't fit one more entry, expand it.
	void expandIfNeeded() {
		if (this->size_ == this->capacity_) {
			realloc(std::max<size_t>(this->capacity_ * 2, 16));
		}
	}
	
	// Initialize the hash cursor.
	// If an exotic hash function is ever needed, this is where it would go.
	size_t hashCurInit(const KeyT& key) const { return std::hash<KeyT>{}(key) % hashCap_; }
	
	// Increments the hash cursor.
	// If clumping gets bad, this is the function to blame.
	void hashInc(size_t& cursor) const { cursor = (cursor + 2) % hashCap_; }
	
	// Finds the desired item and returns its row index, or SIZE_MAX if the item can't be found.
	// `cursor` is left at the location where `index` was found.
	template <bool checkBounds = true> size_t findGoal(const KeyT& key, size_t& cursor) const {
		if constexpr (checkBounds) { if (cursor >= hashCap_) return SIZE_MAX; }
		while (true) {
			IndexT index = hashMap_[cursor];
			if (index == INDEXNUL) { cursor = SIZE_MAX; return SIZE_MAX; }
			if (index != INDEXDEL && this->template at<0>(index) == key) { return (size_t)index; }
			hashInc(cursor);
		}
		return SIZE_MAX; // Unreachable
	}
	
	// Picks up on where findGoal left off to find the next entry with the given key.
	// Basically the same as findGoal except hashInc() is called first.
	size_t findNextGoal(const KeyT& key, size_t& cursor) const {
		if constexpr (!AllowMulti) return SIZE_MAX;
		if (cursor < hashCap_) {
			hashInc(cursor);
			return findGoal<false>(key, cursor);
		} else return SIZE_MAX;
	}

	// Used when inserting an element.
	// Sets `cursor` to where the entry belongs in the hashmap, and returns the index where the item should be placed.
	// If multiple entries are forbidden, returns SIZE_MAX if the given key is encountered.
	size_t findNextEmpty(const KeyT& key, size_t& cursor) const {
		if (cursor >= hashCap_) return SIZE_MAX;
		while (true) {
			IndexT index = hashMap_[cursor];
			if (index == INDEXNUL || index == INDEXDEL) { return this->size_; }
			if constexpr (!AllowMulti) { if (this->template at<0>(index) == key) return SIZE_MAX; }
			hashInc(cursor);
		}
		return SIZE_MAX; // Unreachable
	}
	
  // Erases the entry at the given cursor and index.
  // If the table is not sorted, this is an O(1) operation.
  // If the table is sorted, it's O(n).
	void eraseFound(size_t cursor, size_t index) {
    if constexpr (SortColumn == SIZE_MAX) { eraseFoundSwap(cursor, index); }
		else { eraseFoundShift(index); }
	}

	void eraseFoundSwap(size_t cursor, size_t index) {
    // Swaps the entry at [index] and [back], then deletes the entry at the rear.
		BaseT& base = *this; base.eraseSwap(index);
    // Clear the hashmap entry for the deleted entry.
    hashMap_[cursor] = INDEXDEL;
    // Find and repair the hashmap entry for the moved entry.
    cursor = hashCurInit(this->template at<0>(index));
    while (true) {
      if (hashMap_[cursor] == this->size_) {
        hashMap_[cursor] = index;
        return;
      }
      // This should never happen.
      if (hashMap_[cursor] == INDEXNUL || hashMap_[cursor] == INDEXDEL) {
        throw std::logic_error("Table::eraseFound failed to repair hash.");
      }
      hashInc(cursor);
    }
	}

	// When the table order needs to be maintained, eraseShift is used instead of eraseSwap.
	// Unfortunately, we can't hope to rebuild the hashmap, so a full rehash is required.
	void eraseFoundShift(size_t index) {
    BaseT& base = *this; base.eraseShift(index);
    rehash();
	}

	// Recalculates the entire hashtable.  Only done if absolutely neccesary.
	void rehash() {
		memset(hashMap_, INDEXNUL, sizeof(IndexT) * hashCap_);
		IndexT index{0};
		for (auto& key : this->template viewColumn<0>()) {
			size_t cursor = hashCurInit(key);
      while (true) {
        IndexT index = hashMap_[cursor];
        if (index == INDEXNUL || index == INDEXDEL) break;
        hashInc(cursor);
      }
			hashMap_[cursor] = index++;
		}
	}
	
	// Do the memory thing.
  void realloc(size_t newCapacity) {
    // Round up to nearest 16 for alignment.
    newCapacity += ((newCapacity % 16) ? 16 - (newCapacity % 16) : 0);
    if (newCapacity == this->capacity_) return;
    if (newCapacity > INDEXMAX) throw std::length_error("Table max size exceeded.");
    // We want our hashmap to be twice as big as the container, plus a little
    // extra. +4 is enough extra, plus it satisfies alignment requirements.
    size_t newHashCapacity = newCapacity + newCapacity + 4;
    size_t newHashmapSize  = newHashCapacity * sizeof(uint32_t);

    // Soa<...>::realloc does all the work for us.  We request extra bytes in which to store the hashmap.
    hashMap_ = static_cast<IndexT*>(BaseT::Helper(*this, BaseT::seq).realloc(newCapacity, newHashmapSize));
    
    if (newCapacity > 0) {
      // hashCapacity needs to be odd for better hash clustering performance.
      hashCap_ = newHashCapacity-1;
      rehash();
    }
    else hashCap_ = 0;
  }
};

template <class KeyT, class... ItemTs>                 using Table =            BasicTable<uint32_t, false, SIZE_MAX, KeyT, ItemTs...>;
template <class KeyT, class... ItemTs>                 using MultiTable =       BasicTable<uint32_t, true,  SIZE_MAX, KeyT, ItemTs...>;
template <size_t SortCol, class KeyT, class... ItemTs> using SortedTable =      BasicTable<uint32_t, false, SortCol,  KeyT, ItemTs...>;
template <size_t SortCol, class KeyT, class... ItemTs> using SortedMultiTable = BasicTable<uint32_t, true,  SortCol,  KeyT, ItemTs...>;

namespace test {
  int TableUnitTest();
} // namespace test

}} // namespace wc::etc
#endif  // WC_ETC_TABLE_H