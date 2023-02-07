# CMakeList.txt : CMake project for Witchcraft
project (Witchcraft)
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
  src/*.cpp
  src/sys/*.cpp
)
add_executable (Witchcraft ${SOURCES})

# Set the CWD for Visual Studio debugging (we want it to be the project root)
set_target_properties (Witchcraft PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

############### Add Packages & Dependencies #################

# --- fmt --- #
add_subdirectory("dependencies/fmt")
include_directories("dependencies/fmt/include")
target_link_libraries(Witchcraft fmt)

# --- Lua --- #
#find_package (lua REQUIRED)
#target_include_directories(Witchcraft PRIVATE ${LUA_INCLUDE_DIR})
#target_link_libraries (Witchcraft ${LUA_LIBRARIES})

# --- SDL --- #
add_subdirectory("dependencies/sdl")
include_directories("dependencies/sdl/include")
target_link_libraries(Witchcraft SDL2main)
target_link_libraries(Witchcraft SDL2)
if (WIN32) # Move sdl2.dll over to where we need it
add_custom_command(TARGET Witchcraft POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "$<TARGET_FILE_DIR:Witchcraft>/dependencies/sdl/SDL2d.dll"
  $<TARGET_FILE_DIR:Witchcraft>)
endif ()

# --- Replxx --- #
IF (UNIX)
add_subdirectory("dependencies/replxx")
include_directories("dependencies/replxx/include")
target_link_libraries(Witchcraft replxx)
ENDIF ()


# Include directories

#include_directories ("dependencies/rapidjson-master/include")
#include_directories ("dependencies/SDL2-2.0.18-windows/include")
#link_directories    ("dependencies/SDL2-2.0.18-windows/lib/x64")
#include_directories	("dependencies/luaJIT-2.1.0-beta3/src")
#link_directories	("dependencies/luaJIT-2.1.0-beta3/src")
#include_directories ("dependencies/flecs-3.0.3")
#include_directories	("dependencies/lz4-dev/lib")
#include_directories (${Vulkan_INCLUDE_DIRS})

# Add source to this project's executable
#file (GLOB_RECURSE SOURCES
#	dependencies/flecs-3.0.3/flecs.c
#	dependencies/lz4-dev/lz4.c
#	dependencies/lz4-dev/lz4hc.c
#	src/*.cpp
#	src/sys/*.cpp
#	src/tools/*.cpp
#	src/ecs/*.cpp
#	src/ecs/components/*.cpp
#	src/ecs/systems/*.cpp
#	src/lua/*.cpp
#	src/graphics/*.cpp
#)
#add_executable (Witchcraft ${SOURCES})
#set_target_properties (Witchcraft PROPERTIES
#	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")



# Linked libraries
#target_link_libraries (Witchcraft SDL2)
#target_link_libraries (Witchcraft SDL2main)
#target_link_libraries (Witchcraft lua51)
#target_link_libraries (Witchcraft ${Vulkan_LIBRARIES})


#target_link_libraries (Witchcraft DXGI)
#target_link_libraries (Witchcraft d3d12)

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
