/******************************************************************************
 * sha1_test.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * Unit tests for Witchcraft's implementation of SHA1 hashing.
 *
 * This file is based on public domain code from https://github.com/vog/sha1.
 * Adjustments have been made so the algorithm can be run as constexpr.
 *****************************************************************************/

/*
    test_sha1.cpp - test program of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Diels-Grabsch <v@njh.eu>
    Header-only library
        -- Zlatko Michailov <zlatko@michailov.org>
*/

#include "dbg/debug.h"
#include "sha1.hpp"
#include "sha1.hpp"  // Intentionally included twice for testing purposes

/*
 * This method is defined in test_sha1_file.cpp.
 * The purpose of the split is to test linking of multiple files that include
 * sha1.hpp.
 */
// void test_file(const string& filename);

int compare(std::string_view result, std::string_view expected) {
  if (result != expected) {
    dbg::errmore(
        std::format("Result:   {}\nExpected: {} (FAILURE)", result, expected));
    return 1;
  }
  return 0;
}

int compare(wc::hash::Sha1::Digest result, std::string_view expected) {
  auto resultstr = std::format("{:08x}{:08x}{:08x}{:08x}{:08x}", result[0],
                               result[1], result[2], result[3], result[4]);
  return compare(resultstr, expected);
}

// The 3 test vectors from FIPS PUB 180-1
int runSha1StandardUnitTests() {
  // https://www.di-mgt.com.au/sha_testvectors.html
  // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA1.pdf

  int            numFails = 0;
  wc::hash::Sha1 checksum;

  checksum.update("abc");
  numFails +=
      compare(checksum.final(), "a9993e364706816aba3e25717850c26c9cd0d89d");

  checksum.update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  numFails +=
      compare(checksum.final(), "84983e441c3bd26ebaae4aa1f95129e5e54670f1");

  checksum.update(
      "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  numFails +=
      compare(checksum.final(), "a49b2446a02c645bf419f995b67091253a04a259");

  for (int i = 0; i < 1000000 / 200; ++i) {
    checksum.update(
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  }
  numFails +=
      compare(checksum.final(), "34aa973cd4c4daa4f61eeb2bdbad27316534016f");

  // https://en.wikipedia.org/wiki/SHA-1
  checksum.update("The quick brown fox jumps over the lazy dog");
  numFails +=
      compare(checksum.final(), "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");

  checksum.update("The quick brown fox jumps over the lazy cog");
  numFails +=
      compare(checksum.final(), "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3");

  return numFails;
}

int runSha1SlowUnitTests() {
  // https://www.di-mgt.com.au/sha_testvectors.html

  int            numFails = 0;
  wc::hash::Sha1 checksum;

  for (int i = 0; i < 16777216; ++i) {
    checksum.update(
        "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno");
  }
  numFails +=
      compare(checksum.final(), "7789f0c9ef7bfc40d93311143dfbe69e2017f592");
  return numFails;
}

// Other tests
int runSha1OtherUnitTests() {
  int            numFails = 0;
  wc::hash::Sha1 checksum;

  numFails +=
      compare(checksum.final(), "da39a3ee5e6b4b0d3255bfef95601890afd80709");

  checksum.update("");
  numFails +=
      compare(checksum.final(), "da39a3ee5e6b4b0d3255bfef95601890afd80709");

  checksum.update("abcde");
  numFails +=
      compare(checksum.final(), "03de6c570bfe24bfc328ccd7ca46b76eadaf4334");

  wc::hash::Sha1 checksum1, checksum2;
  checksum1.update("abc");
  numFails += compare(checksum2.final(),
                      "da39a3ee5e6b4b0d3255bfef95601890afd80709"); /* ""
                                                                    */
  numFails += compare(checksum1.final(),
                      "a9993e364706816aba3e25717850c26c9cd0d89d"); /* "abc"
                                                                    */

  checksum.update(
      std::string("a"
                  "\x00"
                  "b"
                  "\x7f"
                  "c"
                  "\x80"
                  "d"
                  "\xff"
                  "e"
                  "\xc3\xf0"
                  "f",
                  12));
  numFails +=
      compare(checksum.final(), "cd0dd10814c0d4f9c6a2a0a4be2304d2371468d3");

  return numFails;
}

int wc::hash::test::runSha1UnitTests() {
  int numFails = 0;
  dbg::info("Running SHA1 unit tests.");
  numFails += runSha1StandardUnitTests();
  numFails += runSha1OtherUnitTests();
  //  numFails += runSha1SlowUnitTests();
  if (numFails == 0)
    dbg::infomore("All clear!");
  return numFails;
}