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

}  // namespace

#ifdef _WIN32  // on Windows, use BCrypt to generate our random number.
void wc::rng::secureBytes(void* ptr, size_t numBytes) {
  auto res = BCryptGenRandom(NULL, (unsigned char*)ptr, numBytes,
                             BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
#elif __linux__  // on Linux, read from /dev/urandom for our random number.
void wc::rng::secureBytes(void* ptr, size_t numBytes) {
  devRandom_s.read((char*)ptr, numBytes);
}
#endif

#include "blake3.h"

class Blake3 {
public:
  Blake3() {
    uint8_t key[BLAKE3_KEY_LEN];
    wc::rng::secureBytes(key, sizeof(key));
    blake3_hasher_init_keyed(&hasher, key);
  }

  blake3_hasher hasher;
  size_t        counter = 0;
};
static Blake3 blake3_s;

void wc::rng::blake3(void* ptr, size_t numBytes) {
  ++blake3_s.counter;
  blake3_hasher_update(&blake3_s.hasher, &blake3_s.counter, sizeof(size_t));
  blake3_hasher_finalize(&blake3_s.hasher, (uint8_t*)ptr, numBytes);
}

/*****************************************************************************
 * "fast" random.
 * Uses xoshiro256+ (https://prng.di.unimi.it/)
 * Chosen for maximum speed without regard for other factors.
 */

namespace {

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

constexpr wc::rng::Fast::Fast(uint64_t seed):
    a(splitmix64(seed)), b(splitmix64(seed)), c(splitmix64(seed)),
    d(splitmix64(seed)) {}

wc::rng::Fast wc::rng::Fast::fastState_s;

uint64_t wc::rng::Fast::next() {
  const uint64_t result = a + b;
  const uint64_t t      = b << 17;
  c ^= a;
  d ^= b;
  b ^= c;
  a ^= d;

  c ^= t;
  d = rotl(d, 45);

  return result;
}