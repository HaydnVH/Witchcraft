#include "dbg/debug.h"
#include "uuid.h"

int wc::test::runUuidUnitTests() {
  int numFails = 0;
  dbg::info("Running Uuid unit tests.");

  for (int i = 0; i < 50; ++i) {
    auto id  = wc::Uuid::makeV4();
    auto b64 = id.toStrBase64();
    if (id != wc::Uuid::fromStrBase64(b64)) {
      dbg::errmore("id doesn't match!");
      ++numFails;
    }
  }

  if (numFails == 0)
    dbg::infomore("All clear!");
  return numFails;
}