#ifndef WC_TOOLS_BASE64_H
#define WC_TOOLS_BASE64_H

#include <string>
#include <string_view>
#include <vector>

namespace wc::base64 {

  /// @return true if the given string is valid base64, false otherwise.
  constexpr bool isBase64(const std::string_view str);

  /// @return An upper bound of characters that could result from encoding
  /// 'numBytes' bytes.
  inline constexpr size_t encodeSize(size_t numBytes) {
    numBytes += numBytes % 3;
    return (numBytes * 4) / 3;
  }

  /// @return An upper bound of bytes that could result from decoding 'numChars'
  /// characters.
  inline constexpr size_t decodeSize(size_t numChars) {
    numChars += numChars % 4;
    return (numChars * 3) / 4;
  }

  /// Encodes a block of data in base64.
  /// @param data: A pointer to the data to encode.
  /// @param len: The number of bytes to encode.
  /// @return A string containing the data encoded as base64.
  constexpr std::string encode(void* data, size_t len);

  /// Decodes a base64 string into binary data.
  /// @param str: A base64 string to decode.
  /// @return A vector of bytes containing the decoded data.
  constexpr std::vector<char> decode(const std::string_view str);

}  // namespace wc::base64

#endif  // WC_TOOLS_BASE64_H