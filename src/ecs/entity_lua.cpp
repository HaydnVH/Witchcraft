#include "entity.h"

#include "lua/luasystem.h"

namespace entity {

	bool initLua() {
		lua_State* L = wc::lua::getState();

		// This is the metatable for the 'entity' userdata type.
		luaL_newmetatable(L, "entity"); {

			// Set the entity's metatable __index to the metatable,
			// so other code can directly add to the entity's functionality.
			luaL_getmetatable(L, "entity");
			lua_setfield(L, -2, "__index");

			// equality check metamethod.
			lua_pushcfunction(L, [](lua_State* L) {
				ID* lhs = (ID*)luaL_checkudata(L, 1, "entity");
				ID* rhs = (ID*)luaL_checkudata(L, 2, "entity");
				lua_pushboolean(L, ((*lhs) == (*rhs)));
				return 1;
			}); lua_setfield(L, -2, "__eq");

			// less-than check metamethod.
			lua_pushcfunction(L, [](lua_State* L) {
				ID* lhs = (ID*)luaL_checkudata(L, 1, "entity");
				ID* rhs = (ID*)luaL_checkudata(L, 1, "entity");
				lua_pushboolean(L, ((*lhs) < (*rhs)));
				return 1;
			}); lua_setfield(L, -2, "__lt");

			// less-than-or-equal check metamethod.
			lua_pushcfunction(L, [](lua_State* L) {
				ID* lhs = (ID*)luaL_checkudata(L, 1, "entity");
				ID* rhs = (ID*)luaL_checkudata(L, 1, "entity");
				lua_pushboolean(L, ((*lhs) <= (*rhs)));
				return 1;
			}); lua_setfield(L, -2, "__le");

			// string conversion metamethod.
			lua_pushcfunction(L, [](lua_State* L) {
				ID* id = (ID*)luaL_checkudata(L, 1, "entity");
				std::string str = entity::toString(*id);
				lua_pushstring(L, str.c_str());
				return 1;
			}); lua_setfield(L, -2, "__tostring");


		} lua_pop(L, 1);


		lua_newtable(L); { // Create the 'entity' table.

			// Create a new entity.
			lua_pushcfunction(L, [](lua_State* L) {
				ID* id = (ID*)lua_newuserdata(L, sizeof(ID));
				if (!id) return luaL_error(L, "Entity creation failure.");
				*id = entity::create();
				luaL_getmetatable(L, "entity"); lua_setmetatable(L, -2);
				return 1;
			}); lua_setfield(L, -2, "create");

			// Creates a lua entity using the same internal entity ID.
			// Used to test whether IDs work properly as table keys.
			lua_pushcfunction(L, [](lua_State* L) {
				static ID id = entity::create();
				ID* result = (ID*)lua_newuserdata(L, sizeof(ID));
				if (!result) return luaL_error(L, "Entity creation failure.");
				*result = id;
				luaL_getmetatable(L, "entity"); lua_setmetatable(L, -2);
				return 1;
			}); lua_setfield(L, -2, "sameness_test");

		}
		lua_setglobal(L, "ENTITY");

		lua_getglobal(L, "SANDBOX");
		lua_getglobal(L, "readonly");
		lua_getglobal(L, "ENTITY");
		lua_call(L, 1, 1);
		lua_setfield(L, -2, "entity");
		lua_pop(L, 1);


		return true;
	}

} // namespace entity