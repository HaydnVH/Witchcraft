/* filedata.h
 * by Haydn V. Harach
 * Created October 2019
 * Last modified January 2022
 * 
 * This class encapsulates data which has been loaded from a file.
 * This provides agnostic behaviour between files loaded from an archive
 * compared to loading an actual file. I decided to do this instead of playing
 * with FILE* or ifstream because a) files are almost always loaded in their
 * entirety before processing, and b) this will provide a consistent interface
 * if we change to memory-mapped file I/O.
 */

#ifndef HVH_WC_FILESYS_FILEDATA_H
#define HVH_WC_FILESYS_FILEDATA_H

#include <filesystem>
#include "archive.h"
#include "package.h"
#include "tools/crossplatform.h"

namespace wc {

	class FileData {
	public:

		FileData() {}
		FileData(const FileData& rhs) = delete;
		FileData(FileData&& rhs) noexcept {
			_fdata = rhs._fdata; rhs._fdata = nullptr;
			_source = rhs._source; rhs._source = nullptr;
			_fsize = rhs._fsize; rhs._fsize = 0;
			_errcode = rhs._errcode; rhs._errcode = FILE_CLOSED;
		}
		FileData& operator = (const FileData& rhs) = delete;
		FileData& operator = (FileData&& rhs) noexcept {
			_fdata = rhs._fdata; rhs._fdata = nullptr;
			_source = rhs._source; rhs._source = nullptr;
			_fsize = rhs._fsize; rhs._fsize = 0;
			_errcode = rhs._errcode; rhs._errcode = FILE_CLOSED;
			return *this;
		}
		~FileData() { close(); }
		void close();
		inline bool is_open() { return (_fdata != nullptr); }

		FileData(const Package* module, const char* u8path);
		FileData(const Archive& archive, const char* u8path);
		FileData(const std::filesystem::path& folder, const char* u8path);

		void* data() { return _fdata; }
		const Package* package() { return _source; }
		size_t size() { return _fsize; }

		enum Enum {
			SUCCESS = 0,
			NOT_INITIALIZED,
			FILE_CLOSED,
			FILE_NOT_FOUND,
			NOT_REGULAR_FILE,
			COULD_NOT_OPEN,
			MALLOC_FAIL
		};

		int getErrorEnum() { return _errcode; }
		const char* getError() {
			switch (_errcode) {
			case (SUCCESS): return nullptr;
			case (NOT_INITIALIZED): return "FileData not initialized.";
			case (FILE_CLOSED): return "FileData has been closed.";
			case (FILE_NOT_FOUND): return "File not found.";
			case (NOT_REGULAR_FILE): return "Not a regular file.";
			case (COULD_NOT_OPEN): return "Failed to open.";
			case (MALLOC_FAIL): return "Memory allocation failure.";
			default: return "Unknown error.";
			}
		}

	private:
		void* _fdata = nullptr;
		const Package* _source = nullptr;
		uint32_t _fsize = 0;
		int _errcode = NOT_INITIALIZED;
	};


} // namespace wc
#endif // HVH_WC_FILESYS_FILEDATA_H