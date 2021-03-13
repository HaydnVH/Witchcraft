#include "appconfig.h"

#ifdef _WIN32
#include <Windows.h>
#include <Shlobj.h>
#endif

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
using namespace rapidjson;

#include "tools/stringhelper.h"
using namespace std;


_AppConfig::_AppConfig() {

	// First, we initialize the installation directory.
	// This determines where we'll find appconfig.json.
	// Eventually this will be wherever the executible is located,
	// for now we use the current working directory.
	install_dir = "";
	install_path = filesystem::u8path(install_dir);

	bool should_write_file = false;

	string filepath = makestr(install_dir, APPCONFIG_FILENAME);
	FILE* file = fopen(filepath.c_str(), "rb");
	if (!file) {
		should_write_file = true;
	}
	else {
		vector<char> buffer(10240);
		FileReadStream stream(file, buffer.data(), buffer.size());

		Document doc;
		doc.ParseStream<0, UTF8<>, FileReadStream>(stream);
		if (doc.HasParseError() || !doc.IsObject()) {
			should_write_file = true;
		}
		else {
			// We have appconfig.json, and it parsed successfully.  Lets get reading!
			if (doc.HasMember("name") && doc["name"].IsString()) {
				name = doc["name"].GetString();
			}
			else { should_write_file = true; }

			if (doc.HasMember("company name") && doc["company name"].IsString()) {
				company_name = doc["company name"].GetString();
			}
			else { should_write_file = true; }

			if (doc.HasMember("version") && doc["version"].IsString()) {
				version_string = doc["version"].GetString();
				sscanf(version_string.c_str(), "%i.%i.%i", &major_ver, &minor_ver, &patch_ver);
			}
			else { should_write_file = true; }
		}
		fclose(file);
		file = nullptr;
	}

	// Create and obtain the user directory using the information we fetched from appconfig.
	{
#ifdef _WIN32
		PWSTR str;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, NULL, &str);
		user_path = filesystem::path(str);
		user_path = user_path / company_name / name;
		error_code ec;
		filesystem::create_directories(user_path, ec);
		user_dir = user_path.u8string();
		strip_backslashes(user_dir);
		CoTaskMemFree(str);
#endif // _WIN32
	}

	// If we failed to load any information, we had to create that data from scratch,
	// and so we should really just re-write the whole config file.
	if (should_write_file) {
		Document doc;
		doc.SetObject();
		doc.AddMember(Value("name", doc.GetAllocator()).Move(), Value(name.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
		doc.AddMember(Value("company name", doc.GetAllocator()).Move(), Value(company_name.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());
		doc.AddMember(Value("version", doc.GetAllocator()).Move(), Value(version_string.c_str(), doc.GetAllocator()).Move(), doc.GetAllocator());

		file = fopen(filepath.c_str(), "wb");
		vector<char> buffer(10240);
		FileWriteStream stream(file, buffer.data(), buffer.size());
		PrettyWriter<FileWriteStream> writer(stream);
		doc.Accept(writer);
		fclose(file);
	}
}