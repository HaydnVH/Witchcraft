/* fixedstring.h
 * A string which contains exactly the specified number of bytes.
 * by Haydn V. Harach
 * October 2019
 *
 * By union'ing the characters of the string with a series of 64-bit integers,
 * it becomes trivial to perform operations such as comparisons and hashing.
 */
#ifndef HVH_TOOLKIT_FIXEDSTRING_H
#define HVH_TOOLKIT_FIXEDSTRING_H

#include <cstdint>
#include <cstring>
#include <functional>

template <size_t LEN>
struct fixedstring {

	static_assert(LEN > 0, "Length of a fixed string cannot be 0.");
	static_assert((LEN % 8) == 0, "Length of a fixed string must be a multiple of 8.");
	static constexpr const size_t NUMINTS = LEN / 8;

	union {
		char c_str[LEN];
		uint64_t raw[NUMINTS];
	};

	fixedstring() = default;
	fixedstring(const fixedstring&) = default;
	fixedstring(fixedstring&&) = default;

	fixedstring(const char* arg) {
		strncpy(c_str, arg, LEN-1);
		c_str[LEN-1] = '\0';
	}

	fixedstring& operator = (const fixedstring&) = default;
	fixedstring& operator = (fixedstring&&) = default;

	inline bool operator == (const fixedstring& rhs) const {
		for (size_t i = 0; i < NUMINTS; ++i) {
			if (raw[i] != rhs.raw[i])
				return false;
		}
		return true;
	}

	inline bool operator != (const fixedstring& rhs) const {
		return (!(*this == rhs));
	}

	// Warning: This is not a lexicographical comparison!
	// It is deterministic, but has nothing to do with the characters of the string.
	inline bool operator < (const fixedstring& rhs) const {
		for (size_t i = 0; i < NUMINTS; ++i) {
			if (raw[i] < rhs.raw[i])
				return true;
			if (raw[i] > rhs.raw[i])
				return false;
		}
		return false;
	}

	friend void swap(fixedstring& lhs, fixedstring& rhs) {
		for (size_t i = 0; i < NUMINTS; ++i) {
			std::swap(lhs.raw[i], rhs.raw[i]);
		}
	}
};

// Specialization so we can use fixedstring with std::hash.
namespace std {
	template <size_t LEN>
	struct hash<fixedstring<LEN>> {
		size_t operator()(const fixedstring<LEN>& x) const {
			uint64_t result = 0;
			for (size_t i = 0; i < fixedstring<LEN>::NUMINTS; ++i) {
				result += x.raw[i];
			}
			return (size_t)result;
		}
	};
}

// Overloading the '<<' operator so fixed strings can easily be used with cout.
template <size_t LEN>
inline std::ostream& operator << (std::ostream& os, fixedstring<LEN> str) {
	os << str.c_str;
	return os;
}

#endif