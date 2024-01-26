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
#include <format>
#include <fstream>
#include <string>
#include <vector>
namespace sfs = std::filesystem;

#include "dbg/debug.h"
#include "module.h"
#include "sys/appconfig.h"
#include "sys/paths.h"
#include "tools/htable.hpp"

namespace {
  wc::Filesystem* uniqueFilesystem_s = nullptr;
}

wc::Filesystem::Filesystem() {

  // Uniqueness check
  if (uniqueFilesystem_s)
    throw dbg::Exception("Only one wc::Filesystem object should exist.");
  uniqueFilesystem_s = this;

  dbg::info("Initializing filesystem and loading modules...");

  // Scan through the user data directory.
  sfs::path userDataPath = wc::getUserPath() / DATA_FOLDER;
  if (!sfs::exists(userDataPath))
    sfs::create_directories(userDataPath);
  for (sfs::directory_iterator it(userDataPath);
       it != sfs::directory_iterator(); ++it) {
    // If this entry is a directory or a .wcmod file...
    if (it->is_directory() || (it->is_regular_file() &&
                               it->path().extension().string() == MODULE_EXT)) {
      // Add it to the list of available modules.
      auto mod = Module::open(it->path());
      if (mod.isError() || !mod) {
        dbg::error(std::format("'{}' is not a valid package,\n{}",
                               trimPathStr(it->path()), mod.msg()));
        continue;
      }
      if (mod.isWarning() && mod.hasMsg())
        dbg::warnmore(mod.msg());
      if (modules_.count(mod->getName().c_str()) > 0)
        dbg::infomore("A package with this name is already present.");
      else
        modules_.insert(mod->getName().c_str(), std::move(*mod));
    } else
      dbg::error(
          std::format("'{}' is not a valid package.", trimPathStr(it->path())));
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
        auto mod = Module::open(it->path());
        if (mod.isError() || !mod) {
          dbg::error(std::format("'{}' is not a valid package,\n{}",
                                 trimPathStr(it->path()), mod.msg()));
          continue;
        }
        if (mod.isWarning() && mod.hasMsg())
          dbg::warnmore(mod.msg());
        if (modules_.count(mod->getName().c_str()) > 0)
          dbg::infomore("A package with this name is already present.");
        else
          modules_.insert(mod->getName().c_str(), std::move(*mod));
      } else
        dbg::error(std::format("'{}' is not a valid package.",
                               trimPathStr(it->path())));
    }
  }

  // Make sure we actually loaded something.
  if (modules_.size() == 0) {
    throw dbg::Exception("Failed to find any modules!\n");
  }

  // Sort the list of modules according to the load order of the modules.
  modules_.sort<1>();

  // Go ahead and load all the modules for now.
  for (size_t modindex = 0; modindex < modules_.size(); ++modindex) {

    size_t numFilesLoaded = loadModule(modindex);
    dbg::infomore(std::format("Loaded package '{}' with {} files(s).",
                              modules_.at<0>(modindex).c_str, numFilesLoaded));
  }
}

wc::Filesystem::~Filesystem() {

  for (auto it : modules_) {
    std::get<1>(it).close();
//    it.get<1>().close();
  }
  modules_.clear();
  files_.clear();
}

wc::Filesystem::FileProxy
    wc::Filesystem::getFile(const std::string_view filename, bool reverseSort) {
  std::vector<size_t> list;
  list.reserve(files_.count(filename));
  // Get the module indices from the map and save them in the list.
  for (auto& it : files_.find(filename)) {
    list.push_back(files_.at<1>(it.index()));
  }
  // Sort the list and return the proxy.
  if (list.size() > 1) {
    if (reverseSort)
      std::sort(list.begin(), list.end(), std::greater<size_t>());
    else
      std::sort(list.begin(), list.end());
  }
  return wc::Filesystem::FileProxy(*this, filename, std::move(list),
                                   reverseSort);
}

wc::FileResult wc::Filesystem::FileProxy::Iterator::load() {
  // If the list index points beyond the list,
  // either we've run out of files or the file we searched for doesn't exist.
  if (proxy_.list_.size() <= 0)
    return Result::error("File not found.");

  if (listIndex_ >= proxy_.list_.size())
    return Result::error("Iterator out of bounds.");

  // Grab the module index for the file we want.
  size_t moduleIndex = proxy_.list_[listIndex_];

  // Load the file data.
  return proxy_.vfs_.modules_.at<1>(moduleIndex)
      .loadFile(proxy_.filename_.c_str());
}

size_t wc::Filesystem::loadModule(size_t moduleIndex) {
  wc::Module& pkg = modules_.at<1>(moduleIndex);
  pkg.loadFileList();
  for (auto filename : pkg.getFileTable().viewColumn<0>()) {
    files_.insert(filename, moduleIndex);
  }
  return pkg.getFileTable().size();
}