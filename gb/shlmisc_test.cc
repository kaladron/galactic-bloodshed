import gblib;
import std.compat;

#include <cassert>

#include "gb/globals.h"

int main() {
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
}
