#include "dbg/debug.h"
#include "dbg/unittest.h"
#include "uuid.h"

int wc::test::runUuidUnitTests() {
  int FAIL_COUNTER {0};

  for (int i = 0; i < 50; ++i) {
    auto id  = wc::Uuid::makeV4();
    auto b64 = id.toStrBase64();
    EXPECT_EQUAL(id, wc::Uuid::fromStrBase64(b64));
  }

  return FAIL_COUNTER;
}