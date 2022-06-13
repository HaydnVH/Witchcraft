The scripting language used by the Witchcraft Engine is Lua, specifically Lua 5.1 as implemented by LuaJIT 2.1.0-beta3.

Every script runs in its own private sandbox environment.  This means that global variables defined in one script are not automatically visible from another.  This sandboxed environment does not have access to all of the standard Lua libraries; a whitelist of functions from the default library are available.  The Following functions and values have the same behaviour as regular Lua 5.1:

- `assert`
- `error`
- `ipairs`
- `next`
- `pairs`
- `pcall`
- `select`
- `tonumber`
- `tostring`
- `type`
- `_VERSION`
- `xpcall`
- Everything in `coroutine`
- Everything in `math`
- Everything in `string`
- From `os`:
  - `clock`
  - `date`
  - `difftime`
  - `time`

The following functions have been redefined to properly work within the engine:

- `print` should work as expected, but outputs are properly colored and sent to the console and log.
- `dofile` does not search the local filesystem, but picks scripts which are properly loaded in the engine's virtual filesystem.  The script is run safely in a private sandbox environment.
- `require`, like `dofile`, but it does not run a script that has already been run, and returns a read-only reference to the required script's environment table.  Use this to access functions defined by other scripts.