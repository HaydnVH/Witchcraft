#ifndef HVH_WC_APPCONFIG_H
#define HVH_WC_APPCONFIG_H

#include <filesystem>
#include <string>

namespace wc {
namespace appconfig {

	constexpr const char* APP_NAME = "Witchcraft Project";
	constexpr const char* COMPANY_NAME = "hvh";
	constexpr const char* APP_VER_STR = "0.0.0";
	constexpr const int APP_MAJOR_VER = 0;
	constexpr const int APP_MINOR_VER = 0;
	constexpr const int APP_PATCH_VER = 0;

	constexpr const char* ENGINE_NAME = "Witchcraft";
	constexpr const char* ENGINE_VER_STR = "0.12.2";
	constexpr const int ENGINE_MAJOR_VER = 0;
	constexpr const int ENGINE_MINOR_VER = 12;
	constexpr const int ENGINE_PATCH_VER = 2;

	const std::string& getInstallDir();
	const std::string& getUserDir();
	const std::filesystem::path& getInstallPath();
	const std::filesystem::path& getUserPath();

}}; // namespace wc::appconfig

#endif // HVH_WC_APPCONFIG_H