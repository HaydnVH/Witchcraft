/******************************************************************************
 * test.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified March 2023
 * ---------------------------------------------------------------------------
 * Performs unit tests for components of the Witchcraft engine.
 *****************************************************************************/

#include "dbg/cli.h"
#include "dbg/debug.h"
//#include "ecs/idtypes.h"
#include "tools/base64.h"
#include "tools/sha1.hpp"
#include "tools/uuid.h"
#include "tools/soa.hpp"
#include "etc/soa.h"
#include "etc/table.h"

#include "etc/result.h"
#include "dbg/unittest.h"

#include <iostream>

int main(int, char**) {
  int failcount {0};
  try {
    
    std::cout << "\x1b[92mRunning unit tests for Witchcraft Engine.\x1b[0m" << std::endl;

    failcount += hvh::runSoaUnitTest();
    failcount += wc::base64::test::runBase64UnitTests();
    failcount += wc::hash::test::runSha1UnitTests();
    failcount += wc::test::runUuidUnitTests();
    failcount += wc::etc::test::SoaUnitTest();
    failcount += wc::etc::test::TableUnitTest();

    if (failcount == 0) {
      std::cout << "\x1b[92mAll tests passed successfully.\x1b[0m" << std::endl;
    } else {
      std::cout << "\x1b[91mSome tests failed.  Total failures: " << failcount << "\x1b[0m" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "\x1b[91mUnexpected exception: " << e.what() << "\x1b[0m" << std::endl;
    failcount = -1;
  }
  return failcount;
}