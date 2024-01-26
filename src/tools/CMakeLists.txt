###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified March 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "tools" subdirectory.
###############################################################################
set (srclist
  "base64.cpp"
  "rng.cpp"
  "uuid.cpp")

set (testsrc
  "base64_test.cpp"
  "sha1_test.cpp"
  "soa_test.cpp"
  "uuid_test.cpp")

target_sources(Witchcraft PRIVATE ${srclist})
target_sources(WcUnitTest PRIVATE ${srclist} ${testsrc})