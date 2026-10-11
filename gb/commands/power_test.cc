// SPDX-License-Identifier: Apache-2.0

/// \file power_test.cc
/// \brief Test power command functionality and report output via
/// CommandDescriptor.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_power_dispatch() {
  std::println(std::cout, "Test: power command dispatch and reporting");

  TestContext ctx;
  ctx.with_standard_universe();
  JsonStore store(ctx.db);

  ctx.em.mutate_universe([](universe_struct& u) {
    u.vn_target(1).hits = 3;
    u.vn_target(2).hits = 7;
  });

  ctx.em.mutate_race(1, [](Race& r) {
    r.victory_score = 100.0;
    r.translate[player_t{1}] = 100;
    r.translate[player_t{2}] = 50;
    r.translate[player_t{3}] = 5;
    r.translate[player_t{4}] = 50;
    r.allied.insert(player_t{2});
    r.atwar.insert(player_t{3});
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.victory_score = 150.0;
    r.translate[player_t{1}] = 50;
    r.translate[player_t{2}] = 100;
    r.allied.insert(player_t{1});
  });
  ctx.em.mutate_race(3, [](Race& r) { r.atwar.insert(player_t{1}); });
  ctx.em.mutate_race(4, [](Race& r) { r.dissolved = true; });

  PowerRepository powers(store);
  powers.save(power{
      .id = 1,
      .troops = 200,
      .popn = 5000,
      .ships_owned = 10,
      .planets_owned = 2,
      .money = 1000,
  });
  powers.save(power{
      .id = 2,
      .troops = 400,
      .popn = 8000,
      .ships_owned = 15,
      .planets_owned = 3,
      .money = 2000,
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{1});

  // 1. All players report (covers dissolved + low-knowledge filtering)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"power"});
  std::string out = g.out.str();
  test::expect_contains(out, "Galactic Bloodshed Power Report");
  test::expect_true(out.contains("Federation") && out.contains("Klingons"));
  test::expect_false(out.contains("Romulans"));
  test::expect_false(out.contains("Cardassians"));
  std::println(std::cout, "    ✓ power all players report succeeded");

  // 2. Specific player filter
  g.out.str("");
  ctx.assert_dispatch_success(g, {"power", "2"});
  out = g.out.str();
  test::expect_contains(out, "Galactic Bloodshed Power Report");
  test::expect_contains(out, "Klingons");
  std::println(std::cout, "    ✓ power target player succeeded");

  // 3. Error case: invalid player, plus God VN mode and JSON mode
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"power", "99"});
  test::expect_contains(g.out.str(), "No such player");
  std::println(std::cout, "    ✓ power rejected non-existent player");

  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  g.race = ctx.em.peek_race(player_t{1});
  ctx.assert_dispatch_success(g, {"power"});
  test::expect_contains(g.out.str(), "VNs");

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"power"});
  test::expect_contains(g.out.str(), "\"type\":\"power\"");

  ctx.verify_universe_invariants();

  powers.remove(3);
  ctx.em.clear_cache();
  g.set_ui_mode(UiMode::ASCII);
  ctx.assert_dispatch_success(g, {"power", "3"});
  test::expect_false(g.out.str().contains("Romulans"));
}

}  // namespace

int main() {
  test_power_dispatch();

  std::println(std::cout, "All power tests passed!");
  return 0;
}
