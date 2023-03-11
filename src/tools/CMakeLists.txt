###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified March 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "tools" subdirectory.
###############################################################################
target_sources(Witchcraft PRIVATE
  "base64.cpp"
  "rng.cpp"
  "uuid.cpp")