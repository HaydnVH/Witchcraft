/******************************************************************************
 * FixedString.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2019 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * A string which contains exactly the specified number of bytes.
 * By union'ing the characters of the string with a series of 64-bit integers,
 * it becomes trivial to perform operations such as comparisons and hashing.
 *****************************************************************************/
#ifndef HVH_TOOLKIT_FIXEDSTRING_H
#define HVH_TOOLKIT_FIXEDSTRING_H

#include <cstdint>
#include <cstring>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>

template <size_t LEN>
struct FixedString {

  static_assert(LEN > 0, "Length of a fixed string cannot be 0.");
  static_assert((LEN % 8) == 0,
                "Length of a fixed string must be a multiple of 8.");
  static constexpr const size_t NUMINTS = LEN / 8;

  union {
    char     c_str[LEN];
    char8_t  u8_str[LEN];
    uint64_t raw[NUMINTS];
  };

  FixedString()                   = default;
  FixedString(const FixedString&) = default;
  FixedString(FixedString&&)      = default;

  FixedString(const char* arg) {
    strncpy(c_str, arg, LEN - 1);
    c_str[LEN - 1] = '\0';
  }

  FixedString(const char8_t* arg) {
    strncpy(u8_str, arg, LEN - 1);
    u8_str[LEN - 1] = '\0';
  }

  FixedString(const std::string_view arg) {
    strncpy(c_str, std::string(arg).c_str(), LEN - 1);
    c_str[LEN - 1] = '\0';
  }

  FixedString& operator=(const FixedString&) = default;
  FixedString& operator=(FixedString&&)      = default;

  inline bool operator==(const FixedString& rhs) const {
    for (size_t i = 0; i < NUMINTS; ++i) {
      if (raw[i] != rhs.raw[i]) return false;
    }
    return true;
  }

  inline bool operator!=(const FixedString& rhs) const {
    return (!(*this == rhs));
  }

  // Warning: This is not a lexicographical comparison!
  // It is deterministic, but has nothing to do with the characters of the
  // string.
  inline bool operator<(const FixedString& rhs) const {
    for (size_t i = 0; i < NUMINTS; ++i) {
      if (raw[i] < rhs.raw[i]) return true;
      if (raw[i] > rhs.raw[i]) return false;
    }
    return false;
  }

  friend void swap(FixedString& lhs, FixedString& rhs) {
    for (size_t i = 0; i < NUMINTS; ++i) { std::swap(lhs.raw[i], rhs.raw[i]); }
  }
};

// Specialization so we can use FixedString with std::hash.
// TODO: Better hash function?
namespace std {
  template <size_t LEN>
  struct hash<FixedString<LEN>> {
    size_t operator()(const FixedString<LEN>& x) const {
      uint64_t result = 0;
      for (size_t i = 0; i < FixedString<LEN>::NUMINTS; ++i) {
        result += x.raw[i];
      }
      return (size_t)result;
    }
  };
}  // namespace std

// Overloading the '<<' operator so fixed strings can easily be used with cout.
template <size_t LEN>
inline std::ostream& operator<<(std::ostream& os, FixedString<LEN> str) {
  os << str.c_str;
  return os;
}

#endif