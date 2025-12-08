// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

namespace {
// Tests for string_to_shipnum
void test_string_to_shipnum() {
  auto a = string_to_shipnum("123");
  assert(*a == 123);
  auto b = string_to_shipnum("#123");
  assert(*b == 123);
  auto c = string_to_shipnum("##123");
  assert(*c == 123);
  auto d = string_to_shipnum("abc");
  assert(!d);
  auto e = string_to_shipnum("##abc");
  assert(!e);
  auto f = string_to_shipnum("");
  assert(!f);
  std::println("✓ string_to_shipnum tests passed");
}

// Tests for get4args
void test_single_values() {
  // Test "5,10" -> (5, 5, 10, 10)
  auto result = get4args("5,10");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 5 && xh == 5 && yl == 10 && yh == 10);
}

void test_x_range_y_single() {
  // Test "1:5,10" -> (1, 5, 10, 10)
  auto result = get4args("1:5,10");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 1 && xh == 5 && yl == 10 && yh == 10);
}

void test_x_single_y_range() {
  // Test "5,3:8" -> (5, 5, 3, 8)
  auto result = get4args("5,3:8");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 5 && xh == 5 && yl == 3 && yh == 8);
}

void test_both_ranges() {
  // Test "1:10,20:30" -> (1, 10, 20, 30)
  auto result = get4args("1:10,20:30");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 1 && xh == 10 && yl == 20 && yh == 30);
}

void test_negative_values() {
  // Test "-5:5,-10:10" -> (-5, 5, -10, 10)
  auto result = get4args("-5:5,-10:10");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == -5 && xh == 5 && yl == -10 && yh == 10);
}

void test_zero_values() {
  // Test "0:0,0:0" -> (0, 0, 0, 0)
  auto result = get4args("0:0,0:0");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 0 && xh == 0 && yl == 0 && yh == 0);
}

void test_empty_string() {
  // Test "" -> nullopt
  auto result = get4args("");
  assert(!result.has_value());
}

void test_missing_comma() {
  // Test "5:10" -> nullopt (missing comma)
  auto result = get4args("5:10");
  assert(!result.has_value());
}

void test_invalid_x_number() {
  // Test "abc,10" -> nullopt
  auto result = get4args("abc,10");
  assert(!result.has_value());
}

void test_invalid_y_number() {
  // Test "10,xyz" -> nullopt
  auto result = get4args("10,xyz");
  assert(!result.has_value());
}

void test_invalid_x_range() {
  // Test "5:abc,10" -> nullopt
  auto result = get4args("5:abc,10");
  assert(!result.has_value());
}

void test_invalid_y_range() {
  // Test "5,10:xyz" -> nullopt
  auto result = get4args("5,10:xyz");
  assert(!result.has_value());
}

void test_trailing_garbage() {
  // Test "5,10extra" -> nullopt (garbage after y)
  auto result = get4args("5,10extra");
  assert(!result.has_value());
}

void test_multiple_commas() {
  // Test "5,10,15" -> Could parse as (5, 5, 10, 10) and ignore rest
  // but current implementation should reject this
  auto result = get4args("5,10,15");
  assert(!result.has_value());
}

void test_whitespace() {
  // Test " 5 , 10 " -> likely nullopt (whitespace not handled)
  auto result = get4args(" 5 , 10 ");
  assert(!result.has_value());
}

void test_large_numbers() {
  // Test "999:9999,8888:7777" -> (999, 9999, 8888, 7777)
  auto result = get4args("999:9999,8888:7777");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 999 && xh == 9999 && yl == 8888 && yh == 7777);
}

void test_single_digit_values() {
  // Test "1,2" -> (1, 1, 2, 2)
  auto result = get4args("1,2");
  assert(result.has_value());
  auto [xl, xh, yl, yh] = *result;
  assert(xl == 1 && xh == 1 && yl == 2 && yh == 2);
}

void test_colon_without_second_value() {
  // Test "5:,10" -> nullopt (missing xh)
  auto result = get4args("5:,10");
  assert(!result.has_value());
}

void test_colon_without_first_value() {
  // Test ":5,10" -> nullopt (missing xl)
  auto result = get4args(":5,10");
  assert(!result.has_value());
}
}  // namespace

int main() {
  std::println("Running shlmisc tests...\n");

  // string_to_shipnum tests
  test_string_to_shipnum();

  // get4args valid input tests
  test_single_values();
  test_x_range_y_single();
  test_x_single_y_range();
  test_both_ranges();
  test_negative_values();
  test_zero_values();
  test_large_numbers();
  test_single_digit_values();

  // get4args invalid input tests
  test_empty_string();
  test_missing_comma();
  test_invalid_x_number();
  test_invalid_y_number();
  test_invalid_x_range();
  test_invalid_y_range();
  test_trailing_garbage();
  test_multiple_commas();
  test_whitespace();
  test_colon_without_second_value();
  test_colon_without_first_value();

  std::println("✓ get4args tests passed");
  std::println("\n✅ All shlmisc tests passed!");
  return 0;
}
