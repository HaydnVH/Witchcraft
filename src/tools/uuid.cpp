#include "uuid.h"

#include <format>
#include <scn/scn.h>

std::string wc::Uuid::toStrCanon() const {
  // TODO: Figure out a way to make this constexpr?
  uint8_t* data = (uint8_t*)this;
  return std::format(
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
  auto     _    = scn::scan(str,
                            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-"
                                   "{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                            *(data++), *(data++), *(data++), *(data++), *(data++),
                            *(data++), *(data++), *(data++), *(data++), *(data++),
                            *(data++), *(data++), *(data++), *(data++), *(data++),
                            *(data++));
  return result;
}