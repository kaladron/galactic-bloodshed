// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create universe with 1 star
  universe_struct us{};
  us.id = 1;
  us.numstars = 1;
  us.ships = 0;

  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Create test race with tech high enough to see crystals
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.tech = 100.0;  // High tech to see crystals
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Create a second race for testing multi-player sectors
  Race race2{};
  race2.Playernum = 2;
  race2.name = "EnemyRace";
  race2.Guest = false;
  race2.governor[0].active = true;
  races.save(race2);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.pnames.emplace_back("TestPlanet");
  ss.explored = (1ULL << 1) | (1ULL << 2);  // Players 1 and 2 explored
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a test planet with a 5x5 grid
  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 5;
  planet.Maxy() = 5;
  planet.explored() = true;
  planet.info(player_t{1}).explored = true;  // Player 1 has explored
  planet.info(player_t{2}).explored = true;  // Player 2 has explored

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create sector map with varied data for testing table output
  SectorMap smap(planet, true);

  // Set up sectors with different owners, types, and values
  // Player 1 owns some sectors
  smap.get(0, 0).set_owner(1);
  smap.get(0, 0).set_eff(80);
  smap.get(0, 0).set_mobilization(50);
  smap.get(0, 0).set_resource(100);
  smap.get(0, 0).set_popn(1000);
  smap.get(0, 0).set_troops(10);
  smap.get(0, 0).set_condition(SectorType::SEC_LAND);

  smap.get(1, 0).set_owner(1);
  smap.get(1, 0).set_eff(90);
  smap.get(1, 0).set_mobilization(60);
  smap.get(1, 0).set_resource(150);
  smap.get(1, 0).set_popn(2000);
  smap.get(1, 0).set_troops(20);
  smap.get(1, 0).set_condition(SectorType::SEC_MOUNT);

  smap.get(2, 0).set_owner(1);
  smap.get(2, 0).set_eff(70);
  smap.get(2, 0).set_mobilization(40);
  smap.get(2, 0).set_resource(80);
  smap.get(2, 0).set_popn(500);
  smap.get(2, 0).set_troops(5);
  smap.get(2, 0).set_condition(SectorType::SEC_FOREST);

  // Player 2 owns some sectors
  smap.get(0, 1).set_owner(2);
  smap.get(0, 1).set_eff(60);
  smap.get(0, 1).set_mobilization(30);
  smap.get(0, 1).set_resource(50);
  smap.get(0, 1).set_popn(800);
  smap.get(0, 1).set_troops(8);
  smap.get(0, 1).set_condition(SectorType::SEC_SEA);

  smap.get(1, 1).set_owner(2);
  smap.get(1, 1).set_eff(50);
  smap.get(1, 1).set_mobilization(25);
  smap.get(1, 1).set_resource(40);
  smap.get(1, 1).set_popn(600);
  smap.get(1, 1).set_troops(6);
  smap.get(1, 1).set_condition(SectorType::SEC_ICE);

  // Unowned sectors
  smap.get(2, 1).set_owner(0);
  smap.get(2, 1).set_eff(0);
  smap.get(2, 1).set_resource(200);
  smap.get(2, 1).set_condition(SectorType::SEC_GAS);

  smap.get(0, 2).set_owner(0);
  smap.get(0, 2).set_eff(0);
  smap.get(0, 2).set_resource(75);
  smap.get(0, 2).set_condition(SectorType::SEC_DESERT);

  // Leave the rest as default (unowned, sea type)
  for (int y = 2; y < 5; y++) {
    for (int x = (y == 2 ? 1 : 0); x < 5; x++) {
      smap.get(x, y).set_owner(0);
      smap.get(x, y).set_eff(0);
      smap.get(x, y).set_resource(10 + x + y);
      smap.get(x, y).set_condition(SectorType::SEC_SEA);
    }
  }

  SectorRepository sectormap_repo(store);
  sectormap_repo.save_map(smap);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println("\n========== Analysis Command Output Test ==========\n");

  std::println("Test 1: Basic analysis (all sectors)");
  {
    command_t argv = {"analysis"};
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    // Bug fix verification: "owned by 4294967295" was shown when ThisPlayer
    // was player_t (unsigned) and -1 was used to mean "all players"
    assert(output.find("4294967295") == std::string::npos &&
           "Bug: player_t -1 overflow should not appear in output");

    // Bug fix verification: Top 5 lists should not be empty when we have
    // valid sector data. The insert() function was taking array by value
    // instead of by reference, so insertions were lost.
    assert(output.find("Troops:") != std::string::npos);
    // Find "Troops:" and check there's content after the colon on that line
    auto troops_pos = output.find("Troops:");
    assert(troops_pos != std::string::npos);
    auto troops_line_end = output.find('\n', troops_pos);
    auto troops_line = output.substr(troops_pos, troops_line_end - troops_pos);
    // Line should have more than just "Troops:" - it should have sector data
    assert(troops_line.length() > 10 &&
           "Bug: insert() by-value bug would leave top 5 lists empty");

    g.out.str("");  // Clear for next test
  }

  std::println("\n========================================\n");
  std::println("Test 2: Analysis with bottom 5 mode");
  {
    command_t argv = {"analysis", "-"};
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    // Verify "Lowest" appears (not "Highest")
    assert(output.find("Lowest") != std::string::npos &&
           "Bottom mode should show 'Lowest'");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 3: Analysis filtered to ocean sectors only");
  {
    command_t argv = {"analysis", "."};  // . is sea
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    // Verify sector type filter is shown
    assert(output.find("Ocean") != std::string::npos &&
           "Sea filter (.) should show 'Ocean' in output");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 4: Analysis filtered to land sectors only");
  {
    command_t argv = {"analysis", "*"};  // * is land
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    // Verify sector type filter is shown
    assert(output.find("Land") != std::string::npos &&
           "Land filter (*) should show 'Land' in output");
    // Should only show land sector data
    assert(output.find("*( 0, 0)") != std::string::npos &&
           "Should show land sector coordinates");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 5: Analysis filtered to mountain sectors only");
  {
    command_t argv = {"analysis", "^"};  // ^ is mountain
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    assert(output.find("Mountain") != std::string::npos &&
           "Mountain filter (^) should show 'Mountain' in output");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 6: Analysis filtered to desert sectors (special 'd')");
  {
    command_t argv = {"analysis", "d"};  // 'd' is desert (special case)
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    assert(output.find("Desert") != std::string::npos &&
           "Desert filter (d) should show 'Desert' in output");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 7: Analysis with player filter");
  {
    command_t argv = {"analysis", "1"};  // Filter to player 1
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    assert(output.find("sectors owned by 1") != std::string::npos &&
           "Player filter should show 'sectors owned by 1' in output");

    g.out.str("");
  }

  std::println("\n========================================\n");
  std::println("Test 8: Combined sector type and player filter");
  {
    command_t argv = {"analysis", "*", "1"};  // Land sectors owned by player 1
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    std::println("\n--- Output ---");
    std::println("{}", output);

    assert(output.find("Land") != std::string::npos &&
           "Should filter by land sectors");
    // Note: When both sector type and player are specified, the current
    // implementation shows the sector type but not the player in the header.
    // The filtering still happens correctly (only land sectors from player 1
    // are shown in the top 5 lists).

    g.out.str("");
  }

  std::println("\n========== Tests Complete ==========\n");
  std::println("All analysis command tests passed!");

  // PlayerFilter logic tests - tested indirectly through command behavior
  std::println("\n========== PlayerFilter Logic Tests (via command behavior) "
               "==========\n");

  // Test AllPlayers mode (default, no player filter)
  {
    std::println("Test: Default filter matches all owners");
    command_t argv = {"analysis"};
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    // Should see stats for all players (1, 2, and 0 for unoccupied)
    assert(output.find(" sectors.\n") != std::string::npos &&
           "Default should show generic description");
    // Check that the table includes multiple players
    assert(output.find("Pl") != std::string::npos &&
           "Should have player column");

    g.out.str("");
    std::println("✓ AllPlayers mode (default) works correctly");
  }

  // Test Unoccupied mode (player 0)
  {
    std::println("Test: Player 0 filter matches only unoccupied");
    command_t argv = {"analysis", "0"};
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    assert(output.find("unoccupied") != std::string::npos &&
           "Should show unoccupied description");

    g.out.str("");
    std::println("✓ Unoccupied mode (player 0) works correctly");
  }

  // Test SpecificPlayer mode
  {
    std::println("Test: Specific player filter matches only that player");
    command_t argv = {"analysis", "1"};
    GB::commands::analysis(argv, g);

    std::string output = g.out.str();
    assert(output.find("owned by 1") != std::string::npos &&
           "Should show player 1 in description");

    g.out.str("");

    // Try player 2
    command_t argv2 = {"analysis", "2"};
    GB::commands::analysis(argv2, g);

    output = g.out.str();
    assert(output.find("owned by 2") != std::string::npos &&
           "Should show player 2 in description");

    g.out.str("");
    std::println("✓ SpecificPlayer mode works correctly for different players");
  }

  std::println("\n========== PlayerFilter Logic Tests Complete ==========\n");

  return 0;
}
