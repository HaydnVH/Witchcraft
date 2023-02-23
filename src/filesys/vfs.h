/******************************************************************************
 * vfs.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Interface for the virtual filesystem used by Witchcraft.
 * Folders and archives can be registered and when a file is requested,
 * each folder and archive which contains the requested file can provide it.
 * This is the fundamental backbone of how modding is implemented.
 *****************************************************************************/
#ifndef WC_FILESYS_VFS_H
#define WC_FILESYS_VFS_H

#include "module.h"
#include "tools/result.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wc {

  using FileResult = wc::Result::Value<std::vector<char>>;

  class Filesystem {

    constexpr static const char* MODULE_EXT      = ".wcmod";
    constexpr static const char* MODULE_SAVE_EXT = ".wcsav";

    constexpr static const char* DATA_FOLDER         = "data/";
    constexpr static const char* SAVE_FOLDER         = "saves/";
    constexpr static const char* DEFAULT_PACKAGE     = "engine_data";
    constexpr static const char* ACTIVE_PACKAGE_PATH = "temp/active.wcs";

    constexpr static const char* LOAD_ORDER_FILENAME = "load_order.json";

  public:
    Filesystem();
    ~Filesystem();

    void printKnownFiles();
    void printAvailablePackages();
    void printLoadedPackages();

    struct FileProxy {
    public:
      FileProxy(Filesystem& vfs, const std::string_view filename,
                std::vector<size_t>&& list, bool reverseSort):
          vfs_(vfs),
          filename_(filename), list_(list), reverseSort_(reverseSort) {}

      struct Iterator {
        Iterator(const FileProxy& proxy, size_t listIndex):
            proxy_(proxy), listIndex_(listIndex) {}

        Iterator& operator++() {
          ++listIndex_;
          return *this;
        }
        Iterator operator++(int) {
          Iterator tmp = *this;
          ++(*this);
          return tmp;
        }

        friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
          return (lhs.listIndex_ == rhs.listIndex_);
        }
        friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
          return !(lhs.listIndex_ == rhs.listIndex_);
        }

        Iterator& operator*() { return *this; }
        Iterator* operator->() { return this; }

        /// @return true if this iterator currently points to an existing file.
        bool exists() const { return listIndex_ < proxy_.list_.size(); }
        /// @return true if this iterator currently points to an existing file.
        operator bool() const { return exists(); }

        /// Loads the version of the file that this iterator currently points
        /// to.
        /// @return An 'std::vector<char>' containing the file's binary data,
        /// or an error message if the operation was unsuccessful.
        FileResult load();

      private:
        const FileProxy& proxy_;
        size_t           listIndex_;
      };

      Iterator begin() { return Iterator(*this, 0); }
      Iterator end() { return Iterator(*this, list_.size()); }

      /// @return true if any file with the desired filename could be found.
      bool exists() const { return list_.size() > 0; }
      /// @return true if any file with the desired filename could be found.
      operator bool() const { return exists(); }

      /// Loads the first version of the file which was searched for.
      /// If reverseSort was false, this will be the lowest-priority version.
      /// Otherwise, it will be the highest-priority version.
      /// @return An 'std::vector<char>' containing the file's binary data,
      /// or an error message if the operation was unsuccessful.
      FileResult load() { return begin().load(); }

      /// Loads the lowest-priority version of the file that was searched for,
      /// regardless of whether reverseSort was set or not.
      /// @return An 'std::vector<char>' containing the file's binary data,
      /// or an error message if the operation was unsuccessful.
      FileResult loadLowest() {
        if (reverseSort_)
          return Iterator(*this, list_.size() - 1).load();
        else
          return begin().load();
      }

      /// Loads the highest-priority version of the file that was searched for,
      /// regardless of whether reverseSort was set or not.
      /// @return An 'std::vector<char>' containing the file's binary data,
      /// or an error message if the operation was unsuccessful.
      FileResult loadHighest() {
        if (reverseSort_)
          return begin().load();
        else
          return Iterator(*this, list_.size() - 1).load();
      }

    private:
      Filesystem&         vfs_;
      std::string         filename_;
      std::vector<size_t> list_;
      bool                reverseSort_;
    };

    /// Searches for a file within the virtual filesystem and returns an object
    /// which provides access to it, if found.
    /// @param filename The name of the file to search for.
    /// @param reverseSort If true, reverses the sorting order of the resulting
    /// proxy.
    /// @return A proxy object which provides access to the file(s) found.
    FileProxy
        getFile(const std::string_view filename, bool reverseSort = false);

  private:
    hvh::Table<FixedString<64>, wc::Module> modules_;
    hvh::Table<FixedString<64>, size_t>     files_;
    size_t                                  loadModule(size_t moduleIndex);
  };

  /*

  // Initializes the virtual filesystem and loads default modules.
  // Returns false if initialization fails.
  bool init();

  // Cleans up the virtual filesystem.
  void shutdown();

  // Returns whether the virtual filesystem has been initialized.
  bool isInitialized();

  // Look at what's available, for debugging.
  void printKnownFiles();
  void printAvailablePackages();
  void printLoadedPackages();

  /// FileProxy is a helper class used to access the files within the virtual
  /// filesystem. It can be used in a range-based for loop to iterate over all
  /// versions of a file, or it can simply be used to load the first version
  /// found.
  struct FileProxy {
    FileProxy(const std::string_view filename, std::vector<size_t>&& list,
              bool reverseSort):
        filename_(filename),
        list_(list), reverseSort_(reverseSort) {}

    struct Iterator {
      Iterator(const FileProxy& proxy, size_t listIndex):
          proxy_(proxy), listIndex_(listIndex) {}

      Iterator& operator++() {
        ++listIndex_;
        return *this;
      }
      Iterator operator++(int) {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
        return (lhs.listIndex_ == rhs.listIndex_);
      }
      friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
        return !(lhs.listIndex_ == rhs.listIndex_);
      }

      Iterator& operator*() { return *this; }
      Iterator* operator->() { return this; }

      /// @return true if this iterator currently points to an existing file.
      bool exists() const { return listIndex_ < proxy_.list_.size(); }
      /// @return true if this iterator currently points to an existing file.
      operator bool() const { return exists(); }

      /// Loads the version of the file that this iterator currently points to.
      /// @return An 'std::vector<char>' containing the file's binary data,
      /// or an error message if the operation was unsuccessful.
      wc::Result::Value<std::vector<char>> load();

    private:
      const FileProxy& proxy_;
      size_t           listIndex_;
    };

    Iterator begin() { return Iterator(*this, 0); }
    Iterator end() { return Iterator(*this, list_.size()); }

    /// @return true if any file with the desired filename could be found.
    bool exists() const { return list_.size() > 0; }
    /// @return true if any file with the desired filename could be found.
    operator bool() const { return exists(); }

    /// Loads the first version of the file which was searched for.
    /// If reverseSort was false, this will be the lowest-priority version.
    /// Otherwise, it will be the highest-priority version.
    /// @return An 'std::vector<char>' containing the file's binary data,
    /// or an error message if the operation was unsuccessful.
    wc::Result::Value<std::vector<char>> load() { return begin().load(); }

    /// Loads the lowest-priority version of the file that was searched for,
    /// regardless of whether reverseSort was set or not.
    /// @return An 'std::vector<char>' containing the file's binary data,
    /// or an error message if the operation was unsuccessful.
    wc::Result::Value<std::vector<char>> loadLowest() {
      if (reverseSort_)
        return Iterator(*this, list_.size() - 1).load();
      else
        return begin().load();
    }

    /// Loads the highest-priority version of the file that was searched for,
    /// regardless of whether reverseSort was set or not.
    /// @return An 'std::vector<char>' containing the file's binary data,
    /// or an error message if the operation was unsuccessful.
    wc::Result::Value<std::vector<char>> loadHighest() {
      if (reverseSort_)
        return begin().load();
      else
        return Iterator(*this, list_.size() - 1).load();
    }

  private:
    std::string         filename_;
    std::vector<size_t> list_;
    bool                reverseSort_;
  };

  /// Searches for a file within the virtual filesystem and returns an object
  /// which provides access to it, if found.
  /// @param filename The name of the file to search for.
  /// @param reverseSort If true, reverses the sorting order of the resulting
  /// proxy.
  /// @return A proxy object which provides access to the file(s) found.
  FileProxy getFile(const std::string_view filename, bool reverseSort = false);
  */
}  // namespace wc

#endif  // WC_FILESYS_VFS_H