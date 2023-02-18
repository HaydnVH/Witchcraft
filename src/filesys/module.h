/******************************************************************************
 * module.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Module objects are the heart and soul of Witchcraft's modability.
 *****************************************************************************/
#ifndef HVH_WC_FILESYS_MODULE_H
#define HVH_WC_FILESYS_MODULE_H

#include "archive.h"
#include "tools/fixedstring.h"
#include "tools/htable.hpp"
#include "tools/result.h"

#include <filesystem>
#include <optional>
#include <vector>

namespace wc {

  constexpr const char* MODINFO_FILENAME = "module.json";

  class Module {
  public:
    /// Attempts to open the module associated with the given path.
    /// This does not load the files inside the module!
    /// @return An empty Result which may hold an error/warning message.
    wc::Result open(const std::filesystem::path& path);

    /// Closes the archive (if one exists) and frees resources.
    void close();

    /// This does the heavy lifting of setting up the list of files in this
    /// package.
    /// @return An empty Result which may hold an error/warning message.
    wc::Result loadFileList();

    /// Load an actual file from this module.
    /// @param filename The name of the file to search for.
    /// @return An 'std::vector<char>' containing the file's binary data,
    /// or an error message if the operation was unsuccessful.
    wc::Result loadFile(const FixedString<64>& filename);

    inline const std::string& getName() const { return name_; }
    inline const std::string& getAuthor() const { return author_; }
    inline const std::string& getCategory() const { return category_; }
    inline const std::string& getDescription() const { return description_; }
    inline const std::filesystem::path& getPath() const { return path_; }
    inline float getPriority() const { return priority_; }
    inline bool  isLoaded() { return loaded_; }

    // Get the file table for this module.
    inline const hvh::Table<FixedString<64>>& getFileTable() const {
      return fileTable_;
    }
    // Get the archive for this module.
    inline Archive& getArchive() { return archive_; }

    inline bool operator<(const Module& rhs) {
      // Compare priority first.
      if (priority_ < rhs.priority_) return true;
      if (priority_ > rhs.priority_) return false;
      // If priority is equal, compare timestamp.
      if (timestamp_ < rhs.timestamp_) return true;
      return false;
    }

    friend inline void swap(Module& lhs, Module& rhs) {
      using std::swap;
      swap(lhs.archive_, rhs.archive_);
      swap(lhs.path_, rhs.path_);
      swap(lhs.name_, rhs.name_);
      swap(lhs.author_, rhs.author_);
      swap(lhs.category_, rhs.category_);
      swap(lhs.description_, rhs.description_);
      swap(lhs.timestamp_, rhs.timestamp_);
      swap(lhs.priority_, rhs.priority_);
      swap(lhs.fileTable_, rhs.fileTable_);
      swap(lhs.enabled_, rhs.enabled_);
      swap(lhs.found_, rhs.found_);
      swap(lhs.loaded_, rhs.loaded_);
    }

  private:
    Archive               archive_;
    std::filesystem::path path_;

    std::string name_;
    std::string author_;
    std::string category_;
    std::string description_;

    uint64_t timestamp_ = 0;
    float    priority_  = 0.0f;

    hvh::Table<FixedString<64>> fileTable_;

    bool enabled_ = false;
    bool found_   = false;
    bool loaded_  = false;
  };

}  // namespace wc
#endif  // HVH_WC_FILESYS_MODULE_H