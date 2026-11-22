// SPDX-License-Identifier: Apache-2.0

/// \file announce_test.cc
/// \brief Test announce, broadcast, shout, and think communication commands and
/// role checks.

import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

TestRaces setup_test_world(TestContext& ctx) {
  const auto races = ctx.with_races();

  // Setup governors and names
  ctx.em.mutate_race(races.empire, [](Race& r) {
    r.leader().name = "President";
    r.appoint_governor(2, {.name = "VicePresident"});
  });

  ctx.em.mutate_race(races.deity, [](Race& r) { r.leader().name = "Emperor"; });
  ctx.create_star("Frontier", 4).with_inhabited(races.newcomer, true).build();
  return races;
}

void test_announce_dispatch() {
  TestContext ctx;
  const auto races = setup_test_world(ctx);

  RecordingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_star(1);

  // 1. Announce in inhabited star system (separator ':')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"announce", "Hello", "System"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        ": Hello System\n");

  // 2. Announce at LEVEL_UNIV or deep-space LEVEL_SHIP (snum == 0) upgrades to
  // broadcast (separator '>')
  g.set_scope_univ();
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {":", "Galaxy", "Wide"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Galaxy Wide\n");

  const shipnum_t deep_probe = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                   .owned_by(races.empire, 1)
                                   .in_deep_space()
                                   .build();
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", deep_probe)});
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"announce", "Deep", "Space"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Deep Space\n");

  // 3. Announce in uninhabited star rejected for mortal, allowed for deity
  g.set_scope_star(4);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"announce", "Trespass"});
  test::expect_contains(g.out.str(), "You do not inhabit this system");

  // Broadcast/think at snum=99 succeed without touching peek_star
  g.set_scope_star(99);
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {";", "Internal", "Memo"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "= Internal Memo\n");

  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"'", "Quick", "Message"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Quick Message\n");

  // 4. Shout rejected for mortal
  g.set_scope_star(4);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"shout", "Deity", "Announcement"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 5. Deity can announce in uninhabited system & shout in JSON mode
  GameObj god_g(ctx.em, registry);
  ctx.setup_game_obj(god_g, races.deity, 1);
  god_g.set_scope_star(4);
  registry.clear_notifications();
  ctx.assert_dispatch_success(god_g, {"announce", "Deity", "Visit"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        ": Deity Visit\n");

  god_g.set_ui_mode(UiMode::JSON);
  god_g.out.str("");
  registry.clear_notifications();
  ctx.assert_dispatch_success(god_g, {"!", "Deity", "Announcement"});
  test::expect_contains(registry.notifications.back().message,
                        "! Deity Announcement\n");
  test::expect_contains(god_g.out.str(), R"("type":"message")");
  test::expect_contains(god_g.out.str(), R"("message":"Deity Announcement")");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_announce_dispatch();
  std::println(std::cout, "✓ announce_test passed!");
  return 0;
}
