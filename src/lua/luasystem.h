#ifndef HVH_WC_LUA_SYSTEM_H
#define HVH_WC_LUA_SYSTEM_H

#include <lua.hpp>

namespace wc {
namespace lua {

	// Initialize the lua system.
	// Returns true on success, false on failure.
	bool init();

	// Get the global lua state.
	// Returns nullptr on failure (if run before Init).
	lua_State* getState();

	// Execute a lua file.
	// Returns true on success, false on failure.
	bool doFile(const char* filename);

	// Execute each instance of a given file found in the virtual filesystem.
	// Returns true on success, false on failure.
	bool doEachFile(const char* filename);

	// Executes the lua source code 'src' within the environment 'env'.
	// If 'env' is null, 'src' will run in _G.
	// Returns true on success, false on failure.
	bool runString(const char* src, const char* env, const char* sourcename = nullptr);



}} // namespace wc::lua

#endif // HVH_WC_LUA_SYSTEM_H