#ifndef HVH_WC_APPCONFIG_H
#define HVH_WC_APPCONFIG_H

#include <filesystem>
#include <string>

namespace appconfig {

	std::string getAppName();
	std::string getCompanyName();
	std::string getVerString();
	int getMajorVer();
	int getMinorVer();
	int getPatchVer();

	std::string getEngineName();
	std::string getEngineVerString();
	int getEngineMajorVer();
	int getEngineMinorVer();
	int getEnginePatchVer();

	std::string getInstallDir();
	std::string getUserDir();
	std::filesystem::path getInstallPath();
	std::filesystem::path getUserPath();

	constexpr static const char* APPCONFIG_FILENAME = "appconfig.json";

	bool Init();
};

#endif // HVH_WC_APPCONFIG_H