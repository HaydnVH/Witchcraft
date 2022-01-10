#include "filedata.h"

namespace wc {

	void FileData::close() {
		if (_fdata) free(_fdata);
		_fdata = nullptr;
		_source = nullptr;
		_fsize = 0;
		_errcode = FILE_CLOSED;
	}

	FileData::FileData(const Package* package, const char* u8path) {
		if (!package) return;
		const Archive& archive = package->getArchive();
		if (archive.is_open()) {
			int64_t nil;
			_fdata = archive.extract_data(u8path, nullptr, _fsize, nil, true);
			if (!_fdata) {
				_errcode = FILE_NOT_FOUND;
				return;
			}
		}
		else {
			std::filesystem::path fullpath = std::filesystem::u8path(package->getPath());
			fullpath /= u8path;
			if (!std::filesystem::exists(fullpath)) {
				_errcode = FILE_NOT_FOUND;
				return;
			}
			if (!std::filesystem::is_regular_file(fullpath)) {
				_errcode = NOT_REGULAR_FILE;
				return;
			}
			FILE* file = fopen_r(fullpath.c_str());
			if (!file) {
				_errcode = COULD_NOT_OPEN;
				return;
			}
			size_t filesize = std::filesystem::file_size(fullpath);
			_fdata = malloc(filesize);
			if (!_fdata) {
				_errcode = MALLOC_FAIL;
				fclose(file);
				return;
			}
			_fsize = (uint32_t)fread(_fdata, 1, filesize, file);
			fclose(file);
		}
		_errcode = SUCCESS;
	}

	FileData::FileData(const Archive& archive, const char* utf8path) {
		int64_t nil;
		_fdata = archive.extract_data(utf8path, nullptr, _fsize, nil, true);
		if (!_fdata) {
			_errcode = FILE_NOT_FOUND;
			return;
		}
		_errcode = SUCCESS;
	}

	FileData::FileData(const std::filesystem::path& folder, const char* utf8path) {
		std::filesystem::path fullpath = folder / utf8path;
		if (!std::filesystem::exists(fullpath)) {
			_errcode = FILE_NOT_FOUND;
			return;
		}
		if (!std::filesystem::is_regular_file(fullpath)) {
			_errcode = NOT_REGULAR_FILE;
			return;
		}
		FILE* file = fopen_r(fullpath.c_str());
		if (!file) {
			_errcode = COULD_NOT_OPEN;
			return;
		}
		size_t filesize = std::filesystem::file_size(fullpath);
		_fdata = malloc(filesize);
		if (!_fdata) {
			_errcode = MALLOC_FAIL;
			fclose(file);
			return;
		}
		_fsize = (uint32_t)fread(_fdata, 1, filesize, file);
		fclose(file);
		_errcode = SUCCESS;
	}

} // namespace wc