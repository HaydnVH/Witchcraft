/******************************************************************************
 * rng.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * Implements the random number generators.
 *****************************************************************************/
#include "rng.h"

/*****************************************************************************
 * "secure" random.
 * Uses BCrypt on Windows, and /dev/urandom on linux.
 * Designed for cryptographic safety without regards for speed.
 */

#ifdef _WIN32
#include <Windows.h>
#include <bcrypt.h>
#elif __linux__
#include <fstream>
#endif

namespace {

#ifdef __linux__
  // On Linux, we use '/dev/urandom' to generate
  // cryptographically secure random numbers.
  std::ifstream devRandom_s("/dev/urandom", std::ios::in | std::ios::binary);
#endif

  inline constexpr uint64_t rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
  }

  inline constexpr uint64_t splitmix64(uint64_t& seed) {
    uint64_t result = seed;
    seed            = result + 0x9E3779B97f4A7C15;
    result          = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
    result          = (result ^ (result >> 27)) * 0x94D049BB133111EB;
    return result ^ (result >> 31);
  }

}  // namespace

wc::Rng::Rng(uint64_t seed):
    a_(splitmix64(seed)), b_(splitmix64(seed)), c_(splitmix64(seed)),
    d_(splitmix64(seed)) {}

uint64_t wc::Rng::next() {
  // Uses xoshiro256+ (https://prng.di.unimi.it/)
  // Chosen for maximum speed without regard for other factors.
  const uint64_t result = a_ + b_;
  const uint64_t t      = b_ << 17;
  c_ ^= a_;
  d_ ^= b_;
  b_ ^= c_;
  a_ ^= d_;

  c_ ^= t;
  d_ = rotl(d_, 45);

  return result;
}

#ifdef _WIN32  // on Windows, use BCrypt to generate our random number.
void wc::Rng::getEntropy(void* ptr, size_t numBytes) {
  auto res = BCryptGenRandom(NULL, (unsigned char*)ptr, numBytes,
                             BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
#elif __linux__  // on Linux, read from /dev/urandom for our random number.
void wc::Rng::getEntropy(void* ptr, size_t numBytes) {
  devRandom_s.read((char*)ptr, numBytes);
}
#endif