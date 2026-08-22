// SPDX-License-Identifier: Apache-2.0

/// \file whois_test.cc
/// \brief Test whois and identify command functionality and output.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_whois_dispatch() {
  std::println(std::cout, "Test: whois and identify command dispatch");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;
  race1.governor[0].name = "Kirk";

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Klingons";
  race2.governor[0].active = true;
  race2.governor[0].name = "Kang";

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Self identification (no args)
  ctx.assert_dispatch_success(g, {"whois"});
  assert(g.out.str().contains("Federation"));
  assert(g.out.str().contains("Kirk"));
  std::println(std::cout, "    ✓ Self whois output verified");

  // 2. Identify another player
  g.out.str("");
  ctx.assert_dispatch_success(g, {"whois", "2"});
  assert(g.out.str().contains("Klingons"));
  std::println(std::cout, "    ✓ Other race whois output verified");

  // 3. Test alias "identify"
  g.out.str("");
  ctx.assert_dispatch_success(g, {"identify", "2"});
  assert(g.out.str().contains("Klingons"));
  std::println(std::cout, "    ✓ Identify alias verified");

  // 4. Invalid player handled gracefully
  g.out.str("");
  ctx.assert_dispatch_success(g, {"whois", "99"});
  assert(g.out.str().contains("Invalid player"));
  std::println(std::cout, "    ✓ Invalid player output verified");
}

}  // namespace

int main() {
  test_whois_dispatch();
  std::println(std::cout, "\n✅ All whois tests passed!");
  return 0;
}
