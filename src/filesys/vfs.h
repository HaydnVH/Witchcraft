#ifndef HVH_WC_FILESYS_VIRTUALFILESYSTEM_H
#define HVH_WC_FILESYS_VIRTUALFILESYSTEM_H

#include <vector>
#include "filedata.h"

namespace wc {
namespace vfs {

	constexpr const char* PACKAGE_EXT = ".wcp";
	constexpr const char* PACKAGE_SAVE_EXT = ".wcs";

	constexpr const char* DATA_FOLDER = "data/";
	constexpr const char* SAVE_FOLDER = "saves/";
	constexpr const char* DEFAULT_PACKAGE = "engine_data";
	constexpr const char* ACTIVE_PACKAGE_PATH = "temp/active.wcs";

	constexpr const char* LOAD_ORDER_FILENAME = "load_order.json";

	// Initializes the virtual filesystem and loads default packages.
	// Returns false if initialization fails.
	bool init();

	// Cleans up the virtual filesystem.
	void shutdown();

	// Returns whether the virtual filesystem has been initialized.
	bool isInitialized();

	// Look at what's available, for debugging.
	void printKnownFiles();
	void printAvailablePackages();
	void printLoadedPackages();

	// LoadFile()
	// Loads the file with the given path.  If the file does not exist, the resulting filedata will not be open (is_open() == false).
	// If filename is NULL, we return the next instance of the file with the same path,
	// or a closed FileData if there are no more files with the same path.
	// Loop example: `for (FileData file = LoadFile(<name>); file.is_open(); file = LoadFile(nullptr)) { ... }`
	// By default, the files will be returned in the same order that the modules were loaded (first loaded, first returned).
	// If 'reverse_sort' is true, the files will be returned in reverse load order (last loaded, first returned).
	// 'reverse_sort' has no effect if the filename is NULL (you're iterating a previously-sorted list).
	FileData LoadFile(const char* u8path, bool reverse_sort = false);

}} // namespace wc::vfs

#endif // HVH_WC_FILESYS_VIRTUALFILESYSTEM_H