#include "userconfig.h"
#include "appconfig.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
using namespace rapidjson;

#include <sstream>
#include <fstream>
using namespace std;

#include "tools/crossplatform.h"
#include "tools/stringhelper.h"

namespace {

	rapidjson::Document doc;
	bool initialized = false;
	bool modified = false;

} // namespace <anon>

namespace wc {
namespace userconfig {

void init() {
	// Open the config file.
	std::filesystem::path configpath = appconfig::getUserPath() / CONFIG_FILENAME;
	ifstream infile(configpath, ios::in | ios::binary);
	if (!infile.is_open()) {
		// The config file doesn't exist, so let's create an empty document to write to.
		doc.SetObject();
		modified = true;
	}
	else {
		stringstream ss;
		ss << infile.rdbuf();
		doc.Parse(ss.str().c_str());
		if (doc.HasParseError() || !doc.IsObject()) {
			doc.SetObject();
			modified = true;
		}
	}

	infile.close();
	initialized = true;
}

void shutdown() {

	if (modified) {
		// Open the config file to write. //
		std::filesystem::path configpath = appconfig::getUserPath() / CONFIG_FILENAME;
		FILE* file = fopen_w(configpath.c_str());

		ofstream outfile(configpath, ios::out | ios::binary);
		if (outfile.is_open()) {
			StringBuffer buffer;
			PrettyWriter writer(buffer);
			doc.Accept(writer);
			outfile << buffer.GetString();
		}
	}
}

bool isInitialized() {
	return initialized;
}

Value* followPath(const char* path, bool may_create) {
	Value* current = nullptr;

	char mystr[128];
	strncpy(mystr, path, 127);
	mystr[127] = '\0'; // Force null terminator.

	for (char* token = strtok(mystr, "."); token; token = strtok(nullptr, ".")) {
		if (current) {
			if (current->HasMember(token)) {
				current = &((*current)[token]);
				if (!current->IsObject())
					return nullptr;
			} else {
				if (may_create) {
					current->AddMember(Value(token, doc.GetAllocator()).Move(), Value(kObjectType).Move(), doc.GetAllocator());
					current = &((*current)[token]);
				} else {
					return nullptr;
		}	}	}
		else {
			if (doc.HasMember(token)) {
				current = &doc[token];
				if (!current->IsObject())
					return nullptr;
			} else {
				if (may_create) {
					doc.AddMember(Value(token, doc.GetAllocator()).Move(), Value(kObjectType).Move(), doc.GetAllocator());
					current = &doc[token];
				} else {
					return nullptr;
	}	}	}	}

	return current;
}

bool exists(const char* path) {
	Value* object = followPath(path, false);
	return (object != nullptr);
}

bool read(const char* path, const char* key, const char*& out_val) {
	Value* object = followPath(path, false);
	if (!object) return false;

	Value& val = (*object)[key];
	if (!val.IsString()) return false;

	out_val = val.GetString();
	return true;
}

bool read(const char* path, const char* key, int& out_val)
{
	Value* object = followPath(path, false);
	if (!object) return false;

	Value& val = (*object)[key];
	if (!val.IsInt()) return false;

	out_val = val.GetInt();
	return true;
}

bool read(const char* path, const char* key, float& out_val)
{
	Value* object = followPath(path, false);
	if (!object) return false;

	Value& val = (*object)[key];
	if (!val.IsFloat()) return false;

	out_val = val.GetFloat();
	return true;
}

bool read(const char* path, const char* key, bool& out_val)
{
	Value* object = followPath(path, false);
	if (!object) return false;

	Value& val = (*object)[key];
	if (!val.IsBool()) return false;

	out_val = val.GetBool();
	return true;
}

std::vector<const char*> readStringArray(const char* path, const char* key) {
	std::vector<const char*> result;
	Value* object = followPath(path, false);
	if (!object) return std::move(result);
	Value& val = (*object)[key];
	if (!val.IsArray()) return std::move(result);
	for (SizeType i = 0; i < val.Size(); ++i) {
		if (val[i].IsString()) {
			result.push_back(val[i].GetString());
		}
	}
	return std::move(result);
}

std::vector<int> readIntArray(const char* path, const char* key) {
	std::vector<int> result;
	Value* object = followPath(path, false);
	if (!object) return std::move(result);
	Value& val = (*object)[key];
	if (!val.IsArray()) return std::move(result);
	for (SizeType i = 0; i < val.Size(); ++i) {
		if (val[i].IsNumber()) {
			result.push_back(val[i].GetInt());
		}
	}
	return std::move(result);
}

std::vector<float> readFloatArray(const char* path, const char* key) {
	std::vector<float> result;
	Value* object = followPath(path, false);
	if (!object) return std::move(result);
	Value& val = (*object)[key];
	if (!val.IsArray()) return std::move(result);
	for (SizeType i = 0; i < val.Size(); ++i) {
		if (val[i].IsNumber()) {
			result.push_back(val[i].GetFloat());
		}
	}
	return std::move(result);
}

std::vector<bool> readBoolArray(const char* path, const char* key) {
	std::vector<bool> result;
	Value* object = followPath(path, false);
	if (!object) return std::move(result);
	Value& val = (*object)[key];
	if (!val.IsArray()) return std::move(result);
	for (SizeType i = 0; i < val.Size(); ++i) {
		if (val[i].IsBool()) {
			result.push_back(val[i].GetBool());
		}
	}
	return std::move(result);
}

void write(const char* path, const char* key, const char* val)
{
	Value* object = followPath(path, true); if (!object) return;
	while (object->HasMember(key)) { object->RemoveMember(key); }
	object->AddMember(Value(key, doc.GetAllocator()).Move(), Value(val, doc.GetAllocator()).Move(), doc.GetAllocator());
	modified = true;
}

void write(const char* path, const char* key, int val)
{
	Value* object = followPath(path, true); if (!object) return;
	while (object->HasMember(key)) { object->RemoveMember(key); }
	object->AddMember(Value(key, doc.GetAllocator()).Move(), val, doc.GetAllocator());
	modified = true;
}

void write(const char* path, const char* key, float val)
{
	Value* object = followPath(path, true); if (!object) return;
	while (object->HasMember(key)) { object->RemoveMember(key); }
	object->AddMember(Value(key, doc.GetAllocator()).Move(), val, doc.GetAllocator());
	modified = true;
}

void write(const char* path, const char* key, bool val)
{
	Value* object = followPath(path, true); if (!object) return;
	while (object->HasMember(key)) { object->RemoveMember(key); }
	object->AddMember(Value(key, doc.GetAllocator()).Move(), val, doc.GetAllocator());
	modified = true;
}

}} // namespace wc::config