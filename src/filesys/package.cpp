#include "package.h"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
using namespace std;
namespace fs = std::filesystem;

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
using namespace rapidjson;

#include "sys/debug.h"

// #include "tools/stringhelper.h"

namespace {

  const hvh::htable<fixedstring<64>> reservedFilenames_s = {
      wc::PACKAGEINFO_FILENAME, "readme.txt", "splash.png", "config.json",
      "load_order.json"};

}  // namespace

bool wc::Package::open(const std::string_view u8path) {

  // Create the path, and make sure something exists there.
  fs::path myPath = fs::u8path(u8path);
  if (!fs::exists(myPath)) {
    dbg::error(fmt::format("'{}' does not exist.", u8path));
    return false;
  }

  vector<char> modinfo;

  // Check to see if the path points to a directory or a file.
  // If it's a file, we'l try to open it like an archive.
  if (fs::is_directory(myPath)) {
    ifstream file(myPath / PACKAGEINFO_FILENAME, ios::binary);
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
  } else if (fs::is_regular_file(myPath)) {
    if (archive_.open(myPath.string().c_str())) {
      Archive::timestamp_t nil;
      archive_.extract_data(PACKAGEINFO_FILENAME, modinfo, nil);
    }
  }

  name_ = myPath.filename().string();

  if (modinfo.empty()) {
    dbg::warning(
        fmt::format("Failed to open '{}/{}'.", u8path, PACKAGEINFO_FILENAME));
  } else {
    // Parse the file contents as json.
    Document doc;
    doc.Parse((char*)modinfo.data(), modinfo.size());
    if (doc.HasParseError() || !doc.IsObject()) {
      dbg::warning(fmt::format("Failed to parse '{}/{}'.", u8path,
                               PACKAGEINFO_FILENAME));
    } else {
      // Weve found and correctly parsed modinfo.json,
      // by this point we can confidently say that we're looking at a module.
      if (doc.HasMember("name")) { name_ = doc["name"].GetString(); }
      if (doc.HasMember("author")) { author_ = doc["author"].GetString(); }
      if (doc.HasMember("category")) {
        category_ = doc["category"].GetString();
      }
      if (doc.HasMember("description")) {
        description_ = doc["description"].GetString();
      }
      if (doc.HasMember("priority")) { priority_ = doc["priority"].GetFloat(); }
    }
  }

  // Timestamp is used to sort modules which have equal priority.
  timestamp_ = fs::last_write_time(myPath).time_since_epoch().count();

  path_  = u8path;
  found_ = true;
  return true;
}

void wc::Package::close() {
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

void loadFileListRecursive(hvh::htable<fixedstring<64>>& fileList,
                           const fs::path& parent, fs::path dir) {
  // Loop through each entry in this directory...
  for (fs::directory_iterator it(parent / dir); it != fs::directory_iterator();
       ++it) {
    // Entry doesn't exist (somehow..?)
    if (!fs::exists(it->path())) continue;

    // Entry is a directory (recursion time!)
    if (fs::is_directory(it->path())) {
      loadFileListRecursive(fileList, parent, dir / it->path().filename());
      continue;
    }

    // Make sure it's a file and not a symlink or pipe or something.
    if (!fs::is_regular_file(it->path())) { continue; }

    // Make sure the path isn't too long (and also turn it into a fixedstring).
    std::string filePath = (dir / it->path().filename()).string();
    if (filePath.size() > 63) { continue; }
    fixedstring<64> myPath = filePath.c_str();

    // Remove backslashes from the path, and ensure the file isn't reserved.
    for (int i = 0; myPath.c_str[i] != '\0'; ++i) {
      if (myPath.c_str[i] == '\\') { myPath.c_str[i] = '/'; }
    }
    // strip_backslashes(mypath.c_str);
    if (reservedFilenames_s.count(myPath) > 0) { continue; }

    // File appeared twice (somehow..?)
    if (fileList.count(myPath) > 0) { continue; }

    // FINALLY we can insert this file path into our list.
    fileList.insert(myPath);
  }
}

void wc::Package::loadFileList() {

  if (!found_) {
    dbg::error("Trying to load package before opening it.\n");
    return;
  }

  if (loaded_) {
    dbg::errmore("Trying to load package more than once.\n");
    return;
  }

  if (archive_.is_open()) {
    fileTable_.reserve(archive_.num_files());

    fixedstring<64> fname;
    for (bool exists = archive_.iterate_files(fname, true); exists;
         exists      = archive_.iterate_files(fname, false)) {
      fileTable_.insert(fname);
    }
  } else {
    loadFileListRecursive(fileTable_, path_, "");
  }
  loaded_ = true;
}
