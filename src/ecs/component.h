#ifndef HVH_WC_ECS_COMPONENT_H
#define HVH_WC_ECS_COMPONENT_H

#include "tools/htable.hpp"
#include "debug.h"

#include "entity.h"


template <typename...  DataTypes>
class Component {
public:
	bool Attach(entity::ID id, const DataTypes&... data) {
		if (!allowMultiple) {
			if (table.find(id) == SIZE_MAX) {
				table.insert(id, data...);
			}
			else {
				debug::error("In Component::Attach(", entity:toString(id), "):\n"));
				debug::errmore("This entity already has a '", componentName, "' attached.\n")
			}
		}
		else {
			table.insert(id, data...);
		}
	}

private:
	hvh::htable<entity::ID, DataTypes...> table;
	bool allowMultiple = false;
	const char* componentName;
};



class TagsComponent {
public:
private:
	hvh::htable<entity::ID, FixedString<32>> table;
	hvh::htable<FixedString<32>, hvh::htable<entity::ID>> tagTables;
};


#endif // HVH_WC_ECS_COMPONENT_H