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
#include <rapidjson/document.h>
#include <string>
#include <string_view>
#include <vector>

namespace wc {
  class SettingsFile {
  public:
    SettingsFile(const std::string_view filename);
    ~SettingsFile();

    bool exists(const std::string_view path);

    template <typename T>
    bool read(const std::string_view path, T& val);

    template <typename T>
    void write(const std::string_view path, const T& val);

  private:
    const std::string   filename_;
    rapidjson::Document doc_;
    bool                modified_ = false;

    rapidjson::Value* followPath(const std::string_view path, bool mayCreate);
  };
}  // namespace wc

#endif  // WC_SYS_SETTINGS_H