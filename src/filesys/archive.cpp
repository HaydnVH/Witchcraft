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
		if (file) close();

		// Get the proper filepath.
		std::filesystem::path archivepath = std::filesystem::u8path(u8filename);

		// If the archive does not exist...
		if (!std::filesystem::exists(archivepath)) {
			// Make sure the parent path for the archive exists,
			if (!std::filesystem::exists(archivepath.parent_path())) {
				// Create it if it doesn't.
				std::filesystem::create_directories(archivepath.parent_path());
			}

			// If the dile doesn't exist yet, then we can't open it with "r+", so we create it here.
			file = fopen_w(archivepath.c_str());

			Archive::Header newheader = {};
			strncpy(newheader.magic, Archive::MAGIC, 8);
			newheader.back = sizeof(Archive::Header);
			newheader.version = Archive::CURRENT_VERSION;

			fwrite(&newheader, sizeof(Archive::Header), 1, file);
			fclose(file);
			modified = true;
		}

		// 'r+' opens the file for both reading and writing, keeping the existing contents.
		file = fopen_rw(archivepath.c_str());
		if (file == nullptr) {
			debug::error("In wc::Archive::open():\n");
			debug::errmore("File '", u8filename, "' could not be opened.\n");
			return false;
		}

		// Load the header.
		fread(&header, sizeof(Archive::Header), 1, file);

		// Check the header to make sure the file format is valid.
		if (strncmp(header.magic, Archive::MAGIC, 8) != 0) {
			debug::error("In wc::Archive::open():\n");
			debug::errmore("File '", u8filename, "' is not a valid archive.\n");
			fclose(file);
			file = nullptr;
			return false;
		}

		if (header.numfiles > 0) {
			// Reserve space for the dictionary.
			size_t dictbytes;
			void* dictmem = dictionary.deserialize(header.numfiles, dictbytes);
			if (!dictmem) {
				debug::error("In wc::Archive::open():\n");
				debug::errmore("Failed to allocate memory for archive dictionary.\n");
				fclose(file);
				file = nullptr;
				return false;
			}

			// Seek to its position,
			fseek64(file, header.back, SEEK_SET);

			// Read the dictionary in.
			size_t bytesread = fread(dictmem, 1, dictbytes, file);
			if (bytesread != dictbytes) {
				debug::error("In wc::Archive::open():\n");
				debug::errmore("Failed to load dictionary data.\n");
				debug::errmore("Expected ", dictbytes, " bytes, read ", bytesread, ".\n");
				fclose(file);
				file = nullptr;
				return false;
			}
		}

		savedpath = u8filename;
		return true;
	}

	void Archive::close() {
		// If the file isn't open, we have nothing to close.
		if (file == nullptr) return;

		// If the archive has been modified, we update the file's header and dictionary.
		if (modified) {

			dictionary.shrink_to_fit();

			// Update the archive header.
			fseek64(file, 0, SEEK_SET);
			fwrite(&header, sizeof(Archive::Header), 1, file);

			// If we deleted or overwrote anything in the archive,
			// we should rebuild the whole thing.
			if (files_deleted) {
				rebuild();
			}
			else {
				// Otherwise, we can just re-write the dictionary at the end.
				fseek64(file, header.back, SEEK_SET);

				size_t dictbytes;
				void* dictmem = dictionary.serialize(dictbytes);
				size_t byteswritten = fwrite(dictmem, 1, dictbytes, file);
				if (byteswritten != dictbytes) {
					debug::error("In wc::Archive::close():\n");
					debug::errmore("Failed to save the dictionary data.\n");
					debug::errmore("Expected ", dictbytes, " bytes, wrote ", byteswritten, ".\n");
				}
			}
		}

		// Close the file.
		fclose(file);
		file = nullptr;

		// Clear out the dictionary.
		dictionary.clear();

		// Reset flags.
		modified = false;
		files_deleted = false;
	}

	void Archive::rebuild() {
		// Make sure the archive is open.
		if (file == nullptr) return;

		// Create a new temporary archive.
		std::filesystem::path temppath = std::filesystem::u8path(std::string(savedpath) + "_TEMP");
		FILE* tempfile = fopen_w(temppath.c_str());

		// First we write the header to the new archive.
		fwrite(&header, sizeof(Archive::Header), 1, tempfile);
		uint64_t newback = sizeof(Archive::Header);

		// Next we go through all of our files.
		for (uint32_t i = 0; i < header.numfiles; ++i) {
			// Allocate a buffer large enough to store the file contents.
			void* buffer = malloc(fileinfos[i].size_compressed);
			if (buffer == nullptr) {
				debug::error("In wc::Archive::rebuild():\n");
				debug::errmore("Memory allocation failure.\n");
				continue;
			}

			// Read the file into memory.
			fseek64(file, fileinfos[i].offset, SEEK_SET);
			fread(buffer, 1, fileinfos[i].size_compressed, file);

			// Write the file to the temporary archive.
			fseek64(tempfile, newback, SEEK_SET);
			fwrite(buffer, 1, fileinfos[i].size_compressed, tempfile);

			// Keep track of where the file is now.
			fileinfos[i].offset = newback;
			newback += fileinfos[i].size_compressed;

			// Clear up the memory we used.
			free(buffer);
		}

		// Now we write the dictionary to the temporary archive.
		size_t dictbytes;
		void* dictmem = dictionary.serialize(dictbytes);
		fwrite(dictmem, 1, dictbytes, tempfile);

		//Since our 'back' has changed, we need to correct it and re-write the header.
		header.back = newback;
		fseek64(tempfile, 0, SEEK_SET);
		fwrite(&header, sizeof(Archive::Header), 1, tempfile);

		// Close both archives.
		fclose(file);
		fclose(tempfile);

		//Have the filesystem clean things for us.
		std::filesystem::path mypath = std::filesystem::u8path(savedpath);
		std::filesystem::rename(temppath, mypath);

		// Re-open the file.
		file = fopen_rw(mypath.c_str());
	}

	int Archive::erase_file(const char* utf8path) {
		// Make sure the archive is open.
		if (file == nullptr) return -1;

		// Find the index of the file we're looking for.
		fixedstring<64> fixedpath(utf8path);
		size_t index = dictionary.find(fixedpath, true);
		if (index == SIZE_MAX) {
			return -1;
		}

		dictionary.erase_found();
		modified = true;
		files_deleted = true;

		return (int)index;
	}

	bool Archive::file_exists(const char* utf8path) const {
		// Make sure the archive is open.
		if (file == nullptr) return false;

		// Look for the file.
		fixedstring<64> fixedpath(utf8path);
		size_t index = dictionary.find(fixedpath);
		return (index != SIZE_MAX);
	}

	void* Archive::extract_data(const char* utf8path, void* buffer, uint32_t& size, int64_t& timestamp, bool alloc) const {
		// Make sure the archive is open.
		if (file == nullptr) { size = 0;  return nullptr; }

		// Search for the file we're looking for.
		fixedstring<64> fixedpath(utf8path);
		size_t index = dictionary.find(fixedpath);
		if (index == SIZE_MAX) {
			size = 0;
			return nullptr;
		}

		const FileInfo& info = fileinfos[index];
		size = info.size_uncompressed;
		timestamp = info.timestamp;

		if (!buffer && !alloc) {
			// We found the file, but we don't have any space allocated for it, and they don't want us allocating space.
			return nullptr;
		}

		fseek64(file, info.offset, SEEK_SET);
		if (feof(file)) {
			// File's offset points to beyond the end of the archive, somehow.
			size = 0;
			return nullptr;
		}

		// If we haven't been given a buffer, and we have permission to allocate, then we allocate a buffer.
		if (!buffer && alloc) {
			buffer = malloc(info.size_uncompressed);
		}

		// If the file's compressed size is equal to its uncompressed size, the file is not compressed.
		if (info.size_compressed == info.size_uncompressed) {
			// Load the file contents straight into memory.
			fread(buffer, 1, info.size_uncompressed, file);
		}
		else {
			// TODO: Handle file decompression!
			if (alloc)
				free(buffer);
			return nullptr;
		}

		// Finally, we return the buffer we just loaded the memory to.
		return buffer;
	}

	void Archive::extract_file(const char* utf8path, const char* utf8dstfile) const {
		// Extract the file data.
		uint32_t size = 0; int64_t timestamp = 0;
		void* buffer = extract_data(utf8path, NULL, size, timestamp, true);

		if (buffer) {
			// Create the directory where we'll place the file.
			std::filesystem::path dstpath = std::filesystem::u8path(utf8dstfile);
			std::error_code ec;
			std::filesystem::create_directories(dstpath.parent_path(), ec);
			if (ec) {
				debug::error("In wc::Archive::extract_file():\n");
				debug::errmore("Filesystem error: ", ec.message(), "\n");
				free(buffer);
				return;
			}

			// Open the file we're extracting to...
			FILE* dstfile = fopen_w(dstpath.c_str());
			if (dstfile) {
				// Write the file data.
				fwrite(buffer, 1, size, dstfile);
				fclose(dstfile);
				// Correct the extracted file's timestamp.
				std::filesystem::last_write_time(dstpath, std::filesystem::file_time_type(std::chrono::seconds(timestamp)));
			}
			free(buffer);
		}
	}

	bool Archive::insert_data(const char* utf8path, void* buffer, uint32_t size, int64_t timestamp, ReplaceEnum replace) {
		// Make sure the archive is open.
		if (file == nullptr) return false;

		// Copy the file's path anf convert backslashes to forward slashes.
		// Functions like extract and exists will simply fail if given backslashes.
		fixedstring<64> newpath = utf8path;
		strip_backslashes(newpath.c_str);

		// Create a dictionary entry for the new file.
		FileInfo newinfo = {};
		newinfo.offset = header.back;
		newinfo.size_compressed = size;
		newinfo.size_uncompressed = size;
		newinfo.timestamp = timestamp;

		// Perform a binary search to find the file we're looking for.
		size_t index = dictionary.find(newpath);
		if (index == SIZE_MAX) {
			dictionary.insert(newpath, newinfo);
		}
		else {
			// The specified file is already in this archive, so use 'replace' to decide what to do.
			switch (replace) {
			case DO_NOT_REPLACE:
				return false;
				break;
			case REPLACE:
				files_deleted = true;
				fileinfos[index] = newinfo;
				break;
			case REPLACE_IF_NEWER:
				if (timestamp > fileinfos[index].timestamp) {
					files_deleted = true;
					fileinfos[index] = newinfo;
				}
				else return false;
				break;
			default:
				return false;
				break;
			}
		}

		// TODO: Handle file compression.

		// If the new file has a non-zero size,
		if (newinfo.size_compressed > 0) {
			// We write the file contents.
			fseek64(file, header.back, SEEK_SET);
			fwrite(buffer, 1, (size_t)newinfo.size_compressed, file);
			header.back += newinfo.size_compressed;
		}

		modified = true;
		return true;
	}

	bool Archive::insert_file(const char* u8path, const char* u8srcfile, ReplaceEnum replace) {
		// Make sure the path isn't too long.
		if (strlen(u8path) > 63) {
			debug::error("In wc::Archive::insert_file():\n");
			debug::error("Failed to insert '", u8path, "'; path too long (max 63 bytes).\n");
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
		bool result = insert_data(u8path, buffer, (uint32_t)realsize, timestamp, replace);

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
		if (file == nullptr) return;

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
		if (file == nullptr) return;

		std::filesystem::path dstpath = std::filesystem::u8path(u8folder);
		if (!std::filesystem::exists(dstpath))
			std::filesystem::create_directories(dstpath);

		if (!std::filesystem::is_directory(dstpath)) {
			debug::error("In wc::Archive::unpack():\n");
			debug::errmore("'", u8folder, "' already exists and is not a directory.\n");
			return;
		}

		// Go through each file in the archive,
		for (size_t i = 0; i < header.numfiles; ++i) {
			// and extract it to disk.
			fixedstring<64>& entry = filepaths[i];
			extract_file(entry.c_str, (dstpath / entry.c_str).u8string().c_str());
		}
	}

	void Archive::merge(const char* utf8other, ReplaceEnum replace) {
		Archive other;
		if (!other.open(utf8other)) return;

		for (size_t i = 0; i < other.header.numfiles; ++i) {
			fixedstring<64> entry = other.filepaths[i];
			uint32_t size; int64_t timestamp;
			void* filedata = other.extract_data(entry.c_str, nullptr, size, timestamp, true);
			insert_data(entry.c_str, filedata, size, timestamp, replace);
			free(filedata);
		}
	}

} // namespace wc