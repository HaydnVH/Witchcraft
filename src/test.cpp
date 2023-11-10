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
#include "ecs/idtypes.h"
#include "tools/base64.h"
#include "tools/sha1.hpp"
#include "tools/uuid.h"

#include "etc/result.h"
#include "dbg/unittest.h"

#include <range/v3/functional.hpp>

#include <iostream>

int main(int, char**) {
  int result = 0;
  try {
    cli::init();
    try {
      dbg::info("Now running unit tests for Witchcraft Engine.");
      dbg::infomore(
          std::format("{}\n{}\n{}\n", wc::ecs::WC_ECS_PARENT_ID.toStrCanon(),
                      wc::ecs::ComponentId::WC_ECS_COMPONENT_ID.toStrCanon(),
                      wc::ecs::ArchetypeId::WC_ECS_ARCHETYPE_ID.toStrCanon()));

      result += wc::base64::test::runBase64UnitTests();
      result += wc::hash::test::runSha1UnitTests();
      result += wc::test::runUuidUnitTests();

      EXPECT_TRUE(false);

    } catch (const dbg::Exception& e) {
      dbg::fatal(e);
      result = -1;
    }
    cli::shutdown();
  } catch (const std::exception& e) {
    std::cerr << "\n" << e.what() << std::endl;
    result = -2;
  }
  return result;
}