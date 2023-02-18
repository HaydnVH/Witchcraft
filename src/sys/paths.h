/******************************************************************************
 * paths.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Provides access to the User and Install directories,
 * where files may be read from and written to.
 *****************************************************************************/
#ifndef WC_SYS_PATHS_H
#define WC_SYS_PATHS_H

#include <filesystem>
#include <string>
#include <string_view>

namespace wc {

  /// Gets the directory where the application is installed.
  /// @return The path to the directory.
  const std::filesystem::path& getInstallPath();

  /// Gets the directory where user files may be kept.
  /// @return The path to the directory.
  const std::filesystem::path& getUserPath();

  constexpr const char* INSTALL_PATH_REPLACEMENT =
      "\x1b[38;2;127;160;255m$INSTALL\x1b[0m";

  constexpr const char* USER_PATH_REPLACEMENT =
      "\x1b[38;2;127;160;255m$USER\x1b[0m";

  /// @return A copy of the given string with the install or user path
  /// shortened and replaced with a replacement token.
  std::string        trimPathStr(std::string pathString);
  inline std::string trimPathStr(const std::filesystem::path& path) {
    return trimPathStr(path.string());
  }

}  // namespace wc

#endif  // WC_SYS_PATHS_H