// SPDX-License-Identifier: Apache-2.0

/// \file land_test.cc
/// \brief Unit tests for land command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Lander";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;

  RaceRepository races(store);
  races.save(race);

  // Setup universe
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 1;
  universe_repo.save(sdata);

  // Create test star with APs
  star_struct ss{};
  ss.star_id = 1;
  ss.name = "LandingStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 10;  // Give player 1 enough APs
  ss.governor[0] = 0;
  ss.pnames.emplace_back("LandingPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 1;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.xpos = 5.0;
  ps.ypos = 5.0;
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create sectormap for the planet
  SectorMap smap(planet, true);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  // Create a ship that can land (shuttle)
  Ship shuttle{};
  shuttle.number() = 1;
  shuttle.owner() = 1;
  shuttle.governor() = 0;
  shuttle.alive() = true;
  shuttle.active() = true;
  shuttle.type() = ShipType::STYPE_SHUTTLE;  // Can land
  shuttle.build_type() = ShipType::STYPE_SHUTTLE;
  shuttle.name() = "TestShuttle";
  shuttle.damage() = 0.0;
  shuttle.armor() = 1;
  shuttle.size() = 10;
  shuttle.max_crew() = 10;
  shuttle.max_resource() = 100;
  shuttle.max_fuel() = 100;
  shuttle.max_destruct() = 10;
  shuttle.max_speed() = 10;
  shuttle.max_hanger() = 0;
  shuttle.base_mass() = 1.0;
  shuttle.mass() = 1.0;
  shuttle.fuel() = 50.0;  // Within max_fuel
  shuttle.resource() = 0;
  shuttle.destruct() = 0;
  shuttle.popn() = 2;  // Within max_crew
  shuttle.troops() = 0;
  shuttle.whatorbits() = ScopeLevel::LEVEL_PLAN;
  shuttle.storbits() = 1;
  shuttle.pnumorbits() = 0;
  shuttle.xpos() = 105.0;  // Close to planet
  shuttle.ypos() = 205.0;
  shuttle.speed() = 5;
  shuttle.docked() = false;

  ShipRepository ships_repo(store);
  ships_repo.save(shuttle);
}

// Test: Land ship on planet coordinates
void test_land_on_planet() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_shipno(1);

  // Land on planet coordinates (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"land", "#1", "5,5"}, 1);
  assert(g.out.str().contains("landed on planet"));

  const auto* s = ctx.em.peek_ship(1);
  assert(s != nullptr);
  // Ship should be docked after landing
  assert(s->docked());
  assert(s->land_coords() == Coordinates(5, 5));
}

// Test: Cannot land docked ship
void test_cannot_land_docked_ship() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // First land the ship so it is docked
  ctx.assert_dispatch_success(g, {"land", "#1", "5,5"}, 1);

  // Ship is already docked from first landing
  const auto* s_before = ctx.em.peek_ship(1);
  bool was_docked = s_before->docked();

  // Try to land again on different coordinates
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"land", "#1", "3,3"});

  // Should still be at original location
  const auto* s_after = ctx.em.peek_ship(1);
  assert(s_after->docked() == was_docked);
}

// Test: Create carrier and shuttle for friendly landing
void test_land_on_friendly_carrier() {
  TestContext ctx;
  setup_test_world(ctx);

  JsonStore store(ctx.db);
  ShipRepository ships_repo(store);

  // Reset shuttle to undocked state with land_coords at 5,5
  {
    auto s_handle = ctx.em.get_ship(1);
    auto& s = *s_handle;
    s.docked() = false;
    s.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s.set_land_coords({5, 5});
  }

  // Create a carrier
  Ship carrier{};
  carrier.number() = 2;
  carrier.owner() = 1;
  carrier.governor() = 0;
  carrier.alive() = true;
  carrier.active() = true;
  carrier.type() = ShipType::STYPE_CARRIER;
  carrier.name() = "TestCarrier";
  carrier.damage() = 0.0;
  carrier.mass() = 1000.0;
  carrier.max_hanger() = 20;  // Capacity for 20 size units
  carrier.hanger() = 0;       // Empty hanger (current usage)
  carrier.whatorbits() = ScopeLevel::LEVEL_PLAN;
  carrier.storbits() = 1;
  carrier.pnumorbits() = 0;
  carrier.xpos() = 105.0;
  carrier.ypos() = 205.0;
  carrier.set_land_coords({5, 5});
  carrier.docked() = true;  // Carrier is landed

  ships_repo.save(carrier);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // Now the shuttle (already at 5,5 landed) can land on carrier
  ctx.assert_dispatch_success(g, {"land", "#1", "#2"}, 0);
  assert(g.out.str().contains("landed on") || g.out.str().contains("loaded onto"));

  const auto* shuttle_after = ctx.em.peek_ship(1);
  assert(shuttle_after->docked());
}

// Test: Insufficient AP rejection
void test_land_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  {
    auto star_handle = ctx.em.get_star(1);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"land", "#1", "5,5"});
  assert(g.out.str().contains("action points"));
}

// Test: Domain validation errors
void test_land_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"land", "#1"});
  assert(g.out.str().contains("Syntax: land <ship> <#mothership | x,y>"));

  // 2. Invalid coordinates format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"land", "#1", "bad_coords"});
  assert(g.out.str().contains("Invalid coordinates format"));
}

}  // namespace

int main() {
  test_land_on_planet();
  test_cannot_land_docked_ship();
  test_land_on_friendly_carrier();
  test_land_insufficient_ap();
  test_land_domain_errors();

  std::println(std::cout, "✓ land_test passed!");
  return 0;
}
