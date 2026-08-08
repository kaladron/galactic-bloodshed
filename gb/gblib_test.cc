// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  std::uint8_t test8 = 0;
  std::uint32_t test32 = 0;
  std::uint64_t test64 = 0;

  setbit(test8, 4U);
  assert(test8 == 16);

  setbit(test32, 22U);
  assert(test32 == 4194304);

  setbit(test64, 48U);
  assert(test64 == std::exp2(48));
}
