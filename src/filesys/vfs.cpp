#include "vfs.h"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
namespace sfs = std::filesystem;

#include "package.h"
#include "sys/appconfig.h"
#include "sys/debug.h"
#include "sys/paths.h"
#include "tools/htable.hpp"
// #include "tools/stringhelper.h"

namespace {

  hvh::htable<fixedstring<64>, wc::Package> packages;
  hvh::htable<fixedstring<64>, size_t>      files;

  size_t loadModule(size_t module_index) {
    wc::Package& pkg = packages.at<1>(module_index);
    pkg.loadFileList();
    for (size_t i = 0; i < pkg.getFileTable().size(); ++i) {
      files.insert(pkg.getFileTable().at<0>(i), module_index);
    }
    return pkg.getFileTable().size();
  }

}  // namespace

bool wc::vfs::init() {
  if (isInitialized()) {
    dbg::fatal("Filesystem cannot be initialized more than once!\n");
    return false;
  }

  // Scan through the user data directory.
  sfs::path user_data_path = wc::getUserPath() / DATA_FOLDER;
  dbg::info(
      fmt::format("Looking for packages in {}...", user_data_path.string()));
  if (!sfs::exists(user_data_path)) sfs::create_directories(user_data_path);
  for (sfs::directory_iterator it(user_data_path);
       it != sfs::directory_iterator(); ++it) {
    // If this entry is a directory or a .wcp file...
    if (it->is_directory() ||
        (it->is_regular_file() &&
         it->path().extension().string() == PACKAGE_EXT)) {
      // Add it to the list of available modules.
      Package pkg;
      if (pkg.open(it->path().string().c_str())) {
        dbg::info(fmt::format("Found package '$user/{}'.",
                              it->path().filename().string().c_str()));
        if (packages.count(pkg.getName().c_str()) > 0)
          dbg::infomore("A package with this name is already present.");
        else
          packages.insert(pkg.getName().c_str(), std::move(pkg));
      } else
        dbg::error(fmt::format("'$user/{}' is not a valid package.",
                               it->path().filename().string().c_str()));
    } else
      dbg::error(fmt::format("'$user/{}' is not a valid package.",
                             it->path().filename().string().c_str()));
  }

  // Scan through the install data directory.
  sfs::path install_data_path = wc::getInstallPath() / DATA_FOLDER;
  dbg::info(
      fmt::format("Looking for packages in {}...", install_data_path.string()));
  if (!sfs::exists(install_data_path))
    sfs::create_directories(install_data_path);
  for (sfs::directory_iterator it(install_data_path);
       it != sfs::directory_iterator(); ++it) {
    // If this entry is a directory or a .wcmod file...
    if (it->is_directory() ||
        (it->is_regular_file() &&
         it->path().extension().string() == PACKAGE_EXT)) {
      // Add it to the list of available modules.
      Package pkg;
      if (pkg.open(it->path().string().c_str())) {
        dbg::info(fmt::format("Found package '$install/{}'.",
                              it->path().filename().string().c_str()));
        if (packages.count(pkg.getName().c_str()) > 0)
          dbg::infomore("A package with this name is already present.");
        else
          packages.insert(pkg.getName().c_str(), std::move(pkg));
      } else
        dbg::error(fmt::format("'$install/{}' is not a valid package.",
                               it->path().filename().string().c_str()));
    } else
      dbg::error(fmt::format("'$install/{}' is not a valid package.",
                             it->path().filename().string().c_str()));
  }

  if (packages.size() == 0) {
    dbg::fatal("Failed to find any packages!\n");
    return false;
  }

  // Sort the list of modules according to the load order of the modules.
  packages.sort<1>();

  // Go ahead and load all the packages for now.
  for (size_t pkgindex = 0; pkgindex < packages.size(); ++pkgindex) {
    dbg::info(
        fmt::format("Loading package '{}'.", packages.at<0>(pkgindex).c_str));
    size_t num_files_loaded = loadModule(pkgindex);
    dbg::infomore(fmt::format("Found {} file(s).", num_files_loaded));
  }

  // Find default modules and load them.
  /*
  std::vector<const char*> default_module_names =
  Config::ReadStringArray("filesys", "default modules"); size_t
  loaded_default_modules = 0; for (const char* name : default_module_names) {
      size_t index = packages.find(name);
      if (index == SIZE_MAX) debug::error("Failed to find default module
  '%s'.\n", name); else { debug::info("Loading package '",
  packages.at<0>(index), "'.\n"); load_module(index);
          ++loaded_default_modules;
      }
  }

  if (loaded_default_modules == 0) {
      debug::fatal("Failed to load any default modules!\n");
      return false;
  }
  */

  return true;
}

void wc::vfs::shutdown() {
  for (size_t i = 0; i < packages.size(); ++i) { packages.at<1>(i).close(); }
  packages.clear();
  files.clear();
}

bool wc::vfs::isInitialized() { return (packages.size() > 0); }

std::optional<std::vector<char>> wc::vfs::LoadFile(const char* u8path,
                                                   bool        reverse_sort) {
  // This persistent list and its index allow us to continue our search after we
  // load the first file.
  static std::vector<size_t> list;
  static size_t              list_index;

  // If path is not null, then we're not continuing from last time.
  if (u8path) {
    // Clear the list and tell it to reserve enough space for our files.
    list.clear();
    list.reserve(files.count(u8path));

    // Get the module indices from the map and save them in the list.
    for (size_t index = files.find(u8path, true); index != SIZE_MAX;
         index        = files.find(u8path, false)) {
      list.push_back(files.at<1>(index));
    }

    // Sort the list and reset the index.
    if (list.size() > 1) {
      if (reverse_sort)
        std::sort(list.begin(), list.end(), std::greater<size_t>());
      else
        std::sort(list.begin(), list.end());
    }
    list_index = 0;
  }

  // If the list index points beyond the list,
  // either we've run out of files or the file we searched for doesn't exist.
  if (list_index >= list.size()) return std::vector<char>();

  // Grab the module index for the file we want, then increment the index for
  // next time.
  size_t module_index = list[list_index];
  ++list_index;

  // Load the file data.
  Package*          pkg = &packages.at<1>(module_index);
  Archive&          arc = pkg->getArchive();
  std::vector<char> result;
  if (arc.is_open()) {
    Archive::timestamp_t nil;
    if (!arc.extract_data(u8path, result, nil)) {
      dbg::error({fmt::format("While loading file '{}'.", u8path),
                  "File not found in archive, somehow."});
      return std::nullopt;
    }
  } else {
    sfs::path fullpath = sfs::u8path(pkg->getPath());
    fullpath /= u8path;
    ifstream file(fullpath.c_str(), ios::binary | ios::in);
    if (!file.is_open()) {
      dbg::error({fmt::format("While loading file '{}'.", fullpath.string()),
                  "Failed to open file."});
      return std::nullopt;
    }
    // Slurp up the file contents.
    file.seekg(0, file.end);
    size_t len = file.tellg();
    file.seekg(0, file.beg);
    result.resize(len);
    file.read(result.data(), result.size());
    if (!file) {
      dbg::error(
          {fmt::format("While loading file '{}'.", fullpath.string()),
           fmt::format("Only read {} bytes out of {}.", file.gcount(), len)});
      return std::nullopt;
    }
  }

  return result;
}