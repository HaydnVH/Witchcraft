#ifndef HVH_WC_FILESYS_VIRTUALFILESYSTEM_H
#define HVH_WC_FILESYS_VIRTUALFILESYSTEM_H

#include <vector>
#include "filedata.h"

namespace wc {
namespace vfs {

	// Initializes the virtual filesystem and loads default packages.
	// Returns false if initialization fails.
	bool init();

	// Cleans up the virtual filesystem.
	void shutdown();

	// Returns whether the virtual filesystem has been initialized.
	bool isInitialized();

	void printKnownFiles();
	void printAvailablePackages();
	void printLoadedPackages();

//	InFile loadFile(const char* filename, std::ios::openmode mode = 0);

}} // namespace wc::vfs

#endif // HVH_WC_FILESYS_VIRTUALFILESYSTEM_H