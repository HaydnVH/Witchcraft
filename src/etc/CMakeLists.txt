###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified March 2023
# ----------------------------------------------------------------------------
# Defines the source files for the "etc" subdirectory.
###############################################################################
set (srclist
  "")

set (testsrc
  "soa_utest.cpp"
  "table_utest.cpp")

target_sources(Witchcraft PRIVATE ${srclist})
target_sources(WcUnitTest PRIVATE ${srclist} ${testsrc})