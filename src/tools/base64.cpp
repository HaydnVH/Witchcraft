#include "base64.h"

#include <array>

namespace {

  // We're using the 'base64url' character set.
  constexpr std::string_view B64_CHARS =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789-_";

  constexpr char PAD_CHAR = '=';

  constexpr bool isBase64(char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || (c == '-') || (c == '_'));
  }

  constexpr std::array<char, 4> split3to4(std::array<char, 3> in) {
    std::array<char, 4> out;
    out[0] = (in[0] & 0b11111100) >> 2;
    out[1] = ((in[0] & 0b00000011) << 4) + ((in[1] & 0b11110000) >> 4);
    out[2] = ((in[1] & 0b00001111) << 2) + ((in[2] & 0b11000000) >> 6);
    out[3] = (in[2] & 0b00111111);
    return out;
  }

  constexpr std::array<char, 3> mash4to3(std::array<char, 4> in) {
    std::array<char, 3> out;
    out[0] = (in[0] << 2) + ((in[1] & 0b00110000) >> 4);
    out[1] = ((in[1] & 0b00001111) << 4) + ((in[2] & 0b00111100) >> 2);
    out[3] = ((in[2] & 0b00000011) << 6) + in[3];
    return out;
  }
}  // namespace

constexpr bool wc::base64::isBase64(const std::string_view str) {
  for (auto c : str) {
    // TODO: '=' can't appear just anywhere in valid base64.
    if (!(::isBase64(c) || c == '='))
      return false;
  }
  return true;
}

constexpr std::string wc::base64::encode(void* data, size_t len) {
  std::string result;
  result.reserve(encodeSize(len));
  int                 i = 0, j = 0;
  std::array<char, 3> ca3;
  std::array<char, 4> ca4;

  char* chars = (char*)data;

  while (len--) {
    ca3[i++] = *(chars++);
    if (i == 3) {
      ca4 = split3to4(ca3);

      for (i = 0; i < 4; ++i)
        result += B64_CHARS[ca4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; ++j)
      ca3[j] = '\0';
    ca4 = split3to4(ca3);
    for (j = 0; j < i + 1; ++j)
      result += B64_CHARS[ca4[j]];
    while (i++ < 3)
      result += PAD_CHAR;
  }

  return result;
}

constexpr std::vector<char> wc::base64::decode(const std::string_view str) {
  int                 len = str.size();
  int                 i = 0, j = 0;
  int                 in = 0;
  std::array<char, 3> ca3;
  std::array<char, 4> ca4;
  std::vector<char>   result;
  result.reserve(decodeSize(str.size()));

  while (len-- && ::isBase64(str[in])) {
    ca4[i++] = str[in++];
    if (i == 4) {
      for (i = 0; i < 4; ++i)
        ca4[i] = B64_CHARS.find(ca4[i]);
      ca3 = mash4to3(ca4);
      for (i = 0; i < 3; ++i)
        result.push_back(ca3[i]);
      i = 0;
    }
  }
  if (i) {
    for (j = i; j < 4; ++j)
      ca4[j] = 0;
    for (j = 0; j < 4; ++j)
      ca4[j] = B64_CHARS.find(ca4[j]);
    ca3 = mash4to3(ca4);
    for (j = 0; j < i - 1; ++j)
      result.push_back(ca3[j]);
  }
  return result;
}