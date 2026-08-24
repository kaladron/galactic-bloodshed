// SPDX-License-Identifier: Apache-2.0

/// \file transfer_test.cc
/// \brief Unit tests for transfer command between players on planets.

import dallib;
import gblib;
import test;
import commands;
import std;

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

  // 1. Scope rejection at UNIV level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"transfer", "Receiver", "r", "100"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Scope rejection at STAR level
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"transfer", "Receiver", "r", "100"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at star level verified");

  // 3. Guest rejection
  {
    auto guest_race_handle = ctx.em.get_race(1);
    guest_race_handle->Guest = true;
  }
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"transfer", "Receiver", "r", "100"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  {
    auto race_handle = ctx.em.get_race(1);
    race_handle->Guest = false;
  }
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println(std::cout, "Transfer resources");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_resource_before = p_before->info(player_t{1}).resource;
    int p2_resource_before = p_before->info(player_t{2}).resource;

    ctx.assert_dispatch_success(g, {"transfer", "Receiver", "r", "100"}, 1);

    ctx.em.clear_cache();
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_eq(p_after->info(player_t{1}).resource,
                    p1_resource_before - 100);
    test::expect_eq(p_after->info(player_t{2}).resource,
                    p2_resource_before + 100);
    std::println(std::cout, "✓ Resources transferred");
  }

  std::println(std::cout, "Transfer fuel");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_fuel_before = p_before->info(player_t{1}).fuel;
    int p2_fuel_before = p_before->info(player_t{2}).fuel;

    ctx.assert_dispatch_success(g, {"transfer", "Receiver", "f", "75"}, 1);

    ctx.em.clear_cache();
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_eq(p_after->info(player_t{1}).fuel, p1_fuel_before - 75);
    test::expect_eq(p_after->info(player_t{2}).fuel, p2_fuel_before + 75);
    std::println(std::cout, "✓ Fuel transferred");
  }

  std::println(std::cout, "Transfer destruct");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_destruct_before = p_before->info(player_t{1}).destruct;
    int p2_destruct_before = p_before->info(player_t{2}).destruct;

    ctx.assert_dispatch_success(g, {"transfer", "Receiver", "d", "50"}, 1);

    ctx.em.clear_cache();
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_eq(p_after->info(player_t{1}).destruct,
                    p1_destruct_before - 50);
    test::expect_eq(p_after->info(player_t{2}).destruct,
                    p2_destruct_before + 50);
    std::println(std::cout, "✓ Destruct transferred");
  }

  std::println(std::cout, "Transfer crystals");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_crystals_before = p_before->info(player_t{1}).crystals;
    int p2_crystals_before = p_before->info(player_t{2}).crystals;

    ctx.assert_dispatch_success(g, {"transfer", "Receiver", "x", "10"}, 1);

    ctx.em.clear_cache();
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_eq(p_after->info(player_t{1}).crystals,
                    p1_crystals_before - 10);
    test::expect_eq(p_after->info(player_t{2}).crystals,
                    p2_crystals_before + 10);
    std::println(std::cout, "✓ Crystals transferred");
  }

  std::println(std::cout, "Cannot transfer more than available");
  {
    const auto* p_before = ctx.em.peek_planet(0, 0);
    int p1_resource_before = p_before->info(player_t{1}).resource;
    int p2_resource_before = p_before->info(player_t{2}).resource;

    // Try to transfer more resources than player has
    ctx.assert_dispatch_rejected(g, {"transfer", "Receiver", "r", "10000"});

    // Should not have changed (command fails with error message)
    ctx.em.clear_cache();
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_eq(p_after->info(player_t{1}).resource, p1_resource_before);
    test::expect_eq(p_after->info(player_t{2}).resource, p2_resource_before);
    std::println(std::cout, "✓ Transfer prevented when insufficient resources");
  }

  std::println(std::cout, "All transfer tests passed!");
  return 0;
}
