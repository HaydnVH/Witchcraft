#ifndef WC_TOOLS_BASE64_H
#define WC_TOOLS_BASE64_H

#include <string>
#include <string_view>
#include <vector>

namespace wc::base64 {

  /// @return true if the given string is valid base64, false otherwise.
  constexpr bool isBase64(const std::string_view str);

  /// @return the number of characters resulting from encoding 'numBytes' bytes.
  inline constexpr size_t encodeSize(size_t numBytes) {
    return ((numBytes * 4) / 3) + ((((numBytes * 4) % 3) > 0) ? 1 : 0);
  }

  /// @return the number of bytes resulting from decoding 'numChars' characters.
  inline constexpr size_t decodeSize(size_t numChars) {
    return ((numChars * 3) / 4);
  }

  /// Encodes a block of data in base64.
  /// @param data: A pointer to the data to encode.
  /// @param len: The number of bytes to encode.
  /// @return A string containing the data encoded as base64.
  std::string encode(const char* data, size_t len);

  /// Decodes a base64 string into binary data.
  /// @param str: A base64 string to decode.
  /// @return A vector of bytes containing the decoded data.
  std::vector<char> decode(const std::string_view str);

  namespace test {
    int runBase64UnitTests();
  }

}  // namespace wc::base64

#endif  // WC_TOOLS_BASE64_H