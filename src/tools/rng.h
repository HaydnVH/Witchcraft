/******************************************************************************
 * rng.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * Provides access to random values for use by the Witchcraft engine.
 *
 *****************************************************************************/
#ifndef WC_TOOLS_RNG_H
#define WC_TOOLS_RNG_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace wc {

  class Rng {
  public:
    /// Default constructor for Rng fills the state with random entropy.
    /// This is technically not the same as using `getEntropy()` for the seed.
    Rng() { getEntropy(this, sizeof(Rng)); }
    /// Construct an `Rng` object and initializes it with the given seed.
    Rng(uint64_t seed);
    Rng(std::string_view seed):
        Rng((uint64_t)std::hash<std::string_view> {}(seed)) {}

    /// Get the next random number for this generator.
    /// @return 8 bytes of random data.
    uint64_t next();

    /// Get the next random number from a global seedless generator.
    /// @return 8 bytes of random data.
    inline static uint64_t staticNext() {
      static Rng rng_s;
      return rng_s.next();
    }

    /// Obtains truly random data from an OS-provided source of entropy.
    /// @param ptr: Where the random bytes should be stored.
    /// @param size: The number of random bytes to obtain.
    static void getEntropy(void* ptr, size_t size);
    /// Obtains truly random data from an OS-provided source of entropy.
    /// @return 8 bytes of random data.
    inline static uint64_t getEntropy() {
      uint64_t result;
      getEntropy(&result, sizeof(uint64_t));
      return result;
    }

  private:
    uint64_t a_, b_, c_, d_;
  };

}  // namespace wc

#endif  // WC_TOOLS_RNG_H