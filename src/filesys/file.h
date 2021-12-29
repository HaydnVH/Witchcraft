#ifndef HVH_WC_FS_FILE_H
#define HVH_WC_FS_FILE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <filesystem>
#include <cstdint>

#include "Archive.h"

namespace wc {
namespace fs {


// InFile
// Basically, it's ifstream, except it can open "files" from blocks of memory instead of only on the hard disc.
// This is very important for creating a unified interface for opening files from, say, a compressed archive.
// Since it inherits from istream, all of the usual file reading functions apply.
class InFile : public std::istream
{
public:
	InFile()
	: std::istream(nullptr)
	{}
	InFile(const std::filesystem::path& filepath, std::ios::openmode mode = 0)
	: std::istream(nullptr)
	{
		open(filepath, std::ios::in | mode);
	}
	InFile(char* ptr, size_t size)
	: std::istream(nullptr)
	{
		open(ptr, size);
	}
	InFile(const InFile& rhs) = delete;
	InFile(InFile&& rhs) noexcept
	: std::istream(nullptr)
	{
		std::swap(fb, rhs.fb);
		std::swap(mb, rhs.mb);
		rhs.set_rdbuf(nullptr);

		if (fb.is_open())
			init(&fb);
		else if (mb.is_open())
			init(&mb);
	}
	~InFile() {
		close();
	}

	InFile& operator = (const InFile& rhs) = delete;
	InFile& operator = (InFile&& rhs) noexcept {
		close();

		std::swap(fb, rhs.fb);
		std::swap(mb, rhs.mb);
		rhs.set_rdbuf(nullptr);

		if (fb.is_open())
			init(&fb);
		else if (mb.is_open())
			init(&mb);

		return *this;
	}

	inline void open(const char* filename, std::ios::openmode mode = 0) {
		close();

		fb.open(filename, std::ios::in | mode);
		init(&fb);
	}

	inline void open(const std::filesystem::path& filepath, std::ios::openmode mode = 0) {
		close();

		fb.open(filepath, std::ios::in | mode);
		init(&fb);
	}

	inline void open(char* ptr, size_t size) {
		close();

		mb.open(ptr, size);
		init(&mb);
		myptr = ptr;
		mysize = size;
	}

	inline void close() {
		fb.close();
		mb.close();
		if (myptr) {
			delete[] myptr;
			myptr = nullptr;
			mysize = 0;
		}
		set_rdbuf(nullptr);
	}

	inline bool is_open() {
		return (fb.is_open() || mb.is_open());
	}

	inline bool is_file() {
		return fb.is_open();
	}

	inline bool is_mem() {
		return mb.is_open();
	}

	inline void get_mem(char** data, size_t* size)
	{
		*data = myptr;
		*size = mysize;
	}

	std::string contents() {
		// This method is faster for loading small files (1MB or less)
		std::ostringstream ss;
		ss << rdbuf();
		return ss.str();
	}

	std::vector<char> contents_bin() {
		// This method is faster when loading large files (1MB or more).  Maybe.
		seekg(0, std::ios::end);
		std::streamsize size = tellg();
		seekg(0, std::ios::beg);

		std::vector<char> buffer((size_t)size);
		read(buffer.data(), size);

		return buffer;
	}


private:

	std::filebuf fb;

	char* myptr = NULL;
	size_t mysize = 0;

	class membuf : public std::streambuf
	{
	public:
		membuf() {}
		membuf(char* ptr, size_t size) {
			setg(ptr, ptr, ptr + size);
			isOpen = true;
		}
		~membuf() { close(); }

		void open(char* ptr, size_t size) {
			if (!isOpen) {
				setg(ptr, ptr, ptr + size);
				isOpen = true;
			}
		}

		void close() {
			isOpen = false;
		}

		bool is_open() { 
			return isOpen;
		}

	private:
		bool isOpen = false;
	} mb;

};

class OutFile : public std::ostream
{
public:
	OutFile()
		: std::ostream(mb.rdbuf())
	{}
	OutFile(const std::filesystem::path& filepath)
	: std::ostream(nullptr)
	{
		fb.open(filepath, std::ios::out);
		init(&fb);
	}
	OutFile(const std::filesystem::path& filepath, std::ios::openmode mode)
	: std::ostream(nullptr)
	{
		fb.open(filepath, std::ios::out | mode);
		init(&fb);
	}
	OutFile(const OutFile& rhs) = delete;
	OutFile(OutFile&& rhs) noexcept
	: std::ostream(rhs.rdbuf())
	{
		std::swap(fb, rhs.fb);
		std::swap(mb, rhs.mb);
	}
	~OutFile() {
		if (myarchive) {
			std::string str = mb.str();
			myarchive->insert_data(archivepath.c_str(), &str[0], str.size(),
				std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count(), ARCHIVE_REPLACE);
		}
	}

	OutFile& operator = (const OutFile& rhs) = delete;
	OutFile& operator = (OutFile&& rhs) noexcept {
		set_rdbuf(rhs.rdbuf());
		std::swap(fb, rhs.fb);
		std::swap(mb, rhs.mb);
	}

	void open(const std::filesystem::path& filepath) {
		fb.open(filepath, std::ios::out);
		init(&fb);
	}

	void open_mem(Archive* archive, const std::string& filepath) {
		mb.clear();
		init(mb.rdbuf());
		myarchive = archive;
		archivepath = filepath;
	}

	void open_mem() {
		mb.clear();
		init(mb.rdbuf());
	}

	void write_mem_to(Archive* archive, const char* filepath, uint8_t replace = ARCHIVE_REPLACE) const {
		if (archive)
		{
			std::string data = mb.str();
			int64_t timestamp = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()).time_since_epoch().count();
			archive->insert_data(filepath, &data[0], data.size(), timestamp, replace);
		}
	}

	void write_mem_to(const std::filesystem::path& filepath) {
		std::ofstream f(filepath, std::ios::binary | std::ios::trunc);
		std::string data = mb.str();
		f.write(&data[0], data.size());
	}

	bool is_open() {
		if (rdbuf() == &fb) {
			return fb.is_open();
		}
		else if (rdbuf() == mb.rdbuf()) {
			return true;
		}
		else
			return false;
	}

	inline std::ostringstream& get_mem_buffer() { return mb; }

private:
	std::filebuf fb;
	std::ostringstream mb;
	Archive* myarchive = nullptr;
	std::string archivepath;
};

}} // namespace wc::fs

#endif // HVH_WC_FS_FILE_H