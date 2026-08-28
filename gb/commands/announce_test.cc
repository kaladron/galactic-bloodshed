// SPDX-License-Identifier: Apache-2.0

/// \file announce_test.cc
/// \brief Test announce, broadcast, shout, and think communication commands and
/// role checks.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Federation", 100.0, false, player_t{1})
      .add_race("Empire", 100.0, false, player_t{2})
      .add_star("Sol", 100, starnum_t{0});

  // Setup governors and names
  ctx.em.mutate_race(1, [](Race& r) {
    r.governor[0].name = "President";
    r.governor[1].active = true;
    r.governor[1].name = "VicePresident";
  });

  ctx.em.mutate_race(2, [](Race& r) { r.governor[0].name = "Emperor"; });

  // Mark star inhabited by both races
  ctx.em.mutate_star(0, [](Star& star) {
    setbit(star.inhabited(), player_t{1});
    setbit(star.inhabited(), player_t{2});
  });
}

void test_announce_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  RecordingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Announce in inhabited star system (separator ':')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"announce", "Hello", "System"});
  test::expect_true(!registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        ": Hello System");

  // 2. Broadcast across galaxy (separator '>')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"broadcast", "Global", "Transmission"});
  test::expect_true(!registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Global Transmission");

  // 3. Broadcast alias "'" (separator '>')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"'", "Quick", "Message"});
  test::expect_true(!registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Quick Message");

  // 4. Think to race governors (separator '=')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"think", "Internal", "Memo"});
  test::expect_true(!registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "= Internal Memo");

  // 5. Shout rejected for mortal
  ctx.assert_dispatch_rejected(g, {"shout", "Deity", "Announcement"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 6. Shout succeeds for deity (separator '!')
  registry.clear_notifications();
  g.set_god(true);
  ctx.assert_dispatch_success(g, {"shout", "Deity", "Announcement"});
  test::expect_true(!registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "! Deity Announcement");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_announce_dispatch();
  std::println(std::cout, "✓ announce_test passed!");
  return 0;
}
