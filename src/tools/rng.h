/******************************************************************************
 * rng.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * Provides access to random values for use by the Witchcraft engine.
 * Two different kinds of generators are provided: "secure" and "fast".
 *
 * A "secure" random number is generated using a cryptographically-secure
 * algorithm, and has all the properties of crytographic security.
 * "secure" random numbers have no seed input or trackable state.
 *
 * A "fast" random number is generated as fast as possible, quality be damned.
 * "fast" random numbers can be seeded and can have their state tracked
 * to generate deterministic but random-looking sequences.
 *****************************************************************************/
#ifndef WC_TOOLS_RNG_H
#define WC_TOOLS_RNG_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace wc::rng {

  /// Generates 'numBytes' bytes of secure random data and stores them in 'ptr'.
  void secureBytes(void* ptr, size_t numBytes);

  /// Generates a secure random 'T'.  Behavior depends on the properties of 'T'.
  /// If 'T' is an integer, the result will be between *_MIN and *_MAX.
  /// If 'T' is floating-point, the result will be between 0 and 1.
  template <typename T>
  inline T secure() {
    static_assert(std::is_integral<T>::value ||
                  std::is_floating_point<T>::value);

    if constexpr (std::is_integral<T>::value) {
      T result;
      secureBytes(&result, sizeof(T));
      return result;
    } else if constexpr (std::is_floating_point<T>::value) {
      uint32_t result;
      secureBytes(&result, sizeof(uint32_t));
      return (T)((double)result / (double)UINT_MAX);
    } else
      return {};
  }

  void blake3(void* ptr, size_t numBytes);

  struct Fast {
    constexpr Fast(uint64_t seed = secure<uint64_t>());

    uint64_t next();

    uint64_t    a, b, c, d;
    static Fast fastState_s;
  };

  inline uint64_t fast() { return Fast::fastState_s.next(); }

}  // namespace wc::rng

#endif  // WC_TOOLS_RNG_H