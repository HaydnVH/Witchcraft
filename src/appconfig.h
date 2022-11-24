/******************************************************************************
 * appconfig.h
 * Part of the Witchcraft Engine 
 * © Copyright Haydn V. Harach 2022
 * Last modified October 2022
 ******************************************************************************
 * This file defines the name and version of the application,
 * in addition to the name and version of the engine used to create it.
 * It is intended to be modified in order to create a unique project.
 * The information defined in this file defines where the user directory should be.
 *****************************************************************************/

#ifndef HVH_WC_APPCONFIG_H
#define HVH_WC_APPCONFIG_H

#include <filesystem>
#include <string>

namespace wc::appconfig {

	// The following fields are intended to be modified on a per-project basis.
	constexpr const char* AppName = "Witchcraft Project";
	constexpr const char* CompanyName = "hvh";
	constexpr const char* AppVer = "0.0.0";
	constexpr const int AppMajorVer = 0;
	constexpr const int AppMinorVer = 0;
	constexpr const int AppPatchVer = 0;

	// The following fields should not be modified unless updating the engine.
	constexpr const char* EngineName = "Witchcraft";
	constexpr const char* EngineVer = "0.12.2";
	constexpr const int EngineMajorVer = 0;
	constexpr const int EngineMinorVer = 12;
	constexpr const int EnginePatchVer = 2;

}; // namespace wc::appconfig

#endif // HVH_WC_APPCONFIG_H