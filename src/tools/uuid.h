#ifndef WC_TOOLS_UUID_H
#define WC_TOOLS_UUID_H

#include "base64.h"

#include <cstdint>

namespace wc {

  struct Uuid {
    /// Generates a V4 UUID using pure random numbers.
    /// The returned ID is about as universally unique as you can hope for.
    static Uuid makeV4Fast();
    static Uuid makeV4Secure();
    static Uuid makeV4Blake();

    /// Generates a V5 UUID by hashing a namespace and identifier.
    /// The same namespace and name will always produce the same ID.
    constexpr static Uuid makeV5(const char* ns, const char* name);

    /// Equality test between two UUIDs.
    inline constexpr friend bool operator==(Uuid lhs, Uuid rhs) noexcept {
      return ((lhs.lo_ == rhs.lo_) && (lhs.hi_ == rhs.hi_));
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
    inline constexpr std::string toStrBase64() const {
      return base64::encode((void*)this, sizeof(Uuid));
    }
    /// Converts a 22-character base64 string back into UUID.
    /// @return the UUID which was used to create the input string.
    inline constexpr static Uuid fromStrBase64(const std::string_view str) {
      auto bytes   = base64::decode(str);
      auto bytes64 = (uint64_t*)(void*)bytes.data();
      Uuid result {bytes64[0], bytes64[1]};
      return result;
    };

    uint64_t lo_, hi_;
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

#endif  // WC_TOOLS_UUID_H