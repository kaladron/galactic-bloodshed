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

constexpr player_t p1 = 2;
constexpr player_t p2 = 3;
constexpr player_t p3 = 4;
constexpr player_t p4 = 5;

void test_power_dispatch() {
  std::println(std::cout, "Test: power command dispatch and reporting");

  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);
  world.add_race("Romulans", 100.0, false, p3);
  world.add_race("Cardassians", 100.0, false, p4);
  JsonStore store(ctx.db);

  ctx.em.mutate_universe([](Universe& u) {
    u.set_vn_hits(p1, 3);
    u.set_vn_hits(p2, 7);
  });

  ctx.em.mutate_race(p1, [](Race& r) {
    r.victory_score = 100.0;
    r.translate[p1] = 100;
    r.translate[p2] = 50;
    r.translate[p3] = 5;
    r.translate[p4] = 50;
    r.allied.insert(p2);
    r.atwar.insert(p3);
  });
  ctx.em.mutate_race(p2, [](Race& r) {
    r.victory_score = 150.0;
    r.translate[p1] = 50;
    r.translate[p2] = 100;
    r.allied.insert(p1);
  });
  ctx.em.mutate_race(p3, [](Race& r) { r.atwar.insert(p1); });
  ctx.em.mutate_race(p4, [](Race& r) { r.dissolved = true; });

  PowerRepository powers(store);
  powers.save(Power{{
      .id = p1,
      .troops = 200,
      .popn = 5000,
      .ships_owned = 10,
      .planets_owned = 2,
      .money = 1000,
  }});
  powers.save(Power{{
      .id = p2,
      .troops = 400,
      .popn = 8000,
      .ships_owned = 15,
      .planets_owned = 3,
      .money = 2000,
  }});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);

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
  ctx.assert_dispatch_success(g, {"power", "3"});
  out = g.out.str();
  test::expect_contains(out, "Galactic Bloodshed Power Report");
  test::expect_contains(out, "Klingons");
  std::println(std::cout, "    ✓ power target player succeeded");

  // 3. Error case: invalid/dissolved player, plus God VN mode and JSON mode
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"power", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  ctx.assert_dispatch_rejected(g, {"power", "5"});
  test::expect_contains(g.out.str(), "That player has dissolved.");
  std::println(std::cout, "    ✓ power rejected non-existent player");

  ctx.em.mutate_race(p1, [](Race& r) { r.God = true; });
  ctx.setup_game_obj(g, p1, 1);
  ctx.assert_dispatch_success(g, {"power"});
  test::expect_contains(g.out.str(), "VNs");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"power"});
  test::expect_contains(g.out.str(), R"("type":"race")");

  ctx.verify_universe_invariants();

  powers.remove(p3);
  ctx.em.clear_cache();
  g.set_ui_mode(UiMode::ASCII);
  ctx.assert_dispatch_success(g, {"power", "4"});
  test::expect_false(g.out.str().contains("Romulans"));
}

}  // namespace

int main() {
  test_power_dispatch();

  std::println(std::cout, "All power tests passed!");
  return 0;
}
