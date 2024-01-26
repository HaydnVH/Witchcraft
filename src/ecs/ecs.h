#ifndef WC_ECS_ECS_H
#define WC_ECS_ECS_H

#include <any>
#include <typeinfo>
#include <vector>
#include "tools/uuid.h"
#include "tools/htable.hpp"

namespace wc {
  
  using EntityId    = Uuid;  // v4
  using ComponentId = Uuid;  // v5
  using ArchetypeId = Uuid;  // v0

  // This UUID is used as the namespace when creating V5 UUIDs for components.
  static constexpr Uuid COMPONENT_ID_NS =
      Uuid::makeV5(Uuid::makeNil(), "WitchcraftComponentUuidNamespace");

  // Given a type T, get the ID for that type.
  template <typename T>
  inline ComponentId makeComponentId() {
    // TODO: typeid(T).name() returns different strings based on platform.
    // this MUST be changed so the same type produces the same ID on EVERY platform.
    return Uuid::makeV5(COMPONENT_ID_NS, typeid(T).name());
  }

  // Given a collection of ComponentId's, create an ArchetypeId.
  // This function produces the same result regardless of the order of component IDs.
  template <typename It>
  inline ArchetypeId makeArchetypeId(It begin, It end) {
    return Uuid::merge(begin, end);
  }

  struct Archetype {
    // The components which make up this archetype.
    hvh::Table<ComponentId, size_t> componentSet;
    // "any", in this case, will become an "std::vector<T>".
    std::vector<std::any>   componentData;
  };

  // Given an archetype id, get the components that make up that archetype.
  static hvh::Table<ArchetypeId, Archetype> archetypeLookup;

  // Information stored per-entity.
  struct Record {
    // Which archetype this entity belongs to.
    ArchetypeId archetype;
    // Which row on the archetype's table does this entity reside?
    size_t      row;
  };
  static hvh::Table<EntityId, Record> entityRecordLookup;

  // Find the archetypes that contain a given component.
  using ArchetypeSet = hvh::Table<ArchetypeId>;
  static hvh::Table<ComponentId, ArchetypeSet> componentArchetypeLookup;

  // Determine whether an entity has an indicated component.
  template <typename T>
  inline bool hasComponent(EntityId entity) {
    // Get the ArchetypeId for this entity.
    auto record = entityRecordLookup.find(entity);
    if (!record->exists()) return false;
    // Get the Archetype for this ArchetypeId.
    auto archetype = archetypeLookup.find(record->get<1>().archetype);
    if (!archetype->exists()) return false;
    // See if the queried component is in the archetype.
    return (archetype->get<1>().componentSet.contains(makeComponentId<T>()));
  }

  // Get a pointer to the indicated component on the given entity.
  // Returns NULL on error or if the entity doesn't have that component.
  template <typename T>
  inline T* getComponent(EntityId entity) {
    auto foundRecord = entityRecordLookup.find(entity);
    if (!foundRecord->exists()) return nullptr; // This can happen if the ID isn't actually in the scene.
    auto record    = foundRecord->get<1>();

    auto foundArchetype = archetypeLookup.find(record.archetype);
    if (!foundArchetype->exists()) return nullptr; // This should never happen.
    auto archetype = foundArchetype->get<1>();

    auto componentId    = makeComponentId<T>();
    auto foundComponent = archetype.componentSet.find(componentId);
    if (!foundComponent->exists()) return nullptr; // This can happen if the entity doesn't have the desired component.
    size_t componentIndex = foundComponent->get<1>();

    std::vector<T>* column = std::any_cast<std::vector<T>>(&archetype.componentData[componentIndex]);
    if (!column) return nullptr; // This should never happen.
    if (record.row >= column->size()) return nullptr; // This should never happen.
    return &column->at(record.row);
  }

}  // namespace wc

#endif // WC_ECS_ECS_H