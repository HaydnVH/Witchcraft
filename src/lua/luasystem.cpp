#include "luasystem.h"

#include <vector>
#include <string>
using namespace std;

#include "tools/stringhelper.h"

#include "debug.h"

#include "corescripts/sandbox.lua.h"
#include "corescripts/math2.lua.h"
#include "corescripts/events.lua.h"

namespace {

	constexpr const char* SCRIPTS_DIR = "scripts/";
	constexpr const char* SCRIPTS_EXT = ".lua";

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

} // namespace <anon>

namespace wc {
namespace lua {

	bool Init() {
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
					debug::print(debug::USER, debug::MAGENTA, "LUA (", short_src, ":", currentline, "): ", debug::CLEAR);
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
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran math2.lua\n");
		}

		if (luaL_loadbuffer(L, (const char*)luaJIT_BC_sandbox, luaJIT_BC_sandbox_SIZE, "@sandbox")) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran sandbox.lua\n");
		}

		if (luaL_loadbuffer(L, (const char*)luaJIT_BC_events, luaJIT_BC_events_SIZE, "@events")) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else if (lua_pcall(L, 0, 0, 0)) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}
		else {
			debug::infomore("Successfully ran events.lua\n");
		}

		return true;
	}

	lua_State* getState() {
		return L;
	}

	bool RunString(const char* str, const char* env, const char* sourcename) {
		if (sourcename) {
			if (luaL_loadbuffer(L, str, strlen(str), sourcename)) {
				debug::error(lua_tostring(L, -1), '\n');
				lua_pop(L, 1);
				return false;
			}
		}
		else {
			if (luaL_loadstring(L, str)) {
				debug::error(lua_tostring(L, -1), '\n');
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
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1); // Pop the error mesage off the stack.
			return false;
		}

		return true;
	}

}} // namespace wc::lua