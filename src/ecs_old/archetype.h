#ifndef WC_ECS_ARCHETYPE_H
#define WC_ECS_ARCHETYPE_H

#include "idtypes.h"
#include "tools/htable.hpp"
#include "tools/uuid.h"

#include <stdexcept>
#include <unordered_map>

namespace wc::ecs {

  // Maps a component ID to the size of that component.
  using ComponentRegistry = std::unordered_map<ComponentId, size_t>;

  class ArchetypeTable {
  public:
    ArchetypeTable(const ComponentRegistry& registry,
                   std::vector<ComponentId> components);
    ~ArchetypeTable();

    // swap(lhs, rhs)
    // swaps the contents of two htables.
    // Complexity: O(1).
    friend inline void swap(ArchetypeTable& lhs, ArchetypeTable& rhs) {
      swap(lhs.componentMap_, rhs.componentMap_);
      std::swap(lhs.sizePerRow_, rhs.sizePerRow_);
      std::swap(lhs.size_, rhs.size_);
      std::swap(lhs.capacity_, rhs.capacity_);
      std::swap(lhs.hashMap_, rhs.hashMap_);
      std::swap(lhs.hashCap_, rhs.hashCap_);
    }

    bool hasComponent(ComponentId id) {
      return componentMap_.find(id)->exists();
    }

    size_t size() { return size_; }
    size_t capacity() { return capacity_; }

    template <typename T>
    inline T* data() {
      auto column = componentMap_.find(T::componentId);
      if (!column->exists() || column->template get<2>() == nullptr)
        throw std::invalid_argument("Bad column.");
      return (T*)column->template get<2>();
    }

    template <typename T>
    inline const T* data() const {
      auto column = componentMap_.find(T::componentId);
      if (!column.exists() || column->template get<2>() == nullptr)
        throw std::invalid_argument("Bad column.");
      return (const T*)column->template get<2>();
    }

    template <typename T>
    inline T& at(size_t row) {
      if (row >= size_)
        throw std::out_of_range("Index out of range.");
      return data<T>()[row];
    }

    template <typename T>
    inline const T& at(size_t row) const {
      if (row >= size_)
        throw std::out_of_range("Index out of range.");
      return data<T>()[row];
    }

    inline void reserve(size_t newCapacity) {
      if (newCapacity > capacity_)
        realloc_(newCapacity);
    }

    inline void shrinkToFit() {
      if ((size_ + (16 - (size_ % 16))) < capacity_)
        realloc_(size_);
    }

    size_t      erase(EntityId id);
    inline bool exists(EntityId id) { return find(id) != SIZE_MAX; }
    size_t      find(EntityId id);
    size_t      insert(EntityId id);

    template <typename... Ts>
    inline auto makeSelectedRowTuple(size_t) const {
      return std::tuple<>();
    }

    template <typename FT, typename... RTs>
    inline auto makeSelectedRowTuple(size_t row) {
      return std::tuple_cat(std::make_tuple(std::ref(at<FT>(row))),
                            makeSelectedRowTuple<RTs...>(row));
    }

    template <typename FT, typename... RTs>
    inline auto makeSelectedRowTuple(size_t row) const {
      return std::tuple_cat(std::make_tuple(std::cref(at<FT>(row))),
                            makeSelectedRowTuple<RTs...>(row));
    }

  private:
    static constexpr uint32_t INDEXNUL = UINT_MAX;
    static constexpr uint32_t INDEXDEL = UINT_MAX - 1;

    inline size_t hashInc_(size_t& h) const { return ((h + 2) % hashCap_); }
    void          nullRow_(size_t row);
    void          realloc_(size_t newCapacity);
    void          rehash_();

    // This table maps a component ID to the size of its type and a pointer to
    // its column within the data. Types with a size of zero are allowed, they
    // will simply not be backed by actual data.
    hvh::Table<ComponentId, size_t, char*> componentMap_;
    size_t                                 sizePerRow_;

    // Here's where we store the data for the actual table itself.
    // At this point, we're mimicing how soa/htable work,
    // except that types are not explicit.
    char*     data_     = nullptr;
    size_t    size_     = 0;
    size_t    capacity_ = 0;
    uint32_t* hashMap_  = nullptr;
    size_t    hashCap_  = 0;
  };

}  // namespace wc::ecs

#endif