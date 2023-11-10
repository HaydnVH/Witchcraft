###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Builds the engine.
###############################################################################
cmake_minimum_required (VERSION 3.19...3.26)
project (Witchcraft VERSION 0.14 
					DESCRIPTION "A game engine written by Haydn V. Harach"
					LANGUAGES C CXX)

# Set some universal definitions
add_definitions(
  -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING -D_SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
  -DRENDERER_VULKAN)

# Platform-specific definition
if (WIN32)
  add_definitions(-DPLATFORM_SDL -DPLATFORM_WIN32 -DSDL_MAIN_HANDLED -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_UNICODE -DUNICODE)
elseif (UNIX)
  add_definitions(-DPLATFORM_SDL -DPLATFORM_LINUX)
endif ()

# Create our executible and define some properties
add_executable(Witchcraft)
set_target_properties(Witchcraft PROPERTIES CXX_STANDARD 20
											CXX_STANDARD_REQUIRED true
											CXX_EXTENSIONS false)

add_executable(WcUnitTest)
set_target_properties(WcUnitTest PROPERTIES CXX_STANDARD 20
											CXX_STANDARD_REQUIRED true
											CXX_EXTENSIONS false)

# Add our source files
target_include_directories(Witchcraft PRIVATE "src")
target_include_directories(WcUnitTest PRIVATE "src")
add_subdirectory("src")

# Add our dependencies
add_subdirectory(dependencies)

# Add third party dependencies
add_subdirectory(thirdparty)

# Create symlinks so the "$INSTALL/data" directory can be found.
add_custom_command(TARGET Witchcraft POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  "${PROJECT_SOURCE_DIR}/data"
  "$<TARGET_FILE_DIR:Witchcraft>/data"
)

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
