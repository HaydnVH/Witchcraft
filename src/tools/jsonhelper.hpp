
#ifndef HVH_TOOLS_JSONHELPER_H
#define HVH_TOOLS_JSONHELPER_H

#include <string>
#include <vector>
#include <cstdint>

class jsonHelper {
public:
	jsonHelper(const char* filename);
	jsonHelper(const jsonHelper& rhs) = delete;
	jsonHelper(jsonHelper&& rhs) { doc = rhs.doc; rhs.doc = nullptr; }
	jsonHelper& operator = (const jsonHelper& rhs) = delete;
	jsonHelper& operator = (jsonHelper&& rhs) { doc = rhs.doc; rhs.doc = nullptr; return *this; }
	~jsonHelper();

	bool isValid();

	bool EnterObject(const char* key);

	bool Read(const char* key, bool& out_val);
	bool Read(const char* key, int32_t& out_val);
	bool Read(const char* key, uint32_t& out_val);
	bool Read(const char* key, int64_t& out_val);
	bool Read(const char* key, uint64_t& out_val);
	bool Read(const char* key, double& out_val);
	bool Read(const char* key, std::string& out_val);

	bool Read(const char* key, std::vector<int32_t>& out_vals);
	bool Read(const char* key, std::vector<uint32_t>& out_vals);
	bool Read(const char* key, std::vector<int64_t>& out_vals);
	bool Read(const char* key, std::vector<uint64_t>& out_vals);
	bool Read(const char* key, std::vector<double>& out_vals);
	bool Read(const char* key, std::vector<std::string>& out_vals);



	template<typename T>
	bool FollowPath(const T& path) {
		return _followPath(path)
	}

	template<typename T, typename... Ts>
	bool FollowPath(const T& firstPath, T& restPaths) {
		if (!_followPath(path)) return false;
		FollowPath(restPaths...);
	}


	struct _JSONDocType;
	struct _JSONObjType;

private:
	bool valid = false;
	_JSONDocType* doc = nullptr;
	std::vector<_JSONObjType*> objStack;


	bool _followPath(const char* key);

}; // namespace jsonHelper

#endif // HVH_TOOLS_JSONHELPER_H
