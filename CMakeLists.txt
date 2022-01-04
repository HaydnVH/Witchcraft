# CMakeList.txt : CMake project for Witchcraft, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("Witchcraft")

# Standards and defines
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
add_definitions(
	-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_UNICODE -DUNICODE
	-DPLATFORM_SDL -DRENDERER_VULKAN)


# Include directories
include_directories ("src")
include_directories ("dependencies/rapidjson-master/include")
include_directories ("dependencies/SDL2-2.0.18-windows/include")
link_directories    ("dependencies/SDL2-2.0.18-windows/lib/x64")
include_directories	("dependencies/luaJIT-2.1.0-beta3/src")
link_directories	("dependencies/luaJIT-2.1.0-beta3/src")

# Packages
find_package (Vulkan REQUIRED)

# Add source to this project's executable
file (GLOB_RECURSE SOURCES
	src/*.cpp
	src/tools/*.cpp
	src/ecs/*.cpp
	src/ecs/components/*.cpp
	src/ecs/systems/*.cpp
	src/lua/*.cpp
	src/sys/*.cpp)
add_executable (Witchcraft ${SOURCES})
set_target_properties (Witchcraft PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Linked libraries
target_link_libraries (Witchcraft SDL2)
target_link_libraries (Witchcraft SDL2main)
target_link_libraries (Witchcraft lua51)


#target_link_libraries (Witchcraft DXGI)
#target_link_libraries (Witchcraft d3d12)

# Move .dlls over to wherever the .exe goes
add_custom_command(TARGET Witchcraft POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.18-windows/lib/x64/SDL2.dll"
	$<TARGET_FILE_DIR:Witchcraft>)

add_custom_command(TARGET Witchcraft POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${PROJECT_SOURCE_DIR}/dependencies/luaJIT-2.1.0-beta3/src/lua51.dll"
	$<TARGET_FILE_DIR:Witchcraft>)

# Compile lua corescripts
file (GLOB_RECURSE LUA_CORESCRIPTS ${PROJECT_SOURCE_DIR}/src/lua/corescripts/*.lua)
foreach(script ${LUA_CORESCRIPTS})
	add_custom_command(TARGET Witchcraft POST_BUILD
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dependencies/luaJIT-2.1.0-beta3/src
		COMMAND luajit.exe -b ${script} ${script}.h
	)
endforeach()
