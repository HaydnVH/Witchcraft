#include "entity.h"

#include <chrono>

#include "tools/rng.h"

namespace {

} // namespace <anon>

namespace entity {

	using namespace std::chrono;

	RNG rng(time_point_cast<milliseconds>(system_clock().now()).time_since_epoch().count());

	ID create() {
		uint64_t timepart = time_point_cast<seconds>(system_clock().now()).time_since_epoch().count();
		uint32_t randpart = rng.next();
		uint64_t result = (timepart << 32) | (uint64_t)(randpart);
		return result;
	}

	void destroy(ID id) {
		if (id == 0) return;

		// TODO: Unlink this entity from every component.
	}

	const char* toString(ID id) {
		static char strbuffer[64];
		if (id == 0) snprintf(strbuffer, 64, "[0] (null)");
		else {
			snprintf(strbuffer, 64, "[%016llx] (no name)", (uint64_t)id);
		}
		return strbuffer;
	}

} // namespace entity