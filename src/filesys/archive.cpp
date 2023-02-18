/******************************************************************************
 * Archive.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * This is a custom archive format which has been optimized for loading speeds.
 *****************************************************************************/
#include "archive.h"

#include "sys/debug.h"
#include "tools/soa.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
// #include "tools/stringhelper.h"
// #include "tools/crossplatform.h"
#include <lz4.h>
#include <lz4hc.h>

bool wc::Archive::open(const char* archiveName) {

  // If the archive is already open, close it first.
  if (isOpen()) close();

  // Get the proper filepath.
  std::filesystem::path archivePath = std::filesystem::u8path(archiveName);

  // If the archive does not exist...
  if (!std::filesystem::exists(archivePath)) {
    // Make sure the parent path for the archive exists,
    if (!std::filesystem::exists(archivePath.parent_path())) {
      // Create it if it doesn't.
      std::error_code ec;
      std::filesystem::create_directories(archivePath.parent_path(), ec);
      if (ec) {
        dbg::error({fmt::format("Failed to open '{}'.", archiveName),
                    fmt::format("Filesystem error: ", ec.message())});
        return false;
      }
    }

    // If the file doesn't exist yet, then we can't open it with "r+", so we
    // create it here.
    file_.open(archivePath, std::ios::out | std::ios::binary);

    Archive::Header newheader = {};
    strncpy(newheader.magic, Archive::MAGIC, 8);
    newheader.back    = sizeof(Archive::Header);
    newheader.version = Archive::CURRENT_VERSION;

    file_.write((const char*)&newheader, sizeof(Archive::Header));
    file_.close();
    modified_ = true;
  }

  // 'r+' opens the file for both reading and writing, keeping the existing
  // contents.
  file_.open(archivePath, std::ios::binary | std::ios::in | std::ios::out);
  if (!file_.is_open()) {
    dbg::error(fmt::format("Archive '{}' could not be opened.", archiveName));
    return false;
  }

  // Load the header.
  file_.read((char*)&header_, sizeof(Archive::Header));

  // Check the header to make sure the file format is valid.
  if (strncmp(header_.magic, Archive::MAGIC, 8) != 0) {
    dbg::error(
        fmt::format("Archive '{}' is not a valid archive.", archiveName));
    file_.close();
    return false;
  }

  if (header_.numfiles > 0) {
    // Reserve space for the dictionary.
    size_t dictbytes;
    void*  dictmem = dictionary_.deserialize(header_.numfiles, dictbytes);
    if (!dictmem) {
      dbg::error("Failed to allocate memory for archive dictionary.");
      file_.close();
      return false;
    }

    // Seek to its position,
    file_.seekg(header_.back);

    // Read the dictionary in.
    file_.read((char*)dictmem, dictbytes);
    size_t bytesread = file_.gcount();
    if (bytesread != dictbytes) {
      dbg::error(
          {"Failed to load archive dictionary data.",
           fmt::format("Expected {} bytes, read {}.", dictbytes, bytesread)});

      file_.close();
      return false;
    }
  }

  savedPath_ = archiveName;
  return true;
}

void wc::Archive::close() {
  // If the file isn't open, we have nothing to close.
  if (!isOpen()) return;

  // If the archive has been modified, we update the file's header and
  // dictionary.
  if (modified_) {

    dictionary_.shrinkToFit();
    header_.numfiles = (uint32_t)dictionary_.size();

    // Update the archive header.
    file_.seekg(0);
    file_.write((const char*)&header_, sizeof(Archive::Header));

    // If we deleted or overwrote anything in the archive,
    // we should rebuild the whole thing.
    if (filesDeleted_) {
      rebuild();
    } else {
      // Otherwise, we can just re-write the dictionary at the end.
      file_.seekg(header_.back);

      size_t dictbytes;
      void*  dictmem = dictionary_.serialize(dictbytes);
      file_.write((const char*)&dictmem, dictbytes);
      // size_t byteswritten = file_.gcount();
      // if (byteswritten != dictbytes) {
      //   dbg::error({"Failed to save archive dictionary data.",
      //               fmt::format("Expected {} bytes, wrote {}.", dictbytes,
      //                           byteswritten)});
      // }
    }
  }

  // Close the file.
  file_.close();

  // Clear out the dictionary.
  dictionary_.clear();

  // Reset flags.
  modified_     = false;
  filesDeleted_ = false;
}

void wc::Archive::rebuild() {
  // Make sure the archive is open.
  if (!isOpen()) return;

  // Create a new temporary archive.
  std::filesystem::path tempPath =
      std::filesystem::u8path(savedPath_ + "_TEMP");
  std::fstream tempFile(tempPath);

  // First we write the header to the new archive.
  tempFile.write((const char*)&header_, sizeof(Archive::Header));
  uint64_t newback = sizeof(Archive::Header);

  // Next we go through all of our files.
  for (uint32_t i = 0; i < header_.numfiles; ++i) {
    // Allocate a buffer large enough to store the file contents.
    void* buffer = malloc(fileInfos_[i].sizeCompressed);
    if (buffer == nullptr) {
      dbg::error("Memory allocation failure while rebuilding archive.");
      continue;
    }

    // Read the file into memory.
    file_.seekg(fileInfos_[i].offset);
    file_.read((char*)buffer, fileInfos_[i].sizeCompressed);

    // Write the file to the temporary archive.
    tempFile.seekg(newback);
    tempFile.write((const char*)buffer, fileInfos_[i].sizeCompressed);

    // Keep track of where the file is now.
    fileInfos_[i].offset = newback;
    newback += fileInfos_[i].sizeCompressed;

    // Clear up the memory we used.
    free(buffer);
  }

  // Now we write the dictionary to the temporary archive.
  size_t dictbytes;
  void*  dictmem = dictionary_.serialize(dictbytes);
  tempFile.write((const char*)dictmem, dictbytes);

  // Since our 'back' has changed, we need to correct it and re-write the
  // header.
  header_.back = newback;
  tempFile.seekg(0);
  tempFile.write((const char*)&header_, sizeof(Archive::Header));

  // Close both archives.
  file_.close();
  tempFile.close();

  // Have the filesystem clean things for us.
  std::filesystem::path myPath = std::filesystem::u8path(savedPath_);
  std::filesystem::rename(tempPath, myPath);

  // Re-open the file.
  file_.open(myPath);
}

int wc::Archive::eraseFile(const char* path) {
  // Make sure the archive is open.
  if (!isOpen()) return -1;

  // Find the index of the file we're looking for.
  FixedString<Archive::FILEPATH_FIXEDLEN> fixedpath(path);

  auto it = dictionary_.find(fixedpath);
  if (!*it) return -1;
  it->erase();
  modified_     = true;
  filesDeleted_ = true;

  return (int)it->index();
}

bool wc::Archive::fileExists(const char* path) const {
  // Make sure the archive is open.
  if (!isOpen()) return false;

  // Look for the file.
  FixedString<Archive::FILEPATH_FIXEDLEN> fixedpath(path);
  return dictionary_.find(fixedpath)->exists();
}

bool wc::Archive::extractData(const char* path, std::vector<char>& buffer,
                              timestamp_t& timestamp) {
  // Make sure the archive is open.
  if (!isOpen()) { return false; }

  // Search for the file we're looking for.
  FixedString<Archive::FILEPATH_FIXEDLEN> fixedpath(path);
  size_t index = dictionary_.find(fixedpath)->index();
  if (index == SIZE_MAX) {
    dbg::errmore(fmt::format("Failed to find file '{}'.", path));
    return false;
  }

  // Fetch the file's information and resize the buffer accordingly.
  const FileInfo& info = fileInfos_[index];
  buffer.resize(info.sizeUncompressed);
  timestamp = info.timestamp;

  // Seek to the file data's location in the archive.
  file_.seekg(info.offset);
  if (file_.eof()) {
    dbg::errmore(fmt::format("Bad offset for file '{}'.", path));
    return false;
  }

  // If the file is not compressed, we can load it straight into memory.
  if (!(info.flags & FILEFLAG_COMPRESSED)) {
    // Load the file contents straight into memory.
    file_.read(buffer.data(), info.sizeUncompressed);
  }
  // Otherwise, the file has been compressed using LZ4.
  // LZ4 default and LZ4 HC are both decompressed using the same function,
  // so the file info doesn't need to know which was used for compression.
  else {
    std::vector<char> compressed_buffer(info.sizeCompressed);
    file_.read(compressed_buffer.data(), info.sizeCompressed);
    int result =
        LZ4_decompress_safe(compressed_buffer.data(), buffer.data(),
                            info.sizeCompressed, info.sizeUncompressed);
    if (result < 0) {
      dbg::error({fmt::format("Failed to decompress data for '{}'.", path),
                  "source stream is malformed."});
      return false;
    }
  }

  // Finally, we return the buffer we just loaded the memory to.
  return true;
}

void wc::Archive::extractFile(const char* path, const char* dstfilename) {
  // Extract the file data.
  uint32_t          size = 0;
  timestamp_t       timestamp;
  std::vector<char> buffer;
  if (!extractData(path, buffer, timestamp)) { return; }

  // Create the directory where we'll place the file.
  std::filesystem::path dstPath = std::filesystem::u8path(dstfilename);
  std::error_code       ec;
  std::filesystem::create_directories(dstPath.parent_path(), ec);
  if (ec) {
    dbg::error({fmt::format("Error extracting file '{}',", path),
                fmt::format("Filesystem error: {}.", ec.message())});
    return;
  }

  // Open the file we're extracting to...
  std::ofstream dstFile(dstPath);
  if (dstFile.is_open()) {
    // Write the file data.
    dstFile.write(buffer.data(), buffer.size());
    dstFile.close();
    // Correct the extracted file's timestamp.
    std::error_code ec;
    std::filesystem::last_write_time(dstPath, (timestamp), ec);
    if (ec) {
      dbg::error(
          {fmt::format("Error extracting file '{}',", path),
           fmt::format("Failed to correct timestamp: {}.", ec.message())});
    }
  }
}

bool wc::Archive::insertData(const char* path, void* buffer, int32_t size,
                             timestamp_t timestamp, ReplaceEnum replace,
                             CompressEnum compress) {
  // Make sure the archive is open.
  if (!isOpen()) return false;

  // Copy the file's path and convert backslashes to forward slashes.
  // Functions like extract and exists will simply fail if given backslashes.
  FixedString<FILEPATH_FIXEDLEN> newpath(path);
  for (int i = 0; newpath.c_str[i] != '\0'; ++i) {
    if (newpath.c_str[i] == '\\') { newpath.c_str[i] = '/'; }
  }

  // Create a dictionary entry for the new file.
  FileInfo newinfo         = {};
  newinfo.offset           = header_.back;
  newinfo.sizeCompressed   = size;
  newinfo.sizeUncompressed = size;
  newinfo.timestamp        = timestamp;

  // Handle file compression.
  std::vector<char> compressed_buffer(LZ4_compressBound(size));
  if (compress != DO_NOT_COMPRESS) {
    if (compress == COMPRESS_FAST) {
      newinfo.sizeCompressed =
          LZ4_compress_default((const char*)buffer, compressed_buffer.data(),
                               size, (int)compressed_buffer.size());
    } else if (compress == COMPRESS_SMALL) {
      newinfo.sizeCompressed =
          LZ4_compress_HC((const char*)buffer, compressed_buffer.data(), size,
                          (int)compressed_buffer.size(), 9);
    }
    // Compression failed
    if (newinfo.sizeCompressed <= 0) {
      dbg::warning({fmt::format("Failed to compress file '{}',", path),
                    "Will store uncompressed."});
      newinfo.sizeCompressed = size;
    }
    // Compression success
    else {
      newinfo.flags |= FILEFLAG_COMPRESSED;
      buffer = compressed_buffer.data();
    }
  }

  // Find the file we're looking for.
  size_t index = dictionary_.find(newpath)->index();
  if (index == SIZE_MAX) {
    dictionary_.insert(newpath, newinfo);
    index = dictionary_.find(newpath)->index();
  } else {
    // The specified file is already in this archive, so use 'replace' to
    // decide what to do.
    switch (replace) {
    case DO_NOT_REPLACE: return false; break;
    case ALWAYS_REPLACE:
      filesDeleted_     = true;
      fileInfos_[index] = newinfo;
      break;
    case REPLACE_IF_NEWER:
      if (timestamp > fileInfos_[index].timestamp) {
        filesDeleted_     = true;
        fileInfos_[index] = newinfo;
      } else
        return false;
      break;
    default: return false; break;
    }
  }

  // If the new file has a non-zero size,
  if (newinfo.sizeCompressed > 0) {
    // We write the file contents.
    file_.seekg(header_.back);
    file_.write((const char*)buffer, newinfo.sizeCompressed);
    header_.back += newinfo.sizeCompressed;
  }

  modified_ = true;
  return true;
}

bool wc::Archive::insertFile(const char* path, const char* srcfilename,
                             ReplaceEnum replace, CompressEnum compress) {
  // Make sure the path isn't too long.
  if (strlen(path) > (Archive::FILEPATH_FIXEDLEN - 1)) {
    dbg::error({fmt::format("Failed to insert '{}',", path),
                fmt::format("Path too long (max {} bytes)",
                            Archive::FILEPATH_FIXEDLEN - 1)});
    return false;
  }

  std::filesystem::path srcpath = std::filesystem::u8path(srcfilename);

  // Get the file's 'last write time' and convert it to a usable integer.
  timestamp_t timestamp = (std::filesystem::last_write_time(srcpath));

  // Open the file.
  std::ifstream srcfile(srcpath, std::ios::binary);
  if (!srcfile.is_open()) {
    dbg::error(
        {fmt::format("Failed to insert '{}',", path), "Failed to open file."});
    return false;
  }

  // Slurp up the file contents.
  srcfile.seekg(0, srcfile.end);
  size_t len = srcfile.tellg();
  srcfile.seekg(0, srcfile.beg);
  std::vector<char> buffer(len);
  srcfile.read(buffer.data(), buffer.size());
  if (!srcfile) {
    dbg::error(
        {fmt::format("Failed to insert '{}',", path),
         fmt::format("Only read {} bytes out of {}.", srcfile.gcount(), len)});
    return false;
  }

  // Insert the file into the archive.
  return insertData(path, buffer.data(), (uint32_t)buffer.size(), timestamp,
                    replace, compress);
}

void recursive_pack(wc::Archive& archive, const std::filesystem::path& parent,
                    std::filesystem::path     child,
                    wc::Archive::ReplaceEnum  replace,
                    wc::Archive::CompressEnum compress) {
  for (std::filesystem::directory_iterator it(parent / child);
       it != std::filesystem::directory_iterator(); ++it) {
    // Path doesn't exist.  This case should never happen, but hey, just in
    // case!
    if (!std::filesystem::exists(it->path())) continue;

    // Path is a directory, so we need to go deeper.
    if (std::filesystem::is_directory(it->path()))
      recursive_pack(archive, parent, child / it->path().filename(), replace,
                     compress);
    // Path is a file, so we insert it into the archive.
    else if (std::filesystem::is_regular_file(it->path()))
      archive.insertFile((child / it->path().filename()).string().c_str(),
                         it->path().string().c_str(), replace, compress);
  }
}

void wc::Archive::pack(const char* srcfolder, ReplaceEnum replace,
                       CompressEnum compress) {
  // Make sure the archive is open.
  if (!isOpen()) return;

  std::filesystem::path srcpath = std::filesystem::u8path(srcfolder);
  if (!std::filesystem::exists(srcpath)) {
    dbg::error(fmt::format("'{}' does not exist.", srcfolder));
    return;
  }
  if (!std::filesystem::is_directory(srcpath)) {
    dbg::error(fmt::format("'{}' is not a directory.", srcfolder));
    return;
  }

  recursive_pack(*this, srcpath, "", replace, compress);
}

void wc::Archive::unpack(const char* dstfolder) {
  // Make sure the archive is open.
  if (!isOpen()) return;

  std::filesystem::path dstpath = std::filesystem::u8path(dstfolder);
  if (!std::filesystem::exists(dstpath))
    std::filesystem::create_directories(dstpath);

  if (!std::filesystem::is_directory(dstpath)) {
    dbg::error(
        fmt::format("'{}' already exists and is not a directory.", dstfolder));
    return;
  }

  // Go through each file in the archive,
  for (size_t i = 0; i < header_.numfiles; ++i) {
    // and extract it to disk.
    FixedString<Archive::FILEPATH_FIXEDLEN>& entry = filePaths_[i];
    extractFile(entry.c_str, (dstpath / entry.c_str).string().c_str());
  }
}

void wc::Archive::merge(const char* othername, ReplaceEnum replace) {
  Archive other;
  if (!other.open(othername)) return;

  std::vector<char> buffer;
  for (size_t i = 0; i < other.header_.numfiles; ++i) {
    FixedString<64> entry = other.filePaths_[i];
    timestamp_t     timestamp;
    other.extractData(entry.c_str, buffer, timestamp);
    insertData(entry.c_str, buffer.data(), (uint32_t)buffer.size(), timestamp,
               replace);
  }
}