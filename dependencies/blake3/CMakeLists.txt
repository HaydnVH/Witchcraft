###############################################################################
# CMakeLists.txt
# Part of the Witchcraft engine by Haydn V. Harach
# https://github.com/HaydnVH/Witchcraft
# (C) Haydn V. Harach 2022 - present
# Last modified February 2023
# ----------------------------------------------------------------------------
# Manages and builds blake3 source files.
###############################################################################
add_library(blake3 STATIC)
target_include_directories(blake3 PUBLIC "include")
target_sources(blake3 PRIVATE
  "src/blake3.c"
  "src/blake3_avx2.c"
  "src/blake3_avx512.c"
  "src/blake3_dispatch.c"
  "src/blake3_portable.c"
  "src/blake3_sse2.c"
  "src/blake3_sse41.c")


