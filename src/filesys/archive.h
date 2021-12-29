#ifndef HVH_WC_FS_ARCHIVE_H
#define HVH_WC_FS_ARCHIVE_H

#include <cstdint>
#include "tools/htable.hpp"
#include "tools/fixedstring.h"

namespace wc {
namespace fs {

	class Archive {
	public:

	private:

		static constexpr const char* MAGIC = "WCARCHV";
		struct Header {
			char magic[8]; // Set to a predefined string, used to identify the correct file type.
			uint64_t back; // Points to the beginning of the dictionary, and defines the data area.
			uint32_t flags;
			uint32_t numfiles; // The number of files in the archive.
			uint16_t version; // The version of the software used to create the archive.
			char _reserved[38];
		};
		Header header;

		struct FileInfo {
			uint64_t offset; // Where in the data region the file is located.
			int64_t timestamp; // The file's timestamp.
			uint32_t size_compressed; // The compressed size of the file, in bytes.
			uint32_t size_uncompressed; // The uncompressed size of the file, in bytes.
			uint32_t flags;
			char _reserved[4];
		};

		hvh::htable<FixedString<64>, FileInfo> dictionary;

		FILE* file;
		const char* saved_path;
		bool was_modified;
		bool files_were_deleted;

	}; // class Archive

}} // namespace wc::fs

#endif // HVH_WC_FS_ARCHIVE_H