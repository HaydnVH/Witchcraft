#ifndef HVH_WC_FILESYS_ARCHIVE_H
#define HVH_WC_FILESYS_ARCHIVE_H

#include <cstdio>
#include <cstdint>
#include <vector>
#include <chrono>

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
			swap(lhs._header, rhs._header);
			swap(lhs._dictionary, rhs._dictionary);
			swap(lhs._file, rhs._file);
			swap(lhs._savedpath, rhs._savedpath);
			swap(lhs._modified, rhs._modified);
			swap(lhs._files_deleted, rhs._files_deleted);
		}

		enum ReplaceEnum
		{
			DO_NOT_REPLACE,
			ALWAYS_REPLACE,
			REPLACE_IF_NEWER
		};

		enum CompressEnum
		{
			DO_NOT_COMPRESS,
			COMPRESS_FAST,	// Uses LZ4 default
			COMPRESS_SMALL	// Uses LZ4 HC
		};

		enum FileInfoFlags
		{
			FILEFLAG_COMPRESSED = 1 << 0
		};

		typedef std::chrono::utc_clock::time_point timestamp_t;


		// Finds an archive on disc and opens it, filling out the archive's header and dictionary as appropriate.
		// Creates an empty archive if it doesn't already exist.
		bool open(const char* archivename);

		// Closes the archive, saving any modifications to disc and freeing resources.
		void close();

		// Rebuilds the archive, sorting data according to filename and erasing any unreferenced blocks.
		void rebuild();

		// Returns whether or not the archive in question is open/valid.
		bool is_open() const {
			return (_file != nullptr);
		}

		// Checks to see if a file exists in the archive.
		bool file_exists(const char* path) const;

		// Finds the file and extracts it to an in-memory buffer.
		// 'buffer' will be resized, and it will contain the file data.
		bool extract_data(const char* path, std::vector<char>& buffer, timestamp_t& timestamp) const;

		// Finds the file and extracts it to a file on disc.
		void extract_file(const char* path, const char* dstfilename) const;

		// Takes a region of memory and, treating it like a single contiguous file, inserts it into the archive.
		bool insert_data(const char* path, void* buffer, uint32_t size, timestamp_t timestamp, ReplaceEnum replace = REPLACE_IF_NEWER, CompressEnum compress = COMPRESS_FAST);

		// Opens a file on disc and inserts its entire contents into the archive.
		bool insert_file(const char* path, const char* srcfilename, ReplaceEnum replace = REPLACE_IF_NEWER, CompressEnum compress = COMPRESS_FAST);

		// Erases a file from the archive.
		// All it really does is remove the file's information from the dictionary, and flag the instance as dirty.
		// When the archive is closed, if one or more files have been deleted, the archive is rebuilt.
		int erase_file(const char* path);

		// Searches a folder recursively and adds every file found to the archive.
		void pack(const char* srcfolder, ReplaceEnum replace = REPLACE_IF_NEWER, CompressEnum compress = COMPRESS_FAST);

		// Extracts every file in the archive and saves them to the given location.
		void unpack(const char* dstfolder);

		// Opens an archive and insers all of its files into this archive.
		void merge(const char* othername, ReplaceEnum = REPLACE_IF_NEWER);

		// Get the number of files in the archive.
		uint32_t num_files() const {
			return _header.numfiles;
		}

		// Iterate over every file in the archive.
		// Returns false when there are no more files to check.
		// Usage: for (bool exists = a.iterate_files(fname, true); exists; exists = a.iterate_files(fname, false)) { ... }
		bool iterate_files(fixedstring<64>& fname, bool restart = true) const {
			static uint32_t i = 0;
			if (restart) i = 0;
			if (i >= _header.numfiles) return false;

			fname = _dictionary.at<0>(i); ++i;
			return true;
		}

	private:

		static constexpr const char* MAGIC = "WCARCHV";
		static constexpr const uint16_t CURRENT_VERSION = 3;
		static constexpr const size_t FILEPATH_FIXEDLEN = 64;

		struct Header {
			char magic[8] = {}; // This is set to a predefined string and used to ensure that the file type is correct.
			uint64_t back = 0; // This points to the beginning of the dictionary, and defines the file data region.
			uint32_t flags = 0;
			uint32_t numfiles = 0;
			uint16_t version = 0; // Which version of this software was used to create the archive?
			char _reserved[38] = {}; // Reserved in case we need it for future versions without having to break compatability.
		} _header;

		struct FileInfo
		{
			uint64_t offset = 0; // Where, relative to the beginning of the archive, is the file located?
			timestamp_t timestamp; // When was the file created before we added it to the archive?
			int32_t size_compressed = 0; // How many bytes in the archive itself does the file take?
			int32_t size_uncompressed = 0; // How many bytes large will the file be after we decompress it?  If this if equal to 'size_compressed', the file is uncompressed.
			uint32_t flags = 0;
			char _reserved[4] = {}; // Reserved for future use.  May or may not actually use.
		};

		hvh::htable<fixedstring<FILEPATH_FIXEDLEN>, FileInfo> _dictionary;
		fixedstring<FILEPATH_FIXEDLEN>* const& _filepaths = _dictionary.data<0>();
		FileInfo* const& _fileinfos = _dictionary.data<1>();

		FILE* _file = nullptr;
		std::string _savedpath;
		bool _modified = false;
		bool _files_deleted = false;
	};

} // namespace wc
#endif // HVH_WC_FILESYS_ARCHIVE_H
