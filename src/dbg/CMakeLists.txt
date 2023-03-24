###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "dbg" subdirectory.
###############################################################################
set (srclist
  "cli.cpp"
  "debug.cpp")

target_sources(Witchcraft PRIVATE ${srclist})
target_sources(WcUnitTest PRIVATE ${srclist})