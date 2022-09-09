#include "filedata.h"

namespace wc {

	void FileData::close() {
		_fdata.clear();
		_source = nullptr;
		_errcode = FILE_CLOSED;
	}

	FileData::FileData(const Package* package, const char* path) {
		if (!package) return;
		const Archive& archive = package->getArchive();
		if (archive.is_open()) {
			Archive::timestamp_t nil;
			if (!archive.extract_data(path, _fdata, nil)) {
				_errcode = FILE_NOT_FOUND;
				return;
			}
		}
		else {
			std::filesystem::path fullpath = std::filesystem::u8path(package->getPath());
			fullpath /= path;
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
			_fdata.resize(filesize);
			fread(_fdata.data(), 1, _fdata.size(), file);
			fclose(file);
		}
		_source = package;
		_errcode = SUCCESS;
	}

	FileData::FileData(const Archive& archive, const char* path) {
		Archive::timestamp_t nil;
		if (!archive.extract_data(path, _fdata, nil)) {
			_errcode = FILE_NOT_FOUND;
			return;
		}
		_errcode = SUCCESS;
	}

	FileData::FileData(const std::filesystem::path& folder, const char* path) {
		std::filesystem::path fullpath = folder / path;
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
		_fdata.resize(filesize);
		fread(_fdata.data(), 1, _fdata.size(), file);
		fclose(file);
		_errcode = SUCCESS;
	}

} // namespace wc