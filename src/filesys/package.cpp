#include "package.h"

#include <filesystem>
namespace fs = std::filesystem;
#include "filedata.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
using namespace rapidjson;

#include "debug.h"

#include "tools/stringhelper.h"

namespace wc {

	namespace {

		const hvh::htable<fixedstring<64>> reserved_filenames = {
			PACKAGEINFO_FILENAME,
			"readme.txt",
			"splash.png",
			"config.json",
			"load_order.json"
		};

	} // namespace <anon>

	bool Package::open(const char* u8path) {
		// Make sure the input isn't NULL.
		if (u8path == nullptr) {
			debug::error("In wc::Package::open():\n");
			debug::errmore("'path' is null.\n");
			return false;
		}

		// Make sure the path isn't too long; we only allocate 256 bytes to store the utf8 path.
		if (strlen(u8path) > sizeof(module_path)) {
			debug::error("In wc::Package::open():\n");
			debug::errmore("Path '", u8path, "' is too long (255 bytes max).\n");
			return false;
		}

		// Create the path, and make sure something exists there.
		fs::path mypath = fs::u8path(u8path);
		if (!fs::exists(mypath)) {
			debug::error("In wc::Package::open():\n");
			debug::errmore("'", u8path, "' does not exist.\n");
			return false;
		}

		FileData modinfo;

		// Check to see if the path points to a directory or a file.
		// If it's a file, we'l try to open it like an archive.
		if (fs::is_directory(mypath)) {
			modinfo = FileData(mypath, PACKAGEINFO_FILENAME);
		}
		else if (fs::is_regular_file(mypath)) {
			if (archive.open(mypath.u8string().c_str())) {
				modinfo = FileData(archive, PACKAGEINFO_FILENAME);
			}
		}

		if (!modinfo.is_open()) {
			debug::error("In wc::Package::open():\n");
			debug::errmore("Failed to open '", u8path, "/", PACKAGEINFO_FILENAME, "': ", modinfo.getError(), "\n");
			return false;
		}

		// Parse the file contents as json.
		Document doc;
		doc.Parse((char*)modinfo.data(), modinfo.size());
		if (doc.HasParseError() || !doc.IsObject()) {
			debug::error("In wc::Package::open():\n");
			debug::errmore("Failed to parse '", u8path, "/", PACKAGEINFO_FILENAME, "'.\n");
			archive.close();
			return false;
		}

		// Weve found and correctly parsed modinfo.json,
		// by this point we can confidently say that we're looking at a module.
		if (doc.HasMember("name")) { strncpy(name, doc["name"].GetString(), sizeof(name)); }
		else { strncpy(name, mypath.filename().u8string().c_str(), sizeof(name)); }
		if (doc.HasMember("author")) { strncpy(author, doc["author"].GetString(), sizeof(author)); }
		if (doc.HasMember("category")) { strncpy(category, doc["category"].GetString(), sizeof(category)); }
		if (doc.HasMember("description")) { strncpy(description, doc["description"].GetString(), sizeof(description)); }
		if (doc.HasMember("priority")) { priority = doc["priority"].GetFloat(); }

		// Timestamp is used to sort modules which have equal priority.
		timestamp = fs::last_write_time(mypath).time_since_epoch().count();

		strncpy(module_path, u8path, sizeof(module_path));
		found = true;
		return true;
	}

	void Package::close() {
		archive.close();
		file_list.clear();
		loaded = false;

		module_path[0] = '\0';
		name[0] = '\0';
		author[0] = '\0';
		description[0] = '\0';
		description[0] = '\0';
		priority = 0.0f;
		timestamp = 0;
		found = false;
	}

	void loadFileListRecursive(hvh::htable<fixedstring<64>>& file_list, const fs::path& parent, fs::path dir) {
		// Loop through each entry in this directory...
		for (fs::directory_iterator it(parent / dir); it != fs::directory_iterator(); ++it) {
			// Entry doesn't exist (somehow..?)
			if (!fs::exists(it->path()))
				continue;

			// Entry is a directory (recursion time!)
			if (fs::is_directory(it->path())) {
				loadFileListRecursive(file_list, parent, dir / it->path().filename());
				continue;
			}

			// Make sure it's a file and not a symlink or pipe or something.
			if (!fs::is_regular_file(it->path())) {
				continue;
			}

			// Make sure the path isn't too long (and also turn it into a fixedstring).
			std::string filepath = (dir / it->path().filename()).u8string();
			if (filepath.size() > 63) {
				continue;
			}
			fixedstring<64> mypath = filepath.c_str();

			// Remove backslashes from the path, and ensure the file isn't reserved.
			strip_backslashes(mypath.c_str);
			if (reserved_filenames.count(mypath) > 0) {
				continue;
			}

			// File appeared twice (somehow..?)
			if (file_list.count(mypath) > 0) {
				continue;
			}

			// FINALLY we can insert this file path into our list.
			file_list.insert(mypath);
		}
	}

	void Package::loadFileList() {

		if (!found) {
			debug::error("In wc::Package::loadFileList():\n");
			debug::errmore("Trying to load module before opening it.\n");
			return;
		}

		if (loaded) {
			debug::error("In wc::Package::loadFileList():\n");
			debug::errmore("Trying to load module '", name, "' more than once.\n");
			return;
		}

		if (archive.is_open()) {
			file_list.reserve(archive.num_files());

			fixedstring<64> fname;
			for (bool exists = archive.iterate_files(fname, true); exists; exists = archive.iterate_files(fname, false)) {
				file_list.insert(fname);
			}
		}
		else {
			loadFileListRecursive(file_list, module_path, "");
		}
		loaded = true;
	}

} // namespace wc