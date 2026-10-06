// SPDX-License-Identifier: Apache-2.0

/// \file map_test.cc
/// \brief Unit tests for map command

import commands;
import dallib;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create universe
  universe_struct us{};

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;
  race.leader().toggle.geography = false;
  race.leader().toggle.inverse = false;
  race.leader().toggle.double_digits = false;
  race.leader().toggle.highlight = 1;
  race.discoveries.crystal = true;

  RaceRepository races(store);
  races.save(race);

  Race race2{};
  race2.Playernum = 2;
  race2.name = "EnemyRace";
  races.save(race2);

  Race race3{};
  race3.Playernum = 3;
  race3.name = "NeutralRace";
  races.save(race3);

  // Create stable star
  Star star0{1, "TestStar", {100.0, 200.0}};
  star0.stability() = 40;  // Stable star (< 50)
  star0.mark_explored_by(1);
  star0.set_planet_name(1, "TestPlanet");
  StarRepository stars_repo(store);
  stars_repo.save(star0);

  // Create planet on star 1
  Planet planet0{1, 1, PlanetType::EARTH, Coordinates{5, 5}};
  planet0.explored() = true;
  planet0.info(player_t{1}).explored = 1;
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
  Star star1{2, "UnstableStar", {300.0, 400.0}};
  star1.stability() = 75;  // Unstable (> 50)
  star1.mark_explored_by(1);
  star1.set_planet_name(1, "UnstablePlanet");
  stars_repo.save(star1);

  // Create planet on star 2
  Planet planet1{2, 1, PlanetType::EARTH, Coordinates{3, 3}};
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
  ctx.setup_game_obj(g, 1, 1);

  // 1. Happy path: Map at planet scope (stable star, ASCII mode)
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_success(g, {"map"});
  test::expect_contains(g.out.str(), "     TestPlanet\n");
  test::expect_contains(g.out.str(), "   01234\n");
  test::expect_contains(g.out.str(), "00 1****\n");
  test::expect_false(g.out.str().contains("$TestPlanet"));
  test::expect_false(
      g.out.str().contains("WARNING! This planet's primary is unstable."));
  std::println(std::cout, "    ✓ Map at planet scope (stable star) succeeded");

  // 1b. JSON mode at planet scope
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"map"});
  test::expect_contains(g.out.str(), "\"type\":\"map\"");
  test::expect_contains(g.out.str(), "\"planet_name\":\"TestPlanet\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Map in JSON mode succeeded");

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

  // 5. Explicit planet path argument and invalid path error reporting
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"map", "/TestStar/TestPlanet"});
  test::expect_contains(g.out.str(), "TestPlanet");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"map", "/TestStar/NoSuchPlanet"});
  test::expect_contains(g.out.str(), "No such planet NoSuchPlanet.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"map", "/NoSuchStar"});
  test::expect_contains(g.out.str(), "No such star NoSuchStar.");
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

  // 2. desshow() troop symbols (own, allied, war, neutral) and geography toggle
  Race r{};
  r.Playernum = 1;
  r.declare_alliance_with(player_t{2});
  r.declare_war_on(player_t{3});

  Sector s{};
  s.set_type(SectorType::SEC_MOUNT);
  s.set_condition(SectorType::SEC_LAND);
  test::expect_eq(s.type_symbol(), CHAR_MOUNT);
  test::expect_eq(s.condition_symbol(), CHAR_LAND);
  s.set_troops_exact(10);

  s.set_owner(1);
  test::expect_eq(desshow(1, 1, r, s), CHAR_MY_TROOPS);
  s.set_owner(2);
  test::expect_eq(desshow(1, 1, r, s), CHAR_ALLIED_TROOPS);
  s.set_owner(3);
  test::expect_eq(desshow(1, 1, r, s), CHAR_ATWAR_TROOPS);
  s.set_owner(4);
  test::expect_eq(desshow(1, 1, r, s), CHAR_NEUTRAL_TROOPS);

  // Geography toggle hides both troops and owner digits
  r.leader().toggle.geography = true;
  test::expect_eq(desshow(1, 1, r, s), CHAR_LAND);
  s.set_troops_exact(0);
  test::expect_eq(desshow(1, 1, r, s), CHAR_LAND);
  r.leader().toggle.geography = false;

  // 3. desshow() owned digits (single digit, double digits on even/odd x,
  // owner_val < 10 with double_digits, inverse highlight)
  s.set_owner(12);
  r.leader().toggle.double_digits = false;
  test::expect_eq(desshow(1, 1, r, s), '2');

  r.leader().toggle.double_digits = true;
  s.set_coords({0, 0});  // Even x, owner 12 >= 10 -> tens digit ('1')
  test::expect_eq(desshow(1, 1, r, s), '1');
  s.set_coords({1, 0});  // Odd x, owner 12 >= 10 -> ones digit ('2')
  test::expect_eq(desshow(1, 1, r, s), '2');

  // Even x, owner_val < 10 with double_digits -> ones digit ('4')
  s.set_coords({0, 0});
  s.set_owner(4);
  test::expect_eq(desshow(1, 1, r, s), '4');

  // Inverse = true, but owner (4) != highlight (12) -> still shows digit '4'
  r.leader().toggle.inverse = true;
  r.leader().toggle.highlight = 12;
  test::expect_eq(desshow(1, 1, r, s), '4');

  // Inverse highlight on owner 12 falls through to crystal / terrain char
  s.set_owner(12);
  s.set_crystals(true);
  r.discoveries.crystal = true;
  r.God = false;
  test::expect_eq(desshow(1, 1, r, s), CHAR_CRYSTAL);
  r.discoveries.crystal = false;
  r.God = true;
  test::expect_eq(desshow(1, 1, r, s), CHAR_CRYSTAL);
  r.God = false;
  test::expect_eq(desshow(1, 1, r, s), CHAR_LAND);
}

void test_show_map_rendering_options() {
  TestContext ctx;
  setup_test_world(ctx);

  // Land a probe on planet 1 at (0,0) and configure high toxicity, Metamorph,
  // enslaved status, and alien war/peace presence
  const auto probe_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                            .owned_by(1, 1)
                            .landed_on(1, 1, Coordinates{0, 0})
                            .build();
  (void)probe_id;

  ctx.em.mutate_race(1, [](Race& r) {
    r.Metamorph = true;
    r.declare_war_on(player_t{2});
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.toxic() = 75;
    p.enslave_to(player_t{2});
    p.info(player_t{2}).numsectsowned = 1;
    p.info(player_t{3}).numsectsowned = 1;
  });

  const auto* race1 = ctx.em.peek_race(1);
  const auto* planet1 = ctx.em.peek_planet(1, 1);
  const auto vm1 = build_planet_map(ctx.em, 1, 1, *planet1, 1, 1, *race1);
  const std::string out1 = GB::presentation::render_ascii_planet_map(vm1);
  test::expect_contains(out1, "Tons of biomass");
  test::expect_contains(out1, "(75% TOXIC)");
  test::expect_contains(out1, "ENSLAVED to player 2;");
  test::expect_contains(out1, "*2 3");
  // Probe ':' at (0, 0) is shown because player has visual IQ
  test::expect_contains(out1, "00 :****\n");

  // Test inverse highlight (ANSI SGR reverse video \x1b[7m...\x1b[27m) and
  // unexplored planet with low tech ("Aliens:???")
  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().toggle.inverse = true;
    r.leader().toggle.highlight = 1;
    r.tech = 0.0;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.explored() = false; });

  const auto vm2 = build_planet_map(ctx.em, 1, 1, *ctx.em.peek_planet(1, 1), 1,
                                    1, *ctx.em.peek_race(1));
  const std::string out2 = GB::presentation::render_ascii_planet_map(vm2);
  test::expect_contains(out2, "Aliens:???");
  // Sector (0,0) and (3,3) are owned by player 1 (highlight=1, inverse=true)
  test::expect_contains(out2, "00 \x1b[7m:\x1b[27m****\n");
  test::expect_contains(out2, "03 ***\x1b[7mx\x1b[27m*\n");

  // Unexplored planet with high tech (>= TECH_EXPLORE) still reveals aliens
  ctx.em.mutate_race(1, [](Race& r) { r.tech = TECH_EXPLORE; });
  const auto vm3 = build_planet_map(ctx.em, 1, 1, *ctx.em.peek_planet(1, 1), 1,
                                    1, *ctx.em.peek_race(1));
  test::expect_false(vm3.aliens_unknown);
  test::expect_eq(vm3.aliens.size(), 2UZ);
}

void test_ship_visual_iq_and_wide_map() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create a wide planet (12x2) on star 1, planet 2 where player 1 owns 0
  // sectors
  ctx.em.mutate_star(1, [](Star& s) { s.set_planet_name(2, "WidePlanet"); });
  Planet wide{1, 2, PlanetType::EARTH, Coordinates{12, 2}};
  wide.explored() = true;
  wide.info(player_t{1}).numsectsowned = 0;
  JsonStore store(ctx.db);
  PlanetRepository(store).save(wide);
  SectorMap wide_smap(wide);
  for (Sector& s : wide_smap) {
    s.set_condition(SectorType::SEC_LAND);
  }
  SectorRepository(store).save_map(wide_smap);

  // 1. Enemy probe, unauthorized governor probe (owned by gov 2 when queried by
  // subordinate gov 3), and unmanned landed pod do NOT grant visual IQ
  ctx.em.mutate_race(1, [](Race& r) {
    r.appoint_governor(2);
    auto& gov3 = r.appoint_governor(3);
    gov3.toggle.inverse = false;
  });
  (void)TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
      .owned_by(2, 1)
      .landed_on(1, 2, Coordinates{1, 0})
      .build();
  (void)TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
      .owned_by(1, 2)
      .landed_on(1, 2, Coordinates{2, 0})
      .build();
  // Unmanned non-probe ship landed at (3, 0) and another at out-of-bounds (99,
  // 99)
  (void)TestShipBuilder(ctx.em, ShipType::STYPE_POD)
      .owned_by(1, 3)
      .with_crew(0, 0)
      .landed_on(1, 2, Coordinates{3, 0})
      .build();
  (void)TestShipBuilder(ctx.em, ShipType::STYPE_POD)
      .owned_by(1, 3)
      .with_crew(0, 0)
      .landed_on(1, 2, Coordinates{99, 99})
      .build();

  const auto vm_no_iq = build_planet_map(
      ctx.em, 1, 2, *ctx.em.peek_planet(1, 2), 1, 3, *ctx.em.peek_race(1));
  // Without visual IQ, landed ships at (1,0), (2,0), (3,0) are hidden ('*')
  test::expect_eq(vm_no_iq.sectors[1].glyph, CHAR_LAND);
  test::expect_eq(vm_no_iq.sectors[2].glyph, CHAR_LAND);
  test::expect_eq(vm_no_iq.sectors[3].glyph, CHAR_LAND);

  // Wide map (>= 10 columns) renders tens-digit and ones-digit X coordinate
  // headers
  const std::string wide_ascii =
      GB::presentation::render_ascii_planet_map(vm_no_iq);
  test::expect_contains(wide_ascii, "   000000000011\n   012345678901\n");

  // 2. Orbiting crewed ship owned by player 1, governor 3 grants visual IQ
  (void)TestShipBuilder(ctx.em, ShipType::STYPE_POD)
      .owned_by(1, 3)
      .with_crew(5, 0)
      .in_planet_orbit(1, 2)
      .build();
  const auto vm_with_iq = build_planet_map(
      ctx.em, 1, 2, *ctx.em.peek_planet(1, 2), 1, 3, *ctx.em.peek_race(1));
  // Now landed ships at (1,0), (2,0), (3,0) are visible
  test::expect_eq(vm_with_iq.sectors[1].glyph, ':');
  test::expect_eq(vm_with_iq.sectors[2].glyph, ':');
  test::expect_eq(vm_with_iq.sectors[3].glyph, 'p');

  // 3. Geography toggle skips ship scanning entirely
  ctx.em.mutate_race(1, [](Race& r) { r.governor(3).toggle.geography = true; });
  const auto vm_geo = build_planet_map(ctx.em, 1, 2, *ctx.em.peek_planet(1, 2),
                                       1, 3, *ctx.em.peek_race(1));
  test::expect_eq(vm_geo.sectors[1].glyph, CHAR_LAND);
}

}  // namespace

int main() {
  test_map_dispatch();
  test_sector_char_and_desshow_branches();
  test_show_map_rendering_options();
  test_ship_visual_iq_and_wide_map();

  std::println(std::cout, "\n✅ All map tests passed!");
  return 0;
}
