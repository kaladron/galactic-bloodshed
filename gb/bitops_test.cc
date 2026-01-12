// SPDX-License-Identifier: Apache-2.0

import gblib;
import std;

#include <cassert>

int main() {
  std::println("Testing bit operation overloads for ID types...");

  // Test with player_t
  std::uint64_t flags = 0;
  player_t p1{1};
  player_t p2{2};
  player_t p5{5};

  // Test setbit with player_t
  setbit(flags, p1);
  assert(isset(flags, p1));
  assert(isclr(flags, p2));

  setbit(flags, p5);
  assert(isset(flags, p5));

  // Test clrbit with player_t
  clrbit(flags, p1);
  assert(isclr(flags, p1));
  assert(isset(flags, p5));

  // Test with governor_t
  std::uint64_t gov_flags = 0;
  governor_t g1{0};
  governor_t g2{1};

  setbit(gov_flags, g1);
  assert(isset(gov_flags, g1));
  assert(isclr(gov_flags, g2));

  clrbit(gov_flags, g1);
  assert(isclr(gov_flags, g1));

  // Test that unsigned still works
  setbit(flags, 3U);
  assert(isset(flags, 3U));

  std::println("✓ All bit operation overload tests passed!");
  return 0;
}
