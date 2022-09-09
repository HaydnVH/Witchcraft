/* crossplatform.h
 * by Haydn V. Harach
 * October 2019
 *
 * This file contains a number of useful macro definitions which are designed
 * to function identically regardless of compiler/platform.
 */
#ifndef HVH_TOOLKIT_CROSSPLATFORM_H
#define HVH_TOOLKIT_CROSSPLATFORM_H

/******************************************************************************
 * Aligned Malloc/Free
 *****************************************************************************/
#include <cstdlib>

// We need access to aligned allocations and deallocations,
// but visual studio doesn't support aligned_alloc from the c11 standard.
// This define lets us have consistent behaviour.
#ifndef aligned_malloc
  #ifdef _MSC_VER
    #define aligned_malloc(alignment,size) _aligned_malloc(size,alignment)
  #else
    #define aligned_malloc(alignment,size) aligned_alloc(alignment,size)
  #endif
#endif

#ifndef aligned_free
  #ifdef _MSC_VER
    #define aligned_free(mem) _aligned_free(mem)
  #else
    #define aligned_free(mem) free(mem)
  #endif
#endif

/******************************************************************************
 * fopen with unicode filenames.
 *****************************************************************************/

// Handle 64-bit file offsets in a platform-agnostic manner.
#ifdef _WIN32
 #define fseek64(file,offset,origin) _fseeki64(file,offset,origin)
 #define fopen_w(filename) _wfopen(filename, L"wb");
 #define fopen_rw(filename) _wfopen(filename, L"r+b");
 #define fopen_r(filename) _wfopen(filename, L"rb");
#else
 #define fseek64(file,offset,origin) fseek(file,offset,origin)
 #define fopen_w(filename) fopen(filename,"wb");
 #define fopen_rw(filename) fopen(filename,"r+b");
 #define fopen_r(filename) fopen(filename,"rb");
#endif

#endif // HVH_TOOLKIT_CROSSPLATFORM_H