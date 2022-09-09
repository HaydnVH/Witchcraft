#ifndef HVH_WC_ECS_SYSTEMS_SYSTEM_H
#define HVH_WC_ECS_SYSTEMS_SYSTEM_H

#include <vector>
#include <cstdint>

template <typename FirstComponentType, typename... RestComponentTypes>
class System: public System<RestComponentTypes...> {
public:
protected:
	System(FirstComponentType& first, RestComponentTypes... rest)
		:System(rest...),
		:component(first)
	{}

	FirstComponentType& component;
	std::vector<size_t> entity_indices;
	uint64_t lastUpdatedOn = 0;
};

#endif // HVH_WC_ECS_SYSTEMS_SYSTEM_H