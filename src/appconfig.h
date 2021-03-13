#ifndef HVH_WC_APPCONFIG_H
#define HVH_WC_APPCONFIG_H

#include <filesystem>
#include <string>

class _AppConfig {
public:

	constexpr static const char* APPCONFIG_FILENAME = "appconfig.json";

	static const _AppConfig& _GetSingleton() { static _AppConfig* instance = new _AppConfig(); return *instance; }

	std::string name = "Witchcraft Project";
	std::string company_name = "hvh";
	std::string version_string = "0.0.0";
	int major_ver = 0;
	int minor_ver = 0;
	int patch_ver = 0;

	std::string engine_name = "Witchcraft";
	std::string engine_version_string = "0.12.0";
	int engine_major_ver = 0;
	int engine_minor_ver = 12;
	int engine_patch_ver = 0;

	std::string install_dir;
	std::string user_dir;
	std::filesystem::path install_path;
	std::filesystem::path user_path;

private:
	_AppConfig();
};

inline const _AppConfig& appconfig() { return _AppConfig::_GetSingleton(); }

#endif // HVH_WC_APPCONFIG_H