// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Giver";
  race1.Guest = false;
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Receiver";
  race2.Guest = false;
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Create test star with APs
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TransferHub";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.AP[0] = 50;  // Give player 1 enough APs
  ss.pnames.emplace_back("TransferPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet with resources for player 1
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
  ps.info[0].resource = 1000;
  ps.info[0].fuel = 500;
  ps.info[0].destruct = 200;
  ps.info[0].crystals = 50;
  ps.info[1].explored = true;
  ps.info[1].numsectsowned = 3;
  ps.info[1].resource = 100;
  ps.info[1].fuel = 50;
  ps.info[1].destruct = 20;
  ps.info[1].crystals = 5;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create GameObj for player 1
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println("Test 1: Transfer resources");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_resource_before = p_before->info(player_t{1}).resource;
    int p2_resource_before = p_before->info(player_t{2}).resource;

    command_t argv = {"transfer", "Receiver", "r", "100"};
    GB::commands::transfer(argv, g);

    const auto* p_after = ctx.em.peek_planet(0, 0);
    assert(p_after->info(player_t{1}).resource == p1_resource_before - 100);
    assert(p_after->info(player_t{2}).resource == p2_resource_before + 100);
    std::println("✓ Resources transferred");
  }

  std::println("Test 2: Transfer fuel");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_fuel_before = p_before->info(player_t{1}).fuel;
    int p2_fuel_before = p_before->info(player_t{2}).fuel;

    command_t argv = {"transfer", "Receiver", "f", "75"};
    GB::commands::transfer(argv, g);

    const auto* p_after = ctx.em.peek_planet(0, 0);
    assert(p_after->info(player_t{1}).fuel == p1_fuel_before - 75);
    assert(p_after->info(player_t{2}).fuel == p2_fuel_before + 75);
    std::println("✓ Fuel transferred");
  }

  std::println("Test 3: Transfer destruct");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_destruct_before = p_before->info(player_t{1}).destruct;
    int p2_destruct_before = p_before->info(player_t{2}).destruct;

    command_t argv = {"transfer", "Receiver", "d", "50"};
    GB::commands::transfer(argv, g);

    const auto* p_after = ctx.em.peek_planet(0, 0);
    assert(p_after->info(player_t{1}).destruct == p1_destruct_before - 50);
    assert(p_after->info(player_t{2}).destruct == p2_destruct_before + 50);
    std::println("✓ Destruct transferred");
  }

  std::println("Test 4: Transfer crystals");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_crystals_before = p_before->info(player_t{1}).crystals;
    int p2_crystals_before = p_before->info(player_t{2}).crystals;

    command_t argv = {"transfer", "Receiver", "x", "10"};
    GB::commands::transfer(argv, g);

    const auto* p_after = ctx.em.peek_planet(0, 0);
    assert(p_after->info(player_t{1}).crystals == p1_crystals_before - 10);
    assert(p_after->info(player_t{2}).crystals == p2_crystals_before + 10);
    std::println("✓ Crystals transferred");
  }

  std::println("Test 5: Cannot transfer more than available");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_resource_before = p_before->info(player_t{1}).resource;
    int p2_resource_before = p_before->info(player_t{2}).resource;

    // Try to transfer more resources than player has
    command_t argv = {"transfer", "Receiver", "r", "10000"};
    GB::commands::transfer(argv, g);

    // Should not have changed (command fails with error message)
    const auto* p_after = ctx.em.peek_planet(0, 0);
    assert(p_after->info(player_t{1}).resource == p1_resource_before);
    assert(p_after->info(player_t{2}).resource == p2_resource_before);
    std::println("✓ Transfer prevented when insufficient resources");
  }

  std::println("All transfer tests passed!");
  return 0;
}
