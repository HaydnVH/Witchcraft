###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Manages and links all dependencies.
###############################################################################

## --- blake3 --- ##
add_subdirectory(blake3)
target_link_libraries(Witchcraft PRIVATE blake3)

## --- fmt --- ##
add_library(fmt STATIC IMPORTED)
find_library(FMT_LIB NAMES fmt fmtd HINTS "fmt/lib/$ENV{PRESET_NAME}")
set_target_properties(fmt PROPERTIES IMPORTED_LOCATION "${FMT_LIB}")
target_include_directories(fmt INTERFACE "fmt/include")
target_link_libraries(fmt INTERFACE "${FMT_LIB}")
target_link_libraries(Witchcraft PRIVATE fmt)

## --- glm --- ##
add_definitions(-DGLM_FORCE_RADIANS, -DGLM_FORCE_DEPTH_ZERO_TO_ONE)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "glm/include")
target_link_libraries(Witchcraft PRIVATE glm)

## --- lua --- ##
add_library(lua STATIC IMPORTED)
find_library(LUA_LIB NAMES lua_static HINTS "lua/lib/$ENV{PRESET_NAME}")
set_target_properties(lua PROPERTIES IMPORTED_LOCATION "${LUA_LIB}")
target_include_directories(lua INTERFACE "lua/include")
target_link_libraries(lua INTERFACE "${LUA_LIB}")
target_link_libraries(Witchcraft PRIVATE lua)

## --- lz4 --- ##
add_library(lz4 STATIC)
target_include_directories(lz4 PUBLIC "lz4/lib")
target_sources(lz4 PRIVATE "lz4/lib/lz4.c" "lz4/lib/lz4hc.c")
target_link_libraries(Witchcraft PRIVATE lz4)

## --- RapidJson --- ## 
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "rapidjson/include")
target_link_libraries(Witchcraft PRIVATE rapidjson)

## --- scn --- ##
add_library(scn STATIC IMPORTED)
find_library(SCN_LIB NAMES scn HINTS "scn/lib/$ENV{PRESET_NAME}")
set_target_properties(scn PROPERTIES IMPORTED_LOCATION "${SCN_LIB}")
target_include_directories(scn INTERFACE "scn/include")
target_link_libraries(scn INTERFACE "${SCN_LIB}")
target_link_libraries(Witchcraft PRIVATE scn)

## --- sdl --- ##
add_library(sdl STATIC IMPORTED)
find_library(SDL_LIB NAMES SDL2 SDL2d HINTS "sdl/lib/$ENV{PRESET_NAME}")
find_library(SDLMAIN_LIB NAMES SDL2main SDL2maind HINTS "sdl/lib/$ENV{PRESET_NAME}")
set_target_properties(sdl PROPERTIES IMPORTED_LOCATION "${SDL_LIB}")
target_include_directories(sdl INTERFACE "sdl/include")
target_link_libraries(sdl INTERFACE "${SDL_LIB}" "${SDLMAIN_LIB}")
target_link_libraries(Witchcraft PRIVATE sdl)

# Move sdl2.dll over to where we need it.
if (WIN32)
  if (CMAKE_BUILD_TYPE MATCHES Debug)
	configure_file("${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2d.dll" "$ENV{BUILD_LOCATION}/" COPYONLY)
  else ()
    configure_file("${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2.dll" "$ENV{BUILD_LOCATION}/" COPYONLY)
  endif ()
endif ()

## --- Vulkan --- ##
find_package(Vulkan REQUIRED)
target_include_directories(Witchcraft PRIVATE ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(Witchcraft PRIVATE VULKAN_HPP_NO_CONSTRUCTORS VULKAN_HPP_NO_EXCEPTIONS)
target_link_libraries(Witchcraft PRIVATE Vulkan::Vulkan)

## --- Other --- ##
if (WIN32)
  target_link_libraries(Witchcraft PRIVATE Bcrypt)
endif ()