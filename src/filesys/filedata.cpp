#include "filedata.h"

namespace wc {

	void FileData::close() {
		if (fdata) free(fdata);
		fdata = nullptr;
		source = nullptr;
		fsize = 0;
		errcode = FILE_CLOSED;
	}

	FileData::FileData(const Package* package, const char* u8path) {
		if (!package) return;
		const Archive& archive = package->getArchive();
		if (archive.is_open()) {
			int64_t nil;
			fdata = archive.extract_data(u8path, nullptr, fsize, nil, true);
			if (!fdata) {
				errcode = FILE_NOT_FOUND;
				return;
			}
		}
		else {
			std::filesystem::path fullpath = std::filesystem::u8path(package->getPath());
			fullpath /= u8path;
			if (!std::filesystem::exists(fullpath)) {
				errcode = FILE_NOT_FOUND;
				return;
			}
			if (!std::filesystem::is_regular_file(fullpath)) {
				errcode = NOT_REGULAR_FILE;
				return;
			}
			FILE* file = fopen_r(fullpath.c_str());
			if (!file) {
				errcode = COULD_NOT_OPEN;
				return;
			}
			size_t filesize = std::filesystem::file_size(fullpath);
			fdata = malloc(filesize);
			if (!fdata) {
				errcode = MALLOC_FAIL;
				fclose(file);
				return;
			}
			fsize = (uint32_t)fread(fdata, 1, filesize, file);
			fclose(file);
		}
		errcode = SUCCESS;
	}

	FileData::FileData(const Archive& archive, const char* utf8path) {
		int64_t nil;
		fdata = archive.extract_data(utf8path, nullptr, fsize, nil, true);
		if (!fdata) {
			errcode = FILE_NOT_FOUND;
			return;
		}
		errcode = SUCCESS;
	}

	FileData::FileData(const std::filesystem::path& folder, const char* utf8path) {
		std::filesystem::path fullpath = folder / utf8path;
		if (!std::filesystem::exists(fullpath)) {
			errcode = FILE_NOT_FOUND;
			return;
		}
		if (!std::filesystem::is_regular_file(fullpath)) {
			errcode = NOT_REGULAR_FILE;
			return;
		}
		FILE* file = fopen_r(fullpath.c_str());
		if (!file) {
			errcode = COULD_NOT_OPEN;
			return;
		}
		size_t filesize = std::filesystem::file_size(fullpath);
		fdata = malloc(filesize);
		if (!fdata) {
			errcode = MALLOC_FAIL;
			fclose(file);
			return;
		}
		fsize = (uint32_t)fread(fdata, 1, filesize, file);
		fclose(file);
		errcode = SUCCESS;
	}

} // namespace wc