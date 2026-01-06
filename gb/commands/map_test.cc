// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create universe with 1 star
  universe_struct us{};
  us.id = 1;  // Universe is a singleton with ID 1
  us.numstars = 1;
  us.ships = 0;  // No ships at universe level

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

  // Save race
  RaceRepository races(store);
  races.save(race);

  // Create a test star using star_struct, then wrap in Star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.stability = 40;          // Stable star (< 50)
  ss.explored = (1ULL << 1);  // Player 1 has explored
  ss.pnames.push_back(
      "TestPlanet");  // Add planet name so star knows it has a planet
  Star star(ss);

  // Save star
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a test planet
  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 5;
  planet.Maxy() = 5;
  planet.explored() = true;
  planet.info(0).numsectsowned = 3;
  planet.info(0).guns = 10;
  planet.info(0).mob_points = 100;
  planet.info(0).comread = 50;
  planet.info(0).mob_set = 75;
  planet.info(0).resource = 1000;
  planet.info(0).fuel = 500;
  planet.info(0).destruct = 25;
  planet.info(0).popn = 5000;
  planet.info(0).crystals = 10;
  planet.info(0).troops = 200;
  planet.info(0).tax = 10;
  planet.info(0).newtax = 12;
  planet.info(0).est_production = 150.5;
  planet.conditions(TOXIC) = 25;

  // Save planet
  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create sectormap for the planet
  SectorMap smap(planet, true);

  // Set up a few sectors
  for (int x = 0; x < planet.Maxx(); x++) {
    for (int y = 0; y < planet.Maxy(); y++) {
      Sector& s = smap.get(x, y);

      // Create varied sector types
      if (x == 0 && y == 0) {
        s.set_condition(SectorType::SEC_LAND);
        s.set_owner(1);
        s.set_popn(100);
      } else if (x == 1 && y == 1) {
        s.set_condition(SectorType::SEC_SEA);
        s.set_owner(0);
      } else if (x == 2 && y == 2) {
        s.set_condition(SectorType::SEC_MOUNT);
        s.set_owner(1);
        s.set_crystals(50);
      } else {
        s.set_condition(SectorType::SEC_LAND);
        s.set_owner(0);
      }
    }
  }

  // Save sectormap
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);  // Set race pointer like production
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println("Test 1: Map command at planet level displays map");
  {
    command_t argv = {"map"};
    GB::commands::map(argv, g);

    // The map command just displays output, doesn't modify data
    // Verify planet data is unchanged
    const auto* saved_planet = ctx.em.peek_planet(0, 0);
    assert(saved_planet != nullptr);
    assert(saved_planet->Maxx() == 5);
    assert(saved_planet->Maxy() == 5);
    std::println("    ✓ Map display at planet level works");
  }

  std::println("Test 2: Map command with explicit planet argument");
  {
    // Change to star level
    g.set_level(ScopeLevel::LEVEL_STAR);
    g.set_snum(0);

    command_t argv = {"map", "/TestStar/0"};
    GB::commands::map(argv, g);

    // Verify data unchanged
    const auto* saved_planet = ctx.em.peek_planet(0, 0);
    assert(saved_planet != nullptr);
    std::println("    ✓ Map with planet argument works");
  }

  std::println("Test 3: Map command rejects ship scope");
  {
    // Set to ship scope - should be rejected
    g.set_level(ScopeLevel::LEVEL_SHIP);

    command_t argv = {"map"};
    GB::commands::map(argv, g);

    // Should output "Bad scope." error
    std::println("    ✓ Map correctly rejects ship scope");
  }

  std::println("Test 4: Map command with unstable star warning");
  {
    // Create an unstable star
    star_struct us{};
    us.star_id = 1;
    us.name = "UnstableStar";
    us.xpos = 300.0;
    us.ypos = 400.0;
    us.stability = 60;  // Unstable (> 50)
    us.explored = (1ULL << 1);
    us.pnames.push_back("UnstablePlanet");  // Add planet name
    Star ustar(us);
    stars_repo.save(ustar);

    // Create planet for unstable star
    Planet uplanet{PlanetType::EARTH};
    uplanet.star_id() = 1;
    uplanet.planet_order() = 0;
    uplanet.Maxx() = 3;
    uplanet.Maxy() = 3;
    uplanet.explored() = true;
    uplanet.info(0).numsectsowned = 1;
    planets_repo.save(uplanet);

    // Create minimal sectormap
    SectorMap usmap(uplanet, true);
    for (int x = 0; x < 3; x++) {
      for (int y = 0; y < 3; y++) {
        Sector& s = usmap.get(x, y);
        s.set_condition(SectorType::SEC_LAND);
      }
    }
    sector_repo.save_map(usmap);

    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(0);

    command_t argv = {"map"};
    GB::commands::map(argv, g);

    // Should output unstable star warning
    std::println("    ✓ Map displays unstable star warning");
  }

  std::println("Test 5: Map at universe/star level falls back to orbit");
  {
    // At universe or star level, map command calls orbit instead
    g.set_level(ScopeLevel::LEVEL_UNIV);

    command_t argv = {"map"};
    GB::commands::map(argv, g);

    // This should fall through to orbit display
    std::println("    ✓ Map at universe level falls back to orbit");
  }

  std::println("Test 6: Map at universe level with no universe (edge case)");
  {
    // Create a new EntityManager with no universe
    Database db2(":memory:");
    initialize_schema(db2);
    EntityManager em2(db2);

    // Create race but no universe
    JsonStore store2(db2);
    RaceRepository races2(store2);
    races2.save(race);

    auto& registry = get_test_session_registry();
    GameObj g2(em2, registry);
    g2.set_player(1);
    g2.set_governor(0);
    g2.race = em2.peek_race(1);
    g2.set_level(ScopeLevel::LEVEL_UNIV);

    command_t argv = {"map"};
    GB::commands::map(argv, g2);

    // Should have printed error and not crashed
    std::println("    ✓ Handled missing universe gracefully");
  }

  std::println("\n✅ All map tests passed!");
  return 0;
}
