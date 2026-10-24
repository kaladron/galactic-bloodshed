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

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Setup governors and names
  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().name = "President";
    r.appoint_governor(2, {.name = "VicePresident"});
  });

  ctx.em.mutate_race(2, [](Race& r) {
    r.God = true;
    r.leader().name = "Emperor";
  });
  ctx.create_star("Frontier", 4).build();
}

void test_announce_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  RecordingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Announce in inhabited star system (separator ':')
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"announce", "Hello", "System"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        ": Hello System\n");

  // 2. Announce at LEVEL_UNIV upgrades to broadcast (separator '>')
  g.set_level(ScopeLevel::LEVEL_UNIV);
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {":", "Galaxy", "Wide"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        "> Galaxy Wide\n");

  // 3. Announce in uninhabited star rejected for mortal, allowed for deity
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(4);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"announce", "Trespass"});
  test::expect_contains(g.out.str(), "You do not inhabit this system");

  // Broadcast/think at snum=99 succeed without touching peek_star
  g.set_snum(99);
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
  g.set_snum(4);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"shout", "Deity", "Announcement"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 5. Deity can announce in uninhabited system & shout in JSON mode
  g.set_god(true);
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"announce", "Deity", "Visit"});
  test::expect_false(registry.notifications.empty());
  test::expect_contains(registry.notifications.back().message,
                        ": Deity Visit\n");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  registry.clear_notifications();
  ctx.assert_dispatch_success(g, {"!", "Deity", "Announcement"});
  test::expect_contains(registry.notifications.back().message,
                        "! Deity Announcement\n");
  test::expect_contains(g.out.str(), "\"type\":\"announce\"");
  test::expect_contains(g.out.str(), "\"mode\":2");
  test::expect_contains(g.out.str(), "\"message\":\"Deity Announcement\"");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_announce_dispatch();
  std::println(std::cout, "✓ announce_test passed!");
  return 0;
}
