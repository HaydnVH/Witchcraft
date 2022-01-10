#include "vfs.h"

#include <vector>
#include <string>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

#include "tools/htable.hpp"
#include "tools/stringhelper.h"
#include "appconfig.h"
#include "debug.h"

namespace wc {
namespace vfs {

	namespace {

		hvh::htable<fixedstring<64>, Package> packages;
		hvh::htable<fixedstring<64>, size_t> files;

		void load_module(size_t module_index) {
			Package& pkg = packages.at<1>(module_index);
			pkg.loadFileList();
			for (size_t i = 0; i < pkg.getFileTable().size(); ++i) {
				files.insert(pkg.getFileTable().at<0>(i), module_index);
			}
		}

	} // namespace <anon>


	bool init() {
		if (isInitialized()) {
			debug::fatal("Filesystem cannot be initialized more than once!\n");
			return false;
		}

		// Scan through the user data directory.
		fs::path user_data_path = appconfig::getUserPath() / DATA_FOLDER;
		if (!fs::exists(user_data_path)) fs::create_directories(user_data_path);
		for (fs::directory_iterator it(user_data_path); it != fs::directory_iterator(); ++it) {
			// If this entry is a directory or a .wcmod file...
			if (it->is_directory() ||
				(it->is_regular_file() && it->path().extension().string() == PACKAGE_EXT))
			{
				// Add it to the list of available modules.
				Package pkg;
				if (pkg.open(it->path().u8string().c_str())) {
					debug::info("Found package '$install/", it->path().filename().u8string().c_str(), "'.\n");
					if (packages.count(pkg.getName().c_str()) > 0)
						debug::infomore("A package with this name is already present.\n");
					else
						packages.insert(pkg.getName().c_str(), std::move(pkg));
				}
				else debug::error("'$user/", it->path().filename().u8string().c_str(), "' is not a valid package.\n");
			}
			else debug::error("'$user/", it->path().filename().u8string().c_str(), "' is not a valid package.\n");
		}

		// Scan through the install data directory.
		fs::path install_data_path = appconfig::getInstallPath() / DATA_FOLDER;
		if (!fs::exists(install_data_path)) fs::create_directories(install_data_path);
		for (fs::directory_iterator it(install_data_path); it != fs::directory_iterator(); ++it) {
			// If this entry is a directory or a .wcmod file...
			if (it->is_directory() ||
				(it->is_regular_file() && it->path().extension().string() == PACKAGE_EXT))
			{
				// Add it to the list of available modules.
				Package pkg;
				if (pkg.open(it->path().u8string().c_str())) {
					debug::info("Found package '$install/", it->path().filename().u8string().c_str(), "'.\n");
					if (packages.count(pkg.getName().c_str()) > 0)
						debug::infomore("A package with this name is already present.\n");
					else
						packages.insert(pkg.getName().c_str(), std::move(pkg));
				}
				else debug::error("'$install/", it->path().filename().u8string().c_str(), "' is not a valid package.\n");
			}
			else debug::error("'$install/", it->path().filename().u8string().c_str(), "' is not a valid package.\n");
		}

		if (packages.size() == 0) {
			debug::fatal("Failed to find any packages!\n");
			return false;
		}

		// Sort the list of modules according to the load order of the modules.
		packages.sort<1>();

		// Find default modules and load them.
		/*
		std::vector<const char*> default_module_names = Config::ReadStringArray("filesys", "default modules");
		size_t loaded_default_modules = 0;
		for (const char* name : default_module_names) {
			size_t index = packages.find(name);
			if (index == SIZE_MAX) debug::error("Failed to find default module '%s'.\n", name);
			else {
				debug::info("Loading module '", packages.at<0>(index), "'.\n");
				load_module(index);
				++loaded_default_modules;
			}
		}

		if (loaded_default_modules == 0) {
			debug::fatal("Failed to load any default modules!\n");
			return false;
		}
		*/

		return true;
	}

	void shutdown() {
		for (size_t i = 0; i < packages.size(); ++i) {
			packages.at<1>(i).close();
		}
		packages.clear();
		files.clear();
	}

	bool isInitialized() {
		return (packages.size() > 0);
	}


}} // namespace wc::vfs