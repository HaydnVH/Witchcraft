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

namespace wc {

  /// Gets the directory where the application is installed.
  /// @return The path to the directory.
  const std::filesystem::path& getInstallPath();

  /// Gets the directory where user files may be kept.
  /// @return The path to the directory.
  const std::filesystem::path& getUserPath();

}  // namespace wc

#endif  // WC_SYS_PATHS_H