// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

/// \file power_test.cc
/// \brief Test power command functionality and report output

void test_power_all_players() {
  std::println(std::cout, "Test: power command - report for all players");

  // Create in-memory database
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create universe
  universe_struct us{};
  us.id = 1;
  us.VN_hitlist[0] = 3;
  us.VN_hitlist[1] = 7;

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

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Run power command for all players
  {
    command_t cmd = {"power"};
    GB::commands::power(cmd, g);

    // Verify report output
    std::string out = g.out.str();
    assert(out.find("Galactic Bloodshed Power Report") != std::string::npos);
    assert(out.find("Terrans") != std::string::npos ||
           out.find("Martians") != std::string::npos);
  }

  std::println(std::cout, "  ✅ Power command for all players passed!");
}

void test_power_target_player() {
  std::println(std::cout, "Test: power command - target player filtering");

  // Create in-memory database
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create universe and races
  universe_struct us{};
  us.id = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  race1.translate[0] = 100;
  race1.translate[1] = 100;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Martians";
  race2.translate[0] = 100;
  race2.translate[1] = 100;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  power p2{};
  p2.id = 2;

  PowerRepository powers(store);
  powers.save(p2);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // TEST: Query specific valid target player #2
  {
    g.out.str("");
    command_t cmd = {"power", "2"};
    GB::commands::power(cmd, g);

    // Verify output contains requested player name
    std::string out = g.out.str();
    assert(out.find("Galactic Bloodshed Power Report") != std::string::npos);
    assert(out.find("Martians") != std::string::npos);
  }

  // TEST: Query invalid player #99
  {
    g.out.str("");
    command_t cmd = {"power", "99"};
    GB::commands::power(cmd, g);

    // Verify error message
    std::string out = g.out.str();
    assert(out.find("No such player") != std::string::npos);
  }

  std::println(std::cout, "  ✅ Target player filtering test passed!");
}

int main() {
  test_power_all_players();
  test_power_target_player();

  std::println(std::cout, "\n✅ All power tests passed!");
  return 0;
}
