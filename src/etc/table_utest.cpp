#include "table.h"
#include "dbg/unittest.h"

#include <cstdio>
#include <string>

int wc::etc::test::TableUnitTest() {
  BEGIN_UNIT_TEST;

  MultiTable<std::string, int> tbl;

  tbl.insert("apple", 61);
  tbl.insert("banana", 12);
  tbl.insert("carrot", 33);
  tbl.insert("donut", 94);
  tbl.insert("eggplant", 55);
  tbl.insert("flowers", 36);
  tbl.insert("ginger", 17);
  tbl.insert("hashbrowns", 28);
  tbl.insert("ice cream", 99);
  tbl.insert("jello", 10);
  tbl.insert("kale", 711);
  tbl.insert("lemon", 112);
  tbl.insert("melon", 313);
  tbl.insert("nougat", 614);
  tbl.insert("onion", 615);
  tbl.insert("parfait", 716);
  tbl.insert("quiche", 217);
  tbl.insert("rice", 318);
  tbl.insert("steak", 919);
  tbl.insert("taco", 220);
  tbl.insert("udon", 21);
  tbl.insert("vinegar", 222);
  tbl.insert("water", 323);
  tbl.insert("xoi", 824);
  tbl.insert("yogurt", 725);
  tbl.insert("zucchini", 626);

  tbl.insert("banana", 42);
  tbl.insert("banana", 9001);
  
  EXPECT_EQUAL(tbl.count("banana"), 3);

  auto found = tbl.find("banana");
  EXPECT_TRUE(found);
  EXPECT_EQUAL(found.get<1>(), 12);
  auto [k, v] = *found;
  EXPECT_EQUAL(k, "banana");
  EXPECT_EQUAL(v, 12);

  int bananas[] = {12, 42, 9001};
  int i {0};
  for (auto [k, v] : tbl.findEach("banana")) {
    EXPECT_EQUAL(v, bananas[i++]);
  }

  EXPECT_EQUAL(tbl.find("jello").get<1>(), 10);
  EXPECT_EQUAL(tbl.find("donut").get<1>(), 94);
  EXPECT_EQUAL(tbl.find("parfait").get<1>(), 716);

  END_UNIT_TEST;
}