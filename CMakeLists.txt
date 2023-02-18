###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Builds the engine.
###############################################################################
project (Witchcraft LANGUAGES C CXX)
cmake_minimum_required (VERSION 3.8)

# Set the C++ standard
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED YES)
set (CMAKE_CXX_EXTENSIONS OFF)

# Set some universal definitions
add_definitions(
  -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING -D_SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
  -DRENDERER_VULKAN -DVULKAN_HPP_NO_EXCEPTIONS)

# Platform-specific definition
IF (WIN32)
  add_definitions(-DPLATFORM_SDL -DPLATFORM_WIN32 -DSDL_MAIN_HANDLED -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_UNICODE -DUNICODE)
ELSE (UNIX)
  add_definitions(-DPLATFORM_SDL -DPLATFORM_LINUX)
ENDIF ()

# Add source to this project's executable
include_directories ("src")
file (GLOB_RECURSE SOURCES
  dependencies/lz4/lib/lz4.c
  dependencies/lz4/lib/lz4hc.c
  src/*.cpp
  src/filesys/*.cpp
  src/lua/*.cpp
  src/sys/*.cpp
)
add_executable (Witchcraft ${SOURCES})

# --------------------------------------------------------------------------- #
# --------------------- # Add Packages & Dependencies # --------------------- #
# --------------------------------------------------------------------------- #

# --- fmt --- #
include_directories	  ("dependencies/fmt/include")
find_library		  (FMT_LIB NAMES fmt fmtd HINTS "dependencies/fmt/lib/$ENV{PRESET_NAME}")
target_link_libraries (Witchcraft ${FMT_LIB})

# --- Lua --- #
include_directories	  ("dependencies/lua/include")
find_library		  (LUA_LIB NAMES lua_static HINTS "dependencies/lua/lib/$ENV{PRESET_NAME}")
target_link_libraries (Witchcraft ${LUA_LIB})

# --- LZ4 --- #
include_directories	  ("dependencies/lz4/lib")

# --- RapidJson --- #
include_directories	  ("dependencies/rapidjson/include")

# --- SDL --- #
include_directories	  ("dependencies/sdl/include")
find_library		  (SDL_LIB NAMES SDL2 SDL2d HINTS "dependencies/sdl/lib/$ENV{PRESET_NAME}")
find_library		  (SDLMAIN_LIB NAMES SDL2main SDL2maind HINTS "dependencies/sdl/lib/$ENV{PRESET_NAME}")
target_link_libraries (Witchcraft ${SDL_LIB})
target_link_libraries (Witchcraft ${SDLMAIN_LIB})
if (WIN32) # Move sdl2.dll over to where we need it
  if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_custom_command(TARGET Witchcraft POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2d.dll"
	$<TARGET_FILE_DIR:Witchcraft>)
  else ()
  add_custom_command(TARGET Witchcraft POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2.dll"
	$<TARGET_FILE_DIR:Witchcraft>)
  endif ()
endif ()

if (UNIX)
  target_link_libraries (Witchcraft pthread)
  target_link_libraries (Witchcraft dl)
endif ()

# Grab some utilities
if (WIN32)
  set (LUAC_EXE "windows/luac.exe")
  set (F2H_EXE "windows/FileToHeader.exe")
elseif (UNIX)
  set (LUAC_EXE "linux/luac")
  set (F2H_EXE "linux/FileToHeader")
endif ()

# Compile Lua scripts
file (GLOB_RECURSE LUA_SCRIPTS ${PROJECT_SOURCE_DIR}/src/lua/scripts/*.lua)
foreach(script ${LUA_SCRIPTS})
  get_filename_component(OUTPUT_FILENAME ${script} NAME)
  set (OUTPUT_FILENAME ${PROJECT_SOURCE_DIR}/src/lua/scripts/built/${OUTPUT_FILENAME})
  add_custom_command(TARGET Witchcraft PRE_BUILD
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies/lua/bin
	COMMAND ${LUAC_EXE} -o ${OUTPUT_FILENAME} ${script} 
  )
  add_custom_command(TARGET Witchcraft PRE_BUILD
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies/f2h/bin
	COMMAND ${F2H_EXE} ${OUTPUT_FILENAME}
  )
endforeach()

# Create symlinks so the "$install/data" directory can be found
add_custom_command(TARGET Witchcraft POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  "${PROJECT_SOURCE_DIR}/data"
  "$<TARGET_FILE_DIR:Witchcraft>/data"
)

# Include directories

#include_directories ("dependencies/rapidjson-master/include")
#include_directories ("dependencies/SDL2-2.0.18-windows/include")
#link_directories    ("dependencies/SDL2-2.0.18-windows/lib/x64")
#include_directories	("dependencies/luaJIT-2.1.0-beta3/src")
#link_directories	("dependencies/luaJIT-2.1.0-beta3/src")
#include_directories ("dependencies/flecs-3.0.3")
#include_directories	("dependencies/lz4-dev/lib")
#include_directories (${Vulkan_INCLUDE_DIRS})

# Linked libraries
#target_link_libraries (Witchcraft SDL2)
#target_link_libraries (Witchcraft SDL2main)
#target_link_libraries (Witchcraft lua51)
#target_link_libraries (Witchcraft ${Vulkan_LIBRARIES})


# Move .dlls over to wherever the .exe goes
#add_custom_command(TARGET Witchcraft POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#	"${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.18-windows/lib/x64/SDL2.dll"
#	$<TARGET_FILE_DIR:Witchcraft>)

#add_custom_command(TARGET Witchcraft POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy_if_different
#	"${PROJECT_SOURCE_DIR}/dependencies/luaJIT-2.1.0-beta3/src/lua51.dll"
#	$<TARGET_FILE_DIR:Witchcraft>)

# Compile Lua scripts
#file (GLOB_RECURSE LUA_SCRIPTS ${PROJECT_SOURCE_DIR}/src/lua/scripts/*.lua)
#foreach(script ${LUA_SCRIPTS})
#	get_filename_component(OUTPUT_FILENAME ${script} NAME)
#	set (OUTPUT_FILENAME ${PROJECT_SOURCE_DIR}"/src/lua/scripts/build/"${OUTPUT_FILENAME}".h")
#	add_custom_command(TARGET Witchcraft POST_BUILD
#		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies/luaJIT-2.1.0-beta3/src
#		COMMAND luajit.exe -b ${script} ${OUTPUT_FILENAME}
#	)
#endforeach()

# Compile GLSL shaders
#file (GLOB_RECURSE GLSL_SHADERS ${PROJECT_SOURCE_DIR}/src/graphics/shaders/*.glsl)
#foreach(shader ${GLSL_SHADERS})
#	get_filename_component(OUTPUT_FILENAME ${shader} NAME_WLE)
#	set (OUTPUT_FILENAME ${PROJECT_SOURCE_DIR}"/src/graphics/shaders/build/"${OUTPUT_FILENAME}".spv")
#	add_custom_command(TARGET Witchcraft POST_BUILD
#		WORKING_DIRECTORY ${Vulkan_INCLUDE_DIR}/../Bin
#		COMMAND glslc.exe ${shader} -o ${OUTPUT_FILENAME}
#	)
#	add_custom_command(TARGET Witchcraft POST_BUILD
#		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies
#		COMMAND FileToHeader.exe ${OUTPUT_FILENAME}
#	)
#endforeach()
