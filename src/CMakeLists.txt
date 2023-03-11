###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Defines the source files that go into the main executable.
###############################################################################
target_sources(Witchcraft PRIVATE "main.cpp")
add_subdirectory("dbg")
add_subdirectory("filesys")
add_subdirectory("gfx")
add_subdirectory("lua")
add_subdirectory("sys")
add_subdirectory("tools")