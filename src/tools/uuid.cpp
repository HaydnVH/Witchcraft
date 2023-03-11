#include "uuid.h"

#include "rng.h"

#include <fmt/core.h>
#include <scn/scn.h>

wc::Uuid wc::Uuid::makeV4Fast() {
  // TODO: Right now, we're using a 64-bit RNG twice.
  // Ideally, it should use a 128-bit RNG.
  // rng::secure() gives us as many bytes as we want, but it's pretty slow.
  Uuid result;
  result.hi_ = (rng::fast() & 0xffffffffffff0fff) | 0x0000000000004000;
  result.lo_ = (rng::fast() & 0x3fffffffffffffff) | 0x8000000000000000;
  return result;
}

wc::Uuid wc::Uuid::makeV4Secure() {
  // TODO: Right now, we're using a 64-bit RNG twice.
  // Ideally, it should use a 128-bit RNG.
  // rng::secure() gives us as many bytes as we want, but it's pretty slow.
  Uuid result;
  rng::secureBytes(&result, sizeof(Uuid));
  result.hi_ = (result.hi_ & 0xffffffffffff0fff) | 0x0000000000004000;
  result.lo_ = (result.lo_ & 0x3fffffffffffffff) | 0x8000000000000000;
  return result;
}

wc::Uuid wc::Uuid::makeV4Blake() {
  Uuid result;
  rng::blake3(&result, sizeof(Uuid));
  result.hi_ = (result.hi_ & 0xffffffffffff0fff) | 0x0000000000004000;
  result.lo_ = (result.lo_ & 0x3fffffffffffffff) | 0x8000000000000000;
  return result;
}

constexpr wc::Uuid wc::Uuid::makeV5(const char* ns, const char* name) {
  return {};
}

std::string wc::Uuid::toStrCanon() const {
  // TODO: Figure out a way to make this constexpr?
  uint8_t* data = (uint8_t*)this;
  return fmt::format(
      "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-"
      "{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
      *(data++), *(data++), *(data++), *(data++), *(data++), *(data++),
      *(data++), *(data++), *(data++), *(data++), *(data++), *(data++),
      *(data++), *(data++), *(data++), *(data++));
}

wc::Uuid wc::Uuid::fromStrCanon(const std::string_view str) {
  // TODO: Figure out a way to make this constexpr?
  Uuid     result;
  uint8_t* data = (uint8_t*)&result;
  scn::scan(str,
            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-"
            "{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            *(data++), *(data++), *(data++), *(data++), *(data++), *(data++),
            *(data++), *(data++), *(data++), *(data++), *(data++), *(data++),
            *(data++), *(data++), *(data++), *(data++));
  return result;
}