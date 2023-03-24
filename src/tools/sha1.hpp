/******************************************************************************
 * sha1.hpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * An implementation of the SHA1 hash algorithm.
 * WARNING! SHA1 is cryptographically broken!
 * This algorithm must NEVER be used for any purpose where security is
 * important! The main use case for this algorithm in the Witchcraft engine is
 * to generate V5 UUIDs, often using constexpr strings (class names etc).
 *
 * This file is based on public domain code from https://github.com/vog/sha1.
 * Adjustments have been made so the algorithm can be run as constexpr.
 * Requires C++20 for constexpr containers and concepts.
 *****************************************************************************/

/*
    sha1.hpp - source code of

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
    Safety fixes
        -- Eugene Hopkinson <slowriot at voxelstorm dot com>
    Header-only library
        -- Zlatko Michailov <zlatko@michailov.org>
*/

#ifndef WC_TOOLS_SHA1_HPP
#define WC_TOOLS_SHA1_HPP

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace wc::hash {

  // StringViewable is a concept for types that can be converted to a
  // std::string_view.
  template <typename T>
  concept StringViewable = requires(T x) { std::string_view(x); };

  // CharIterator is a concept for iterator types that can be dereferenced for a
  // char.
  template <typename T>
  concept CharIterator = requires(T x) {
                           { *x } -> std::convertible_to<char>;
                         };

  class Sha1 {
  public:
    using Digest = std::array<uint32_t, 5>;

    constexpr Sha1() {
      buffer.reserve(BLOCK_BYTES + 1);
      reset();
    }
    /// Feed the SHA1 state data from a stream.
    /// This could be a file, stringstream, etc.
    /*
    inline void update(std::istream& is) {
      while (true) {
        std::array<char, BLOCK_BYTES> sbuf;
        is.read(sbuf.data(), BLOCK_BYTES - buffer.size());
        buffer.insert(std::end(buffer), std::begin(sbuf),
                      std::begin(sbuf) + is.gcount());
        if (buffer.size() != BLOCK_BYTES) {
          return;
        }
        Block block;
        buffer_to_block(buffer, block);
        transform(block);
        buffer.clear();
      }
    }
    */

    /// Feed the SHA1 state using a range of characters.
    /// @param begin: An iterator to the beginning of the range.
    /// @param end: An iterator to the end of the range.
    template <typename It>
      requires(CharIterator<It>)
    inline constexpr void update(It begin, It end) {

      while (true) {
        size_t numToCopy = BLOCK_BYTES - buffer.size();
        size_t dataSize  = (end - begin);
        if (numToCopy < dataSize) {
          buffer.insert(buffer.end(), begin, begin + numToCopy);
          begin += numToCopy;
        } else {
          buffer.insert(buffer.end(), begin, end);
          begin = end;
        }

        if (buffer.size() != BLOCK_BYTES)
          return;
        Block block;
        buffer_to_block(buffer, block);
        transform(block);
        buffer.clear();
      }
    }

    /// Feed the SHA1 a string.
    inline constexpr void update(std::string_view str) {
      update(str.begin(), str.end());
    }

    /// Feed the SHA1 an arbitrary non-string object.
    template <typename T>
      requires(!StringViewable<T>)
    inline constexpr void update(const T& item) {
      auto data = std::bit_cast<std::array<char, sizeof(T)>, T>(item);
      update(data.begin(), data.end());
    }

    /// Feed the SHA1 state a range of objects.
    template <typename It>
      requires(!CharIterator<It>)
    inline constexpr void update(It begin, It end) {
      // Go through each entry in the container,
      for (auto here = begin; here != end; ++begin) {
        update(*here);
      }
    }

    /// Add padding and return the message digest.
    inline constexpr Digest final() {
      // Total number of hashed bits.
      uint64_t total_bits = ((transforms * BLOCK_BYTES) + buffer.size()) * 8;

      // Padding
      buffer.push_back((char)0x80);
      size_t orig_size = buffer.size();
      while (buffer.size() < BLOCK_BYTES) {
        buffer.push_back((char)0x00);
      }

      Block block;
      buffer_to_block(buffer, block);

      if (orig_size > BLOCK_BYTES - 8) {
        transform(block);
        for (size_t i = 0; i < BLOCK_INTS - 2; i++) {
          block[i] = 0;
        }
      }

      // Append total_bits, split this uint64_t into two uint32_t.
      block[BLOCK_INTS - 1] = (uint32_t)total_bits;
      block[BLOCK_INTS - 2] = (uint32_t)(total_bits >> 32);
      transform(block);

      // Reset for the next run and return our result.
      Digest result = digest;
      reset();
      return result;
    }
    /*
    inline static Digest from_file(std::ifstream& stream) {
      Sha1 checksum;
      checksum.update(stream);
      return checksum.final();
    }
    */

  private:
    // Number of 32-bit integers per SHA1 block
    static constexpr size_t BLOCK_INTS = 16;
    // Number of bytes per SHA1 block.
    static constexpr size_t BLOCK_BYTES = BLOCK_INTS * sizeof(uint32_t);

    using Block = std::array<uint32_t, BLOCK_INTS>;

    Digest            digest;
    std::vector<char> buffer;
    uint64_t          transforms;

    inline constexpr void reset() {
      // SHA1 initialization constants.
      digest[0] = 0x67452301;
      digest[1] = 0xefcdab89;
      digest[2] = 0x98badcfe;
      digest[3] = 0x10325476;
      digest[4] = 0xc3d2e1f0;

      // Reset counters.
      buffer.clear();
      transforms = 0;
    }

    inline static constexpr uint32_t
        rol(const uint32_t value, const size_t bits) {
      return (value << bits) | (value >> (32 - bits));
    }

    inline static constexpr uint32_t blk(Block& block, const size_t i) {
      return rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^
                     block[(i + 2) & 15] ^ block[i],
                 1);
    }

    // (R0+R1), R2, R3, R4 are the different operations used in SHA1

    inline static constexpr void
        R0(Block& block, const uint32_t v, uint32_t& w, const uint32_t x,
           const uint32_t y, uint32_t& z, const size_t i) {
      z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
      w = rol(w, 30);
    }

    inline static constexpr void
        R1(Block& block, const uint32_t v, uint32_t& w, const uint32_t x,
           const uint32_t y, uint32_t& z, const size_t i) {
      block[i] = blk(block, i);
      z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
      w = rol(w, 30);
    }

    inline static constexpr void
        R2(Block& block, const uint32_t v, uint32_t& w, const uint32_t x,
           const uint32_t y, uint32_t& z, const size_t i) {
      block[i] = blk(block, i);
      z += (w ^ x ^ y) + block[i] + 0x6ed9eba1 + rol(v, 5);
      w = rol(w, 30);
    }

    inline static constexpr void
        R3(Block& block, const uint32_t v, uint32_t& w, const uint32_t x,
           const uint32_t y, uint32_t& z, const size_t i) {
      block[i] = blk(block, i);
      z += (((w | x) & y) | (w & x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
      w = rol(w, 30);
    }

    inline static constexpr void
        R4(Block& block, const uint32_t v, uint32_t& w, const uint32_t x,
           const uint32_t y, uint32_t& z, const size_t i) {
      block[i] = blk(block, i);
      z += (w ^ x ^ y) + block[i] + 0xca62c1d6 + rol(v, 5);
      w = rol(w, 30);
    }

    /// Hash a single 512-bit block. This is the core of the algorithm.
    inline constexpr void transform(Block& block) {
      // Copy digest[] to working vars.
      uint32_t a = digest[0];
      uint32_t b = digest[1];
      uint32_t c = digest[2];
      uint32_t d = digest[3];
      uint32_t e = digest[4];

      // 4 rounds of 20 operations each. Loop unrolled.
      R0(block, a, b, c, d, e, 0);
      R0(block, e, a, b, c, d, 1);
      R0(block, d, e, a, b, c, 2);
      R0(block, c, d, e, a, b, 3);
      R0(block, b, c, d, e, a, 4);
      R0(block, a, b, c, d, e, 5);
      R0(block, e, a, b, c, d, 6);
      R0(block, d, e, a, b, c, 7);
      R0(block, c, d, e, a, b, 8);
      R0(block, b, c, d, e, a, 9);
      R0(block, a, b, c, d, e, 10);
      R0(block, e, a, b, c, d, 11);
      R0(block, d, e, a, b, c, 12);
      R0(block, c, d, e, a, b, 13);
      R0(block, b, c, d, e, a, 14);
      R0(block, a, b, c, d, e, 15);
      R1(block, e, a, b, c, d, 0);
      R1(block, d, e, a, b, c, 1);
      R1(block, c, d, e, a, b, 2);
      R1(block, b, c, d, e, a, 3);
      R2(block, a, b, c, d, e, 4);
      R2(block, e, a, b, c, d, 5);
      R2(block, d, e, a, b, c, 6);
      R2(block, c, d, e, a, b, 7);
      R2(block, b, c, d, e, a, 8);
      R2(block, a, b, c, d, e, 9);
      R2(block, e, a, b, c, d, 10);
      R2(block, d, e, a, b, c, 11);
      R2(block, c, d, e, a, b, 12);
      R2(block, b, c, d, e, a, 13);
      R2(block, a, b, c, d, e, 14);
      R2(block, e, a, b, c, d, 15);
      R2(block, d, e, a, b, c, 0);
      R2(block, c, d, e, a, b, 1);
      R2(block, b, c, d, e, a, 2);
      R2(block, a, b, c, d, e, 3);
      R2(block, e, a, b, c, d, 4);
      R2(block, d, e, a, b, c, 5);
      R2(block, c, d, e, a, b, 6);
      R2(block, b, c, d, e, a, 7);
      R3(block, a, b, c, d, e, 8);
      R3(block, e, a, b, c, d, 9);
      R3(block, d, e, a, b, c, 10);
      R3(block, c, d, e, a, b, 11);
      R3(block, b, c, d, e, a, 12);
      R3(block, a, b, c, d, e, 13);
      R3(block, e, a, b, c, d, 14);
      R3(block, d, e, a, b, c, 15);
      R3(block, c, d, e, a, b, 0);
      R3(block, b, c, d, e, a, 1);
      R3(block, a, b, c, d, e, 2);
      R3(block, e, a, b, c, d, 3);
      R3(block, d, e, a, b, c, 4);
      R3(block, c, d, e, a, b, 5);
      R3(block, b, c, d, e, a, 6);
      R3(block, a, b, c, d, e, 7);
      R3(block, e, a, b, c, d, 8);
      R3(block, d, e, a, b, c, 9);
      R3(block, c, d, e, a, b, 10);
      R3(block, b, c, d, e, a, 11);
      R4(block, a, b, c, d, e, 12);
      R4(block, e, a, b, c, d, 13);
      R4(block, d, e, a, b, c, 14);
      R4(block, c, d, e, a, b, 15);
      R4(block, b, c, d, e, a, 0);
      R4(block, a, b, c, d, e, 1);
      R4(block, e, a, b, c, d, 2);
      R4(block, d, e, a, b, c, 3);
      R4(block, c, d, e, a, b, 4);
      R4(block, b, c, d, e, a, 5);
      R4(block, a, b, c, d, e, 6);
      R4(block, e, a, b, c, d, 7);
      R4(block, d, e, a, b, c, 8);
      R4(block, c, d, e, a, b, 9);
      R4(block, b, c, d, e, a, 10);
      R4(block, a, b, c, d, e, 11);
      R4(block, e, a, b, c, d, 12);
      R4(block, d, e, a, b, c, 13);
      R4(block, c, d, e, a, b, 14);
      R4(block, b, c, d, e, a, 15);

      // Add the working vars back into digest[].
      digest[0] += a;
      digest[1] += b;
      digest[2] += c;
      digest[3] += d;
      digest[4] += e;

      // Count the number of transformations.
      transforms++;
    }

    inline static constexpr void
        buffer_to_block(const std::vector<char>& buffer, Block& block) {
      // Convert the byte buffer to a uint32_t array (MSB).
      for (size_t i = 0; i < BLOCK_INTS; i++) {
        block[i] =
            (buffer[4 * i + 3] & 0xff) | (buffer[4 * i + 2] & 0xff) << 8 |
            (buffer[4 * i + 1] & 0xff) << 16 | (buffer[4 * i + 0] & 0xff) << 24;
      }
    }
  };

}  // namespace wc::hash

namespace wc::hash::test {
  int runSha1UnitTests();
}  // namespace wc::hash::test

#endif  // WC_TOOLS_SHA1_HPP
