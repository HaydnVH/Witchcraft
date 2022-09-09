#include "luasystem.h"

#include <vector>
#include <string>
using namespace std;

#include "filesys/vfs.h"
#include "tools/stringhelper.h"
#include "debug.h"

#include "scripts/build/sandbox.lua.h"
#include "scripts/build/math2.lua.h"
#include "scripts/build/events.lua.h"

namespace {

	constexpr const char* SCRIPT_DIR = "scripts/";
	constexpr const char* SCRIPT_EXT = ".lua";

	lua_State* L = nullptr;

	int getToTable(string_view path, bool cancreate, bool skiplast)
	{
		int pushes = 0;
		lua_getglobal(L, "_G"); pushes++; // push
		string_view token = tokenize(path, "", ".");
		while (token != string_view()) {
			string tokenstring(token);
			// Traverse the list of names and for each name (maybe except the last) go into that table.
			lua_getfield(L, -1, tokenstring.c_str()); pushes++; // push
			if (lua_isnil(L, -1))
			{
				if (!cancreate)
				{
					lua_pop(L, pushes);
					return -1;
				}

				lua_pop(L, 1); // pop nil
							   // If the table we're looking for doesn't exist, create it.
				lua_newtable(L); // push
				lua_setfield(L, -2, tokenstring.c_str()); // pop
				lua_getfield(L, -1, tokenstring.c_str()); // push
			}
			token = tokenize("", "", ".");
		}

		return pushes;
	}

	const string nilstr = makestr(debug::ERRCOLR, "nil", debug::CLEAR);

	void printLuaError(int errindex) {
		string errstr = lua_tostring(L, errindex);
		if (errstr.find(": ") != string::npos) {
			errstr.replace(errstr.find(": "), 2, "]: \033[0m"); // debug::CLEAR
			debug::print(debug::ERROR, debug::ERRCOLR, "ERROR [", errstr, '\n');
		}
		else {
			debug::error(errstr, '\n');
		}
	}

} // namespace <anon>

namespace wc {
namespace lua {

	bool init() {
		L = luaL_newstate();
		luaL_openlibs(L);

		// Access the Global table "_G" so we can place new functions there.
		lua_getglobal(L, "_G");
			
			// print(...)
			// Redefined here so that it can use our debug CLI/logging functionality.
			lua_pushcfunction(L, [](lua_State* L) {
				// Get information about where 'print' was called from.
				const char* short_src = nullptr;
				lua_Integer currentline = 0;
				lua_getglobal(L, "debug"); // push _G.debug onto the stack
				lua_getfield(L, -1, "getinfo"); // push _G.debug.getinfo onto the stack
				lua_pushnumber(L, 2); // push '2' onto the stack
				lua_pushstring(L, "Sl"); // push "Sl" onto the stack
				lua_call(L, 2, 1); // call '_G.debug.getinfo(2, "Sl")'
				if (lua_istable(L, -1)) {
					lua_getfield(L, -1, "short_src");
					  if (lua_isstring(L, -1)) short_src = lua_tostring(L, -1);
					lua_pop(L, 1);
					lua_getfield(L, -1, "currentline");
					  if (lua_isnumber(L, -1)) currentline = lua_tointeger(L, -1);
					lua_pop(L, 1);
					debug::print(debug::USER, debug::MAGENTA, "LUA [", short_src, ":", currentline, "]: ", debug::CLEAR);
				}
				else {
					debug::print(debug::USER, debug::MAGENTA, "LUA: ", debug::CLEAR);
				}
				lua_pop(L, 2); // pop _G.debug and our returned table/nil off the stack.

				int argc = lua_gettop(L);
				stringstream ss;
				for (int i = 1; i <= argc; ++i) {
					if (lua_isnil(L, i)) {
						ss << nilstr;
					}
					else {
						const char* str = lua_tostring(L, i);
						if (str) { ss << str; }
					}
				}
				debug::print(debug::USER, ss.str(), '\n');
				return 0;
			});
			lua_setfield(L, -2, "print");

			// printmore(...)
			// Like 'print' but less verbose.
			// Good for follow-up information.
			lua_pushcfunction(L, [](lua_State* L) {
				int argc = lua_gettop(L);
				stringstream ss;
				for (int i = 1; i <= argc; ++i) {
					if (lua_isnil(L, i)) {
						ss << nilstr;
					}
					else {
						const char* str = lua_tostring(L, i);
						if (str) { ss << str; }
					}
				}
				debug::print(debug::USER, debug::MAGENTA, "  -   ", debug::CLEAR, ss.str(), '\n');
				return 0;
			});
			lua_setfield(L, -2, "printmore");

			// dofile(scriptname)
			// Execute a file as a lua script.
			// Unlike the version from the lua standard library,
			// This runs the script properly in a protected environment.
			lua_pushcfunction(L, [](lua_State* L) {
				const char* filename = luaL_checkstring(L, 1);
				bool result = doFile(filename);
				lua_pushboolean(L, result);
				return 1;
			});
			lua_setfield(L, -2, "dofile");

		lua_pop(L, 1); // Pop '_G' off the stack.

		debug::info("Finished initalizing lua system.\n");
		lua_getglobal(L, "_VERSION");
		  debug::infomore(lua_tostring(L, -1), '\n');
		lua_pop(L, 1);
		lua_getglobal(L, "jit");
		  lua_getfield(L, -1, "version");
		    debug::infomore(lua_tostring(L, -1), '\n');
		lua_pop(L, 2);

		// Run core scripts here.
		if (luaL_loadbuffer(L, (const char*)luaJIT_BC_math2, luaJIT_BC_math2_SIZE, "@math2")) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran math2.lua\n");
		}

		if (luaL_loadbuffer(L, (const char*)luaJIT_BC_sandbox, luaJIT_BC_sandbox_SIZE, "@sandbox")) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran sandbox.lua\n");
		}

		if (luaL_loadbuffer(L, (const char*)luaJIT_BC_events, luaJIT_BC_events_SIZE, "@events")) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran events.lua\n");
		}

		// More global functions, these require functionality defined by built-in scripts.
		lua_getglobal(L, "_G"); {

			// require(scriptname)
			// Exexute a file as a lua script only if it has not been run before,
			// then return a readonly reference to its environment table.
			// Runs in a protected environment, like dofile.
			lua_pushcfunction(L, [](lua_State* L) {
				const char* filename = luaL_checkstring(L, 1);
				// Get the SCRIPT_ENV[filename] table and put it on the stack.
				lua_getglobal(L, "SCRIPT_ENV");
				lua_getfield(L, -1, filename);
				// If it's not a table, call 'dofile' to run it.
				if (!lua_istable(L, -1)) {
					lua_pop(L, 1);
					doFile(filename);
					lua_getfield(L, -1, filename);
					// If it's still not a table, something failed.
					if (!lua_istable(L, -1)) {
						lua_pop(L, 1);
						luaL_error(L, "Requested script environment could not be found.\n");
						return 0;
					}
				}
				// Remove 'SCRIPT_ENV' from the stack, so only '[filename]' remains.
				lua_remove(L, -2);
				// Go grab 'readonly' and move it below '[filename]'.
				lua_getglobal(L, "readonly"); lua_insert(L, -2);
				// Call 'readonly([filename])'.
				lua_call(L, 1, 1);
				return 1;
			});
			lua_setfield(L, -2, "require");
			lua_getglobal(L, "SANDBOX");
			lua_getglobal(L, "require");
			lua_setfield(L, -2, "require");
			lua_pop(L, 1);

		} lua_pop(L, 1); // Pop _G


		return true;
	}

	lua_State* getState() {
		return L;
	}

	bool runString(const char* str, const char* env, const char* sourcename) {
		if (sourcename) {
			if (luaL_loadbuffer(L, str, strlen(str), sourcename)) {
				printLuaError(-1);
				lua_pop(L, 1);
				return false;
			}
		}
		else {
			if (luaL_loadstring(L, str)) {
				printLuaError(-1);
				lua_pop(L, 1);
				return false;
			}
		}

		if (env) {
			int pushes = getToTable(env, false, false);
			if (pushes == -1) {
				debug::error("Attempting to run Lua string in an environment which does not exist.\n");
				debug::errmore("Requested environment: '", env, "'.\n");
				lua_pop(L, 1);
				return false;
			}

			lua_setfenv(L, -(pushes + 1));
			lua_pop(L, pushes-1);
		}

		if (lua_pcall(L, 0, 0, 0)) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}

		return true;
	}

	bool doFile(const char* filename) {
		string path = makestr(SCRIPT_DIR, filename, SCRIPT_EXT);

		// Open and load the script file.
		FileData file = vfs::LoadFile(path.c_str(), true);
		if (!file.is_open()) {
			debug::error("In wc::lua::doFile():\n");
			debug::errmore("Script '", filename, "' does not exist.\n");
			return false;
		}

		// Adjust the source name for debugging.
		if (file.getSourceName().size() > 0) { path = makestr('@', file.getSourceName(), "::", path); }
		else { path = makestr('@', path); }

		// Load the script into lua.
		if (luaL_loadbuffer(L, (const char*)file.data(), file.size(), path.c_str())) {
			debug::error("In wc::lua::doFile():\n");
			debug::errmore("Error loading script:\n");
			lua_pop(L, 1);
			return false;
		}

		// Set up the protected environment.
		lua_getglobal(L, "setup_script_env");
		lua_pushstring(L, filename);
		if (lua_pcall(L, 1, 0, 0)) {
			debug::error("In wc::lua::doFile():\n");
			debug::errmore("Failed to set up environment for script '", filename, "':\n");
			debug::errmore(lua_tostring(L, -1), '\n');
			lua_pop(L, 2); // pop the error and the loaded script function
			return false;
		}

		// Get the protected environment for the script.
		lua_getglobal(L, "SCRIPT_ENV");
		lua_getfield(L, -1, filename); // SCRIPT_ENV[filename] is the environment used by the script.

		lua_setfenv(L, -3); // pops [filename] and assigns it as the environment for "-3" (the script code).
		lua_pop(L, 1); // pops SCRIPT_ENV.

		// Actually run the script.
		if (lua_pcall(L, 0, 0, 0)) {
			printLuaError(-1);
			lua_pop(L, 1);
			return false;
		}

		return true;
	}

}} // namespace wc::lua