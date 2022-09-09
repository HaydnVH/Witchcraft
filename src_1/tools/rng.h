#ifndef HVH_TOOLKIT_RNG_H
#define HVH_TOOLKIT_RNG_H

#include <cstdint>
#include <climits>

class RNG {
public:
	RNG(uint64_t seed) {
		uint64_t tmp = splitmix64(seed);
		a = (uint32_t)tmp;
		b = (uint32_t)(tmp >> 32);
		tmp = splitmix64(seed);
		c = (uint32_t)tmp;
		d = (uint32_t)(tmp >> 32);
	}

	uint32_t next() {
		// Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs"
		uint32_t t = d;
		uint32_t s = a;
		d = c;
		c = b;
		b = s;

		t ^= t >> 2;
		t ^= t << 1;
		t ^= s ^ (s << 4);
		a = t;

		counter += 362437;
		return t + counter;
	}

	constexpr static inline uint32_t MAX() { return UINT_MAX; }

private:

	// Only used to initialize the state when a new RNG is created.
	uint64_t splitmix64(uint64_t& seed) {
		uint64_t result = seed;
		seed = result + 0x9E3779B97f4A7C15;
		result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
		result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
		return result ^ (result >> 31);
	}

	uint32_t a, b, c, d;
	uint32_t counter = 0;
};

#endif // HVH_TOOLKIT_RNG_H