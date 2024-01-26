#ifndef WC_ECS_REGISTRY_H
#define WC_ECS_REGISTRY_H

#include "archetype.h"
#include "component.h"
#include "entity.h"
#include "idtypes.h"
#include "tools/uuid.h"

#include <algorithm>
#include <functional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace wc::ecs {

  class Registry {
  public:
    Registry() {}
    Registry(const Registry&) = delete;
    Registry(Registry&& other) { swap(*this, other); }
    ~Registry() {}

    friend inline void swap(Registry& lhs, Registry& rhs) {
      swap(lhs.archetypeRegistry, rhs.archetypeRegistry);
      std::swap(lhs.componentRegistry, rhs.componentRegistry);
      std::swap(lhs.entityRegistry, rhs.entityRegistry);
    }

  private:
    // Archetype needs a dedicated specialized data structure.
    // It should be a two-dimensional hashtable,
    // accepting ComponentIds to select column, and EntityIds to select row.
    // Given a type T, it should return a T* or T& corresponding to that column.

    // We register every archetype.
    hvh::Table<ArchetypeId, ArchetypeTable> archetypeRegistry;
    // Each component refers to a set of archetypes which contain that
    // component.
    using ArchetypeSet = std::unordered_set<ArchetypeId>;
    std::unordered_map<ComponentId, ArchetypeSet> componentRegistry;
    // Each entity refers to its archetype.
    std::unordered_map<EntityId, ArchetypeId> entityRegistry;

    // Return a list of every archetype which has all of the components in the
    // requested list. O(mn), where n is the size of `components`, and m is the
    // number of archetypes which contain the element of `components` with the
    // fewest archetypes. The archetype struct will need to support the
    // "selectColumns" interface from soa!
    std::vector<ArchetypeId> query(const std::vector<ComponentId>& components) {
      std::vector<ArchetypeId> result;
      if (components.size() == 0)
        return result;

      // Find the component which has the shortest list of archetypes.
      size_t shortest      = SIZE_MAX;
      size_t whereShortest = 0;
      for (size_t i = 0; i < components.size(); ++i) {
        size_t numArchetypes = componentRegistry[components[i]].size();
        if (numArchetypes < shortest) {
          shortest      = numArchetypes;
          whereShortest = i;
        }
      }

      // Go through each archetype which has the component with the shortest
      // list of archetypes.
      for (auto& a : componentRegistry[components[whereShortest]]) {
        // Go through each requested component
        bool match = true;
        for (auto& c : components) {
          if (c.isNot()) {
            // If the excluded component IS in this archetype, skip it.
            if (archetypeRegistry.find(a)->get<1>().hasComponent(~c) == true) {
              match = false;
              break;
            }
          } else {
            // If the desired component is NOT in this archetype, skip it.
            if (archetypeRegistry.find(a)->get<1>().hasComponent(c) == false) {
              match = false;
              break;
            }
          }
        }
        if (match)
          result.push_back(a);
      }
      return result;
    }

    template <typename... OutTypes, typename... InTypes>
    void runSystem(std::function<std::tuple<OutTypes...>(InTypes...)> func) {
      std::vector<ComponentId> components;
      (components.push_back(OutTypes::componentId), ...);
      (components.push_back(InTypes::componentId), ...);
      auto archetypes = query(components);
      for (auto& archetypeId : archetypes) {
        auto& archetype = archetypeRegistry.find(archetypeId)->get<1>();
        // We are now in pseudocode territory.
        // Iterate over every entity in the archetype.
        for (size_t i = 0; i < archetype.size(); ++i) {
          // Get a const reference tuple to each of our input types.
          const auto inTuple = archetype.makeSelectedRowTuple<InTypes...>(i);
          // Get a non-const reference tuple to each of our output types.
          auto outTuple = archetype.makeSelectedRowTuple<OutTypes...>(i);
          // Execute the system on this entity.
          outTuple = std::apply(func, inTuple);
        }
      }
    }
  };

}  // namespace wc::ecs

#endif  // WC_ECS_REGISTRY_H