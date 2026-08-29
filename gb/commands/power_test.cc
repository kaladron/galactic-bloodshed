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
  JsonStore store(ctx.db);

  // Setup: Create universe
  universe_struct us{};
  us.id = 1;
  us.VN_hitlist[player_t{1}] = 3;
  us.VN_hitlist[player_t{2}] = 7;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Setup: Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  race1.victory_score = 100.0;
  race1.translate[0] = 100;
  race1.translate[1] = 50;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Martians";
  race2.victory_score = 150.0;
  race2.translate[0] = 50;
  race2.translate[1] = 100;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup: Create power records
  power p1{};
  p1.id = 1;
  p1.money = 1000;
  p1.popn = 5000;
  p1.troops = 200;
  p1.ships_owned = 10;
  p1.planets_owned = 2;

  power p2{};
  p2.id = 2;
  p2.money = 2000;
  p2.popn = 8000;
  p2.troops = 400;
  p2.ships_owned = 15;
  p2.planets_owned = 3;

  PowerRepository powers(store);
  powers.save(p1);
  powers.save(p2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, player_t{1}, governor_t{0});
  g.race = ctx.em.peek_race(g.player());

  // 1. All players report
  g.out.str("");
  ctx.assert_dispatch_success(g, {"power"});
  std::string out = g.out.str();
  test::expect_contains(out, "Galactic Bloodshed Power Report");
  test::expect_true(out.contains("Terrans") || out.contains("Martians"));
  std::println(std::cout, "    ✓ power all players report succeeded");

  // 2. Specific player filter
  g.out.str("");
  ctx.assert_dispatch_success(g, {"power", "2"});
  out = g.out.str();
  test::expect_contains(out, "Galactic Bloodshed Power Report");
  test::expect_contains(out, "Martians");
  std::println(std::cout, "    ✓ power target player succeeded");

  // 3. Error case: invalid player
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"power", "99"});
  test::expect_contains(g.out.str(), "No such player");
  std::println(std::cout, "    ✓ power rejected non-existent player");
}

}  // namespace

int main() {
  test_power_dispatch();

  std::println(std::cout, "All power tests passed!");
  return 0;
}
