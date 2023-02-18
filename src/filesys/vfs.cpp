/******************************************************************************
 * vfs.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Implementation for the virtual filesystem used by Witchcraft.
 * Folders and archives can be registered and when a file is requested,
 * each folder and archive which contains the requested file can provide it.
 * This is the fundamental backbone of how modding is implemented.
 *****************************************************************************/
#include "vfs.h"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <vector>
namespace sfs = std::filesystem;

#include "module.h"
#include "sys/appconfig.h"
#include "sys/debug.h"
#include "sys/paths.h"
#include "tools/htable.hpp"

namespace {

  hvh::Table<FixedString<64>, wc::Module> modules;
  hvh::Table<FixedString<64>, size_t>     files;

  size_t loadModule(size_t moduleIndex) {
    wc::Module& pkg = modules.at<1>(moduleIndex);
    pkg.loadFileList();
    for (auto& it : pkg.getFileTable()) {
      files.insert(it.get<0>(), moduleIndex);
    }
    return pkg.getFileTable().size();
  }

}  // namespace

bool wc::vfs::init() {
  if (isInitialized()) {
    dbg::fatal("Filesystem cannot be initialized more than once!\n");
    return false;
  }

  dbg::info("Initializing filesystem and loading modules...");

  // Scan through the user data directory.
  sfs::path userDataPath = wc::getUserPath() / DATA_FOLDER;
  if (!sfs::exists(userDataPath)) sfs::create_directories(userDataPath);
  for (sfs::directory_iterator it(userDataPath);
       it != sfs::directory_iterator(); ++it) {
    // If this entry is a directory or a .wcmod file...
    if (it->is_directory() || (it->is_regular_file() &&
                               it->path().extension().string() == MODULE_EXT)) {
      // Add it to the list of available modules.
      Module mod;
      auto   openResult = mod.open(it->path());
      if (openResult.isError()) {
        dbg::error({fmt::format("'{}' is not a valid package.",
                                trimPathStr(it->path())),
                    openResult.message()});
        continue;
      }
      if (openResult.isWarning() && openResult.hasMessage())
        dbg::warnmore(openResult.message());
      if (modules.count(mod.getName().c_str()) > 0)
        dbg::infomore("A package with this name is already present.");
      else
        modules.insert(mod.getName().c_str(), std::move(mod));
    } else
      dbg::error(
          fmt::format("'{}' is not a valid package.", trimPathStr(it->path())));
  }

  // Scan through the install data directory.
  sfs::path installDataPath = wc::getInstallPath() / DATA_FOLDER;
  if (sfs::exists(installDataPath)) {
    for (sfs::directory_iterator it(installDataPath);
         it != sfs::directory_iterator(); ++it) {
      // If this entry is a directory or a .wcmod file...
      if (it->is_directory() ||
          (it->is_regular_file() &&
           it->path().extension().string() == MODULE_EXT)) {
        // Add it to the list of available modules.
        Module mod;
        auto   openResult = mod.open(it->path());
        if (openResult.isError()) {
          dbg::error({fmt::format("'{}' is not a valid package.",
                                  trimPathStr(it->path())),
                      openResult.message()});
          continue;
        }
        if (openResult.isWarning() && openResult.hasMessage())
          dbg::warnmore(openResult.message());
        if (modules.count(mod.getName().c_str()) > 0)
          dbg::infomore("A package with this name is already present.");
        else
          modules.insert(mod.getName().c_str(), std::move(mod));
      } else
        dbg::error(fmt::format("'{}' is not a valid package.",
                               trimPathStr(it->path())));
    }
  }

  if (modules.size() == 0) {
    dbg::fatal("Failed to find any modules!\n");
    return false;
  }

  // Sort the list of modules according to the load order of the modules.
  modules.sort<1>();

  // Go ahead and load all the modules for now.
  for (size_t modindex = 0; modindex < modules.size(); ++modindex) {

    size_t numFilesLoaded = loadModule(modindex);
    dbg::infomore(fmt::format("Loaded package '{}' with {} files(s).",
                              modules.at<0>(modindex).c_str, numFilesLoaded));
  }

  return true;
}

void wc::vfs::shutdown() {

  for (auto& it : modules) { it.get<1>().close(); }
  modules.clear();
  files.clear();
}

bool wc::vfs::isInitialized() { return (modules.size() > 0); }

wc::vfs::FileProxy wc::vfs::getFile(const std::string_view filename,
                                    bool                   reverseSort) {
  std::vector<size_t> list;
  list.reserve(files.count(filename));
  // Get the module indices from the map and save them in the list.
  for (auto& it : files.find(filename)) {
    list.push_back(files.at<1>(it.index()));
  }
  // Sort the list and return the proxy.
  if (list.size() > 1) {
    if (reverseSort)
      std::sort(list.begin(), list.end(), std::greater<size_t>());
    else
      std::sort(list.begin(), list.end());
  }
  return wc::vfs::FileProxy(filename, std::move(list), reverseSort);
}

wc::Result wc::vfs::FileProxy::Iterator::load() {
  // If the list index points beyond the list,
  // either we've run out of files or the file we searched for doesn't exist.
  if (proxy_.list_.size() <= 0) return Result::error("File not found.");

  if (listIndex_ >= proxy_.list_.size())
    return Result::error("Iterator out of bounds.");

  // Grab the module index for the file we want.
  size_t moduleIndex = proxy_.list_[listIndex_];

  // Load the file data.
  return modules.at<1>(moduleIndex).loadFile(proxy_.filename_.c_str());
}