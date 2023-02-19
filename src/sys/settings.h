/******************************************************************************
 * settings.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Provides access to settings.json which contains per-user settings.
 *****************************************************************************/
#ifndef WC_SYS_SETTINGS_H
#define WC_SYS_SETTINGS_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wc::settings {

  constexpr const char* SETTINGS_FILENAME = "settings.json";

  void init();
  void shutdown();

  bool isInitialized();

  /// @return True if a value at the given path exists, false otherwise.
  bool exists(const std::string_view path);

  template <typename T>
  bool read(const std::string_view path, T& val);

  template <typename T>
  void write(const std::string_view path, const T& val);

}  // namespace wc::settings
#endif  // WC_SYS_SETTINGS_H