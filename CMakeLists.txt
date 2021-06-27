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
	-DRENDERER_D3D12)

# Directories
include_directories ("src")
include_directories ("dependencies/rapidjson-master/include")

# External library directories
# Update as needed for your installation

# Add source to this project's executable
file (GLOB_RECURSE SOURCES
	src/*.cpp
	src/ecs/*.cpp
	src/ecs/components/*.cpp
	src/ecs/systems/*.cpp
	src/sys/*.cpp)
add_executable (Witchcraft ${SOURCES})
set_target_properties (Witchcraft PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Linked libraries
target_link_libraries (Witchcraft DXGI)
target_link_libraries (Witchcraft d3d12)

# Move .dll over to wherever the .exe goes
