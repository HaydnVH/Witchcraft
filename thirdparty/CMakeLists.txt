###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2023 - present
# Last modified November 2023
# ----------------------------------------------------------------------------
# Manages and links third party dependencies.
###############################################################################

## -- FileToHeader -- ##
add_subdirectory(FileToHeader)
add_dependencies(Witchcraft FileToHeader)
add_dependencies(Witchcraft FileToHeader)

## -- lua -- ##
add_subdirectory(Lua)
target_link_libraries(Witchcraft PRIVATE lua_static)
target_link_libraries(WcUnitTest PRIVATE lua_static)
target_include_directories(Witchcraft PRIVATE "Lua/lua-5.4.6/include")
target_include_directories(WcUnitTest PRIVATE "Lua/lua-5.4.6/include")
add_dependencies(Witchcraft luac)
add_dependencies(WcUnitTest luac)

## -- lz4 -- ##
add_library(lz4 STATIC)
target_include_directories(lz4 PUBLIC "lz4/lib")
target_sources(lz4 PRIVATE "lz4/lib/lz4.c" "lz4/lib/lz4hc.c")
target_link_libraries(Witchcraft PRIVATE lz4)
target_link_libraries(WcUnitTest PRIVATE lz4)

## --- range v3 --- ##
add_subdirectory(range-v3)
target_include_directories(Witchcraft PRIVATE "range-v3/include")
target_include_directories(WcUnitTest PRIVATE "range-v3/include")

## -- rapidjson --- ##
add_subdirectory(rapidjson)
target_link_libraries(Witchcraft PRIVATE RapidJSON)
target_link_libraries(WcUnitTest PRIVATE RapidJSON)
target_include_directories(Witchcraft PRIVATE "rapidjson/include")
target_include_directories(WcUnitTest PRIVATE "rapidjson/include")

## --- scnlib --- ##
add_subdirectory(scnlib)
target_link_libraries(Witchcraft PRIVATE scn)
target_link_libraries(WcUnitTest PRIVATE scn)
target_include_directories(Witchcraft PRIVATE "scnlib/include")
target_include_directories(WcUnitTest PRIVATE "scnlib/include")