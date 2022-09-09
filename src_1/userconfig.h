#ifndef HVH_WC_CONFIG_H
#define HVH_WC_CONFIG_H

#include <vector>
#include <string>

namespace wc {
namespace userconfig {

	// The filename of the config file, relative to the user directory.
	constexpr const char* CONFIG_FILENAME = "config.json";

	// Opens the config file and loads its contents.  Creates an empty document if no config file exists.
	void init();

	// Saves the config document and closes the file.
	void shutdown();

	// Returns true after the config has been loaded (or created); false otherwise.
	bool isInitialized();

	// Returns true if a path already exists in the config, or false if it should be created.
	bool exists(const char* path);

	// Reads a value from config, writing the result to 'out_val'.
	// Returns false and doesn't modify 'out_val' if anything goes wrong,
	// returns true if all is well.
	bool read(const char* path, const char* key, std::string& out_val);
	bool read(const char* path, const char* key, int& out_val);
	bool read(const char* path, const char* key, float& out_val);
	bool read(const char* path, const char* key, bool& out_val);

	std::vector<const char*> readStringArray(const char* path, const char* key);
	std::vector<int>		 readIntArray(const char* path, const char* key);
	std::vector<float>		 readFloatArray(const char* path, const char* key);
	std::vector<bool>		 readBoolArray(const char* path, const char* key);

	// Writes a value to the config document.
	// After shutdown() is called, changes made will be reflected in the config file.
	void write(const char* path, const char* key, const char* val);
	void write(const char* path, const char* key, int val);
	void write(const char* path, const char* key, float val);
	void write(const char* path, const char* key, bool val);

	void writeArray(const char* path, const char* key, const std::vector<const char*>& vals);
	void writeArray(const char* path, const char* key, const std::vector<int>& vals);
	void writeArray(const char* path, const char* key, const std::vector<float>& vals);
	void writeArray(const char* path, const char* key, const std::vector<bool>& vals);

}} // namespace wc::config
#endif // HVH_WC_CONFIG_H