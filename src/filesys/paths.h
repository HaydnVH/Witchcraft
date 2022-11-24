#ifndef HVH_WC_FILESYS_PATHS_H
#define HVH_WC_FILESYS_PATHS_H

#include "appconfig.h"

#include <filesystem>

namespace wc {
	bool initPaths();
	const std::filesystem::path& getInstallPath();
	const std::filesystem::path& getUserPath();
}

#endif // HVH_WC_FILESYS_PATHS_H