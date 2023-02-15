/******************************************************************************
 * luasystem.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Implements the core of the lua engine.
 *****************************************************************************/
#include "luasystem.h"

#include <fmt/core.h>
#include <string>
#include <vector>
using namespace std;

// #include "filesys/vfs.h"
// #include "tools/stringhelper.h"
#include "scripts/built/events.lua.h"
#include "scripts/built/math2.lua.h"
#include "scripts/built/sandbox.lua.h"
#include "sys/debug.h"
#include "tools/strtoken.h"

namespace {

  constexpr const char* SCRIPT_DIR = "scripts/";
  constexpr const char* SCRIPT_EXT = ".lua";

  lua_State* L = nullptr;

  int getToTable(string_view path, bool cancreate, bool skiplast) {
    int pushes = 0;
    lua_getglobal(L, "_G");
    pushes++;  // push
    StrTokenState sts;
    string_view   token = strToken(path, sts, "", ".");
    while (token != string_view()) {
      string tokenstring(token);
      // Traverse the list of names and for each name (maybe except the last) go
      // into that table.
      lua_getfield(L, -1, tokenstring.c_str());
      pushes++;  // push
      if (lua_isnil(L, -1)) {
        if (!cancreate) {
          lua_pop(L, pushes);
          return -1;
        }

        lua_pop(L,
                1);  // pop nil
                     // If the table we're looking for doesn't exist, create it.
        lua_newtable(L);                           // push
        lua_setfield(L, -2, tokenstring.c_str());  // pop
        lua_getfield(L, -1, tokenstring.c_str());  // push
      }
      token = strToken(std::nullopt, sts, "", ".");
    }

    return pushes;
  }

  const string nilstr = fmt::format("{}nil{}", dbg::ERRCOLR, dbg::CLEAR);

  void printLuaError(int errindex) {
    string errstr = lua_tostring(L, errindex);
    auto   pos    = errstr.find(": ");
    if (pos != string::npos) {
      auto errsrc = errstr.substr(0, pos);
      errstr.erase(0, pos + 2);
      dbg::error(errstr, errsrc);
    } else {
      dbg::error(fmt::format("{}", errstr));
    }
  }

}  // namespace

bool wc::lua::init() {
  L = luaL_newstate();
  luaL_openlibs(L);

  // Access the Global table "_G" so we can place new functions there.
  lua_getglobal(L, "_G");

  // print(...)
  // Redefined here so that it can use our debug CLI/logging functionality.
  lua_pushcfunction(L, [](lua_State* L) {
    // Get information about where 'print' was called from.
    const char* shortSrc    = nullptr;
    lua_Integer currentLine = 0;
    lua_getglobal(L, "debug");       // push _G.debug onto the stack
    lua_getfield(L, -1, "getinfo");  // push _G.debug.getinfo onto the stack
    lua_pushnumber(L, 2);            // push '2' onto the stack
    lua_pushstring(L, "Sl");         // push "Sl" onto the stack
    lua_call(L, 2, 1);               // call '_G.debug.getinfo(2, "Sl")'
    if (lua_istable(L, -1)) {
      lua_getfield(L, -1, "short_src");
      if (lua_isstring(L, -1)) shortSrc = lua_tostring(L, -1);
      lua_pop(L, 1);
      lua_getfield(L, -1, "currentline");
      if (lua_isnumber(L, -1)) currentLine = lua_tointeger(L, -1);
      lua_pop(L, 1);
      dbg::luaPrint({}, fmt::format("{}:{}", shortSrc, currentLine));
    } else {
      dbg::luaPrint({}, "");
    }
    lua_pop(L, 2);  // pop _G.debug and our returned table/nil off the stack.

    int argc = lua_gettop(L);
    for (int i = 1; i <= argc; ++i) {
      if (lua_isnil(L, i)) {
        dbg::luaPrintmore(nilstr);
      } else {
        dbg::luaPrintmore(lua_tostring(L, i));
      }
    }
    return 0;
  });
  lua_setfield(L, -2, "print");

  // printmore(...)
  // Like 'print' but less verbose.
  // Good for follow-up information.
  lua_pushcfunction(L, [](lua_State* L) {
    int          argc = lua_gettop(L);
    stringstream ss;
    for (int i = 1; i <= argc; ++i) {
      if (lua_isnil(L, i)) {
        dbg::luaPrintmore(nilstr);
      } else {
        dbg::luaPrintmore(lua_tostring(L, i));
      }
    }
    return 0;
  });
  lua_setfield(L, -2, "printmore");

  // dofile(scriptname)
  // Execute a file as a lua script.
  // Unlike the version from the lua standard library,
  // This runs the script properly in a protected environment.
  lua_pushcfunction(L, [](lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    bool        result   = doFile(filename);
    lua_pushboolean(L, result);
    return 1;
  });
  lua_setfield(L, -2, "dofile");

  lua_pop(L, 1);  // Pop '_G' off the stack.

  dbg::info("Finished initalizing lua system.");
  lua_getglobal(L, "_VERSION");
  dbg::infomore(fmt::format("{}", lua_tostring(L, -1)));
  lua_pop(L, 1);

  // Run core scripts here.
  if (luaL_loadbuffer(L, (const char*)FILE_math2_lua_DATA, FILE_math2_lua_SIZE,
                      "@math2")) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else if (lua_pcall(L, 0, 0, 0)) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else {
    dbg::infomore("Successfully ran math2.lua");
  }

  if (luaL_loadbuffer(L, (const char*)FILE_sandbox_lua_DATA,
                      FILE_sandbox_lua_SIZE, "@sandbox")) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else if (lua_pcall(L, 0, 0, 0)) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else {
    dbg::infomore("Successfully ran sandbox.lua");
  }

  if (luaL_loadbuffer(L, (const char*)FILE_events_lua_DATA,
                      FILE_events_lua_SIZE, "@events")) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else if (lua_pcall(L, 0, 0, 0)) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  } else {
    dbg::infomore("Successfully ran events.lua");
  }

  // More global functions, these require functionality defined by built-in
  // scripts.
  lua_getglobal(L, "_G");
  {

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
          luaL_error(L, "Requested script environment could not be found.");
          return 0;
        }
      }
      // Remove 'SCRIPT_ENV' from the stack, so only '[filename]' remains.
      lua_remove(L, -2);
      // Go grab 'readonly' and move it below '[filename]'.
      lua_getglobal(L, "readonly");
      lua_insert(L, -2);
      // Call 'readonly([filename])'.
      lua_call(L, 1, 1);
      return 1;
    });
    lua_setfield(L, -2, "require");
    lua_getglobal(L, "SANDBOX");
    lua_getglobal(L, "require");
    lua_setfield(L, -2, "require");
    lua_pop(L, 1);
  }
  lua_pop(L, 1);  // Pop _G

  return true;
}

lua_State* wc::lua::getState() { return L; }

bool wc::lua::runString(const char* str, const char* env,
                        const char* sourcename) {
  if (sourcename) {
    if (luaL_loadbuffer(L, str, strlen(str), sourcename)) {
      printLuaError(-1);
      lua_pop(L, 1);
      return false;
    }
  } else {
    if (luaL_loadstring(L, str)) {
      printLuaError(-1);
      lua_pop(L, 1);
      return false;
    }
  }

  if (env) {
    int pushes = getToTable(env, false, false);
    if (pushes == -1) {
      dbg::error({"Attempting to run Lua string in an environment "
                  "which does not exist.",
                  fmt::format("Requested environment: '{}'.", env)});
      lua_pop(L, 1);
      return false;
    }

    lua_setupvalue(L, -(pushes + 1), 1);
    lua_pop(L, pushes - 1);
  }

  if (lua_pcall(L, 0, 0, 0)) {
    printLuaError(-1);
    lua_pop(L, 1);
    return false;
  }

  return true;
}

bool wc::lua::doFile(const char* filename) {
  /*
  string path = makestr(SCRIPT_DIR, filename, SCRIPT_EXT);

  // Open and load the script file.
  vector<char> fdata = vfs::LoadFile(path.c_str(), true);
  if (fdata.empty()) {
      dbg::error("In wc::lua::doFile():\n");
      dbg::errmore("Could not load script '", filename, "'.\n");
      return false;
  }

  // Adjust the source name for debugging.
  path = makestr('@', path);

  // Load the script into lua.
  if (luaL_loadbuffer(L, (const char*)fdata.data(), fdata.size(), path.c_str()))
  { dbg::error("In wc::lua::doFile():\n"); dbg::errmore("Error loading
  script:\n"); lua_pop(L, 1); return false;
  }

  // Set up the protected environment.
  lua_getglobal(L, "setup_script_env");
  lua_pushstring(L, filename);
  if (lua_pcall(L, 1, 0, 0)) {
      dbg::error("In wc::lua::doFile():\n");
      dbg::errmore("Failed to set up environment for script '", filename,
  "':\n"); dbg::errmore(lua_tostring(L, -1), '\n'); lua_pop(L, 2); // pop the
  error and the loaded script function return false;
  }

  // Get the protected environment for the script.
  lua_getglobal(L, "SCRIPT_ENV");
  lua_getfield(L, -1, filename); // SCRIPT_ENV[filename] is the environment used
  by the script.

  lua_setfenv(L, -3); // pops [filename] and assigns it as the environment for
  "-3" (the script code). lua_pop(L, 1); // pops SCRIPT_ENV.

  // Actually run the script.
  if (lua_pcall(L, 0, 0, 0)) {
      printLuaError(-1);
      lua_pop(L, 1);
      return false;
  }
  */
  return true;
}
