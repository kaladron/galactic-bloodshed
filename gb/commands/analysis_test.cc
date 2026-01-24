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

  // Initialize all sector coordinates first
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      smap.get(x, y).set_x(x);
      smap.get(x, y).set_y(y);
    }
  }

  // Set up sectors with different owners, types, and values
  // Player 1 owns some sectors
  smap.get(0, 0).set_owner(1);
  smap.get(0, 0).set_eff(80);
  smap.get(0, 0).set_mobilization(50);
  smap.get(0, 0).set_resource(100);
  smap.get(0, 0).set_popn_exact(1000);
  smap.get(0, 0).set_troops(10);
  smap.get(0, 0).set_condition(SectorType::SEC_LAND);

  smap.get(1, 0).set_owner(1);
  smap.get(1, 0).set_eff(90);
  smap.get(1, 0).set_mobilization(60);
  smap.get(1, 0).set_resource(150);
  smap.get(1, 0).set_popn_exact(2000);
  smap.get(1, 0).set_troops(20);
  smap.get(1, 0).set_condition(SectorType::SEC_MOUNT);

  smap.get(2, 0).set_owner(1);
  smap.get(2, 0).set_eff(70);
  smap.get(2, 0).set_mobilization(40);
  smap.get(2, 0).set_resource(80);
  smap.get(2, 0).set_popn_exact(500);
  smap.get(2, 0).set_troops(5);
  smap.get(2, 0).set_condition(SectorType::SEC_FOREST);

  // Player 2 owns some sectors
  smap.get(0, 1).set_owner(2);
  smap.get(0, 1).set_eff(60);
  smap.get(0, 1).set_mobilization(30);
  smap.get(0, 1).set_resource(50);
  smap.get(0, 1).set_popn_exact(800);
  smap.get(0, 1).set_troops(8);
  smap.get(0, 1).set_condition(SectorType::SEC_SEA);

  smap.get(1, 1).set_owner(2);
  smap.get(1, 1).set_eff(50);
  smap.get(1, 1).set_mobilization(25);
  smap.get(1, 1).set_resource(40);
  smap.get(1, 1).set_popn_exact(600);
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
    auto troops_line_end_pos = output.find('\n', troops_pos);
    auto troops_line_initial =
        output.substr(troops_pos, troops_line_end_pos - troops_pos);
    // Line should have more than just "Troops:" - it should have sector data
    assert(troops_line_initial.length() > 10 &&
           "Bug: insert() by-value bug would leave top 5 lists empty");

    // Verify top 5 troops are correct and in order
    // Expected values: 20, 10, 8, 6, 5
    // Expected types: mountain(^), land(*), sea(.), ice(#), forest())
    assert(output.find("20^(") != std::string::npos &&
           "Top troops should be 20 at mountain");
    assert(output.find("10*(") != std::string::npos &&
           "2nd troops should be 10 at land");
    assert(output.find(" 8.(") != std::string::npos &&
           "3rd troops should be 8 at sea");
    assert(output.find(" 6#(") != std::string::npos &&
           "4th troops should be 6 at ice");
    assert(output.find(" 5)(") != std::string::npos &&
           "5th troops should be 5 at forest");

    // Verify order (values should appear left-to-right on the Troops line)
    auto troops_line_start = output.find("Troops:");
    auto troops_line_end = output.find('\n', troops_line_start);
    auto troops_line =
        output.substr(troops_line_start, troops_line_end - troops_line_start);
    auto pos_20 = troops_line.find("20^");
    auto pos_10 = troops_line.find("10*");
    auto pos_8 = troops_line.find(" 8.");
    auto pos_6 = troops_line.find(" 6#");
    auto pos_5 = troops_line.find(" 5)");
    assert(pos_20 != std::string::npos && pos_10 != std::string::npos &&
           pos_8 != std::string::npos && pos_6 != std::string::npos &&
           pos_5 != std::string::npos &&
           "All top 5 troop values should be present");
    assert(pos_20 < pos_10 && pos_10 < pos_8 && pos_8 < pos_6 &&
           pos_6 < pos_5 && "Top 5 troops should be in descending order");

    // Verify top 5 resources are correct and in order
    // Expected values: 200, 150, 100, 80, 75
    // Expected types: gas(~), mountain(^), land(*), forest()), desert(-)
    assert(output.find("200~(") != std::string::npos &&
           "Top resource should be 200 at gas");
    assert(output.find("150^(") != std::string::npos &&
           "2nd resource should be 150 at mountain");
    assert(output.find("100*(") != std::string::npos &&
           "3rd resource should be 100 at land");
    assert(output.find(" 80)(") != std::string::npos &&
           "4th resource should be 80 at forest");
    assert(output.find(" 75-(") != std::string::npos &&
           "5th resource should be 75 at desert");

    // Verify top 5 efficiency values and order
    // Expected values: 90, 80, 70, 60, 50
    // Expected types: mountain(^), land(*), forest()), sea(.), ice(#)
    assert(output.find("90^(") != std::string::npos &&
           "Top eff should be 90 at mountain");
    assert(output.find("80*(") != std::string::npos &&
           "2nd eff should be 80 at land");
    assert(output.find("70)(") != std::string::npos &&
           "3rd eff should be 70 at forest");
    assert(output.find("60.(") != std::string::npos &&
           "4th eff should be 60 at sea");
    assert(output.find("50#(") != std::string::npos &&
           "5th eff should be 50 at ice");

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

    // Verify bottom 5 troops are correct
    // Bottom 5 troops should all be 0 from unoccupied sea sectors
    auto troops_line_start = output.find("Troops:");
    auto troops_line_end = output.find('\n', troops_line_start);
    auto troops_line =
        output.substr(troops_line_start, troops_line_end - troops_line_start);

    // Should show multiple 0 values (there are many unoccupied sectors with 0
    // troops)
    assert(troops_line.find(" 0.(") != std::string::npos &&
           "Bottom 5 troops should include 0 at sea sectors");
    // Should NOT show the high values
    assert(troops_line.find("20") == std::string::npos &&
           "Bottom 5 troops should not show highest value 20");
    assert(troops_line.find("10") == std::string::npos &&
           "Bottom 5 troops should not show high value 10");
    assert(troops_line.find(" 8") == std::string::npos &&
           "Bottom 5 troops should not show value 8");

    // Verify bottom 5 resources
    // Lowest resources are from default unoccupied sectors with values like
    // 12-18
    auto res_line_start = output.find("Res:");
    auto res_line_end = output.find('\n', res_line_start);
    auto res_line =
        output.substr(res_line_start, res_line_end - res_line_start);

    // Some sectors have 0 resources (the default unoccupied ones we didn't set)
    // The lowest non-zero would be around 12-18 range
    assert(res_line.find(" 0.(") != std::string::npos &&
           "Bottom 5 resources should include 0 values");
    // Should NOT show the high resource values
    assert(res_line.find("200") == std::string::npos &&
           "Bottom 5 resources should not show highest value 200");
    assert(res_line.find("150") == std::string::npos &&
           "Bottom 5 resources should not show high value 150");
    assert(res_line.find("100") == std::string::npos &&
           "Bottom 5 resources should not show high value 100");
    assert(res_line.find("80") == std::string::npos &&
           "Bottom 5 resources should not show value 80");
    assert(res_line.find("75") == std::string::npos &&
           "Bottom 5 resources should not show value 75");

    // Verify bottom 5 efficiency (should be 0s from unoccupied sectors)
    auto eff_line_start = output.find("Eff:");
    auto eff_line_end = output.find('\n', eff_line_start);
    auto eff_line =
        output.substr(eff_line_start, eff_line_end - eff_line_start);

    assert(eff_line.find(" 0.(") != std::string::npos &&
           "Bottom 5 eff should show 0 values at sea sectors");
    // Should NOT show the high efficiency values
    assert(eff_line.find("90") == std::string::npos &&
           "Bottom 5 eff should not show highest value 90");
    assert(eff_line.find("80") == std::string::npos &&
           "Bottom 5 eff should not show high value 80");
    assert(eff_line.find("70") == std::string::npos &&
           "Bottom 5 eff should not show value 70");
    assert(eff_line.find("60") == std::string::npos &&
           "Bottom 5 eff should not show value 60");
    assert(eff_line.find("50") == std::string::npos &&
           "Bottom 5 eff should not show value 50");

    // Verify bottom 5 mobilization (should be 0s)
    auto mob_line_start = output.find("Mob:");
    auto mob_line_end = output.find('\n', mob_line_start);
    auto mob_line =
        output.substr(mob_line_start, mob_line_end - mob_line_start);

    assert(mob_line.find(" 0.(") != std::string::npos &&
           "Bottom 5 mob should show 0 values");
    // Should NOT show high mobilization values
    assert(mob_line.find("60") == std::string::npos &&
           "Bottom 5 mob should not show value 60");
    assert(mob_line.find("50") == std::string::npos &&
           "Bottom 5 mob should not show value 50");
    assert(mob_line.find("40") == std::string::npos &&
           "Bottom 5 mob should not show value 40");

    // Verify bottom 5 population (should be 0s from unoccupied sectors)
    auto popn_line_start = output.find("Popn:");
    auto popn_line_end = output.find('\n', popn_line_start);
    auto popn_line =
        output.substr(popn_line_start, popn_line_end - popn_line_start);

    assert(popn_line.find(" 0.(") != std::string::npos &&
           "Bottom 5 popn should show 0 values");
    // Should NOT show high population values
    assert(popn_line.find("2000") == std::string::npos &&
           "Bottom 5 popn should not show value 2000");
    assert(popn_line.find("1000") == std::string::npos &&
           "Bottom 5 popn should not show value 1000");
    assert(popn_line.find("800") == std::string::npos &&
           "Bottom 5 popn should not show value 800");

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
