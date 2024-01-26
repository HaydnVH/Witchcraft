#ifndef WC_ESC_COMPONENT_H
#define WC_ESC_COMPONENT_H

#include "idtypes.h"

#include <unordered_set>

namespace wc::ecs {

  struct Component {

    std::unordered_set<ArchetypeId> archetypes;
  };

}  // namespace wc::ecs

#endif  // WC_ESC_COMPONENT_H