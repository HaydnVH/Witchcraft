#include "archive.h"

#include <filesystem>
#include <chrono>
#include <algorithm>
#include "debug.h"
#include "tools/soa.hpp"
#include "tools/stringhelper.h"
#include "tools/crossplatform.h"

namespace wc {

	bool Archive::open(const char* u8filename) {

		// If the archive is already open, close it first.
		if (_file) close();

		// Get the proper filepath.
		std::filesystem::path archivepath = std::filesystem::u8path(u8filename);

		// If the archive does not exist...
		if (!std::filesystem::exists(archivepath)) {
			// Make sure the parent path for the archive exists,
			if (!std::filesystem::exists(archivepath.parent_path())) {
				// Create it if it doesn't.
				std::filesystem::create_directories(archivepath.parent_path());
			}

			// If the file doesn't exist yet, then we can't open it with "r+", so we create it here.
			_file = fopen_w(archivepath.c_str());

			Archive::Header newheader = {};
			strncpy(newheader.magic, Archive::MAGIC, 8);
			newheader.back = sizeof(Archive::Header);
			newheader.version = Archive::CURRENT_VERSION;

			fwrite(&newheader, sizeof(Archive::Header), 1, _file);
			fclose(_file);
			_modified = true;
		}

		// 'r+' opens the file for both reading and writing, keeping the existing contents.
		_file = fopen_rw(archivepath.c_str());
		if (_file == nullptr) {
			debug::error("In wc::Archive::open():\n");
			debug::errmore("File '", u8filename, "' could not be opened.\n");
			return false;
		}

		// Load the header.
		fread(&_header, sizeof(Archive::Header), 1, _file);

		// Check the header to make sure the file format is valid.
		if (strncmp(_header.magic, Archive::MAGIC, 8) != 0) {
			debug::error("In wc::Archive::open():\n");
			debug::errmore("File '", u8filename, "' is not a valid archive.\n");
			fclose(_file);
			_file = nullptr;
			return false;
		}

		if (_header.numfiles > 0) {
			// Reserve space for the dictionary.
			size_t dictbytes;
			void* dictmem = _dictionary.deserialize(_header.numfiles, dictbytes);
			if (!dictmem) {
				debug::error("In wc::Archive::open():\n");
				debug::errmore("Failed to allocate memory for archive dictionary.\n");
				fclose(_file);
				_file = nullptr;
				return false;
			}

			// Seek to its position,
			fseek64(_file, _header.back, SEEK_SET);

			// Read the dictionary in.
			size_t bytesread = fread(dictmem, 1, dictbytes, _file);
			if (bytesread != dictbytes) {
				debug::error("In wc::Archive::open():\n");
				debug::errmore("Failed to load dictionary data.\n");
				debug::errmore("Expected ", dictbytes, " bytes, read ", bytesread, ".\n");
				fclose(_file);
				_file = nullptr;
				return false;
			}
		}

		_savedpath = u8filename;
		return true;
	}

	void Archive::close() {
		// If the file isn't open, we have nothing to close.
		if (_file == nullptr) return;

		// If the archive has been modified, we update the file's header and dictionary.
		if (_modified) {

			_dictionary.shrink_to_fit();

			// Update the archive header.
			fseek64(_file, 0, SEEK_SET);
			fwrite(&_header, sizeof(Archive::Header), 1, _file);

			// If we deleted or overwrote anything in the archive,
			// we should rebuild the whole thing.
			if (_files_deleted) {
				rebuild();
			}
			else {
				// Otherwise, we can just re-write the dictionary at the end.
				fseek64(_file, _header.back, SEEK_SET);

				size_t dictbytes;
				void* dictmem = _dictionary.serialize(dictbytes);
				size_t byteswritten = fwrite(dictmem, 1, dictbytes, _file);
				if (byteswritten != dictbytes) {
					debug::error("In wc::Archive::close():\n");
					debug::errmore("Failed to save the dictionary data.\n");
					debug::errmore("Expected ", dictbytes, " bytes, wrote ", byteswritten, ".\n");
				}
			}
		}

		// Close the file.
		fclose(_file);
		_file = nullptr;

		// Clear out the dictionary.
		_dictionary.clear();

		// Reset flags.
		_modified = false;
		_files_deleted = false;
	}

	void Archive::rebuild() {
		// Make sure the archive is open.
		if (_file == nullptr) return;

		// Create a new temporary archive.
		std::filesystem::path temppath = std::filesystem::u8path(std::string(_savedpath) + "_TEMP");
		FILE* tempfile = fopen_w(temppath.c_str());

		// First we write the header to the new archive.
		fwrite(&_header, sizeof(Archive::Header), 1, tempfile);
		uint64_t newback = sizeof(Archive::Header);

		// Next we go through all of our files.
		for (uint32_t i = 0; i < _header.numfiles; ++i) {
			// Allocate a buffer large enough to store the file contents.
			void* buffer = malloc(_fileinfos[i].size_compressed);
			if (buffer == nullptr) {
				debug::error("In wc::Archive::rebuild():\n");
				debug::errmore("Memory allocation failure.\n");
				continue;
			}

			// Read the file into memory.
			fseek64(_file, _fileinfos[i].offset, SEEK_SET);
			fread(buffer, 1, _fileinfos[i].size_compressed, _file);

			// Write the file to the temporary archive.
			fseek64(tempfile, newback, SEEK_SET);
			fwrite(buffer, 1, _fileinfos[i].size_compressed, tempfile);

			// Keep track of where the file is now.
			_fileinfos[i].offset = newback;
			newback += _fileinfos[i].size_compressed;

			// Clear up the memory we used.
			free(buffer);
		}

		// Now we write the dictionary to the temporary archive.
		size_t dictbytes;
		void* dictmem = _dictionary.serialize(dictbytes);
		fwrite(dictmem, 1, dictbytes, tempfile);

		//Since our 'back' has changed, we need to correct it and re-write the header.
		_header.back = newback;
		fseek64(tempfile, 0, SEEK_SET);
		fwrite(&_header, sizeof(Archive::Header), 1, tempfile);

		// Close both archives.
		fclose(_file);
		fclose(tempfile);

		//Have the filesystem clean things for us.
		std::filesystem::path mypath = std::filesystem::u8path(_savedpath);
		std::filesystem::rename(temppath, mypath);

		// Re-open the file.
		_file = fopen_rw(mypath.c_str());
	}

	int Archive::erase_file(const char* utf8path) {
		// Make sure the archive is open.
		if (_file == nullptr) return -1;

		// Find the index of the file we're looking for.
		fixedstring<Archive::FILEPATH_FIXEDLEN> fixedpath(utf8path);
		size_t index = _dictionary.find(fixedpath, true);
		if (index == SIZE_MAX) {
			return -1;
		}

		_dictionary.erase_found();
		_modified = true;
		_files_deleted = true;

		return (int)index;
	}

	bool Archive::file_exists(const char* utf8path) const {
		// Make sure the archive is open.
		if (_file == nullptr) return false;

		// Look for the file.
		fixedstring<Archive::FILEPATH_FIXEDLEN> fixedpath(utf8path);
		size_t index = _dictionary.find(fixedpath);
		return (index != SIZE_MAX);
	}

	bool Archive::extract_data(const char* u8path, std::vector<char>& buffer, int64_t& timestamp) const {
		// Make sure the archive is open.
		if (_file == nullptr) { return false; }

		// Search for the file we're looking for.
		fixedstring<Archive::FILEPATH_FIXEDLEN> fixedpath(u8path);
		size_t index = _dictionary.find(fixedpath);
		if (index == SIZE_MAX) {
			debug::error("In wc::Archive::extract_data(\"", u8path, "\"):\n");
			debug::errmore("Failed to find file.\n");
			return false;
		}

		// Fetch the file's information and resize the buffer accordingly.
		const FileInfo& info = _fileinfos[index];
		buffer.resize(info.size_uncompressed);
		timestamp = info.timestamp;

		// Seek to the file data's location in the archive.
		fseek64(_file, info.offset, SEEK_SET);
		if (feof(_file)) {
			debug::error("In wc::Archive::extract_data(\"", u8path, "\"):\n");
			debug::errmore("File offset points beyond the end of the archive, somehow.\n");
			return false;
		}

		// If the file is not compressed, we can load it straight into memory.
		if (!(info.flags & FILEFLAG_COMPRESSED)) {
			// Load the file contents straight into memory.
			fread(buffer.data(), 1, info.size_uncompressed, _file);
		}
		// Otherwise, the file has been compressed using LZ4.
		// LZ4 default and LZ4 HC are both decompressed using the same function,
		// so the file info doesn't need to know which was used for compression.
		else {
			/*
			std::vector<char> compressed_buffer(info.size_compressed);
			fread(compressed_buffer.data(), 1, info.size_compressed, _file);
			int result = LZ4_decompress_safe(compressed_buffer.data(), buffer.data(), info.size_compressed, info.size_uncompressed);
			if (result < 0) {
				debug::error("In wc::Archive::extract_data(\"", u8path, "\"):\n);
				debug::errmore("Failed to decompress; source stream is malformed.\n");
				return false;
			}
			*/
			return false;
		}

		// Finally, we return the buffer we just loaded the memory to.
		return true;
	}

	void Archive::extract_file(const char* utf8path, const char* utf8dstfile) const {
		// Extract the file data.
		uint32_t size = 0; int64_t timestamp = 0;
		std::vector<char> buffer;
		if (!extract_data(utf8path, buffer, timestamp)) { return; }

		// Create the directory where we'll place the file.
		std::filesystem::path dstpath = std::filesystem::u8path(utf8dstfile);
		std::error_code ec;
		std::filesystem::create_directories(dstpath.parent_path(), ec);
		if (ec) {
			debug::error("In wc::Archive::extract_file():\n");
			debug::errmore("Filesystem error: ", ec.message(), "\n");
			return;
		}

		// Open the file we're extracting to...
		FILE* dstfile = fopen_w(dstpath.c_str());
		if (dstfile) {
			// Write the file data.
			fwrite(buffer.data(), 1, buffer.size(), dstfile);
			fclose(dstfile);
			// Correct the extracted file's timestamp.
			std::filesystem::last_write_time(dstpath, std::filesystem::file_time_type(std::chrono::seconds(timestamp)));
		}
	}

	bool Archive::insert_data(const char* utf8path, void* buffer, uint32_t size, int64_t timestamp, ReplaceEnum replace, CompressEnum compress) {
		// Make sure the archive is open.
		if (_file == nullptr) return false;

		// Copy the file's path anf convert backslashes to forward slashes.
		// Functions like extract and exists will simply fail if given backslashes.
		fixedstring<64> newpath = utf8path;
		strip_backslashes(newpath.c_str);

		// Create a dictionary entry for the new file.
		FileInfo newinfo = {};
		newinfo.offset = _header.back;
		newinfo.size_compressed = size;
		newinfo.size_uncompressed = size;
		newinfo.timestamp = timestamp;

		// TODO: Handle file compression.
		/*
		std::vector<char> compressed_buffer(size);
		switch (compress) {
			case COMPRESSED_FAST:
				newinfo.size_compressed = LZ4_compress_default((const char*)buffer, compressed_buffer.data(), size, size);
				newinfo.flags |= FILEFLAG_COMPRESSED;
				buffer = compressed_buffer.data();
				break;
			case COMPRESSED_SMALL:
				newinfo.size_compressed = LZ4_compress_HC((const char*)buffer, compressed_buffer.data(), size, size, 1);
				newinfo.flags |= FILEFLAG_COMPRESSED;
				buffer = compressed_buffer.data();
				break;
			default:
				break;
		}
		*/

		// Find the file we're looking for.
		size_t index = _dictionary.find(newpath);
		if (index == SIZE_MAX) {
			_dictionary.insert(newpath, newinfo);
			index = _dictionary.find(newpath);
		}
		else {
			// The specified file is already in this archive, so use 'replace' to decide what to do.
			switch (replace) {
			case DO_NOT_REPLACE:
				return false;
				break;
			case ALWAYS_REPLACE:
				_files_deleted = true;
				_fileinfos[index] = newinfo;
				break;
			case REPLACE_IF_NEWER:
				if (timestamp > _fileinfos[index].timestamp) {
					_files_deleted = true;
					_fileinfos[index] = newinfo;
				}
				else return false;
				break;
			default:
				return false;
				break;
			}
		}

		

		// If the new file has a non-zero size,
		if (newinfo.size_compressed > 0) {
			// We write the file contents.
			fseek64(_file, _header.back, SEEK_SET);
			fwrite(buffer, 1, (size_t)newinfo.size_compressed, _file);
			_header.back += newinfo.size_compressed;
		}

		_modified = true;
		return true;
	}

	bool Archive::insert_file(const char* u8path, const char* u8srcfile, ReplaceEnum replace, CompressEnum compress) {
		// Make sure the path isn't too long.
		if (strlen(u8path) > (Archive::FILEPATH_FIXEDLEN-1)) {
			debug::error("In wc::Archive::insert_file():\n");
			debug::error("Failed to insert '", u8path, "'; path too long (max ", Archive::FILEPATH_FIXEDLEN-1, " bytes).\n");
			return false;
		}

		std::filesystem::path srcpath = std::filesystem::u8path(u8srcfile);

		// Get the file's 'last write time' and convert it to a usable integer.
		// TODO: Make sure that the units/types line up properly!
		int64_t timestamp = std::filesystem::last_write_time(srcpath).time_since_epoch().count();

		// Open the file.
		FILE* srcfile = fopen_r(srcpath.c_str());
		if (srcfile == nullptr)
			return false;

		// Slurp up the file contents.
		size_t filesize = std::filesystem::file_size(srcpath);
		char* buffer = (char*)malloc(filesize);
		if (!buffer) {
			debug::error("In wc::Archive::insert_file():\n");
			debug::errmore("Failed to insert '", u8path, "'; memory allocation failure.\n");
			fclose(srcfile);
			return false;
		}
		size_t realsize = fread(buffer, 1, filesize, srcfile);

		// Insert the file into the archive.
		bool result = insert_data(u8path, buffer, (uint32_t)realsize, timestamp, replace, compress);

		// Clean up after ourselves.
		fclose(srcfile);
		free(buffer);

		return result;
	}

	void recursive_pack(Archive& archive, const std::filesystem::path& parent, std::filesystem::path child, Archive::ReplaceEnum replace) {
		for (std::filesystem::directory_iterator it(parent / child); it != std::filesystem::directory_iterator(); ++it) {
			// Path doesn't exist.  This case should never happen, but hey, just in case!
			if (!std::filesystem::exists(it->path()))
				continue;

			// Path is a directory, so we need to go deeper.
			if (std::filesystem::is_directory(it->path()))
				recursive_pack(archive, parent, child / it->path().filename(), replace);
			// Path is a file, so we insert it into the archive.
			else if (std::filesystem::is_regular_file(it->path()))
				archive.insert_file((child / it->path().filename()).u8string().c_str(), it->path().u8string().c_str(), replace);
		}
	}

	void Archive::pack(const char* u8folder, ReplaceEnum replace) {
		// Make sure the archive is open.
		if (_file == nullptr) return;

		std::filesystem::path srcpath = std::filesystem::u8path(u8folder);
		if (!std::filesystem::exists(srcpath)) {
			debug::error("In wc::Archive::pack():\n");
			debug::errmore("'", u8folder, "' does not exist.\n");
			return;
		}
		if (!std::filesystem::is_directory(srcpath)) {
			debug::error("In wc::Archive::pack():\n");
			debug::errmore("'", u8folder, "' is not a directory.\n");
			return;
		}

		recursive_pack(*this, srcpath, "", replace);
	}

	void Archive::unpack(const char* u8folder) {
		// Make sure the archive is open.
		if (_file == nullptr) return;

		std::filesystem::path dstpath = std::filesystem::u8path(u8folder);
		if (!std::filesystem::exists(dstpath))
			std::filesystem::create_directories(dstpath);

		if (!std::filesystem::is_directory(dstpath)) {
			debug::error("In wc::Archive::unpack():\n");
			debug::errmore("'", u8folder, "' already exists and is not a directory.\n");
			return;
		}

		// Go through each file in the archive,
		for (size_t i = 0; i < _header.numfiles; ++i) {
			// and extract it to disk.
			fixedstring<64>& entry = _filepaths[i];
			extract_file(entry.c_str, (dstpath / entry.c_str).u8string().c_str());
		}
	}

	void Archive::merge(const char* u8other, ReplaceEnum replace) {
		Archive other;
		if (!other.open(u8other)) return;

		std::vector<char> buffer;
		for (size_t i = 0; i < other._header.numfiles; ++i) {
			fixedstring<64> entry = other._filepaths[i];
			int64_t timestamp;
			other.extract_data(entry.c_str, buffer, timestamp);
			insert_data(entry.c_str, buffer.data(), (uint32_t)buffer.size(), timestamp, replace);
		}
	}

} // namespace wc