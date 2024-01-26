#ifndef WC_ECS_ARCHETYPE_TABLE_H
#define WC_ECS_ARCHETYPE_TABLE_H
// The archetype table is a 2-way hashtable.  It needs to use entity IDs to search for rows, and component id's to search for columns.
// Inspiration should be taken from hvh::table as it ought to be tightly-packed, but it can't use templates to define column types.

#include <vector>
#include <cstdint>
#include "tools/htable.hpp"

namespace wc {
namespace ecs {

  using ComponentId = uint64_t;
  using EntityId = uint64_t;

  class ArchetypeTable {
  public:

    template <typename T>
    struct ColumnView {
      T* begin() {
        return static_cast<T*>(
            &table.rowData_[table.columnTable_.at<CTBL_OFFSET>(colIndex)]);
      }
      T*              end() { return begin() + table.columnTable_.size(); }
      T&              operator[](size_t i) { return *(begin() + i); }
      bool            operator() { return (colIndex != SIZE_MAX); }

      ArchetypeTable& table;
      size_t          colIndex {SIZE_MAX};
    };
    template <typename T>
    ColumnView<T> column() {
      return ColumnView<T> {*this, findColumn<T>()};
    }

    template <typename T>
    struct ConstColumnView {
      const T* begin() {
        return static_cast<const T*>(
            &table.rowData_[table.columns_.at<CTBL_OFFSET>(colIndex)]);
      }
      const T*              end() { return begin() + table.columns_.size(); }
      const T&              operator[](size_t i) { return *(begin() + i); }
      bool                  operator() { return (colIndex != SIZE_MAX); }

      const ArchetypeTable& table;
      size_t                colIndex {SIZE_MAX};
    };
    template <typename T>
    ConstColumnView<T> column() const {
      return ConstColumnView<T> {*this, findColumn<T>()};
    }

    // Reserve at least enough space for newSize entries.  Cannot shrink the container.
    void reserve(size_t newSize) {
      if (newSize > capacity_)
        realloc(newSize);
    }

    // Shrinks the container to the smallest size that will hold all of its current entries.
    // Capacity may still be greater than size for alignment purposes.
    void shrinkToFit() { realloc(size_); }

    // Entities are added to an archetype in one of four circumstances:
    // - A brand new empty entity is born.
    // - A prefab entity is spawned as a copy from a template.
    // - A component is being added or removed from an entity.
    // - A scene is being loaded.
    // `insert` only cares about the first two cases.  Components being added/removed requires its own logic, and scenes being loaded will simply load an entire table which has been saved to disk.
    void insert(EntityId id, void* data, size_t dataSize) {
      // `data` and `dataSize` do not count the size of the entity Id.
      if (!data && dataSize != 0)
        return; // Error: data is null but dataSize is non-zero.
      if (dataSize != bytesPerRow() - sizeof(EntityId))
        return; // Error: dataSize does not match the size of a row.
    }

    // Erases the entity with the given id from this table.
    void erase(EntityId id) {
      size_t hashCursor {SIZE_MAX};
      size_t index = findRow(id, &hashCursor);

      // If the current index isn't valid, don't erase anything!
      if (index >= size_)
        return;

      // Copy the item at the rear of the list into the spot being erased.
      for (size_t c {0}; c < columns_.size(); ++c) {
        size_t            itemSize = columns_.at<CTBL_SIZE>(c);
        memcpy(&data_[columns_.at<CTBL_OFFSET>(c) + (index * itemSize)],
               &data_[columns_.at<CTBL_OFFSET>(c) + ((size_-1))*itemSize)], itemSize);
      }
      size_--;
      hashmap_[hashCursor] = INDEXDEL;

      // Get the hash of the key that we just moved into the deleted item's place.
      size_t hash = std::hash<KeyT> {}(ids_[index]) % hashCapacity_;
      // Scan through looking for the reference so we can repair it.
      while (1) {
        uint32_t newindex = hashmap_[hash];
        if (newindex == size_) {
          // Repair the link.
          hashmap_[hash] = (uint32_t)index;
          break;
        }
        if (newindex == INDEXDEL) {
          // Error! Can't repair the link!
          rehash();
          return;
        }
        hashInc_(hash);
      }
    }

  private:
    hvh::Table<ComponentId, size_t, size_t> columns_;
    constexpr const static CTBL_ID {0}; // The Component ID for this column.
    constexpr const static CTBL_SIZE {1}; // The size, in bytes, of the component for this column.
    constexpr const static CTBL_OFFSET {2}; // The data offset for this column.  Not a pointer.

    constexpr size_t bytesPerRow() const {
      size_t result = 0;
      for (size_t i {0}; i < columns_.size(); ++i) {
        result += columns_.at<CTBL_SIZE>(i);
      }
      return result;
    }

    // Finds the row index with the given id.  If there is no entry with that id
    // in the container, SIZE_MAX is returned. If hashCursor is non-null, it
    // will be written with the position of the hash cursor when the entry was
    // found; unmodified if no entry found.
    size_t findRow(EntityId id, size_t* hashCursor = nullptr) {
      if (size_ == 0)
        return SIZE_MAX;
      size_t hashc = std::hash<EntityId> {}(id) % hashCapacity_;
      while (1) {
        uint32_t index = hashmap_[hashc];
        if (index == INDEXNUL)
          return SIZE_MAX;
        if (index != INDEXDEL && ids_[index] == id) {
          if (hashCursor)
            *hashCursor = hashc;
          return (size_t)index;
        }
        hashInc_(hashc)
      }
    }

    template <typename T>
    size_t findColumn() const {
      ComponentId goal {7}; // 7 is a dummy placeholder; get a platform-independent typename from T, then hash it to make an index into the hashmap. 
      return columns_.find(goal)->index();
    }

    // TODO: guarantee that the data allocated by std::vector<char> is aligned to 16-byte boundaries for SIMD data.
    std::vector<char> data_;       // Where per-row data is actually stored.  This is the bulk of the table.
    uint32_t*         hashmap_{nullptr};    // Pointer to the hashmap for row searches.
    size_t            hashCapacity_{0} // The current maximum number of hash entries.
    uint32_t          size_{0}; // Number of rows (max of 2.1G).
    uint32_t          capacity_ {0}; // Number of rows which could be stored without a reallocation.
    EntityId* ids_ {nullptr};  // Pointer to the first column, which is always EntityId.
    // Because entries may be added very frequently, capacity may be much greater than size.

    // Resizes the table to fit at least `newSize` rows.
    // If `newSize` is smaller than the existing table, elements at the end of the table may be lost.
    // O(n).
    void realloc(size_t newSize) {
      // If we're requesting a newSize of 0, just delete all of our data.
      if (newSize == 0) {
        data_.clear();
        hashmap_      = nullptr;
        hashCapacity_ = 0;
        size_         = 0;
        capacity_     = 0;
        ids_          = nullptr;
        return;
      }

      // For alignment, we must have a multiple of 4 items.
      // Each component should be a multiple of 4 bytes, otherwise padding is added.
      // This gives us a guaranteed 16-byte alignment for each column.
      if (newSize % 4 != 0)
        newSize += 4 - (newSize % 4);

      // Bail out if we're already the requested size.
      if (newSize == capacity_)
        return;

      // Hash capacity needs to be odd and just greater than double the list
      // capacity, but it also needs to conform to 16-byte alignment.
      hashCapacity_     = newSize + newSize + 4;
      size_t htableSize = hashCapacity_ * sizeof(uint32_t);
      --hashCapacity_;

      // Allocate new memory.
      // TODO: guarantee that the data allocated by std::vector<char> is aligned to 16-byte boundaries for SIMD data.
      std::vector<char> newMem((bytesPerRow() * newSize) + htableSize);
      size_t            currentOffset {0};

      // Grab the pointer to where we'll store the hashMap, and advance our pointer.
      hashmap_ = (uint32_t*)&newMem[currentOffset];
      currentOffset += htableSize;

      // "id" is always the first column, let's grab it now.
      ids_ = (EntityId*)&newMem[currentOffset];

      // Copy the old data into new memory one column at a time.
      for (size_t c {0}; c < columns_.size(); ++c) {
        memcpy(&newMem[currentOffset], &data_[columns_.at<CTBL_OFFSET>(c)],
               columns_.at<CTBL_SIZE>(c) * std::max(newSize, size_));
        columns_.at<CTBL_OFFSET>(c) = currentOffset;
        currentOffset += columns_.at<CTBL_SIZE>(c) * newSize;
      }
      
      // This swap should exchange the old data with the new data in O(1) time.
      // The "old data" will then be destructed when this function exits.
      std::swap(data_,newMem);
      capacity_ = newSize;
      rehash();
      return true;
    }

    // rehash()
    // Recalculates the hash for all keys in the table.
    // Called automatically if the table is resized, and can be used to clear up
    // deleted indices in the map. Complexity: O(n).
    void rehash() {
      memset(hashmap_, INDEXNUL, sizeof(uint32_t) * hashCapacity_);
      for (size_t i = 0; i < size_; ++i) {
        // Get the hash for this key.
        size_t hash = std::hash<EntityId> {}(ids_[i]) % hashCapacity_;
        // Figure out where to put it.
        while (1) {
          // If this spot is NULL or DELETED, we can put our reference here.
          uint32_t index = hashmap_[hash];
          if (index == INDEXNUL || index == INDEXDEL) {
            hashMap_[hash] = (uint32_t)i;
            break;
          }
          // Otherwise, keep looking.
          hashInc_(hash);
        }
      }
    }

    constexpr const static INDEXNUL {UINT32_MAX};   // The index which indicates a "null entry" in the row hashmap.
    constexpr const static INDEXDEL {UINT32_MAX-1}; // The index which indicates a "deleted entry" in the row hashmap.
    constexpr const static INDEXMAX {UINT32_MAX-2}; // The highest valid index for the row hashmap.

    inline void hashInc_(size_t& h) const { h = ((h + 2) % hashCapacity_); }

  };

}}  // namespace wc::ecs

#endif // WC_ECS_ARCHETYPE_TABLE_H