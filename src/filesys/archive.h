#ifndef WC_FILESYS_ARCHIVE_H
#define WC_FILESYS_ARCHIVE_H

#include "tools/fixedstring.h"
#include "tools/htable.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <vector>

namespace wc {

  class Archive {
  public:
    Archive() {}
    Archive(const Archive&) = delete;
    Archive(Archive&& rhs) noexcept { swap(*this, rhs); }
    Archive& operator=(const Archive&) = delete;
    Archive& operator=(Archive&& rhs) noexcept {
      swap(*this, rhs);
      return *this;
    }
    ~Archive() { close(); }

    friend inline void swap(Archive& lhs, Archive& rhs) {
      using std::swap;
      swap(lhs.header_, rhs.header_);
      swap(lhs.dictionary_, rhs.dictionary_);
      swap(lhs.file_, rhs.file_);
      swap(lhs.savedPath_, rhs.savedPath_);
      swap(lhs.modified_, rhs.modified_);
      swap(lhs.filesDeleted_, rhs.filesDeleted_);
    }

    enum ReplaceEnum { DO_NOT_REPLACE, ALWAYS_REPLACE, REPLACE_IF_NEWER };

    enum CompressEnum {
      DO_NOT_COMPRESS,
      COMPRESS_FAST,  // Uses LZ4 default
      COMPRESS_SMALL  // Uses LZ4 HC
    };

    enum FileInfoFlags { FILEFLAG_COMPRESSED = 1 << 0 };

    typedef std::chrono::file_clock::time_point timestamp_t;

    // Finds an archive on disc and opens it, filling out the archive's header
    // and dictionary as appropriate. Creates an empty archive if it doesn't
    // already exist.
    bool open(const char* archiveName);

    // Closes the archive, saving any modifications to disc and freeing
    // resources.
    void close();

    // Rebuilds the archive, sorting data according to filename and erasing any
    // unreferenced blocks.
    void rebuild();

    // Returns whether or not the archive in question is open/valid.
    bool isOpen() const { return (file_.is_open()); }

    // Checks to see if a file exists in the archive.
    bool fileExists(const char* path) const;

    // Finds the file and extracts it to an in-memory buffer.
    // 'buffer' will be resized, and it will contain the file data.
    bool extractData(const char* path, std::vector<char>& buffer,
                     timestamp_t& timestamp);

    // Finds the file and extracts it to a file on disc.
    void extractFile(const char* path, const char* dstfilename);

    // Takes a region of memory and, treating it like a single contiguous file,
    // inserts it into the archive.
    bool insertData(const char* path, void* buffer, int32_t size,
                    timestamp_t  timestamp,
                    ReplaceEnum  replace  = REPLACE_IF_NEWER,
                    CompressEnum compress = COMPRESS_FAST);

    // Opens a file on disc and inserts its entire contents into the archive.
    bool insertFile(const char* path, const char* srcfilename,
                    ReplaceEnum  replace  = REPLACE_IF_NEWER,
                    CompressEnum compress = COMPRESS_FAST);

    // Erases a file from the archive.
    // All it really does is remove the file's information from the dictionary,
    // and flag the instance as dirty. When the archive is closed, if one or
    // more files have been deleted, the archive is rebuilt.
    int eraseFile(const char* path);

    // Searches a folder recursively and adds every file found to the archive.
    void pack(const char* srcfolder, ReplaceEnum replace = REPLACE_IF_NEWER,
              CompressEnum compress = COMPRESS_FAST);

    // Extracts every file in the archive and saves them to the given location.
    void unpack(const char* dstfolder);

    // Opens an archive and insers all of its files into this archive.
    void merge(const char* othername, ReplaceEnum = REPLACE_IF_NEWER);

    // Get the number of files in the archive.
    uint32_t numFiles() const { return header_.numfiles; }

    auto begin() { return dictionary_.begin(); }
    auto end() { return dictionary_.end(); }

  private:
    static constexpr const char*    MAGIC             = "WCARCHV";
    static constexpr const uint16_t CURRENT_VERSION   = 3;
    static constexpr const size_t   FILEPATH_FIXEDLEN = 64;

    struct Header {
      // This is set to a predefined string and used to ensure that the file
      // type is correct.
      char magic[8] = {};
      // This points to the beginning of the dictionary, and defines the file
      // data region.
      uint64_t back     = 0;
      uint32_t flags    = 0;
      uint32_t numfiles = 0;
      // Which version of this software was used to create the archive?
      uint16_t version = 0;

    private:
      // Reserved in case we need it for future versions without having to break
      // compatability.
      char reserved[38] = {};

    } header_;

    struct FileInfo {
      // Where, relative to the beginning of the archive, is the file located?
      uint64_t offset = 0;
      // When was the file created before we added it to the archive?
      timestamp_t timestamp;
      // How many bytes in the archive itself does the file take?
      int32_t sizeCompressed = 0;
      // How many bytes large will the file be after we decompress it?
      // If this if equal to 'size_compressed', the file is uncompressed.
      int32_t sizeUncompressed = 0;

      uint32_t flags = 0;

    private:
      // Reserved for future use.  May or may not actually use.
      char reserved[4] = {};
    };

    hvh::Table<fixedstring<FILEPATH_FIXEDLEN>, FileInfo> dictionary_;
    fixedstring<FILEPATH_FIXEDLEN>* const& filePaths_ = dictionary_.data<0>();
    FileInfo* const&                       fileInfos_ = dictionary_.data<1>();

    std::fstream file_;
    std::string  savedPath_;
    bool         modified_     = false;
    bool         filesDeleted_ = false;
  };

}  // namespace wc
#endif  // WC_FILESYS_ARCHIVE_H
