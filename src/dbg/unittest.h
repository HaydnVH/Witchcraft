/******************************************************************************
 * unittest.hpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2024 - present
 * Last modified January 2024
 * ---------------------------------------------------------------------------
 * Provides a simple framework for performing unit tests.
 * Before calling any of these macros, a local variable of any integral type
 * named "FAIL_COUNTER" must be declared and initialized to 0.
 * The tests below check to see if a value is what the programmer expects.
 * If it isn't, FAIL_COUNTER is incremented and a message is shown to cout.
 *****************************************************************************/
#ifndef WC_DBG_UNITTEST_H
#define WC_DGB_UNITTEST_H

#include <iostream>

// Expression should be true.
#define EXPECT_TRUE(exp) \
  [&] { if (exp) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not true." << std::endl; ++FAIL_COUNTER;}}()

// Expression should be false.
#define EXPECT_FALSE(exp) \
  [&] { if (!(exp)) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not false." << std::endl; ++FAIL_COUNTER;}}()

// Expression should be nullptr.
#define EXPECT_NULL(exp) \
  [&] { if ((exp) == nullptr) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not null." << std::endl; ++FAIL_COUNTER;}}()

// Expression should be non-null.
#define EXPECT_NOTNULL(exp) \
  [&] { if ((exp) != nullptr) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is null." << std::endl; ++FAIL_COUNTER;}}()

// Each expression should evaluate to the same value.
#define EXPECT_EQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; if (lhseval == rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is not equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// Each expression should evaluate to different values.
#define EXPECT_NEQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; if (lhseval != rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// Use this instead of EXPECT_EQUAL when working with floating-point values.
#define EXPECT_FEQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; auto diff {lhseval - rhseval}; if ((diff*diff) < 0.000001) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is not equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// Use this instead of EXPECT_NEQUAL when working with floating-point values.
#define EXPECT_NFEQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; auto diff {lhseval - rhseval}; if ((diff*diff) > 0.000001) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// The first expression should be greater than the second.
#define EXPECT_GREATER(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; if (lhseval > rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is not greater than '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// The first expression should be greater than or equal to the second.
#define EXPECT_GEQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; if (lhseval >= rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is greater than or equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// The first expression should be less than the second.
#define EXPECT_LESS(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs};  if (lhseval < rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is not less than '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// The first expression should be less than or equal to the second.
#define EXPECT_LEQUAL(lhs,rhs) \
  [&] { auto lhseval{lhs}; auto rhseval{rhs}; if (lhseval <= rhseval) return; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "'(" << lhseval << ") is not less than or equal to '" << #rhs << "'(" << rhseval << ")." << std::endl; ++FAIL_COUNTER;}}()

// Use this instead of 'cout' or 'printf' to print a custom failure message.
#define CUSTOM_FAIL(msg) \
  [&] {std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m " << msg << std::endl; ++FAIL_COUNTER;}()

// Catches the exception thrown when executing the expression, and ensures it's the expected type.
#define EXPECT_EXCEPTION(exp, etype) [&] { \
	try { exp; } catch (const etype&) { return; } \
	catch (...) { std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m Wrong exception type caught.  Expected '" << #etype << "'." << std::endl; } \
	std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m No exception caught.  Expected '" << #etype << "'." << std::endl; ++FAIL_COUNTER; }()

#endif //WC_DBG_UNITTEST_H