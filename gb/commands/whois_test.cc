// SPDX-License-Identifier: Apache-2.0

/// \file whois_test.cc
/// \brief Test whois and identify command functionality and output.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_whois_dispatch() {
  std::println(std::cout, "Test: whois and identify command dispatch");
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Federation", 50.0, false, player_t{1})
      .add_race("Klingons", 50.0, false, player_t{2});

  ctx.em.mutate_race(player_t{1},
                     [](Race& race1) { race1.leader().name = "Kirk"; });
  ctx.em.mutate_race(player_t{2},
                     [](Race& race2) { race2.leader().name = "Kang"; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Self identification (no args)
  ctx.assert_dispatch_success(g, {"whois"});
  test::expect_contains(g.out.str(), "Federation");
  test::expect_contains(g.out.str(), "Kirk");
  std::println(std::cout, "    ✓ Self whois output verified");

  // 2. Identify another player and self in multi-arg call
  ctx.assert_dispatch_success(g, {"whois", "1", "2"});
  test::expect_contains(g.out.str(), "Kirk");
  test::expect_contains(g.out.str(), "Klingons");
  std::println(std::cout, "    ✓ Other race whois output verified");

  // 3. Test alias "identify"
  ctx.assert_dispatch_success(g, {"identify", "2"});
  test::expect_contains(g.out.str(), "Klingons");
  std::println(std::cout, "    ✓ Identify alias verified");

  // 4. Invalid player handled gracefully
  ctx.assert_dispatch_success(g, {"whois", "99"});
  test::expect_contains(g.out.str(), "Invalid player");
  std::println(std::cout, "    ✓ Invalid player output verified");

  // 5. JSON mode rendering
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"whois", "1", "2"});
  test::expect_contains(g.out.str(), "\"type\":\"whois\"");
  test::expect_contains(g.out.str(), "\"governor_name\":\"Kirk\"");
  test::expect_contains(g.out.str(), "\"race_name\":\"Klingons\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ JSON mode whois output verified");
}

}  // namespace

int main() {
  test_whois_dispatch();
  std::println(std::cout, "\n✅ All whois tests passed!");
  return 0;
}
