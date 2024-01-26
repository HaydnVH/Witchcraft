#include "archetype.h"

#include <new>

namespace {
// We need access to aligned allocations and deallocations,
// but visual studio doesn't support aligned_alloc from the c11 standard.
// This define lets us have consistent behaviour.
#ifdef _MSC_VER
#define alignedMalloc(alignment, size) _aligned_malloc(size, alignment)
#define alignedFree(mem)               _aligned_free(mem)
#else
#define alignedMalloc(alignment, size) aligned_alloc(alignment, size)
#define alignedFree(mem)               free(mem)
#endif
}  // namespace

wc::ecs::ArchetypeTable::ArchetypeTable(const ComponentRegistry& registry,
                                        std::vector<ComponentId> components) {
  componentMap_.insert(EntityId::componentId, sizeof(EntityId), nullptr);
  sizePerRow_ = sizeof(EntityId);
  for (auto& c : components) {
    if (componentMap_.find(c)->exists()) {
      throw std::invalid_argument(
          "Archetype table cannot hold duplicate components.");
    }
    size_t componentSize = ((registry.count(c)) ? registry.at(c) : 0);
    componentMap_.insert(c, componentSize, nullptr);
    sizePerRow_ += componentSize;
  }
}

wc::ecs::ArchetypeTable::~ArchetypeTable() {
  if (hashMap_)
    alignedFree(hashMap_);
}

size_t wc::ecs::ArchetypeTable::erase(EntityId id) {
  EntityId* keys = data<EntityId>();
  // Get the hash for this key.
  size_t hash = std::hash<EntityId> {}(id) % hashCap_;
  while (1) {
    uint32_t index = hashMap_[hash];
    if (index == INDEXNUL || index == INDEXDEL)
      return SIZE_MAX;
    if (keys[index] == id) {
      keys[index]    = {};
      hashMap_[hash] = INDEXDEL;
      return index;
    }
    hash = hashInc_(hash);
  }
  // This should never happen.
  return SIZE_MAX;
}

size_t wc::ecs::ArchetypeTable::find(EntityId id) {
  const EntityId* keys = data<EntityId>();
  // Get the hash for this key.
  size_t hash = std::hash<EntityId> {}(id) % hashCap_;
  // Figure out where it was put.
  while (1) {
    // Get the index for this hash value.
    uint32_t index = hashMap_[hash];
    // If the index is NUL or DEL, the entity is not in this table.
    if (index == INDEXNUL || index == INDEXDEL)
      return SIZE_MAX;
    // If the item at the index is our entity, we've found the spot.
    if (keys[index] == id)
      return (size_t)index;
    // Otherwise, keep looking.
    hash = hashInc_(hash);
  }
  // This should never happen.
  return SIZE_MAX;
}

size_t wc::ecs::ArchetypeTable::insert(EntityId id) {
  // Make sure we don't add an entry to the table more than once.
  if (exists(id))
    return SIZE_MAX;
  // Make sure the container is big enough to support another entry.
  if (size_ == capacity_)
    reserve(std::min((size_t)16, size_ * 2));

  // Get the hash for the key.
  size_t hash = std::hash<EntityId> {}(id) % hashCap_;
  // Look through the table for a place to put it...
  while (1) {
    uint32_t index = hashMap_[hash];
    if (index == INDEXNUL || index == INDEXDEL) {
      index          = (uint32_t)size_++;
      hashMap_[hash] = index;
      nullRow_(index);
      at<EntityId>(index) = id;
      return (size_t)index;
    }
    hashInc_(hash);
  }
  // This should never happen.
  return SIZE_MAX;
}

void wc::ecs::ArchetypeTable::nullRow_(size_t row) {
  for (auto [colSize, colPtr] : componentMap_.selectColumns<1, 2>()) {
    memset((void*)(colPtr + (row * colSize)), 0, colSize);
  }
}

void wc::ecs::ArchetypeTable::realloc_(size_t newCapacity) {
  // Always round capacity up to the nearest 16 bytes for alignment.
  newCapacity += (16 - (newCapacity % 16));
  char*        newData     = nullptr;
  const size_t newDataSize = sizePerRow_ * newCapacity;
  size_t       newHashCap  = 0;
  uint32_t*    newHashMap  = nullptr;
  // If the new capacity is non-zero, allocate some data for it.
  if (newCapacity > 0) {
    // Hash capacity needs to be odd and just greater than double the list
    // capacity, but it also needs to conform to 16-byte alignment.
    newHashCap               = newCapacity + newCapacity + 3;
    const size_t hashMapSize = (newHashCap + 1) * sizeof(uint32_t);
    // Allocate our data.
    newData    = (char*)alignedMalloc(16, newDataSize + hashMapSize);
    newHashMap = (uint32_t*)newData;
    newData += hashMapSize;
  }

  // If there was data there before,
  if (size_ > 0) {
    // Cap size_ to the new capacity.
    if (size_ > newCapacity)
      size_ = newCapacity;
    // If we have somewhere to copy the old data, copy it over.
    if (newData)
      memcpy(newData, data_, sizePerRow_ * size_);
    // Free the old buffer.
    alignedFree(hashMap_);
  }

  data_     = newData;
  capacity_ = newCapacity;
  hashMap_  = newHashMap;
  hashCap_  = newHashCap;

  // Fix the pointers in the column headers.
  char* runningPtr = data_;
  for (auto [colSize, colPtr] : componentMap_.selectColumns<1, 2>()) {
    if (capacity_ == 0)
      colPtr = nullptr;
    else if (colSize > 0) {
      colPtr = runningPtr;
      runningPtr += colSize * capacity_;
    }
  }

  // Fix the hashmap
  rehash_();
}

void wc::ecs::ArchetypeTable::rehash_() {
  memset(hashMap_, INDEXNUL, sizeof(uint32_t) * hashCap_);
  const EntityId* keys = data<EntityId>();
  for (size_t i = 0; i < size_; ++i) {
    // Get the hash for this key.
    size_t hash = std::hash<EntityId> {}(keys[i]) % hashCap_;
    // Figure out where to put it.
    while (1) {
      // If this spot is NULL or DELETED, we can put our reference here.
      uint32_t index = hashMap_[hash];
      if (index == INDEXNUL || index == INDEXDEL) {
        hashMap_[hash] = (uint32_t)i;
        break;
      }
      // Otherwise, keep looking.
      hash = hashInc_(hash);
    }
  }
}