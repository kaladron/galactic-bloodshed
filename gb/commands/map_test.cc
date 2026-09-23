// SPDX-License-Identifier: Apache-2.0

/// \file map_test.cc
/// \brief Unit tests for map command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create universe with 2 stars
  universe_struct us{};
  us.id = 1;
  us.numstars = 2;

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
  race.governor[0].toggle.inverse = false;
  race.governor[0].toggle.double_digits = false;
  race.governor[0].toggle.highlight = 1;
  race.discoveries.crystal = true;

  RaceRepository races(store);
  races.save(race);

  // Create stable star
  star_struct ss0{};
  ss0.star_id = 1;
  ss0.name = "TestStar";
  ss0.coordinates = {100.0, 200.0};
  ss0.stability = 40;  // Stable star (< 50)
  ss0.explored.set(player_t{1});
  ss0.pnames.push_back("TestPlanet");
  Star star0(ss0);
  StarRepository stars_repo(store);
  stars_repo.save(star0);

  // Create planet on star 1
  Planet planet0{PlanetType::EARTH, Coordinates{5, 5}};
  planet0.star_id() = 1;
  planet0.planet_order() = 1;
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
  planet0.toxic() = 25;

  PlanetRepository planets_repo(store);
  planets_repo.save(planet0);

  // Create sectormap for planet 1
  SectorMap smap(planet0);
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
  ss1.star_id = 2;
  ss1.name = "UnstableStar";
  ss1.coordinates = {300.0, 400.0};
  ss1.stability = 75;  // Unstable (> 50)
  ss1.explored.set(player_t{1});
  ss1.pnames.push_back("UnstablePlanet");
  Star star1(ss1);
  stars_repo.save(star1);

  // Create planet on star 2
  Planet planet1{PlanetType::EARTH, Coordinates{3, 3}};
  planet1.star_id() = 2;
  planet1.planet_order() = 1;
  planet1.explored() = true;
  planet1.info(player_t{1}).numsectsowned = 1;
  planets_repo.save(planet1);

  SectorMap usmap(planet1);
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
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_success(g, {"map"});
  test::expect_false(
      g.out.str().contains("WARNING! This planet's primary is unstable."));
  std::println(std::cout, "    ✓ Map at planet scope (stable star) succeeded");

  // 2. Happy path: Map at planet scope (unstable star warning)
  g.set_snum(2);
  g.set_pnum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"map"});
  test::expect_contains(g.out.str(),
                        "WARNING! This planet's primary is unstable.");
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
  test::expect_contains(g.out.str(), "Bad scope");
  std::println(std::cout, "    ✓ Map rejected at ship scope");
}

void test_sector_char_and_desshow_branches() {
  // 1. All 9 terrain characters + fail-fast on invalid SectorType
  test::expect_eq(get_sector_char(SectorType::SEC_SEA), CHAR_SEA);
  test::expect_eq(get_sector_char(SectorType::SEC_LAND), CHAR_LAND);
  test::expect_eq(get_sector_char(SectorType::SEC_MOUNT), CHAR_MOUNT);
  test::expect_eq(get_sector_char(SectorType::SEC_GAS), CHAR_GAS);
  test::expect_eq(get_sector_char(SectorType::SEC_ICE), CHAR_ICE);
  test::expect_eq(get_sector_char(SectorType::SEC_FOREST), CHAR_FOREST);
  test::expect_eq(get_sector_char(SectorType::SEC_DESERT), CHAR_DESERT);
  test::expect_eq(get_sector_char(SectorType::SEC_PLATED), CHAR_PLATED);
  test::expect_eq(get_sector_char(SectorType::SEC_WASTED), CHAR_WASTED);

  bool threw_on_invalid_sector = false;
  try {
    (void)get_sector_char(static_cast<SectorType>(99));
  } catch (const std::domain_error&) {
    threw_on_invalid_sector = true;
  }
  test::expect_true(threw_on_invalid_sector);

  // 2. desshow() troop symbols (own, allied, war, neutral)
  Race r{};
  r.Playernum = 1;
  r.allied.set(player_t{2});
  r.atwar.set(player_t{3});

  Sector s{};
  s.set_type(SectorType::SEC_MOUNT);
  s.set_condition(SectorType::SEC_LAND);
  test::expect_eq(s.type_symbol(), CHAR_MOUNT);
  test::expect_eq(s.condition_symbol(), CHAR_LAND);
  s.set_troops_exact(10);

  s.set_owner(1);
  test::expect_eq(desshow(1, 0, r, s), CHAR_MY_TROOPS);
  s.set_owner(2);
  test::expect_eq(desshow(1, 0, r, s), CHAR_ALLIED_TROOPS);
  s.set_owner(3);
  test::expect_eq(desshow(1, 0, r, s), CHAR_ATWAR_TROOPS);
  s.set_owner(4);
  test::expect_eq(desshow(1, 0, r, s), CHAR_NEUTRAL_TROOPS);

  // 3. desshow() owned digits (single digit, double digits on even/odd x,
  // inverse highlight, color toggle, geography toggle)
  s.set_troops_exact(0);
  s.set_owner(12);
  r.governor[0].toggle.double_digits = false;
  test::expect_eq(desshow(1, 0, r, s), '2');

  r.governor[0].toggle.double_digits = true;
  s.set_coords({0, 0});  // Even x -> tens digit ('1')
  test::expect_eq(desshow(1, 0, r, s), '1');
  s.set_coords({1, 0});  // Odd x -> ones digit ('2')
  test::expect_eq(desshow(1, 0, r, s), '2');

  // Inverse highlight on owner 12 falls through to crystal / terrain char
  r.governor[0].toggle.inverse = true;
  r.governor[0].toggle.highlight = 12;
  s.set_crystals(true);
  r.discoveries.crystal = false;
  r.God = true;
  test::expect_eq(desshow(1, 0, r, s), CHAR_CRYSTAL);
  r.God = false;
  test::expect_eq(desshow(1, 0, r, s), CHAR_LAND);
}

void test_show_map_rendering_options() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // Land a probe on planet 1 at (0,0) and configure color/inverse, high
  // toxicity, Metamorph, enslaved status, and alien war/peace presence
  const auto probe_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                            .owned_by(1, 0)
                            .landed_on(1, 1, Coordinates{0, 0})
                            .build();
  (void)probe_id;

  ctx.em.mutate_race(1, [](Race& r) {
    r.Metamorph = true;
    r.atwar.set(player_t{2});
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.toxic() = 75;
    p.enslave_to(player_t{2});
    p.info(player_t{2}).numsectsowned = 1;
    p.info(player_t{3}).numsectsowned = 1;
  });
  ctx.setup_game_obj(g, 1, 0);

  g.out.str("");
  show_map(g, 1, 1, *ctx.em.peek_planet(1, 1));
  test::expect_contains(g.out.str(), "Tons of biomass");
  test::expect_contains(g.out.str(), "(75% TOXIC)");
  test::expect_contains(g.out.str(), "ENSLAVED to player 2;");
  test::expect_contains(g.out.str(), "*2");

  // Test inverse highlight and unexplored planet ("Aliens:???")
  ctx.em.mutate_race(1, [](Race& r) {
    r.governor[0].toggle.inverse = true;
    r.governor[0].toggle.highlight = 1;
    r.tech = 0.0;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.explored() = false; });
  ctx.setup_game_obj(g, 1, 0);

  g.out.str("");
  show_map(g, 1, 1, *ctx.em.peek_planet(1, 1));
  test::expect_contains(g.out.str(), "Aliens:???");
}

}  // namespace

int main() {
  test_map_dispatch();
  test_sector_char_and_desshow_branches();
  test_show_map_rendering_options();

  std::println(std::cout, "\n✅ All map tests passed!");
  return 0;
}
