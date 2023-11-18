###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2023 - present
# Last modified November 2023
# ----------------------------------------------------------------------------
# Manages and links third party dependencies.
###############################################################################

set(INCLUDE_DIRS "")
set(LINK_DIRS "")
set(LINK_LIBS "")
set(DEPENDENCIES "")
set(DEFINES "")

## -- FileToHeader -- ##
add_subdirectory(FileToHeader)
list(APPEND DEPENDENCIES "FileToHeader")

## -- lua -- ##
add_subdirectory(Lua)
list(APPEND LINK_LIBS "lua_static")
list(APPEND INCLUDE_DIRS "Lua/lua-5.4.6/include")
list(APPEND DEPENDENCIES "luac")

## -- lz4 -- ##
add_library(lz4 STATIC)
target_include_directories(lz4 PUBLIC "lz4/lib")
target_sources(lz4 PRIVATE "lz4/lib/lz4.c" "lz4/lib/lz4hc.c")
list(APPEND LINK_LIBS "lz4")

## --- range v3 --- ##
add_subdirectory(range-v3)
list(APPEND INCLUDE_DIRS "range-v3/include")

## -- rapidjson --- ##
add_subdirectory(rapidjson)
list(APPEND LINK_LIBS "RapidJSON")
list(APPEND INCLUDE_DIRS "rapidjson/include")

## --- scnlib --- ##
add_subdirectory(scnlib)
list(APPEND LINK_LIBS "scn")
list(APPEND INCLUDE_DIRS "scnlib/include")

## --- SDL --- ##
add_subdirectory(SDL)
list(APPEND LINK_LIBS "SDL2" "SDL2main")
list(APPEND INCLUDE_DIRS "SDL/include")

# Move sdl2.dll over to where we need it.
#if (WIN32)
#  if (CMAKE_BUILD_TYPE MATCHES Debug)
#	configure_file("${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2d.dll" "$ENV{BUILD_LOCATION}/" COPYONLY)
#  else ()
#    configure_file("${PROJECT_SOURCE_DIR}/dependencies/sdl/lib/$ENV{PRESET_NAME}/SDL2.dll" "$ENV{BUILD_LOCATION}/" COPYONLY)
#  endif ()
#endif ()


## --- Vulkan --- ##
find_package(Vulkan REQUIRED)
list(APPEND LINK_LIBS "Vulkan::Vulkan")
list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
List(APPEND DEFINES "VULKAN_HPP_NO_CONSTRUCTORS" "VULKAN_HPP_NO_EXCEPTIONS")

## --- Other --- ##
if (WIN32)
  list(APPEND LINK_LIBS "Bcrypt")
endif ()

target_link_directories(Witchcraft PRIVATE ${LINK_DIRS})
target_link_libraries(Witchcraft PRIVATE ${LINK_LIBS})
target_include_directories(Witchcraft PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(Witchcraft PRIVATE ${DEFINES})
add_dependencies(Witchcraft ${DEPENDENCIES})

target_link_directories(WcUnitTest PRIVATE ${LINK_DIRS})
target_link_libraries(WcUnitTest PRIVATE ${LINK_LIBS})
target_include_directories(WcUnitTest PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(WcUnitTest PRIVATE ${DEFINES})
add_dependencies(WcUnitTest ${DEPENDENCIES})