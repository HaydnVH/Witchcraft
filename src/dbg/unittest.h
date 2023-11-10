#ifndef WC_DBG_UNITTEST_H
#define WC_DGB_UNITTEST_H

#include <iostream>

#define EXPECT_TRUE(exp) \
  [] { if (exp) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not true." << std::endl; return 1;}}()

#define EXPECT_FALSE(exp) \
  [] { if (!(exp)) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not false." << std::endl; return 1;}}()

#define EXPECT_NULL(exp) \
  [] { if ((exp) == nullptr) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is not null." << std::endl; return 1;}}()

#define EXPECT_NOTNULL(exp) \
  [] { if ((exp) != nullptr) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #exp << "' is null." << std::endl; return 1;}}()

#define EXPECT_EQUAL(lhs,rhs) \
  [] { if (lhs == rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is not equal to '" << #rhs << "'." << std::endl; return 1;}}()

#define EXPECT_NEQUAL(lhs,rhs) \
  [] { if (lhs != rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is equal to '" << #rhs << "'." << std::endl; return 1;}}()

#define EXPECT_GREATER(lhs,rhs) \
  [] { if (lhs > rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is not greater than '" << #rhs << "'." << std::endl; return 1;}}()

#define EXPECT_GEQUAL(lhs,rhs) \
  [] { if (lhs >= rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is not greater than or equal to '" << #rhs << "'." << std::endl; return 1;}}()

#define EXPECT_LESS(lhs,rhs) \
  [] { if (lhs < rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is not less than '" << #rhs << "'." << std::endl; return 1;}}()

#define EXPECT_LEQUAL(lhs,rhs) \
  [] { if (lhs <= rhs) return 0; else { \
  std::cout << "\x1b[91m" << __FILE__ << "(" << __LINE__ << ")\x1b[0m '" << #lhs << "' is not less than or equal to '" << #rhs << "'." << std::endl; return 1;}}()

#endif //WC_DBG_UNITTEST_H