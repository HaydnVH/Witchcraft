#ifndef HVH_WC_APPCONFIG_H
#define HVH_WC_APPCONFIG_H

#include <filesystem>
#include <string>

namespace wc {
namespace appconfig {

	const std::string& getAppName();
	const std::string& getCompanyName();
	const std::string& getVerString();
	int getMajorVer();
	int getMinorVer();
	int getPatchVer();

	const std::string& getEngineName();
	const std::string& getEngineVerString();
	int getEngineMajorVer();
	int getEngineMinorVer();
	int getEnginePatchVer();

	const std::string& getInstallDir();
	const std::string& getUserDir();
	const std::filesystem::path& getInstallPath();
	const std::filesystem::path& getUserPath();

	constexpr const char* APPCONFIG_FILENAME = "appconfig.json";

	bool Init();

}}; // namespace wc::appconfig

#endif // HVH_WC_APPCONFIG_H