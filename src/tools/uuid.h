#ifndef WC_TOOLS_UUID_H
#define WC_TOOLS_UUID_H

#include "base64.h"
#include "rng.h"
#include "sha1.hpp"

#include <cstdint>
#include <string_view>

namespace wc {

  struct Uuid {

    /// Generates a Nil UUID (all 0s).
    inline constexpr static Uuid makeNil() { return {}; }

    /// Generates a V4 UUID using pure random numbers.
    /// The returned ID is about as universally unique as you can hope for.
    static Uuid makeV4() {
      // TODO: Right now, we're using a 64-bit RNG twice.
      // Ideally, it should use a 128-bit RNG.
      // The static RNG uses a 256-bit entropy seed so it should be fine?
      Uuid result {Rng::staticNext(), Rng::staticNext()};
      result.setVersionBits(4);
      return result;
    }

    /// Generates a V5 UUID by hashing a namespace and some data.
    /// The same namespace and data will always produce the same ID.
    template <typename T>
    constexpr static Uuid makeV5(Uuid ns, const T& data) {
      wc::hash::Sha1 checksum;
      checksum.update(ns);
      checksum.update(data);
      auto myhash = checksum.final();
      Uuid result {((uint64_t)myhash[0] << 32 | (uint64_t)myhash[1]),
                   ((uint64_t)myhash[2] << 32 | (uint64_t)myhash[3])};
      result.setVersionBits(5);
      return result;
    }

    template <typename It>
    constexpr static Uuid makeV5(Uuid ns, It begin, It end) {
      wc::hash::Sha1 checksum;
      checksum.update(ns);
      checksum.update(begin, end);
      auto myhash = checksum.final();
      Uuid result {((uint64_t)myhash[0] << 32 | (uint64_t)myhash[1]),
                   ((uint64_t)myhash[2] << 32 | (uint64_t)myhash[3])};
      result.setVersionBits(5);
      return result;
    }

    /// Equality test between two UUIDs.
    inline constexpr friend bool operator==(Uuid lhs, Uuid rhs) noexcept {
      return ((lhs.lo_ == rhs.lo_) && (lhs.hi_ == rhs.hi_));
    }

    /// Less-than test between two UUIDs for sorting.
    inline constexpr friend bool operator<(Uuid lhs, Uuid rhs) noexcept {
      return (lhs.hi_ < lhs.hi_) ||
             ((lhs.hi_ == rhs.hi_) && (lhs.lo_ < rhs.lo_));
    }

    /// Bitwise NOT operator
    /// Inverts the id.  An inverted id used in a query indicates
    /// that this id is not wanted and should be excluded.
    inline constexpr Uuid operator~() const noexcept {
      // Since the "version" halfbyte is always either 4 or 5,
      // it's easy to detect when the bits of a UUID have been reversed.
      return {~lo_, ~hi_};
    }

    /// @return true if this UUID has been inverted.
    inline constexpr bool isNot() const noexcept {
      // In a proper (non-Not) id, the version halfbyte is either 4 (0100) or 5
      // (0101). If the id has been flipped, that halfbyte will be either B
      // (1011) or A (1010). We'll use the 4th bit of the version halfbyte,
      // checked with & 8 (1000). This will work even if V6 or v7 UUIDs are
      // adopted, but V8 would cause problems.
      return ((hi_ & 0x0000000000008000) != 0);
    }

    /// Converts a UUID to a canonical string representation:
    /// "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    /// @return a 36-character string which represents the UUID.
    std::string toStrCanon() const;

    /// Converts a canonical string representation back to a UUID.
    /// @return the UUID which created the string.
    static Uuid fromStrCanon(const std::string_view str);

    /// Converts a UUID to a base64 string encoding.
    /// @return a 22-character string which can be decoded to the UUID.
    inline std::string toStrBase64() const {
      return base64::encode((const char*)this, sizeof(Uuid));
    }
    /// Converts a 22-character base64 string back into UUID.
    /// @return the UUID which was used to create the input string.
    inline static Uuid fromStrBase64(const std::string_view str) {
      auto bytes   = base64::decode(str);
      auto bytes64 = (uint64_t*)(void*)bytes.data();
      Uuid result {bytes64[0], bytes64[1]};
      return result;
    };

    uint64_t lo_ = 0, hi_ = 0;

  private:
    inline constexpr void setVersionBits(unsigned char version) {
      hi_ = (hi_ & 0xffffffffffff0fff) | ((uint64_t)(version & 0x0f) << 12);
      lo_ = (lo_ & 0x3fffffffffffffff) | 0x8000000000000000;
    }
  };

}  // namespace wc

/// Template specialization so wc::Uuid can be used with std::hash,
/// allowing it to be used as a hashtable key.
template <>
struct std::hash<wc::Uuid> {
  // TODO: Maybe something more "proper" than just bitwise xor?
  size_t operator()(const wc::Uuid& id) const noexcept {
    return (size_t)(id.lo_ ^ id.hi_);
  }
};

namespace wc::test {
  int runUuidUnitTests();
}

#endif  // WC_TOOLS_UUID_H