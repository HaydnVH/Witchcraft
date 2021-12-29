module;

#ifdef PLATFORM_WIN32
  #include <Windows.h>
  #include <Shlobj.h>
#elif PLATFORM_SDL
  #include <SDL.h>
#endif

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

#include <filesystem>
#include <string>
#include <fstream>
#include <sstream>
#include "tools/stringhelper.h"

export module appconfig;

namespace wc {
namespace appconfig {

	std::string app_name = "Witchcraft Project";
	export const std::string& getAppName() { return app_name; }

	std::string company_name = "hvh";
	export const std::string& getCompanyName() { return company_name; }

	std::string version_string = "0.0.0";
	export const std::string& getVerString() { return version_string; }

	int version_major = 0;
	export int getMajorVer() { return version_major; }

	int version_minor = 0;
	export int getMinorVer() { return version_minor; }

	int version_patch = 0;
	export int getPatchVer() { return version_patch; }

	std::string engine_name = "Witchcraft Engine";
	export const std::string& getEngineName() { return engine_name; }

	std::string engine_version_string = "0.12.1";
	export const std::string& getEngineVerString() { return engine_version_string; }

	int engine_version_major = 0;
	export int getEngineMajorVer() { return engine_version_major; }

	int engine_version_minor = 12;
	export int getEngineMinorVer() { return engine_version_minor; }

	int engine_version_patch = 1;
	export int getEnginePatchVer() { return engine_version_patch; }

	std::string directory_install = "";
	export const std::string& getInstallDir() { return directory_install; }

	std::string directory_user = "";
	export const std::string& getUserDir() { return directory_user; }

	std::filesystem::path path_install;
	export const std::filesystem::path& getInstallPath() { return path_install; }

	std::filesystem::path path_user;
	export const std::filesystem::path& getUserPath() { return path_user; }



	constexpr const char* APPCONFIG_FILENAME = "appconfig.json";

	export bool Init() {

		using namespace rapidjson;
		using namespace std;

		// First, we initialize the installation directory.
		// This determines where we'll find appconfig.json.
		// Eventually this will be wherever the executible is located,
		// for now we use the current working directory.
		directory_install = "";
		path_install = filesystem::u8path(directory_install);

		bool should_write_file = false;

		// Here we attempt to open up appconfig.json.
		string filepath = makestr(directory_install, APPCONFIG_FILENAME);
		ifstream file(filepath, ios::in | ios::binary);
		if (!file.is_open()) {
			should_write_file = true;
		}
		else {

			stringstream ss;
			ss << file.rdbuf();
			string contents = ss.str();

			Document doc;
			doc.Parse(contents.c_str());
			if (doc.HasParseError() || !doc.IsObject()) {
				should_write_file = true;
			}
			else {
				// We have appconfig.json, and it parsed successfully.  Lets get reading!
				if (doc.HasMember("name") && doc["name"].IsString()) {
					app_name = doc["name"].GetString();
				}
				else { should_write_file = true; }

				if (doc.HasMember("company name") && doc["company name"].IsString()) {
					company_name = doc["company name"].GetString();
				}
				else { should_write_file = true; }

				if (doc.HasMember("version") && doc["version"].IsString()) {
					version_string = doc["version"].GetString();
					sscanf(version_string.c_str(), "%i.%i.%i", &version_major, &version_minor, &version_patch);
				}
				else { should_write_file = true; }
			}
		}

		// Create and obtain the user directory using the information we fetched from appconfig.
		{
#ifdef PLATFORM_WIN32
			PWSTR str;
			HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &str);
			path_user = filesystem::path(str);
			path_user = path_user / company_name / app_name;
			error_code ec;
			filesystem::create_directories(path_user, ec);
			directory_user = path_user.u8string();
			strip_backslashes(directory_user);
			CoTaskMemFree(str);
#elif PLATFORM_SDL
			char* prefpath = SDL_GetPrefPath(company_name.c_str(), app_name.c_str());
			directory_user = prefpath;
			path_user = filesystem::u8path(directory_user);
			strip_backslashes(directory_user);
			SDL_free(prefpath);
#endif
		}

		// If we failed to load any information, we had to create that data from scratch,
		// and so we should really just re-write the whole config file.
		if (should_write_file) {
			Document doc;
			doc.SetObject();
			doc.AddMember(Value("name", doc.GetAllocator()).Move(), Value(app_name.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
			doc.AddMember(Value("company name", doc.GetAllocator()).Move(), Value(company_name.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
			doc.AddMember(Value("version", doc.GetAllocator()).Move(), Value(version_string.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());

			ofstream file(filepath, ios::out | ios::binary);
			if (file.is_open()) {
				StringBuffer buffer;
				PrettyWriter writer(buffer);
				doc.Accept(writer);
			}
		}

		return true;
	}

}}; // namespace wc::appconfig
