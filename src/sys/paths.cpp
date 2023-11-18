/******************************************************************************
 * paths.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Creates the user and install paths.  These paths are initialized the first
 * time they are used, so there's no need to initialize explicitly.
 *****************************************************************************/
#include "paths.h"

#ifdef PLATFORM_SDL
  #include <SDL2/SDL.h>
#elif PLATFORM_WIN32
  #include <Shlobj.h>
  #include <Windows.h>
#endif

#include "appconfig.h"

#include <regex>

namespace sfs = std::filesystem;

namespace {

  /// The install path should point to the location of the executable.
  sfs::path   installPath_s;
  std::string installStr_s;
  bool        installPathInitialized_s = false;
  /// The user path should point to a folder where we can save and load per-user
  /// files.
  sfs::path   userPath_s;
  std::string userStr_s;
  bool        userPathInitialized_s = false;

}  // namespace

const sfs::path& wc::getInstallPath() {

  if (!installPathInitialized_s) {
    // Get the path where the executable is located.
    // TODO: Actually do this properly!
    installPath_s            = sfs::canonical(sfs::current_path());
    installStr_s             = installPath_s.string();
    installPathInitialized_s = true;
  }

  return installPath_s;
}

const sfs::path& wc::getUserPath() {

  if (!userPathInitialized_s) {
    // Get the path where user files ought to be stored.
    // This is platform-specific.
#ifdef PLATFORM_SDL
    // In SDL we use SDL_GetPrefPath to find the user dir:
    // "%APPDATA%/companyName/appName/" on Windows,
    // "~/.local/share/appName/" on Linux.
    char* prefpath = SDL_GetPrefPath(wc::COMPANY_NAME, wc::APP_NAME);
    if (!prefpath) {
      userPath_s.clear();
      return userPath_s;
    }
    userPath_s = sfs::canonical(sfs::u8path(prefpath));
    SDL_free(prefpath);
#elif PLATFORM_WIN32
    // In Win32 we have to use SHGetKnownFolderPath to find "%APPDATA%".
    PWSTR   str;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData,
                                          KF_FLAG_CREATE, NULL, &str);
    userPath_s     = sfs::path(str);
    // Create "%APPDATA%/companyName/appName/"
    userPath_s = userPath_s / wc::COMPANY_NAME / wc::APP_NAME;
    std::error_code ec;
    sfs::create_directories(userPath_s, ec);
    if (ec) {
      userPath.clear();
      return userPath_s;
    }
    CoTaskMemFree(str);
#endif

    userStr_s             = userPath_s.string();
    userPathInitialized_s = true;
  }

  return userPath_s;
}

std::string wc::trimPathStr(std::string pathString) {
  std::string result;
  getInstallPath();
  getUserPath();
  size_t foundPos = std::string::npos;
  if ((foundPos = pathString.find(installStr_s)) != std::string::npos) {
    result = pathString.replace(foundPos, installStr_s.size(),
                                INSTALL_PATH_REPLACEMENT);
  } else if ((foundPos = pathString.find(userStr_s)) != std::string::npos) {
    result =
        pathString.replace(foundPos, userStr_s.size(), USER_PATH_REPLACEMENT);
  } else {
    result = std::move(pathString);
  }

  for (auto& c : result) {
    if (c == '\\') c = '/';
  }
  return std::move(result);
}