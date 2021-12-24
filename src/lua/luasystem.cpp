#include "luasystem.h"

#include <vector>
#include <string>
using namespace std;

#include "tools/stringhelper.h"

#include "debug.h"

namespace {

	constexpr const char* SCRIPTS_DIR = "scripts/";
	constexpr const char* SCRIPTS_EXT = ".lua";

	lua_State* L = nullptr;

	int GetToTable(string_view path, bool cancreate, bool skiplast)
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
		}

		return pushes;
	}

	const string nilstr = makestr(debug::ERRCOLR, "nil", debug::CLEAR);

} // namespace <anon>

namespace lua {

	bool Init() {
		L = luaL_newstate();
		luaL_openlibs(L);

		lua_getglobal(L, "_G");
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
				debug::info(ss.str(), '\n');
				return 0;
			});
			lua_setfield(L, -2, "print");
		lua_pop(L, 1);

		lua_getglobal(L, "_VERSION");
			debug::info("Finished initalizing lua system.\n");
			debug::infomore("Using '", lua_tostring(L, -1), "'.\n");
		lua_pop(L, 1);
		return true;
	}

	lua_State* getState() {
		return L;
	}

	bool RunString(const char* str, const char* env) {
		if (luaL_loadstring(L, str)) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1);
			return false;
		}

		if (env) {
			int pushes = GetToTable(env, false, false);
			if (pushes == -1) {
				debug::error("Attempting to run Lua string in an environment which does not exist.\n");
				debug::errmore("Requested environment: '", env, "'.\n");
				lua_pop(L, 1);
				return false;
			}

			lua_setfenv(L, -(pushes + 1));
			lua_pop(L, pushes);
		}

		if (lua_pcall(L, 0, 0, 0)) {
			debug::error(lua_tostring(L, -1), '\n');
			lua_pop(L, 1); // Pop the error mesage off the stack.
			return false;
		}

		return true;
	}

} // namespace lua