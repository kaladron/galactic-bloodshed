// SPDX-License-Identifier: Apache-2.0

/// \file map_test.cc
/// \brief Unit tests for map command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create universe with 2 stars
  universe_struct us{};
  us.id = 1;
  us.numstars = 2;
  us.ships = 0;

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;
  race.governor[0].active = true;
  race.governor[0].toggle.geography = false;
  race.governor[0].toggle.color = false;
  race.governor[0].toggle.inverse = false;
  race.governor[0].toggle.double_digits = false;
  race.governor[0].toggle.highlight = 1;
  race.discoveries[D_CRYSTAL] = true;

  RaceRepository races(store);
  races.save(race);

  // Create stable star
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "TestStar";
  ss0.xpos = 100.0;
  ss0.ypos = 200.0;
  ss0.stability = 40;          // Stable star (< 50)
  ss0.explored = (1ULL << 1);  // Player 1 has explored
  ss0.pnames.push_back("TestPlanet");
  Star star0(ss0);
  StarRepository stars_repo(store);
  stars_repo.save(star0);

  // Create planet on star 0
  Planet planet0{PlanetType::EARTH};
  planet0.star_id() = 0;
  planet0.planet_order() = 0;
  planet0.Maxx() = 5;
  planet0.Maxy() = 5;
  planet0.explored() = true;
  planet0.info(player_t{1}).numsectsowned = 3;
  planet0.info(player_t{1}).guns = 10;
  planet0.info(player_t{1}).mob_points = 100;
  planet0.info(player_t{1}).comread = 50;
  planet0.info(player_t{1}).mob_set = 75;
  planet0.info(player_t{1}).resource = 1000;
  planet0.info(player_t{1}).fuel = 500;
  planet0.info(player_t{1}).destruct = 25;
  planet0.info(player_t{1}).popn = 5000;
  planet0.info(player_t{1}).crystals = 10;
  planet0.info(player_t{1}).troops = 200;
  planet0.info(player_t{1}).tax = 10;
  planet0.info(player_t{1}).newtax = 12;
  planet0.info(player_t{1}).est_production = 150.5;
  planet0.conditions(TOXIC) = 25;

  PlanetRepository planets_repo(store);
  planets_repo.save(planet0);

  // Create sectormap for planet 0
  SectorMap smap(planet0, true);
  for (auto [coord, s] : smap.indexed_sectors()) {
    if (coord.x == 0 && coord.y == 0) {
      s.set_condition(SectorType::SEC_LAND);
      s.set_owner(1);
      s.set_popn_exact(100);
    } else if (coord.x == 1 && coord.y == 1) {
      s.set_condition(SectorType::SEC_SEA);
      s.set_owner(0);
      s.set_popn_exact(0);
    } else if (coord.x == 2 && coord.y == 2) {
      s.set_condition(SectorType::SEC_ICE);
      s.set_owner(2);
      s.set_popn_exact(50);
    } else if (coord.x == 3 && coord.y == 3) {
      s.set_condition(SectorType::SEC_LAND);
      s.set_owner(1);
      s.set_crystals(true);
      s.set_popn_exact(200);
    } else {
      s.set_condition(SectorType::SEC_LAND);
      s.set_owner(0);
      s.set_popn_exact(0);
    }
  }
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  // Create unstable star
  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "UnstableStar";
  ss1.xpos = 300.0;
  ss1.ypos = 400.0;
  ss1.stability = 75;  // Unstable (> 50)
  ss1.explored = (1ULL << 1);
  ss1.pnames.push_back("UnstablePlanet");
  Star star1(ss1);
  stars_repo.save(star1);

  // Create planet on star 1
  Planet planet1{PlanetType::EARTH};
  planet1.star_id() = 1;
  planet1.planet_order() = 0;
  planet1.Maxx() = 3;
  planet1.Maxy() = 3;
  planet1.explored() = true;
  planet1.info(player_t{1}).numsectsowned = 1;
  planets_repo.save(planet1);

  SectorMap usmap(planet1, true);
  for (Sector& s : usmap) {
    s.set_condition(SectorType::SEC_LAND);
  }
  sector_repo.save_map(usmap);
}

void test_map_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Happy path: Map at planet scope (stable star)
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_success(g, {"map"});
  assert(!g.out.str().contains("WARNING! This planet's primary is unstable."));
  std::println(std::cout, "    ✓ Map at planet scope (stable star) succeeded");

  // 2. Happy path: Map at planet scope (unstable star warning)
  g.set_snum(1);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"map"});
  assert(g.out.str().contains("WARNING! This planet's primary is unstable."));
  std::println(std::cout, "    ✓ Map displayed unstable star warning");

  // 3. Happy path: Map at universe level falls back to orbit
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"map"});
  std::println(std::cout, "    ✓ Map at universe level fell back to orbit");

  // 4. Bad scope: Map at ship level
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"map"});
  assert(g.out.str().contains("Bad scope"));
  std::println(std::cout, "    ✓ Map rejected at ship scope");
}

}  // namespace

int main() {
  test_map_dispatch();

  std::println(std::cout, "\n✅ All map tests passed!");
  return 0;
}
