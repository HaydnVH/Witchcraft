#ifndef HVH_WC_ECS_COMPONENTS_NAME_H
#define HVH_WC_ECS_COMPONENTS_NAME_H

#include "../entity.h"
#include "tools/htable.hpp"
#include "tools/fixedstring.h"

class NameComponent {
public:
	void setName(entity::ID id, FixedString<32> name) {
		removeName(id);
		table.insert(id, name);
		lookup.insert(name, id);
	}

	FixedString<32> getName(entity::ID id) {
		size_t table_index = table.find(id);
		if (table_index == SIZE_MAX) {
			return "";
		}
		else {
			return table.at<1>(table_index);
		}
	}

	bool hasName(entity::ID id) {
		return (table.find(id) != SIZE_MAX);
	}

	void removeName(entity::ID id) {
		size_t table_index = table.find(id);
		if (table_index == SIZE_MAX) return;
		for (size_t lookup_index = lookup.find(id, true); lookup_index != SIZE_MAX; lookup_index = lookup.find(id, false)) {
			if (lookup.at<1>(lookup_index) == id) {
				lookup.erase_found();
				break;
			}
		}
		table.erase_found();
	}

	entity::ID findWithName(FixedString<32> name, bool restart = true) {
		size_t table_index = lookup.find(name, restart);
		if (table_index == SIZE_MAX) return 0;
		return lookup.get<1>(table_index);
	}

private:
	hvh::htable<entity::ID, FixedString<32>> table;
	hvh::htable<FixedString<32>, entity::ID> lookup;
};

#endif // HVH_WC_ECS_COMPONENTS_NAME_H