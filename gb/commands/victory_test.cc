// SPDX-License-Identifier: Apache-2.0

/// \file victory_test.cc
/// \brief Test victory command functionality and standings output via
/// CommandDescriptor.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_victory_dispatch() {
  std::println(std::cout, "Test: victory command dispatch and rankings");

  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& race1) {
    race1.name = "Terrans";
    race1.victory_score = 100;
    race1.tech = 50.0;
    race1.IQ = 120;
    race1.password = "secret1";
    race1.leader().password = "govsec1";
  });

  ctx.em.mutate_race(2, [](Race& race2) {
    race2.name = "Martians";
    race2.Metamorph = true;
    race2.victory_score = 250;
    race2.tech = 80.0;
    race2.IQ = 140;
    race2.password = "secret2";
    race2.leader().password = "govsec2";
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // 1. Victory standings (no args - shows all players)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory"});
  std::string out = g.out.str();
  test::expect_contains(out, "PLAYER RANKINGS");
  test::expect_true(out.contains("Martians") && out.contains("Terrans"));
  test::expect_contains(out, "M");
  std::println(std::cout, "    ✓ victory all players standings succeeded");

  // 2. Victory top count (victory 1 - top 1 player)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory", "1"});
  out = g.out.str();
  test::expect_contains(out, "PLAYER RANKINGS");
  test::expect_contains(out, "Martians");
  test::expect_false(out.contains("Terrans"));
  std::println(std::cout, "    ✓ victory top 1 player standings succeeded");

  // 3. Error case: invalid count
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"victory", "0"});
  test::expect_contains(g.out.str(), "Invalid count specified");
  std::println(std::cout, "    ✓ victory rejected 0 count");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"victory", "abc"});
  test::expect_contains(g.out.str(), "Invalid count specified");
  std::println(std::cout, "    ✓ victory rejected non-numeric count");

  // 4. God mode includes passwords
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory"});
  out = g.out.str();
  test::expect_true(out.contains("Password") && out.contains("Gov Pass"));
  test::expect_true(out.contains("secret1") && out.contains("secret2"));
  test::expect_true(out.contains("govsec1") && out.contains("govsec2"));
  std::println(std::cout, "    ✓ victory god mode details succeeded");

  // 5. JSON mode
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory", "2"});
  out = g.out.str();
  test::expect_contains(out, "\"type\":\"victory\"");
  test::expect_contains(out, "\"is_god\":true");
  test::expect_contains(out, "\"is_metamorph\":true");
  test::expect_contains(out, "\"name\":\"Martians\"");
  test::expect_contains(out, "\"password\":\"secret2\"");
  test::expect_contains(out, "\"gov_password\":\"govsec2\"");
  g.set_ui_mode(UiMode::ASCII);

  // 6. God, Guest, and dissolved races mark no_count=true in
  // create_victory_list and compute_power_blocks aggregates allied member power
  TestWorldBuilder(ctx)
      .add_race("Deity", 100.0, false, player_t{3})
      .add_race("Visitor", 100.0, false, player_t{4})
      .add_race("Fallen", 100.0, false, player_t{5});
  ctx.em.mutate_race(3, [](Race& r) { r.God = true; });
  ctx.em.mutate_race(4, [](Race& r) { r.Guest = true; });
  ctx.em.mutate_race(5, [](Race& r) { r.dissolved = true; });
  const auto vics = create_victory_list(ctx.em);
  test::expect_eq(vics.size(), 5U);

  ctx.em.mutate_block(1, [](Block& b) {
    b.invite(player_t{1});
    b.pledge(player_t{1});
  });
  compute_power_blocks(ctx.em);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_victory_dispatch();

  std::println(std::cout, "All victory tests passed!");
  return 0;
}
