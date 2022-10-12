#include "appconfig.h"

#ifdef PLATFORM_WIN32
  #include <Windows.h>
  #include <Shlobj.h>
#elif PLATFORM_SDL
  #include <SDL.h>
#endif

//#include <rapidjson/document.h>
//#include <rapidjson/filereadstream.h>
//#include <rapidjson/filewritestream.h>
//#include <rapidjson/prettywriter.h>
//using namespace rapidjson;

//#include <fstream>
//#include <sstream>
#include "tools/stringhelper.h"
using namespace std;

namespace wc {
namespace appconfig {



	static std::string install_dir;
	static std::filesystem::path install_path;
	static bool install_path_initialized = false;
	void initInstallPath(std::string& dir, std::filesystem::path& path) {
		path = std::filesystem::current_path();
		dir = path.string();
	}

	const std::string& getInstallDir() {
		if (!install_path_initialized) {
			initInstallPath(install_dir, install_path);
			install_path_initialized = true;
		}
		return install_dir;
	}
	const std::filesystem::path& getInstallPath() {
		if (!install_path_initialized) {
			initInstallPath(install_dir, install_path);
			install_path_initialized = true;
		}
		return install_path;
	}

	static std::string user_dir;
	static std::filesystem::path user_path;
	static bool user_path_initialized = false;
	void initUserPath(std::string& dir, std::filesystem::path& path) {
#ifdef PLATFORM_WIN32
		PWSTR str;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &str);
		path = filesystem::path(str);
		path = user_path / COMPANY_NAME / APP_NAME;
		error_code ec;
		filesystem::create_directories(path, ec);
		dir = user_path.string();
		strip_backslashes(dir);
		CoTaskMemFree(str);
#elif PLATFORM_SDL
		char* prefpath = SDL_GetPrefPath(COMPANY_NAME, APP_NAME);
		dir = prefpath;
		path = filesystem::u8path(dir);
		strip_backslashes(dir);
		SDL_free(prefpath);
#endif
	}

	const std::string& getUserDir() {
		if (!user_path_initialized) {
			initUserPath(user_dir, user_path);
			user_path_initialized = true;
		}
		return user_dir;
	}
	const std::filesystem::path& getUserPath() {
		if (!user_path_initialized) {
			initUserPath(user_dir, user_path);
			user_path_initialized = true;
		}
		return user_path;
	}

}} // namespace wc::appconfig