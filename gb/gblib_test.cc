// SPDX-License-Identifier: Apache-2.0

/// \file gblib_test.cc
/// \brief Unit tests for bit setting operations across integer widths.

import dallib;
import gblib;
import test;
import std;

int main() {
  std::uint8_t test8 = 0;
  std::uint32_t test32 = 0;
  std::uint64_t test64 = 0;

  setbit(test8, 4U);
  test::expect_eq(test8, 16);

  setbit(test32, 22U);
  test::expect_eq(test32, 4194304);

  setbit(test64, 48U);
  test::expect_eq(test64, static_cast<std::uint64_t>(std::exp2(48)));

  std::println(std::cout, "✓ gblib_test passed!");
  return 0;
}
