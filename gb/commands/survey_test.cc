// SPDX-License-Identifier: Apache-2.0

/// \file survey_test.cc
/// \brief Test survey command for edge cases and output formatting

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

void test_survey_no_args_planet_scope() {
  std::println("Test: survey command with no arguments at planet scope");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;
  race.conditions[METHANE] = 5;
  race.conditions[OXYGEN] = 20;
  race.conditions[CO2] = 10;
  race.conditions[HYDROGEN] = 5;
  race.conditions[NITROGEN] = 50;
  race.conditions[SULFUR] = 5;
  race.conditions[HELIUM] = 3;
  race.conditions[OTHER] = 2;
  race.conditions[TEMP] = 280;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Sol";
  star.xpos = 100.0;
  star.ypos = 200.0;
  star.gravity = 1.0;
  star.stability = 50;
  star.temperature = 5;
  star.pnames.push_back("Earth");

  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 10.0;
  planet.ypos() = 20.0;
  planet.conditions(METHANE) = 5;
  planet.conditions(OXYGEN) = 20;
  planet.conditions(CO2) = 10;
  planet.conditions(HYDROGEN) = 5;
  planet.conditions(NITROGEN) = 50;
  planet.conditions(SULFUR) = 5;
  planet.conditions(HELIUM) = 3;
  planet.conditions(OTHER) = 2;
  planet.conditions(TEMP) = 280;
  planet.conditions(RTEMP) = 280;
  planet.conditions(TOXIC) = 15;
  planet.info(0).numsectsowned = 5;
  planet.info(0).fuel = 1000;
  planet.info(0).resource = 2000;
  planet.info(0).destruct = 100;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create a sector map (required for getsmap in survey)
  SectorMap smap(planet, true);
  putsmap(smap, planet);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 1;
  g.pnum = 0;

  // TEST: Survey command with no arguments - should show full planet survey
  std::println("  Testing: survey (no args) at planet scope");
  {
    command_t cmd = {"survey"};
    GB::commands::survey(cmd, g);

    // Verify output contains expected information
    std::string out_str = g.out.str();
    // Note: notify() output doesn't go to g.out, so we can't check for planet
    // name Check for information that IS written to g.out
    assert(out_str.find("======== Planetary conditions: ========") !=
           std::string::npos);
    assert(out_str.find("atmosphere concentrations") != std::string::npos);
    std::println("    ✓ Output contains planet survey information");
  }

  std::println("  ✅ Survey with no args test passed!");
}

void test_survey_sector_mode_no_args() {
  std::println("Test: survey command in CSP mode with no arguments");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Sol";
  star.pnames.push_back("Earth");

  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(0).numsectsowned = 5;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create a sector map (required for getsmap in survey)
  SectorMap smap(planet, true);
  putsmap(smap, planet);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 1;
  g.pnum = 0;

  // TEST: Use 'map' command (mode=1) with no arguments - THIS IS THE BUG!
  // Previously this would access argv[1] without checking argv.size()
  std::println("  Testing: map (no args) at planet scope - bug fix check");
  {
    command_t cmd = {"map"};  // map uses mode=1
    GB::commands::survey(cmd, g);

    // Should not crash and should produce valid output
    std::string out_str = g.out.str();
    // The command should work without segfaulting
    std::println("    ✓ Command executed without crash (bug fix verified)");
  }

  std::println("  ✅ Survey CSP mode test passed!");
}

void test_survey_sector_range_with_header() {
  std::println("Test: survey command with sector range shows header");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;
  race.conditions[TEMP] = 280;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Sol";
  star.pnames.push_back("Earth");

  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet with sectors
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.conditions(TOXIC) = 10;
  planet.info(0).numsectsowned = 5;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create some sectors
  SectorMap smap(planet, true);
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
      auto& s = smap.get(x, y);
      s.set_condition(SectorType::SEC_LAND);
      s.set_type(SectorType::SEC_LAND);
      s.set_owner(1);
      s.set_race(1);
      s.set_eff(50);
      s.set_mobilization(10);
      s.set_fert(60);
      s.set_resource(40);
      s.set_troops(100);
      s.set_popn(1000);
      s.set_crystals(false);
    }
  }

  // Save sector map using legacy putsmap function
  putsmap(smap, planet);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 1;
  g.pnum = 0;

  // TEST: Survey specific sector range in non-CSP mode
  std::println("  Testing: survey 0:2,0:2 (should show header)");
  {
    command_t cmd = {"survey", "0:2,0:2"};
    GB::commands::survey(cmd, g);

    // Verify output contains the header line (tabulate format)
    std::string out_str = g.out.str();
    assert(out_str.find("x,y") != std::string::npos);
    assert(out_str.find("cond/type") != std::string::npos);
    assert(out_str.find("owner") != std::string::npos);
    assert(out_str.find("xtals") != std::string::npos);
    assert(out_str.find("0,0") != std::string::npos);
    std::println("    ✓ Output contains header and sector data");
  }

  std::println("  ✅ Sector range with header test passed!");
}

void test_survey_star_scope() {
  std::println("Test: survey command at star scope");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 60.0;  // Above TECH_SEE_STABILITY (50)

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Sol";
  star.xpos = 100.0;
  star.ypos = 200.0;
  star.gravity = 1.0;
  star.stability = 45;
  star.temperature = 5;
  star.pnames.push_back("Mercury");
  star.pnames.push_back("Venus");
  star.pnames.push_back("Earth");

  StarRepository stars(store);
  stars.save(star);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_STAR;
  g.snum = 1;

  // TEST: Survey command at star scope
  std::println("  Testing: survey at star scope");
  {
    command_t cmd = {"survey"};
    GB::commands::survey(cmd, g);

    // Verify output contains expected information
    std::string out_str = g.out.str();
    assert(out_str.find("Star Sol") != std::string::npos);
    assert(out_str.find("100,200") != std::string::npos);
    assert(out_str.find("Gravity") != std::string::npos);
    assert(out_str.find("Instability") != std::string::npos);
    assert(out_str.find("45%") != std::string::npos);  // Stability value
    assert(out_str.find("planets are") != std::string::npos);
    std::println("    ✓ Output contains star information");
  }

  std::println("  ✅ Star scope survey test passed!");
}

int main() {
  test_survey_no_args_planet_scope();
  test_survey_sector_mode_no_args();
  test_survey_sector_range_with_header();
  test_survey_star_scope();
  std::println("\n✅ All survey tests passed!");
  return 0;
}
