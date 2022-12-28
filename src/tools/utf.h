#ifndef WC_TOOLS_UTF_H
#define WC_TOOLS_UTF_H

#include <codecvt>
#include <locale>

/// Converts a UTF-8 string to UTF-16.
inline std::wstring utf8_to_utf16(const std::string& in) {
  std::wstring_convert<
      std::codecvt_utf8_utf16<wchar_t, 1114111UL, std::little_endian>, wchar_t>
      converter;
  return converter.from_bytes(in);
}

/// Converts a UTF-16 string to UTF-8.
inline std::string utf16_to_utf8(const std::wstring& in) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
  return converter.to_bytes(in);
}

/// Converts a UTF-8 string to UTF-32.
inline std::u32string utf8_to_utf32(const std::string& in) {

  std::wstring_convert<
      std::codecvt_utf8<char32_t, 1114111UL, std::little_endian>, char32_t>
      converter;
  return converter.from_bytes(in);
}

/// Converts a UTF-32 string to UTF-8.
inline std::string utf32_to_utf8(const std::u32string& in) {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  return converter.to_bytes(in);
}

#endif // WC_TOOLS_UTF_H