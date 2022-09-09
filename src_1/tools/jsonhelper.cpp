
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

using namespace rapidjson;

#include "jsonhelper.hpp"

using namespace std;

struct jsonHelper::_JSONDocType{
	Document doc;
};

static vector<char> buffer(10240);

jsonHelper::jsonHelper(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) return;
	FileReadStream stream(file, buffer.data(), buffer.size());

	this->doc = new _JSONDocType;
	doc->doc.ParseStream<0, UTF8<>, FileReadStream>(stream);
	if (doc->doc.HasParseError() || !doc->doc.IsObject()) return;

	valid = true;
}

jsonHelper::~jsonHelper() {
	if (doc) {
		delete doc;
	}
}

