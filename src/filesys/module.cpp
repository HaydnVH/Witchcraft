/******************************************************************************
 * module.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Module objects are the heart and soul of Witchcraft's modability.
 *****************************************************************************/
#include "module.h"

#include <filesystem>
#include <format>
#include <fstream>
namespace sfs = std::filesystem;

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
using namespace rapidjson;

#include "dbg/debug.h"
#include "sys/paths.h"

// #include "tools/stringhelper.h"

namespace {

  const hvh::Table<FixedString<64>> reservedFilenames_s = {
      wc::MODINFO_FILENAME, "readme.txt", "splash.png", "config.json",
      "load_order.json"};

}  // namespace

wc::Result::Value<wc::Module>
    wc::Module::open(const std::filesystem::path& myPath) {

  // Make sure something exists there.
  if (!sfs::exists(myPath)) {
    return Result::error(
        std::format("'{}' does not exist.", trimPathStr(myPath)));
  }

  Module                     result;
  std::optional<std::string> warnMsg = std::nullopt;
  std::vector<char>          modinfo;

  // Check to see if the path points to a directory or a file.
  // If it's a file, we'l try to open it like an archive.
  if (sfs::is_directory(myPath)) {
    std::ifstream file(myPath / MODINFO_FILENAME, std::ios::binary);
    if (file.is_open()) {
      file.seekg(0, file.end);
      size_t len = file.tellg();
      file.seekg(0, file.beg);
      modinfo.resize(len);
      file.read(modinfo.data(), modinfo.size());
      if (!file) {
        // Error when loading file.
        modinfo.clear();
      }
    }
    // modinfo = FileData(mypath, PACKAGEINFO_FILENAME);
  } else if (sfs::is_regular_file(myPath)) {
    if (result.getArchive().open(myPath.string().c_str())) {
      Archive::timestamp_t nil;
      result.getArchive().extractData(MODINFO_FILENAME, modinfo, nil);
    }
  }

  result.name_ = myPath.filename().string();

  if (modinfo.empty()) {
    warnMsg = std::format("Failed to open '{}/{}'.", trimPathStr(myPath),
                          MODINFO_FILENAME);
  } else {
    // Parse the file contents as json.
    Document doc;
    doc.Parse((char*)modinfo.data(), modinfo.size());
    if (doc.HasParseError() || !doc.IsObject()) {
      warnMsg = std::format("Failed to parse '{}/{}'.", trimPathStr(myPath),
                            MODINFO_FILENAME);
    } else {
      // We've found and correctly parsed module.json,
      // by this point we can confidently say that we're looking at a module.
      if (doc.HasMember("name")) {
        result.name_ = doc["name"].GetString();
      }
      if (doc.HasMember("author")) {
        result.author_ = doc["author"].GetString();
      }
      if (doc.HasMember("category")) {
        result.category_ = doc["category"].GetString();
      }
      if (doc.HasMember("description")) {
        result.description_ = doc["description"].GetString();
      }
      if (doc.HasMember("priority")) {
        result.priority_ = doc["priority"].GetFloat();
      }
    }
  }

  // Timestamp is used to sort modules which have equal priority.
  result.timestamp_ = sfs::last_write_time(myPath).time_since_epoch().count();

  result.path_  = myPath;
  result.found_ = true;
  if (warnMsg)
    return Result::warning(*warnMsg, result);
  else
    return Result::success(result);
}

void wc::Module::close() {
  archive_.close();
  fileTable_.clear();
  loaded_ = false;

  path_.clear();
  name_.clear();
  author_.clear();
  description_.clear();
  priority_  = 0.0f;
  timestamp_ = 0;
  found_     = false;
}

void loadFileListRecursive(hvh::Table<FixedString<64>>& fileList,
                           const sfs::path& parent, sfs::path dir) {
  // Loop through each entry in this directory...
  for (sfs::directory_iterator it(parent / dir);
       it != sfs::directory_iterator(); ++it) {
    // Entry doesn't exist (somehow..?)
    if (!sfs::exists(it->path()))
      continue;

    // Entry is a directory (recursion time!)
    if (sfs::is_directory(it->path())) {
      loadFileListRecursive(fileList, parent, dir / it->path().filename());
      continue;
    }

    // Make sure it's a file and not a symlink or pipe or something.
    if (!sfs::is_regular_file(it->path())) {
      continue;
    }

    // Make sure the path isn't too long (and also turn it into a FixedString).
    std::string filePath = (dir / it->path().filename()).string();
    if (filePath.size() > 63) {
      continue;
    }
    FixedString<64> myPath = filePath.c_str();

    // Remove backslashes from the path, and ensure the file isn't reserved.
    for (int i = 0; myPath.c_str[i] != '\0'; ++i) {
      if (myPath.c_str[i] == '\\') {
        myPath.c_str[i] = '/';
      }
    }
    // strip_backslashes(mypath.c_str);
    if (reservedFilenames_s.count(myPath) > 0) {
      continue;
    }

    // File appeared twice (somehow..?)
    if (fileList.count(myPath) > 0) {
      continue;
    }

    // FINALLY we can insert this file path into our list.
    fileList.insert(myPath);
  }
}

wc::Result::Empty wc::Module::loadFileList() {

  if (!found_) {
    return Result::error("Trying to load module before opening it.");
  }

  if (loaded_) {
    return Result::error("Trying to load module more than once.");
  }

  if (archive_.isOpen()) {
    fileTable_.reserve(archive_.numFiles());

    for (auto it : archive_) {
      fileTable_.insert(std::get<0>(it));
    }
  } else {
    loadFileListRecursive(fileTable_, path_, "");
  }
  loaded_ = true;
  return Result::success();
}

wc::Result::Value<std::vector<char>>
    wc::Module::loadFile(const FixedString<64>& filename) {
  std::vector<char> fileData;
  if (archive_.isOpen()) {
    Archive::timestamp_t nil;
    if (!archive_.extractData(filename.c_str, fileData, nil)) {
      return Result::error("File not found in archive, somehow.");
    }
  } else {
    sfs::path fullPath = getPath();
    fullPath /= filename.c_str;
    std::ifstream file(fullPath, std::ios::binary | std::ios::in);
    if (!file.is_open()) {
      return Result::error("Failed to open file.");
    }
    // Slurp up the file contents.
    file.seekg(0, file.end);
    size_t len = file.tellg();
    file.seekg(0, file.beg);
    fileData.resize(len);
    file.read(fileData.data(), fileData.size());
    if (!file) {
      return Result::error(
          std::format("Only read {} bytes out of {}.", file.gcount(), len));
    }
  }

  return Result::success(fileData);
}
