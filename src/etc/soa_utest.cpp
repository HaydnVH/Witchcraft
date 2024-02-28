#include "soa.h"

#include <string>
using namespace std;

#include "dbg/unittest.h"

static const int    testdata0[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
                                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
static const string testdata1[] = {
    "zero",     "one",      "two",      "three",   "four",    "five",
    "six",      "seven",    "eight",    "nine",    "ten",     "eleven",
    "twelve",   "thirteen", "fourteen", "fifteen", "sixteen", "seventeen",
    "eighteen", "nineteen", "twenty"};

int wc::etc::test::SoaUnitTest() {
  BEGIN_UNIT_TEST;

  using TestSoa = Soa<int, string, short, double>;
  TestSoa soa;
  int* const&                          ints    = soa.data<0>();
  string* const&                       strings = soa.data<1>();
  short* const&                        shorts  = soa.data<2>();
  double* const&                       doubles = soa.data<3>();

  static_assert(TestSoa::columnSize<0>() == sizeof(int));
  static_assert(TestSoa::columnSize<1>() == sizeof(string));
  static_assert(TestSoa::columnSize<2>() == sizeof(short));
  static_assert(TestSoa::columnSize<3>() == sizeof(double));
  static_assert((TestSoa::columnSize<0>() + TestSoa::columnSize<1>() + TestSoa::columnSize<2>() + TestSoa::columnSize<3>()) == TestSoa::rowSize());

  TestSoa::NthType<0> testInt {42};
  static_assert(is_same_v<decltype(testInt), int>);

  EXPECT_EQUAL(soa.capacity(), 0);
  EXPECT_EQUAL(soa.size(), 0);
  EXPECT_NULL(ints);
  EXPECT_NULL(strings);
  EXPECT_NULL(shorts);
  EXPECT_NULL(doubles);
  soa.pushBack(0, "zero", 0, 0.0);
  EXPECT_EQUAL(soa.size(), 1);
  EXPECT_EQUAL(soa.capacity(), 8);

  for (int i {1}; i < 16; ++i) {
    soa.pushBack(i, testdata1[i], (short)-i, (double)i);
  }

  EXPECT_EQUAL(soa.size(), 16);
  EXPECT_EQUAL(soa.capacity(), 16);

  {
  size_t intsRawPtr    {(size_t)ints};
  size_t stringsRawPtr {(size_t)strings};
  size_t shortsRawPtr  {(size_t)shorts};
  size_t doublesRawPtr {(size_t)doubles};

  EXPECT_EQUAL(stringsRawPtr, intsRawPtr + (sizeof(int) * soa.capacity()));
  EXPECT_EQUAL(shortsRawPtr, stringsRawPtr + (sizeof(string) * soa.capacity()));
  EXPECT_EQUAL(doublesRawPtr, shortsRawPtr + (sizeof(short) * soa.capacity()));
  }

  for (int i{16}; i < 21; ++i) {
    soa.pushBack(i, testdata1[i], (short)-i, (double)i);
  }

  EXPECT_EQUAL(soa.size(), 21);
  EXPECT_EQUAL(soa.capacity(), 32);

  for (int i = 0; i < 21; ++i) {
    // Pointer array access should be the same as `at`.
    EXPECT_EQUAL(ints[i], soa.at<0>(i));
    EXPECT_EQUAL(strings[i], soa.at<1>(i));
    EXPECT_EQUAL(shorts[i], soa.at<2>(i));
    EXPECT_FEQUAL(doubles[i], soa.at<3>(i));

    // Stored data should match expected data.
    EXPECT_EQUAL(ints[i], i);
    EXPECT_EQUAL(strings[i], testdata1[i]);
    EXPECT_EQUAL(shorts[i], (short)-i);
    EXPECT_FEQUAL(doubles[i], (double)i);
  }

  // Testing viewColumn<K>
  {
    int i {0};
    for (auto& it : soa.viewColumn<0>()) {
      EXPECT_EQUAL(it, ints[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<1>()) {
      EXPECT_EQUAL(it, strings[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<2>()) {
      EXPECT_EQUAL(it, shorts[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<3>()) {
      EXPECT_EQUAL(it, doubles[i++]);
    }
  }

  // Testing viewColumn<T>
  {
    int i {0};
    for (auto& it : soa.viewColumn<int>()) {
      EXPECT_EQUAL(it, ints[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<string>()) {
      EXPECT_EQUAL(it, strings[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<short>()) {
      EXPECT_EQUAL(it, shorts[i++]);
    }

    i = 0;
    for (auto& it : soa.viewColumn<double>()) {
      EXPECT_EQUAL(it, doubles[i++]);
    }
  }

  // Testing viewColumns
  {
    int i {0};
    for (auto [a, b, c, d] : soa.viewColumns<1, 2, 0, 2>()) {
      EXPECT_EQUAL(a, strings[i]);
      EXPECT_EQUAL(b, shorts[i]);
      EXPECT_EQUAL(c, ints[i]);
      EXPECT_EQUAL(d, shorts[i]);
      EXPECT_EQUAL(b, d);
      ++i;
    }

    i = 0;
    for (auto [a, b, c, d] : soa) {
      EXPECT_EQUAL(a, i);
      EXPECT_EQUAL(b, testdata1[i]);
      EXPECT_EQUAL(c, (short)-i);
      EXPECT_EQUAL(d, (double)i);
      ++i;
    }
  }

  soa.reserve(1010);
  EXPECT_EQUAL(soa.capacity(), 1016);
  EXPECT_EQUAL(soa.size(), 21);

  size_t index = soa.lowerBound<0>(10);
  if (index >= soa.size() || soa.at<0>(index) != 10) {
    CUSTOM_FAIL("lowerBound() failed to find '10'.");
  }

  soa.insert(index, 10, "10", 1010, 1010.0);
  soa.insert(index, 10, "TEN", -1010, -1010.0);
  soa.insert(index, 10, "TEEEEEEEN", 11010, 101010.0);

  size_t begin = soa.lowerBound<0>(10);
  size_t end   = soa.upperBound<0>(10);
  if ((end - begin) != 4) {
    CUSTOM_FAIL("There should be 4 entries with key '10' in the list. begin=" << begin << ", end=" << end);
  }
  EXPECT_EQUAL(soa.at<1>(begin++), "TEEEEEEEN");
  EXPECT_EQUAL(soa.at<1>(begin++), "TEN");
  EXPECT_EQUAL(soa.at<1>(begin++), "10");
  EXPECT_EQUAL(soa.at<1>(begin), "ten");

  EXPECT_EQUAL(soa.lowerBound<0>(42), soa.size());
  EXPECT_EQUAL(soa.front<1>(), "zero");
  EXPECT_EQUAL(soa.back<1>(), "twenty");

  // soa.emplace_back(21, "twenty one", std::ignore, 21.0);
  // soa.emplace_back(22, std::make_tuple( 2, '2' ), -22, 22.0);

  // if (soa.back<1>() != "22") {
  //	printf("Emplace_back failed to put a tuple in spot 22.");
  //	++failures;
  // }

  index = soa.lowerBound<0>(10);
  soa.eraseShift(index);
  soa.eraseShift(index);
  soa.eraseShift(index);

  soa.popBack();
  soa.popBack();

  for (int i = 0; i < soa.size(); ++i) {
    // Stored data should match expected data.
    EXPECT_EQUAL(ints[i], i);
    EXPECT_EQUAL(strings[i], testdata1[i]);
    EXPECT_EQUAL(shorts[i], (short)-i);
    EXPECT_FEQUAL(doubles[i], (double)i);
  }

  EXPECT_EXCEPTION(soa.at<0>(30) = 30, std::out_of_range);

  // Check constructing a new Soa using the viewColumns of another.
  { Soa<int, string> spliced(soa.viewColumns<0,1>());
    EXPECT_EQUAL(soa.size(), spliced.size());
    for (int i {0}; i < soa.size(); ++i) {
      EXPECT_EQUAL(spliced.at<0>(i), soa.at<0>(i));
      EXPECT_EQUAL(spliced.at<1>(i), soa.at<1>(i));
    }
  }

  // Check constructing a new Soa using the viewColumn of another.
  { Soa<short> spliced(soa.viewColumn<2>());
    EXPECT_EQUAL(soa.size(), spliced.size());
    for (int i {0}; i < soa.size(); ++i) {
      EXPECT_EQUAL(spliced.at<0>(i), soa.at<2>(i));
    }
  }

  // Check serialization and deserialization.
  { Soa<int, short, double> before(soa.viewColumns<0, 2, 3>());
    auto [numEntries, numBytes, data] = before.serialize();
    Soa<int, short, double> after;
    after.deserialize(numEntries, numBytes, data);
    EXPECT_EQUAL(before.size(), after.size());
    EXPECT_EQUAL(before.capacity(), after.capacity());
    for (int i {0}; i < before.size(); ++i) {
      EXPECT_EQUAL(before.at<0>(i), after.at<0>(i));
      EXPECT_EQUAL(before.at<1>(i), after.at<1>(i));
      EXPECT_EQUAL(before.at<2>(i), after.at<2>(i));
    }
  }

  // Check initializer list constructor.
  Soa<int, string> newsoa1({{0, "zero"}, {1, "one"}, {2, "two"}, {3, "three"}});
  EXPECT_EQUAL(newsoa1.size(), 4);

  soa.clear();
  EXPECT_EQUAL(soa.size(), 0);
  EXPECT_EQUAL(soa.capacity(), 1016);
  soa.shrinkToFit();
  EXPECT_EQUAL(soa.capacity(), 0);

  // Array pointers should be null after shrinking capacity to 0.
  EXPECT_NULL(ints);
  EXPECT_NULL(strings);
  EXPECT_NULL(shorts);
  EXPECT_NULL(doubles);

  END_UNIT_TEST;
}