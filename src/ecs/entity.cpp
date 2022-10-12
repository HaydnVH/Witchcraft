#include "entity.h"

#include <chrono>

#include "tools/rng.h"
#include "tools/stringhelper.h"

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

	std::string toString(ID id) {
		std::string result;
		if (id == 0) {
			result = "[0] (null)";
		}
		else {
			result = makestr("[", hexid(id), "] (no name)");
		}
		return result;
	}

} // namespace entity