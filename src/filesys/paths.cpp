#include "paths.h"

#ifdef PLATFORM_WIN32
  #include <Windows.h>
  #include <Shlobj.h>
#elif PLATFORM_SDL
  #include <SDL.h>
#endif


namespace sfs = std::filesystem;

namespace {

	// The install path and user path are determined when the application starts,
	// and will never change afterwards.
	sfs::path installPath_s;
	sfs::path userPath_s;

} // namespace <anon>

bool wc::initPaths() {

	// Get the path where the executable is located
	// TODO: Actually do this properly instead of just getting the current working directory.
	installPath_s = sfs::current_path();

	// Get the path where user files ought to be stored.
	// This is platform-specific.
#ifdef PLATFORM_WIN32
	// In Win32 we have to use SHGetKnownFolderPath to find "%APPDATA%".
	PWSTR str;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &str);
	userPath_s = filesystem::path(str);
	// Create "%APPDATA%/companyName/appName/"
	userPath_s = userPath_s / wc::appconfig::CompanyName / wc::appconfig::AppName;
	error_code ec;
	filesystem::create_directories(userPath_s, ec);
	if (ec) { return false; }
	CoTaskMemFree(str);
#elif PLATFORM_SDL
	// In SDL we use SDL_GetPrefPath to find the user dir:
	// "%APPDATA%/companyName/appName/" on Windows,
	// "~/.local/share/appName/" on Linux.
	char* prefpath = SDL_GetPrefPath(wc::appconfig::CompanyName, wc::appconfig::AppName);
	if (!prefpath) { return false; }
	userPath_s = std::filesystem::u8path(prefpath);
	SDL_free(prefpath);
#endif

	return true;
}

const sfs::path& wc::getInstallPath() {
	return installPath_s;
}

const sfs::path& wc::getUserPath() {
	return userPath_s;
}