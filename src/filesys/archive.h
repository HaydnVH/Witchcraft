#ifndef HVH_WC_FILESYS_ARCHIVE_H
#define HVH_WC_FILESYS_ARCHIVE_H

#include <cstdio>
#include <cstdint>

#include "tools/htable.hpp"
#include "tools/fixedstring.h"

namespace wc {

	class Archive {
	public:

		Archive() {}
		Archive(const Archive&) = delete;
		Archive(Archive&& rhs) noexcept {
			swap(*this, rhs);
		}
		Archive& operator = (const Archive&) = delete;
		Archive& operator = (Archive&& rhs) noexcept {
			swap(*this, rhs);
			return *this;
		}
		~Archive() { close(); }

		friend inline void swap(Archive& lhs, Archive& rhs) {
			using std::swap;
			swap(lhs.header, rhs.header);
			swap(lhs.dictionary, rhs.dictionary);
			swap(lhs.file, rhs.file);
			swap(lhs.savedpath, rhs.savedpath);
			swap(lhs.modified, rhs.modified);
			swap(lhs.files_deleted, rhs.files_deleted);
		}

		enum ReplaceEnum
		{
			DO_NOT_REPLACE,
			REPLACE,
			REPLACE_IF_NEWER
		};


		// Finds an archive on disc and opens it, filling out the archive's header and dictionary as appropriate.
		bool open(const char* u8filename);

		// Closes the archive, saving any modifications to disc and freeing resources.
		void close();

		// Rebuilds the archive, sorting data according to filename and erasing any unreferenced blocks.
		void rebuild();

		// Returns whether or not the archive in question is open/valid.
		bool is_open() const {
			return (file != nullptr);
		}

		// Checks to see if a file exists in the archive.
		bool file_exists(const char* u8path) const;

		// Finds the file and extracts it to an in-memory buffer.
		// If alloc is true, extract_data will allocate a buffer on the heap which must be freed using aligned_free.
		void* extract_data(const char* u8path, void* buffer, uint32_t& size, int64_t& timestamp, bool alloc = false) const;

		// Finds the file and extracts it to a file on disc.
		void extract_file(const char* u8path, const char* utf8dstfile) const;

		// Takes a region of memory and, treating it like a single contiguous file, inserts it into the archive.
		bool insert_data(const char* u8path, void* buffer, uint32_t size, int64_t timestamp, ReplaceEnum replace = REPLACE_IF_NEWER);

		// Opens a file on disc and inserts its entire contents into the archive.
		bool insert_file(const char* u8path, const char* utf8srcfile, ReplaceEnum replace = REPLACE_IF_NEWER);

		// Erases a file from the archive.
		// All it really does is remove the file's information from the dictionary, and flag the instance as dirty.
		// When the archive is closed, if one or more files have been deleted, the archive is rebuilt.
		int erase_file(const char* u8path);

		// Searches a folder recursively and adds every file found to the archive.
		void pack(const char* u8folder, ReplaceEnum = REPLACE_IF_NEWER);

		// Extracts every file in the archive and saves them to the given location.
		void unpack(const char* u8folder);

		// Opens an archive and insers all of its files into this archive.
		void merge(const char* u8other, ReplaceEnum = REPLACE_IF_NEWER);

		// Get the number of files in the archive.
		uint32_t num_files() const {
			return header.numfiles;
		}

		// Iterate over every file in the archive.
		// Returns false when there are no more files to check.
		// Usage: for (bool exists = a.iterate_files(f, true); exists; exists = a.iterate_files(f, false)) { ... }
		bool iterate_files(fixedstring<64>& fname, bool restart = true) const {
			static uint32_t i = 0;
			if (restart) i = 0;
			if (i >= header.numfiles) return false;

			fname = dictionary.at<0>(i); ++i;
			return true;
		}

	private:

		static constexpr const char* MAGIC = "WCARCHV";
		static constexpr const uint16_t CURRENT_VERSION = 3;

		struct Header {
			char magic[8] = {}; // This is set to a predefined string and used to ensure that the file type is correct.
			uint64_t back = 0; // This points to the beginning of the dictionary, and defines the file data region.
			uint32_t flags = 0;
			uint32_t numfiles = 0;
			uint16_t version = 0; // Which version of this software was used to create the archive?
			char _reserved[38] = {}; // Reserved in case we need it for future versions without having to break compatability.
		} header;

		struct FileInfo
		{
			uint64_t offset = 0; // Where, relative to the beginning of the archive, is the file located?
			int64_t timestamp = 0; // When was the file created before we added it to the archive?
			uint32_t size_compressed = 0; // How many bytes in the archive itself does the file take?
			uint32_t size_uncompressed = 0; // How many bytes large will the file be after we decompress it?  If this if equal to 'size_compressed', the file is uncompressed.
			uint32_t flags = 0;
			char _reserved[4] = {}; // Reserved for future use.  May or may not actually use.
		};

		hvh::htable<fixedstring<64>, FileInfo> dictionary;
		fixedstring<64>* const& filepaths = dictionary.data<0>();
		FileInfo* const& fileinfos = dictionary.data<1>();

		FILE* file = nullptr;
		const char* savedpath = nullptr;
		bool modified = false;
		bool files_deleted = false;
	};

} // namespace wc
#endif // HVH_WC_FILESYS_ARCHIVE_H
