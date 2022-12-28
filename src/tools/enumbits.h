#ifndef WC_TOOLS_ENUMBITS_H
#define WC_TOOLS_ENUMBITS_H

/// This macro adds operator overloads so an enum class can be used as a
/// proper bitfield.
#define ENUM_BITFIELD_OPERATORS(T)                                        \
  inline T operator&(T lhs, T rhs) {                                      \
    return static_cast<T>(static_cast<int>(lhs) & static_cast<int>(rhs)); \
  }                                                                       \
  inline T operator|(T lhs, T rhs) {                                      \
    return static_cast<T>(static_cast<int>(lhs) | static_cast<int>(rhs)); \
  }                                                                       \
  inline T operator^(T lhs, T rhs) {                                      \
    return static_cast<T>(static_cast<int>(lhs) ^ static_cast<int>(rhs)); \
  }                                                                       \
  inline T operator<<(T lhs, int rhs) {                                   \
    return static_cast<T>(static_cast<int>(lhs) << rhs);                  \
  }                                                                       \
  inline T operator>>(T lhs, int rhs) {                                   \
    return static_cast<T>(static_cast<int>(lhs) >> rhs);                  \
  }                                                                       \
  inline bool operator!(T in) {                                           \
    return static_cast<int>(in) == 0;                                     \
  }

#endif // WC_TOOLS_ENUMBITS_H