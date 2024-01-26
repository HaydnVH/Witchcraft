#ifndef WC_ECS_IDTYPES_H
#define WC_ECS_IDTYPES_H

#include "tools/uuid.h"

#include <algorithm>

namespace wc::ecs {

  constexpr Uuid WC_ECS_PARENT_ID = wc::Uuid::makeV5({}, "WC_ECS_PARENT_ID");

  struct ComponentId: public wc::Uuid {
    constexpr ComponentId(Uuid id): Uuid(id) {}
    constexpr static Uuid WC_ECS_COMPONENT_ID =
        wc::Uuid::makeV5(WC_ECS_PARENT_ID, "WC_ECS_COMPONENT_ID");
  };
  inline constexpr ComponentId makeComponentId(std::string_view str) {
    return wc::Uuid::makeV5(ComponentId::WC_ECS_COMPONENT_ID, str);
  }

  struct EntityId: public wc::Uuid {
    constexpr EntityId(): Uuid({}) {}
    constexpr EntityId(Uuid id): Uuid(id) {}
    constexpr static ComponentId componentId = makeComponentId("EntityId");
  };
  inline EntityId createEntity() { return wc::Uuid::makeV4(); }

  struct ArchetypeId: wc::Uuid {
    constexpr ArchetypeId(Uuid id): Uuid(id) {}
    constexpr static Uuid WC_ECS_ARCHETYPE_ID =
        wc::Uuid::makeV5(WC_ECS_PARENT_ID, "WC_ECS_ARCHETYPE_ID");
  };
  inline constexpr ArchetypeId
      makeArchetypeId(std::vector<ComponentId> components) {
    std::sort(components.begin(), components.end());
    return wc::Uuid::makeV5(ArchetypeId::WC_ECS_ARCHETYPE_ID,
                            components.begin(), components.end());
  }

}  // namespace wc::ecs

template <>
struct std::hash<wc::ecs::ArchetypeId> {
  inline size_t operator()(const wc::ecs::ArchetypeId& id) const noexcept {
    return std::hash<wc::Uuid> {}(id);
  }
};

template <>
struct std::hash<wc::ecs::ComponentId> {
  inline size_t operator()(const wc::ecs::ComponentId& id) const noexcept {
    return std::hash<wc::Uuid> {}(id);
  }
};

template <>
struct std::hash<wc::ecs::EntityId> {
  inline size_t operator()(const wc::ecs::EntityId& id) const noexcept {
    return std::hash<wc::Uuid> {}(id);
  }
};

#endif  // WC_ECS_IDTYPES_H