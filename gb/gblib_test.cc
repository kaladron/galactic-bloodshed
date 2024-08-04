import gblib;
import std.compat;

#include <cassert>

#include "gb/globals.h"

int main() {
  uint8_t test8 = 0;
  uint32_t test32 = 0;
  uint64_t test64 = 0;

  setbit(test8, 4U);
  assert(test8 == 16);

  setbit(test32, 22U);
  assert(test32 == 4194304);

  setbit(test64, 48U);
  assert(test64 == exp2(48));
}
