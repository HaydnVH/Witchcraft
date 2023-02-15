#ifndef HVH_WC_FILESYS_PACKAGE_H
#define HVH_WC_FILESYS_PACKAGE_H

#include "archive.h"
#include "tools/fixedstring.h"
#include "tools/htable.hpp"

namespace wc {

  constexpr const char* PACKAGEINFO_FILENAME = "package.json";

  class Package {
  public:
    // Attempts to open the package associated with the given path.
    // Returns false if the package does not exist, or if the file is not a
    // package. This does not load the files inside the package!
    bool open(const std::string_view u8path);

    // Closes the archive (if one exists) and frees resources.
    void close();

    // This does the heavy lifting of setting up the list of files in this
    // package.
    void loadFileList();

    inline const std::string& getName() const { return name_; }
    inline const std::string& getAuthor() const { return author_; }
    inline const std::string& getCategory() const { return category_; }
    inline const std::string& getDescription() const { return description_; }
    inline const std::string& getPath() const { return path_; }
    inline float              getPriority() const { return priority_; }
    inline bool               isLoaded() { return loaded_; }

    inline const hvh::htable<fixedstring<64>>& getFileTable() const {
      return fileTable_;
    }
    inline Archive& getArchive() { return archive_; }

    inline bool operator<(const Package& rhs) {
      // Compare priority first.
      if (priority_ < rhs.priority_) return true;
      if (priority_ > rhs.priority_) return false;
      // If priority is equal, compare timestamp.
      if (timestamp_ < rhs.timestamp_) return true;
      return false;
    }

    friend inline void swap(Package& lhs, Package& rhs) {
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
    Archive     archive_;
    std::string path_;

    std::string name_;
    std::string author_;
    std::string category_;
    std::string description_;

    uint64_t timestamp_ = 0;
    float    priority_  = 0.0f;

    hvh::htable<fixedstring<64>> fileTable_;

    bool enabled_ = false;
    bool found_   = false;
    bool loaded_  = false;
  };

}  // namespace wc
#endif  // HVH_WC_FILESYS_PACKAGE_H