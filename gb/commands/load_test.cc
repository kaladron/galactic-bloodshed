// SPDX-License-Identifier: Apache-2.0

/// \file load_test.cc
/// \brief Unit tests for load and unload commands

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "LoadTester";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;
  race.absorb = false;
  race.Metamorph = false;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "LoadStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.pnames.emplace_back("LoadPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet with resources
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.dimensions = {10, 10};
  ps.info[player_t{1}].fuel = 1000;
  ps.info[player_t{1}].resource = 500;
  ps.info[player_t{1}].destruct = 200;
  ps.info[player_t{1}].crystals = 50;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a landed ship to load cargo onto
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_CARGO;
  ship.name() = "CargoHauler";
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.whatdest() =
      ScopeLevel::LEVEL_PLAN;  // Important: must be PLAN for planet loading
  ship.deststar() = 0;
  ship.destpnum() = 0;
  ship.set_land_coords({5, 5});
  ship.docked() = 1;  // CRITICAL: Ship must be docked to load/unload
  ship.fuel() = 100.0;
  ship.max_fuel() = 500.0;
  ship.resource() = 0;
  ship.max_resource() = 1000;
  ship.destruct() = 0;
  ship.max_destruct() = 300;
  ship.crystals() = 0;
  ship.mass() = 100.0;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);
}

void test_load_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println(std::cout, "Load fuel from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    double initial_ship_fuel = s_before->fuel();
    int initial_planet_fuel = p_before->info(player_t{1}).fuel;

    ctx.assert_dispatch_success(g, {"load", "#1", "f", "100"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->fuel(), initial_ship_fuel + 100);
    test::expect_eq(p_after->info(player_t{1}).fuel, initial_planet_fuel - 100);
    std::println(std::cout, "✓ Fuel loaded from planet to ship");
  }

  std::println(std::cout, "Load resources from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_resource = s_before->resource();
    int initial_planet_resource = p_before->info(player_t{1}).resource;

    ctx.assert_dispatch_success(g, {"load", "#1", "r", "200"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->resource(), initial_ship_resource + 200);
    test::expect_eq(p_after->info(player_t{1}).resource,
                    initial_planet_resource - 200);
    std::println(std::cout, "✓ Resources loaded from planet to ship");
  }

  std::println(std::cout, "Load destruct from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_destruct = s_before->destruct();
    int initial_planet_destruct = p_before->info(player_t{1}).destruct;

    ctx.assert_dispatch_success(g, {"load", "#1", "d", "50"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->destruct(), initial_ship_destruct + 50);
    test::expect_eq(p_after->info(player_t{1}).destruct,
                    initial_planet_destruct - 50);
    std::println(std::cout, "✓ Destruct loaded from planet to ship");
  }

  std::println(std::cout, "Load crystals from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_crystals = s_before->crystals();
    int initial_planet_crystals = p_before->info(player_t{1}).crystals;

    ctx.assert_dispatch_success(g, {"load", "#1", "x", "10"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->crystals(), initial_ship_crystals + 10);
    test::expect_eq(p_after->info(player_t{1}).crystals,
                    initial_planet_crystals - 10);
    std::println(std::cout, "✓ Crystals loaded from planet to ship");
  }
}

void test_unload_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // First load resources, then unload
  ctx.assert_dispatch_success(g, {"load", "#1", "r", "200"});
  test::expect_eq(ctx.em.peek_ship(1)->resource(), 200);

  ctx.assert_dispatch_success(g, {"unload", "#1", "r", "50"});
  test::expect_eq(ctx.em.peek_ship(1)->resource(), 150);
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(player_t{1}).resource, 350);
  std::println(std::cout, "✓ Resources unloaded from ship to planet");
}

void test_load_syntax_and_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"load"});
  test::expect_contains(g.out.str(),
                        "Syntax: load <ship> <commodity> [<amount>]");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unload", "#1"});
  test::expect_contains(g.out.str(),
                        "Syntax: unload <ship> <commodity> [<amount>]");

  // 2. Unknown commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"load", "#1", "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");
}

}  // namespace

int main() {
  test_load_happy_path();
  test_unload_happy_path();
  test_load_syntax_and_errors();

  std::println(std::cout, "\n✅ All load command tests passed!");
  std::println(std::cout,
               "The load command correctly transfers cargo and persists "
               "changes via EntityManager.");
  return 0;
}
