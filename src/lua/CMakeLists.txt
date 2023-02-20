###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "lua" subdirectory.
###############################################################################
target_sources(Witchcraft PRIVATE
  "luasystem.cpp")

# Compile the lua scripts to binary, and then transform those files into headers
# which can be included directly into the executable.

# Grab the executables that'll help us compile the lua scripts.
if (WIN32)
  set (LUAC_EXE "dependencies/lua/apps/windows/luac.exe")
  set (F2H_EXE "dependencies/f2h/apps/windows/FileToHeader.exe")
elseif (UNIX)
  set (LUAC_EXE "dependencies/lua/apps/linux/luac")
  set (F2H_EXE "dependencies/f2h/apps/linux/FileToHeader")
endif ()

# Find all the script files in the scripts folder
file(GLOB_RECURSE LUA_SCRIPTS "${PROJECT_SOURCE_DIR}/src/lua/scripts_src/*.lua")

# For each script we found...
foreach(SCRIPT ${LUA_SCRIPTS})
  # Set some filenames.
  get_filename_component(SCRIPT_FILENAME "${SCRIPT}" NAME)
  set (BINARY_FILENAME "${PROJECT_SOURCE_DIR}/src/lua/scripts_bin/${SCRIPT_FILENAME}c")
  set (HEADER_FILENAME "${PROJECT_SOURCE_DIR}/src/lua/scripts_inc/${SCRIPT_FILENAME}.h")
  # Compile the lua source scripts into binary files,
  # then convert the binary files into C headers.
  add_custom_command(PRE_BUILD
	WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
	OUTPUT "${HEADER_FILENAME}"
	COMMAND ${LUAC_EXE} -o "${BINARY_FILENAME}" "${SCRIPT}"
	COMMAND ${F2H_EXE} -i "${BINARY_FILENAME}" -o "${HEADER_FILENAME}"
	DEPENDS "${SCRIPT}"
  )
  # Create a list of the files we created.
  list(APPEND LUA_BINARY_FILES "${HEADER_FILENAME}")
endforeach(SCRIPT)

# Create a dependency for the scripts.
add_custom_target(
	LuaScripts
	DEPENDS ${LUA_BINARY_FILES}
)
add_dependencies(Witchcraft LuaScripts)