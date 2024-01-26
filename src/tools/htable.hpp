/******************************************************************************
 * htable.hpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2019 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * A Hash Table implementation utilizing a Struct-Of-Arrays.
 * By storing a lightweight hashmap alongside a struct-of-arrays, memory
 * efficiency is improved compared to a traditional hash table.
 * Any number of items per entry can be stored, and multiple entries with
 * the same key can be present in the table simultaneously.
 *****************************************************************************/
#ifndef HVH_TOOLS_HASHTABLESOA_H
#define HVH_TOOLS_HASHTABLESOA_H

#include "soa.hpp"
#include <optional>

namespace hvh {

  template <typename KeyT, typename... ItemTs>
  class Table: public Soa<KeyT, ItemTs...> {
  public:
    // Table()
    // Default constructor for a hash table.
    // Initial size, capacity, and hashmap size are 0.
    // Complexity: O(1).
    Table() {}
    // Table(...)
    // Constructs a hash table using a list of tuples.
    // Initializes the table with the entries from the list; the leftmost item
    // is the key. Complexity: O(n).
    Table(const std::initializer_list<std::tuple<KeyT, ItemTs...>>& initList) {
      reserve(initList.size());
      for (auto& entry : initList) {
        std::apply(
            [this](const KeyT& key, const ItemTs&... items) {
              this->insert(key, items...);
            },
            entry);
      }
    }
    // Table(&& rhs)
    // Move constructor for a hash table.
    // Moves the entries from the rhs hash table into ourselves.
    // Complexity: O(1).
    Table(Table<KeyT, ItemTs...>&& other) { swap(*this, other); }
    // Table(const& rhs)
    // Copy constructor for a hash table.
    // Initializes the hash table as a copy of rhs.
    // Complexity: O(n).
    Table(const Table<KeyT, ItemTs...>& other) {
      reserve(other.capacity());
      memcpy(hashMap_, other.hashMap_, sizeof(uint32_t) * hashCapacity_);
      impl::SoaBase<KeyT, ItemTs...>&       base      = *this;
      const impl::SoaBase<KeyT, ItemTs...>& otherbase = other;
      base.copy(otherbase);
    }
    // operator = (&& rhs)
    // Move-assignment operator for a hash table.
    // Moves the entries from the rhs hash table into ourselves, replacing old
    // contents. Complexity: O(1).
    Table<KeyT, ItemTs...>& operator=(Table&& other) {
      swap(*this, other);
      return *this;
    }
    // operator = (& rhs)
    // Copy-assignment operator for a hash table.
    // Copies the entries from the rhs hash table into ourselves, replacing old
    // contents. Complexity: O(n).
    Table<KeyT, ItemTs...>& operator=(Table other) {
      swap(*this, other);
      return *this;
    }
    // ~Table()
    // Destructor for a hash table.
    // Calls the destructor for all contained keys and items, then frees held
    // memory. Complexity: O(n).
    ~Table() {
      impl::SoaBase<KeyT, ItemTs...>& base = *this;
      base.destructRange_(0, this->size_);
      base.nullify_();
      this->size_     = 0;
      this->capacity_ = 0;
      if (hashMap_)
        SoaAlignedFree(hashMap_);
    }

    // swap(lhs, rhs)
    // swaps the contents of two htables.
    // Complexity: O(1).
    friend inline void
        swap(Table<KeyT, ItemTs...>& lhs, Table<KeyT, ItemTs...>& rhs) {
      std::swap(lhs.hashMap_, rhs.hashMap_);
      std::swap(lhs.hashCapacity_, rhs.hashCapacity_);
      Soa<KeyT, ItemTs...>& lhsbase = lhs;
      Soa<KeyT, ItemTs...>& rhsbase = rhs;
      swap(lhsbase, rhsbase);
    }

    // clear()
    // Erases all entries from the htable, destructing all keys and items.
    // The capacity of the hash table is unchanged.
    // Complexity: O(n).
    inline void clear() {
      memset(hashMap_, INDEXNUL, sizeof(uint32_t) * hashCapacity_);
      Soa<KeyT, ItemTs...>& base = *this;
      base.clear();
    }

    // rehash()
    // Recalculates the hash for all keys in the table.
    // Called automatically if the table is resized, and can be used to clear up
    // deleted indices in the map. Complexity: O(n).
    void rehash() {
      memset(hashMap_, INDEXNUL, sizeof(uint32_t) * hashCapacity_);
      for (size_t i = 0; i < this->size_; ++i) {
        // Get the hash for this key.
        size_t hash =
            std::hash<KeyT> {}(this->template at<0>(i)) % hashCapacity_;
        // Figure out where to put it.
        while (1) {
          // If this spot is NULL or DELETED, we can put our reference here.
          uint32_t index = hashMap_[hash];
          if (index == INDEXNUL || index == INDEXDEL) {
            hashMap_[hash] = (uint32_t)i;
            break;
          }
          // Otherwise, keep looking.
          hashInc_(hash);
        }
      }
    }

    // reserve(n)
    // Ensures that the hash table is large enough to hold at least n entries.
    // Called auomatically when trying to insert an entry into a full table.
    // Returns false if a memory allocation error occurs, true otherwise.
    // Complexity: O(n).
    bool reserve(size_t newsize) {
      // For alignment, we must have a multiple of 16 items.
      if (newsize % 16 != 0)
        newsize += 16 - (newsize % 16);

      // We need at least 16 elements.
      if (newsize == 0)
        newsize = 16;

      // We can't shrink the actual memory.
      if (newsize <= this->capacity_)
        return true;

      // Hash capacity needs to be odd and just greater than double the list
      // capacity, but it also needs to conform to 16-byte alignment.
      hashCapacity_      = newsize + newsize + 4;
      size_t htable_size = hashCapacity_ * sizeof(uint32_t);
      --hashCapacity_;

      // Remember the old memory so we can free it.
      void* oldmem = hashMap_;

      // Allocate new memory.
      impl::SoaBase<KeyT, ItemTs...>& base = *this;
      void*                           alloc_result =
          SoaAlignedMalloc(16, (base.sizePerEntry_() * newsize) + htable_size);
      if (!alloc_result)
        return false;

      hashMap_ = (uint32_t*)alloc_result;

      // Copy the old data into the new memory.
      this->capacity_ = newsize;
      base.divyBuffer_(((char*)alloc_result) + htable_size);

      // Free the old memory.
      if (oldmem)
        SoaAlignedFree(oldmem);
      rehash();
      return true;
    }

    // shrink_to_fit()
    // Shrinks the capacity of the hash table to the smallest possible size
    // capable of holding the existing entries. Returns false if a memory
    // allocation error occurs, true otherwise. Complexity: O(n).
    bool shrinkToFit() {
      // For alignment, we must have a multiple of 16 items.
      size_t newsize = this->size_;
      if (newsize % 16 != 0)
        newsize += 16 - (newsize % 16);

      // If the container is already as small as it can be, bail out now.
      if (newsize == this->capacity_)
        return true;

      // Remember the old memory so we can free it.
      impl::SoaBase<KeyT, ItemTs...>& base   = *this;
      void*                           oldmem = hashMap_;

      if (newsize > 0) {
        // Hash capacity needs to be odd and just greater than double the list
        // capacity, but it also needs to conform to 16-byte memory alignment.
        hashCapacity_      = newsize + newsize + 4;
        size_t htable_size = hashCapacity_ * sizeof(uint32_t);
        --hashCapacity_;

        // Allocate new memory.
        void* alloc_result = SoaAlignedMalloc(
            16, (base.sizePerEntry_() * newsize) + htable_size);
        if (!alloc_result)
          return false;

        hashMap_ = (uint32_t*)alloc_result;

        // Copy the old data into the new memory.
        this->capacity_ = newsize;
        base.divyBuffer_(((char*)alloc_result) + htable_size);
      } else {
        base.nullify_();
        this->capacity_ = 0;
        hashCapacity_   = 0;
        hashMap_        = nullptr;
      }

      // Free the old memory.
      if (oldmem)
        SoaAlignedFree(oldmem);
      rehash();
      return true;
    }

    // insert(key, items...)
    // Inserts a new entry into the hash table.
    // Hash tables may store multiple entries with the same key.
    // Returns false if a memory allocation failure occurs in reserve(), true
    // otherwise. Complexity: O(1) amortized.
    template <typename... Ts>
    bool insert(const KeyT& key, Ts&&... items) {
      if (this->size_ == maxSize())
        return false;
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2))
          return false;
      }
      // Get the hash for the key.
      size_t hash = std::hash<KeyT> {}(key) % hashCapacity_;
      // Look through the table for a place to put it...
      while (1) {
        uint32_t index = hashMap_[hash];
        if (index == INDEXNUL || index == INDEXDEL) {
          index                      = (uint32_t)this->size_;
          Soa<KeyT, ItemTs...>& base = *this;
          base.pushBack(key, std::forward<Ts>(items)...);
          hashMap_[hash] = index;
          break;
        }
        hashInc_(hash);
      }
      return true;
    }

    // emplace(key, args...)
    // Constructs a new entry in the hash table in-place.
    // Hash tables may store multiple entries with the same key.
    // Returns false if a memory allocation failure occurs in reserve(), true
    // otherwise. Complexity: O(1) amortized.
    template <typename... CTypes>
    bool emplace(const KeyT& key, CTypes&&... cargs) {
      if (this->size_ == maxSize())
        return false;
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2))
          return false;
      }
      // Get the hash for the key.
      size_t hash = std::hash<KeyT> {}(key) % hashCapacity_;
      // Look through the table for a place to put it...
      while (1) {
        uint32_t index = hashMap_[hash];
        if (index == INDEXNUL || index == INDEXDEL) {
          index                      = (uint32_t)this->size_;
          Soa<KeyT, ItemTs...>& base = *this;
          base.emplaceBack(key, std::forward<CTypes>(cargs)...);
          hashMap_[hash] = index;
          break;
        }
        hashInc_(hash);
      }
      return true;
    }

    // insertSorted<K>(key, items...)
    // Inserts a new entry into the hash table sorted according to the Kth
    // array. Possibly useful if the data needs to be sorted for some reason
    // other than searching. Returns false if a memory allocation failure occurs
    // in reserve(), true otherwise. Complexity: O(n).
    template <size_t K>
    bool insertSorted(const KeyT& key, const ItemTs&... items) {
      if (this->size_ == maxSize())
        return false;
      if (this->size_ == this->capacity_) {
        if (!reserve(this->capacity_ * 2))
          return false;
      }
      Soa<KeyT, ItemTs...>& base = *this;
      size_t where = base.template lowerBoundRow<K>(key, items...);
      base.insert(where, key, items...);
      rehash();
    }

    // find(key)
    // Searches for the entry with the indicated key.
    // Returns a FindProxy object which can provide an iterator to access
    // the found entry.
    auto find(const KeyT& key) { return FindProxy(*this, key); }
    auto find(const KeyT& key) const { return ConstFindProxy(*this, key); }

    // count(key)
    // Returns the number of entries in the table which have the indicated key.
    // If no entries in the table have the indicated key, 0 is returned.
    // Complexity: O(1) amortized.
    inline size_t count(const KeyT& key) const {
      size_t result = 0;
      for (auto& i : find(key)) {
        ++result;
      }
      return result;
    }

    // contains(key)
    // Returns true if there is at least one entry in the table with the indicated key.
    inline bool contains(const KeyT& key) const {
      auto found = find(key);
      return found->exists();
    }

    // swap_entries(first, second)
    // Swaps the position of two entries and repairs the hashes for each.
    // Complexity: O(1) amortized.
    void swapEntries(size_t first, size_t second) {
      // Swap the two entries.
      Soa<KeyT, ItemTs...>& base = *this;
      base.swapEntries(first, second);

      // Find the hash position for the first entry.
      size_t hash =
          std::hash<KeyT> {}(this->template at<0>(first)) % hashCapacity_;
      size_t firstHashPos = SIZE_MAX;
      while (1) {
        uint32_t index = hashMap_[hash];
        if (index == (uint32_t)first) {
          firstHashPos = hash;
          break;
        }
        if (index == INDEXNUL) {
          // ERROR! We can't repair the link!
          break;
        }
        hashInc_(hash);
      }

      // Find the hash position for the second entry.
      hash = std::hash<KeyT> {}(this->template at<0>(second)) % hashCapacity_;
      size_t secondHashPos = SIZE_MAX;
      while (1) {
        uint32_t index = hashMap_[hash];
        if (index == (uint32_t)second) {
          secondHashPos = hash;
          break;
        }
        if (index == INDEXNUL) {
          // ERROR! We can't repair the link!
          break;
        }
        hashInc_(hash);
      }

      // Swap the hash positions.
      hashMap_[firstHashPos]  = (uint32_t)second;
      hashMap_[secondHashPos] = (uint32_t)first;
    }

    // erase(key)
    // Finds the entry with the indicated key and erases it.
    // If no entries have the indicated key, the table is unchanged.
    // Returns the number of items erased (0 or 1).
    // Complexity: O(1) amortized.
    inline size_t erase(const KeyT& key) { return find(key).erase(); }

    // eraseAll(key)
    // Erases all entries with the indicated key from the table.
    // If no entries have the indicated key, the table is unchanged.
    // Returns the number of items erased.
    // Complexity: O(1) amortized.
    inline size_t eraseAll(const KeyT& key) {
      size_t result = 0;
      for (auto it : find(key)) {
        result += it.erase();
      }
      return result;
    }

    // maxSize()
    // Returns the greatest number of entries that this hash table could
    // theoretically hold. Does not account for running out of memory.
    inline constexpr size_t maxSize() const { return UINT_MAX - 2; }

    // seeMap()
    // Used for debugging to see if there are any big clumps in the hash map.
    const uint32_t* seeMap(size_t& cap) {
      cap = hashCapacity_;
      return hashMap_;
    }

    // serialize()
    // Shrinks the container to the smallest capacity that can contain its
    // entries, then returns a pointer to the raw data buffer that stores the
    // container's data. num_bytes is filled with the number of bytes in that
    // buffer. This function should be used in tandem with 'deserialize' to save
    // and load a container to disk.
    void* serialize(size_t& num_bytes) {
      shrinkToFit();
      num_bytes = (this->sizePerEntry_() * this->capacity_) +
                  (sizeof(uint32_t) * hashCapacity_);
      return hashMap_;
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
      num_bytes = (this->sizePerEntry_() * this->capacity_) +
                  (sizeof(uint32_t) * hashCapacity_);
      this->size_ = num_elements;
      return hashMap_;
    }

    // sort<K>()
    // Sorts the entries in the table according to the Kth array, then does a
    // rehash. Potentially useful if the data needs to be sorted for some reason
    // other than searching. Returns the exact number of swaps performed while
    // sorting the data. Complexity: O(nlogn).
    template <size_t K>
    size_t sort() {
      size_t result =
          this->quicksort(this->template data<K>(), 0, this->size_ - 1);
      rehash();
      return result;
    }

    /// find()
    /// Searches for the given key and returns the index of the first matching entry.
    /// If no entry with the given key could be found, SIZE_MAX is returned
    /*
    size_t find(const KeyT& key) {
      auto hashCursor = std::hash<KeyT> {}(key) % hashCapacity_;
      while (1) {
        uint32_t index = hashMap_[hashCursor];
        if (index == INDEXNUL)
          return SIZE_MAX;
        if (index != INDEXDEL && this->template at<0>(index) == key)
          return index;
        hashInc_(hashCursor);
      }
    }
    */

    /// FindProxy
    /// This object is returned by find() and provides iterator access to
    /// what was found.  It can be used in a range-based for loop.
    struct FindProxy {
      FindProxy(Table& table, const KeyT& key):
          tbl_(table), key_(key),
          begin_(*this, std::hash<KeyT> {}(key_) % tbl_.hashCapacity_),
          end_(*this, 0) {}

      struct Iterator {

        Iterator(FindProxy& find, size_t cursor):
            fh_(find), cur_(cursor), index_(0) {
          update();
        }

        Iterator& operator++() {
          hashInc(cur_);
          update();
          return *this;
        }
        Iterator operator++(int) {
          Iterator tmp = *this;
          ++(*this);
          return tmp;
        }

        friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
          return (lhs.cur_ == rhs.cur_);
        }
        friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
          return (lhs.cur_ != rhs.cur_);
        }

        Iterator& operator*() { return *this; }
        Iterator* operator->() { return this; }

        bool   exists() const { return (index_ != SIZE_MAX); }
               operator bool() const { return exists(); }
        size_t index() const { return index_; }
        auto   row() const { return fh_.tbl_.makeRowTuple(index_); }
        template <size_t K>
        auto& get() const {
          return fh_.tbl_.template at<K>(index_);
        }

        size_t erase() {
          // If the current index isn't valid, don't erase anything!
          if (index_ == SIZE_MAX || index_ == INDEXNUL || index_ == INDEXDEL)
            return 0;

          // Swap the entry with the rear of the list and erase it.
          Soa<KeyT, ItemTs...>& base = fh_.tbl_;
          base.eraseSwap(index_);
          fh_.tbl_.hashMap_[cur_] = INDEXDEL;

          // Get the hash of the key that we just moved into the deleted item's
          // place.
          size_t hash = std::hash<KeyT> {}(fh_.tbl_.template at<0>(index_)) %
                        fh_.tbl_.hashCapacity_;
          // Scan through looking for the reference so we can repair it.
          while (1) {
            uint32_t newindex = fh_.tbl_.hashMap_[hash];
            if (newindex == fh_.tbl_.size_) {
              // Repair the link.
              fh_.tbl_.hashMap_[hash] = (uint32_t)index_;
              break;
            }
            if (newindex == INDEXDEL) {
              // Error! Can't repair the link!
              break;
            }
            hashInc(hash);
          }
          return 1;
        }

        size_t eraseSorted() {
          // If the current index isn't valid, don't erase anything!
          if (index_ == SIZE_MAX || index_ == INDEXNUL || index_ == INDEXDEL)
            return 0;

          // Erase the entry and shift further entries back by 1.
          Soa<KeyT, ItemTs...>& base = *this;
          base.eraseShift(index_);
          fh_.tbl_.hashMap_[cur_] = INDEXDEL;
          fh_.tbl_.rehash();
          cur_   = SIZE_MAX;
          index_ = SIZE_MAX;
        }

      private:
        FindProxy& fh_;
        size_t     cur_;
        size_t     index_;

        void hashInc(size_t& h) {
          if (h == SIZE_MAX)
            return;
          h = ((h + 2) % fh_.tbl_.hashCapacity_);
        }

        void update() {
          if (cur_ >= fh_.tbl_.hashCapacity_) {
            index_ = SIZE_MAX;
            cur_   = SIZE_MAX;
            return;
          }
          while (1) {
            uint32_t index = fh_.tbl_.hashMap_[cur_];
            if (index == INDEXNUL) {
              index_ = (int)SIZE_MAX;
              cur_   = SIZE_MAX;
              return;
            }
            if (index != INDEXDEL &&
                fh_.tbl_.template at<0>(index) == fh_.key_) {
              index_ = (size_t)index;
              return;
            }
            hashInc(cur_);
          }
        }
      };

      Iterator begin() { return begin_; }
      Iterator end() { return end_; }

      const Iterator& operator*() { return begin_; }
      Iterator*       operator->() { return &begin_; }

    private:
      Table&      tbl_;
      const KeyT& key_;
      Iterator    begin_;
      Iterator    end_;
    };

    /// ConstFindProxy
    /// This object is returned by find() and provides iterator access to
    /// what was found.  It can be used in a range-based for loop.
    struct ConstFindProxy {
      ConstFindProxy(const Table& table, const KeyT& key):
          tbl_(table), key_(key),
          begin_(*this, (tbl_.hashCapacity_ > 0) ?
                            std::hash<KeyT> {}(key_) % tbl_.hashCapacity_ :
                            SIZE_MAX),
          end_(*this, SIZE_MAX) {}

      struct Iterator {

        Iterator(ConstFindProxy& find, size_t cursor):
            fh_(find), cur_(cursor), index_(0) {
          update();
        }

        Iterator& operator++() {
          hashInc(cur_);
          update();
          return *this;
        }
        Iterator operator++(int) {
          Iterator tmp = *this;
          ++(*this);
          return tmp;
        }

        friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
          return (lhs.cur_ == rhs.cur_);
        }
        friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
          return (lhs.cur_ != rhs.cur_);
        }

        const Iterator& operator*() const { return *this; }
        const Iterator* operator->() const { return this; }

        bool   exists() const { return (index_ != SIZE_MAX); }
               operator bool() const { return exists(); }
        size_t index() const { return index_; }
        auto   row() const { return fh_.tbl_.makeRowTuple(index_); }
        template <size_t K>
        const auto& get() const {
          return fh_.tbl_.template at<K>(index_);
        }

      private:
        ConstFindProxy& fh_;
        size_t          cur_;
        size_t          index_;

        void hashInc(size_t& h) {
          if (h == SIZE_MAX)
            return;
          h = ((h + 2) % fh_.tbl_.hashCapacity_);
        }

        void update() {
          if (cur_ >= fh_.tbl_.hashCapacity_) {
            index_ = SIZE_MAX;
            cur_   = SIZE_MAX;
            return;
          }
          while (1) {
            uint32_t index = fh_.tbl_.hashMap_[cur_];
            if (index == INDEXNUL) {
              index_ = (int)SIZE_MAX;
              cur_   = SIZE_MAX;
              return;
            }
            if (index != INDEXDEL &&
                fh_.tbl_.template at<0>(index) == fh_.key_) {
              index_ = (size_t)index;
              return;
            }
            hashInc(cur_);
          }
        }
      };

      Iterator begin() { return begin_; }
      Iterator end() { return end_; }

      const Iterator& operator*() const { return begin_; }
      const Iterator* operator->() const { return &begin_; }

    private:
      const Table& tbl_;
      const KeyT&  key_;
      Iterator     begin_;
      Iterator     end_;
    };

  protected:
    inline void hashInc_(size_t& h) const { h = ((h + 2) % hashCapacity_); }

    static const uint32_t INDEXNUL = UINT_MAX;
    static const uint32_t INDEXDEL = UINT_MAX - 1;

    uint32_t* hashMap_      = nullptr;
    size_t    hashCapacity_ = 0;

    // Ban certain inherited methods.
    //	using soa<KeyT, ItemTs...>::clear;
    //	using soa<KeyT, ItemTs...>::reserve;
    //	using soa<KeyT, ItemTs...>::shrink_to_fit;
    using Soa<KeyT, ItemTs...>::resize;
    using Soa<KeyT, ItemTs...>::pushBack;
    using Soa<KeyT, ItemTs...>::emplaceBack;
    using Soa<KeyT, ItemTs...>::popBack;
    //	using soa<KeyT, ItemTs...>::insert;
    using Soa<KeyT, ItemTs...>::eraseSwap;
    using Soa<KeyT, ItemTs...>::eraseShift;
    //	using soa<KeyT, ItemTs...>::swap;
    //	using soa<KeyT, ItemTs...>::swap_entries;
  };

}  // namespace hvh

#endif  // HVH_TOOLS_HASHTABLESOA_H