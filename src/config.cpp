#include "config.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
using namespace rapidjson;

#include <filesystem>
#include "tools/crossplatform.h"

namespace wc {
namespace config {


	namespace {

		Document doc;

	} // namespace <anon>

	/*
	bool Init(const char* userpath_utf8) {
		// Open the config file.
		std::filesystem::path logpath = std::filesystem::u8path(userpath_utf8) / CONFIG_FILENAME;
		FILE* file = fopen_r(logpath.c_str());
		if (!file) {
			debug::warning("In wc::config::Init():\n");
			debug::warnmore("Failed to open '", CONFIG_FILENAME, "'.\n");
			debug::warnmore("Using default settings.\n");
			fclose(file); return false;
		}

		// Read the config file.
		FileReadStream stream(file, buffer.data(), buffer.size());
		doc.ParseStream<0, UTF8<>, FileReadStream>(stream);
		if (doc.HasParseError() || !doc.IsObject()) {
			debug::error("In wc::config::Init():\n");
			debug::errmore("Failed to parse '", CONFIG_FILENAME, "'.\n");
			debug::errmore("Using default settings.\n");
			fclose(file); return false;
		}
	
	}
	*/


}} // namespace wc::config