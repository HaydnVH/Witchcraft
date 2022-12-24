#include "paths.h"

#ifdef PLATFORM_WIN32
  #include <Shlobj.h>
  #include <Windows.h>
#elif PLATFORM_SDL
  #include <SDL.h>
#endif

#include "appconfig.h"

namespace sfs = std::filesystem;

namespace {

  /// The install path should point to the location of the executable.
  sfs::path installPath_s;
  bool      installPathInitialized_s = false;
  /// The user path should point to a folder where we can save and load per-user
  /// files.
  sfs::path userPath_s;
  bool      userPathInitialized_s = false;

}  // namespace

const sfs::path& wc::getInstallPath() {

  if (!installPathInitialized_s) {
    // Get the path where the executable is located.
    // TODO: Actually do this properly!
    installPath_s            = sfs::current_path();
    installPathInitialized_s = true;
  }

  return installPath_s;
}

const sfs::path& wc::getUserPath() {

  if (!userPathInitialized_s) {
    // Get the path where user files ought to be stored.
    // This is platform-specific.
#ifdef PLATFORM_WIN32
    // In Win32 we have to use SHGetKnownFolderPath to find "%APPDATA%".
    PWSTR   str;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData,
                                          KF_FLAG_CREATE, NULL, &str);
    userPath_s = sfs::path(str);
    // Create "%APPDATA%/companyName/appName/"
    userPath_s = userPath_s / wc::COMPANY_NAME / wc::APP_NAME;
    std::error_code ec;
    sfs::create_directories(userPath_s, ec);
    if (ec) {
      userPath.clear();
      return userPath_s;
    }
    CoTaskMemFree(str);
#elif PLATFORM_SDL
    // In SDL we use SDL_GetPrefPath to find the user dir:
    // "%APPDATA%/companyName/appName/" on Windows,
    // "~/.local/share/appName/" on Linux.
    char* prefpath = SDL_GetPrefPath(wc::COMPANY_NAME, wc::APP_NAME);
    if (!prefpath) {
      userPath_s.clear();
      return userPath_s;
    }
    userPath_s = std::filesystem::u8path(prefpath);
    SDL_free(prefpath);
#endif

    userPathInitialized_s = true;
  }

  return userPath_s;
}