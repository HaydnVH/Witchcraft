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

		inline const char* getName() const { return name; }
		inline const char* getAuthor() const { return author; }
		inline const char* getCategory() const { return category; }
		inline const char* getDescription() const { return description; }
		inline const char* getPath() const { return module_path; }
		inline float getPriority() const { return priority; }
		inline bool isLoaded() { return loaded; }

		inline const hvh::htable<fixedstring<64>>& getFileList() const { return file_list; }
		inline const Archive& getArchive() const { return archive; }

		inline bool operator < (const Package& rhs) {
			// Compare priority first.
			if (priority < rhs.priority) return true;
			if (priority > rhs.priority) return false;
			// If priority is equal, compare timestamp.
			if (timestamp < rhs.timestamp) return true;
			return false;
		}

		friend inline void swap(Package& lhs, Package& rhs) {
			using std::swap;
			swap(lhs.archive, rhs.archive);
			swap(lhs.module_path, rhs.module_path);
			swap(lhs.name, rhs.name);
			swap(lhs.author, rhs.author);
			swap(lhs.category, rhs.category);
			swap(lhs.description, rhs.description);
			swap(lhs.timestamp, rhs.timestamp);
			swap(lhs.priority, rhs.priority);
			swap(lhs.file_list, rhs.file_list);
			swap(lhs.enabled, rhs.enabled);
			swap(lhs.found, rhs.found);
			swap(lhs.loaded, rhs.loaded);
		}

	private:

		Archive archive;
		char module_path[256] = "";

		char name[32] = "";
		char author[16] = "";
		char category[16] = "";
		char description[256] = "";
		uint64_t timestamp = 0;
		float priority = 0.0f;

		hvh::htable<fixedstring<64>> file_list;

		bool enabled = false;
		bool found = false;
		bool loaded = false;

	};


} // namespace wc
#endif // HVH_WC_FILESYS_PACKAGE_H