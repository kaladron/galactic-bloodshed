#include "shlmisc.h"

#include <cassert>
#include <optional>
#include <string>

#include "GB_server.h"
#include "buffers.h"
#include "build.h"
#include "globals.h"
#include "map.h"

// TODO(jeffbailey): Puke.  Detangling the dependencies is too
// big right now, so this code is duplicated.
std::optional<shipnum_t> string_to_shipnum(std::string_view s) {
  if (s.size() > 1 && s[0] == '#') {
    s.remove_prefix(1);
    return string_to_shipnum(s);
  }

  if (s.size() > 0 && std::isdigit(s[0])) {
    return (std::stoi(std::string(s.begin(), s.end())));
  }
  return {};
}

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
