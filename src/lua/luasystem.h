/******************************************************************************
 * luasystem.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Defines the interface for the lua scripting engine.
 *****************************************************************************/
#ifndef WC_LUA_LUASYSTEM_H
#define WC_LUA_LUASYSTEM_H

#include "filesys/vfs.h"
#include "tools/result.h"

#include <lua.hpp>

namespace wc {

  class Lua {

    static constexpr const char* SCRIPT_DIR = "scripts/";
    static constexpr const char* SCRIPT_EXT = ".lua";
    static constexpr const char* NILSTR     = "nil";

  public:
    Lua(Filesystem& vfs);
    ~Lua();

    lua_State*       operator*() { return L_; }
    const lua_State* operator*() const { return L_; }
    lua_State*       operator->() { return L_; }
    const lua_State* operator->() const { return L_; }

    /// Executes the lua source code 'src' within the environment 'env'.
    /// If 'env' is null, 'src' will run in _G.
    /// @return an empty Result which may contain an error message.
    wc::Result::Empty runString(const char* src, const char* env,
                                const char* srcName = nullptr);

    /// Execute a lua file.
    /// @return An empty Result which may contain an error message.
    wc::Result::Empty doFile(const char* filename);

    /// Execute each instance of a given file found in the virtual filesystem.
    /// @return An empty Result which may contain an error message.
    wc::Result::Empty doEachFile(const char* filename);

  private:
    Filesystem& vfs_;
    lua_State*  L_ = nullptr;

    int  getToTable(std::string_view path, bool mayCreate, bool skipLast);
    void printLuaError(int errindex);
  };
}  // namespace wc

#endif  // HVH_LUA_LUASYSTEM_H