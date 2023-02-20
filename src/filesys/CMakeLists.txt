###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "filesys" subdirectory.
###############################################################################
target_sources(Witchcraft PRIVATE
  "archive.cpp"
  "module.cpp"
  "vfs.cpp")