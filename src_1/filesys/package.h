#ifndef HVH_WC_FILESYS_PACKAGE_H
#define HVH_WC_FILESYS_PACKAGE_H

#include "archive.h"
#include "tools/fixedstring.h"
#include "tools/htable.hpp"

namespace wc {

	constexpr const char* PACKAGEINFO_FILENAME = "package.json";

	class Package {
	public:
		// Attempts to open the package associated with the given path.
		// Returns false if the package does not exist, or if the file is not a package.
		// This does not load the files inside the package!
		bool open(const char* u8path);

		// Closes the archive (if one exists) and frees resources.
		void close();

		// This does the heavy lifting of setting up the list of files in this package.
		void loadFileList();

		inline const std::string& getName() const { return _name; }
		inline const std::string& getAuthor() const { return _author; }
		inline const std::string& getCategory() const { return _category; }
		inline const std::string& getDescription() const { return _description; }
		inline const std::string& getPath() const { return _path; }
		inline float getPriority() const { return _priority; }
		inline bool isLoaded() { return _loaded; }

		inline const hvh::htable<fixedstring<64>>& getFileTable() const { return _file_table; }
		inline const Archive& getArchive() const { return _archive; }

		inline bool operator < (const Package& rhs) {
			// Compare priority first.
			if (_priority < rhs._priority) return true;
			if (_priority > rhs._priority) return false;
			// If priority is equal, compare timestamp.
			if (_timestamp < rhs._timestamp) return true;
			return false;
		}

		friend inline void swap(Package& lhs, Package& rhs) {
			using std::swap;
			swap(lhs._archive, rhs._archive);
			swap(lhs._path, rhs._path);
			swap(lhs._name, rhs._name);
			swap(lhs._author, rhs._author);
			swap(lhs._category, rhs._category);
			swap(lhs._description, rhs._description);
			swap(lhs._timestamp, rhs._timestamp);
			swap(lhs._priority, rhs._priority);
			swap(lhs._file_table, rhs._file_table);
			swap(lhs._enabled, rhs._enabled);
			swap(lhs._found, rhs._found);
			swap(lhs._loaded, rhs._loaded);
		}

	private:

		Archive _archive;
		std::string _path;

		std::string _name;
		std::string _author;
		std::string _category;
		std::string _description;

		uint64_t _timestamp = 0;
		float _priority = 0.0f;

		hvh::htable<fixedstring<64>> _file_table;

		bool _enabled = false;
		bool _found = false;
		bool _loaded = false;

	};


} // namespace wc
#endif // HVH_WC_FILESYS_PACKAGE_H