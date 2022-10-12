#ifndef HVH_WC_ECS_ENTITY_H
#define HVH_WC_ECS_ENTITY_H

#include <cstdint>
#include <string>
#include <sstream>

namespace entity {
	typedef uint64_t ID;

	ID create();
	void destroy(ID id);

	inline std::string hexid(ID id) {
		std::stringstream ss;
		ss << std::hex << id;
		return ss.str();
	}

	std::string toString(ID id);

	bool initLua();
}

#endif // HVH_WC_ECS_ENTITY_H