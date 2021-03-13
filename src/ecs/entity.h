#ifndef HVH_WC_ECS_ENTITY_H
#define HVH_WC_ECS_ENTITY_H

#include <cstdint>

namespace entity {
	typedef uint64_t ID;

	ID create();
	void destroy(ID id);

	const char* toString(ID id);

	bool init();
}

#endif // HVH_WC_ECS_ENTITY_H