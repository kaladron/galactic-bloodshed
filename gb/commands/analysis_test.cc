// SPDX-License-Identifier: Apache-2.0

/// \file analysis_test.cc
/// \brief Unit tests for analysis command output and player/sector filtering

import dallib;
import gblib;
import test;
import commands;
import std;

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
  smap.get(0, 0).set_efficiency_bounded(80);
  smap.get(0, 0).set_mobilization(50);
  smap.get(0, 0).set_resource(100);
  smap.get(0, 0).set_popn_exact(1000);
  smap.get(0, 0).set_troops(10);
  smap.get(0, 0).set_condition(SectorType::SEC_LAND);

  smap.get(1, 0).set_owner(1);
  smap.get(1, 0).set_efficiency_bounded(90);
  smap.get(1, 0).set_mobilization(60);
  smap.get(1, 0).set_resource(150);
  smap.get(1, 0).set_popn_exact(2000);
  smap.get(1, 0).set_troops(20);
  smap.get(1, 0).set_condition(SectorType::SEC_MOUNT);

  smap.get(2, 0).set_owner(1);
  smap.get(2, 0).set_efficiency_bounded(70);
  smap.get(2, 0).set_mobilization(40);
  smap.get(2, 0).set_resource(80);
  smap.get(2, 0).set_popn_exact(500);
  smap.get(2, 0).set_troops(5);
  smap.get(2, 0).set_condition(SectorType::SEC_FOREST);

  // Player 2 owns some sectors
  smap.get(0, 1).set_owner(2);
  smap.get(0, 1).set_efficiency_bounded(60);
  smap.get(0, 1).set_mobilization(30);
  smap.get(0, 1).set_resource(50);
  smap.get(0, 1).set_popn_exact(800);
  smap.get(0, 1).set_troops(8);
  smap.get(0, 1).set_condition(SectorType::SEC_SEA);

  smap.get(1, 1).set_owner(2);
  smap.get(1, 1).set_efficiency_bounded(50);
  smap.get(1, 1).set_mobilization(25);
  smap.get(1, 1).set_resource(40);
  smap.get(1, 1).set_popn_exact(600);
  smap.get(1, 1).set_troops(6);
  smap.get(1, 1).set_condition(SectorType::SEC_ICE);

  // Unowned sectors
  smap.get(2, 1).set_owner(0);
  smap.get(2, 1).set_efficiency_bounded(0);
  smap.get(2, 1).set_resource(200);
  smap.get(2, 1).set_condition(SectorType::SEC_GAS);

  smap.get(0, 2).set_owner(0);
  smap.get(0, 2).set_efficiency_bounded(0);
  smap.get(0, 2).set_resource(75);
  smap.get(0, 2).set_condition(SectorType::SEC_DESERT);

  // Leave the rest as default (unowned, sea type)
  for (int y = 2; y < 5; y++) {
    for (int x = (y == 2 ? 1 : 0); x < 5; x++) {
      smap.get(x, y).set_owner(0);
      smap.get(x, y).set_efficiency_bounded(0);
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

  std::println(std::cout,
               "\n========== Analysis Command Output Test ==========\n");

  std::println(std::cout, "Basic analysis (all sectors)");
  {
    command_t argv = {"analysis"};
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    // Bug fix verification: "owned by 4294967295" was shown when ThisPlayer
    // was player_t (unsigned) and -1 was used to mean "all players"
    test::expect_false(output.contains("4294967295"),
                       "Bug: player_t -1 overflow should not appear in output");

    // Bug fix verification: Top 5 lists should not be empty when we have
    // valid sector data. The insert() function was taking array by value
    // instead of by reference, so insertions were lost.
    test::expect_contains(output, "Troops:");
    // Find "Troops:" and check there's content after the colon on that line
    auto troops_pos = output.find("Troops:");
    test::expect_ne(troops_pos, std::string::npos);
    auto troops_line_end_pos = output.find('\n', troops_pos);
    auto troops_line_initial =
        output.substr(troops_pos, troops_line_end_pos - troops_pos);
    // Line should have more than just "Troops:" - it should have sector data
    test::expect_gt(troops_line_initial.length(), 10u,
                    "Bug: insert() by-value bug would leave top 5 lists empty");

    // Verify top 5 troops are correct and in order
    // Expected values: 20, 10, 8, 6, 5
    // Expected types: mountain(^), land(*), sea(.), ice(#), forest())
    test::expect_contains(output, "20^(",
                          "Top troops should be 20 at mountain");
    test::expect_contains(output, "10*(", "2nd troops should be 10 at land");
    test::expect_contains(output, " 8.(", "3rd troops should be 8 at sea");
    test::expect_contains(output, " 6#(", "4th troops should be 6 at ice");
    test::expect_contains(output, " 5)(", "5th troops should be 5 at forest");

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
    test::expect_ne(pos_20, std::string::npos,
                    "Top 5 troop value 20 should be present");
    test::expect_ne(pos_10, std::string::npos,
                    "Top 5 troop value 10 should be present");
    test::expect_ne(pos_8, std::string::npos,
                    "Top 5 troop value 8 should be present");
    test::expect_ne(pos_6, std::string::npos,
                    "Top 5 troop value 6 should be present");
    test::expect_ne(pos_5, std::string::npos,
                    "Top 5 troop value 5 should be present");
    test::expect_lt(pos_20, pos_10,
                    "Top 5 troops should be in descending order");
    test::expect_lt(pos_10, pos_8,
                    "Top 5 troops should be in descending order");
    test::expect_lt(pos_8, pos_6, "Top 5 troops should be in descending order");
    test::expect_lt(pos_6, pos_5, "Top 5 troops should be in descending order");

    // Verify top 5 resources are correct and in order
    // Expected values: 200, 150, 100, 80, 75
    // Expected types: gas(~), mountain(^), land(*), forest()), desert(-)
    test::expect_contains(output, "200~(", "Top resource should be 200 at gas");
    test::expect_contains(output, "150^(",
                          "2nd resource should be 150 at mountain");
    test::expect_contains(output, "100*(",
                          "3rd resource should be 100 at land");
    test::expect_contains(output, " 80)(",
                          "4th resource should be 80 at forest");
    test::expect_contains(output, " 75-(",
                          "5th resource should be 75 at desert");

    // Verify top 5 efficiency values and order
    // Expected values: 90, 80, 70, 60, 50
    // Expected types: mountain(^), land(*), forest()), sea(.), ice(#)
    test::expect_contains(output, "90^(", "Top eff should be 90 at mountain");
    test::expect_contains(output, "80*(", "2nd eff should be 80 at land");
    test::expect_contains(output, "70)(", "3rd eff should be 70 at forest");
    test::expect_contains(output, "60.(", "4th eff should be 60 at sea");
    test::expect_contains(output, "50#(", "5th eff should be 50 at ice");

    g.out.str("");  // Clear for next test
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis with bottom 5 mode");
  {
    command_t argv = {"analysis", "-"};
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    // Verify "Lowest" appears (not "Highest")
    test::expect_contains(output, "Lowest", "Bottom mode should show 'Lowest'");

    // Verify bottom 5 troops are correct
    // Bottom 5 troops should all be 0 from unoccupied sea sectors
    auto troops_line_start = output.find("Troops:");
    auto troops_line_end = output.find('\n', troops_line_start);
    auto troops_line =
        output.substr(troops_line_start, troops_line_end - troops_line_start);

    // Should show multiple 0 values (there are many unoccupied sectors with 0
    // troops)
    test::expect_contains(troops_line, " 0.(",
                          "Bottom 5 troops should include 0 at sea sectors");
    // Should NOT show the high values
    test::expect_false(troops_line.contains("20"),
                       "Bottom 5 troops should not show highest value 20");
    test::expect_false(troops_line.contains("10"),
                       "Bottom 5 troops should not show high value 10");
    test::expect_false(troops_line.contains(" 8"),
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
    test::expect_contains(res_line, " 0.(",
                          "Bottom 5 resources should include 0 values");
    // Should NOT show the high resource values
    test::expect_false(res_line.contains("200"),
                       "Bottom 5 resources should not show highest value 200");
    test::expect_false(res_line.contains("150"),
                       "Bottom 5 resources should not show high value 150");
    test::expect_false(res_line.contains("100"),
                       "Bottom 5 resources should not show high value 100");
    test::expect_false(res_line.contains("80"),
                       "Bottom 5 resources should not show value 80");
    test::expect_false(res_line.contains("75"),
                       "Bottom 5 resources should not show value 75");

    // Verify bottom 5 efficiency (should be 0s from unoccupied sectors)
    auto eff_line_start = output.find("Eff:");
    auto eff_line_end = output.find('\n', eff_line_start);
    auto eff_line =
        output.substr(eff_line_start, eff_line_end - eff_line_start);

    test::expect_contains(eff_line, " 0.(",
                          "Bottom 5 eff should show 0 values at sea sectors");
    // Should NOT show the high efficiency values
    test::expect_false(eff_line.contains("90"),
                       "Bottom 5 eff should not show highest value 90");
    test::expect_false(eff_line.contains("80"),
                       "Bottom 5 eff should not show high value 80");
    test::expect_false(eff_line.contains("70"),
                       "Bottom 5 eff should not show value 70");
    test::expect_false(eff_line.contains("60"),
                       "Bottom 5 eff should not show value 60");
    test::expect_false(eff_line.contains("50"),
                       "Bottom 5 eff should not show value 50");

    // Verify bottom 5 mobilization (should be 0s)
    auto mob_line_start = output.find("Mob:");
    auto mob_line_end = output.find('\n', mob_line_start);
    auto mob_line =
        output.substr(mob_line_start, mob_line_end - mob_line_start);

    test::expect_contains(mob_line, " 0.(",
                          "Bottom 5 mob should show 0 values");
    // Should NOT show high mobilization values
    test::expect_false(mob_line.contains("60"),
                       "Bottom 5 mob should not show value 60");
    test::expect_false(mob_line.contains("50"),
                       "Bottom 5 mob should not show value 50");
    test::expect_false(mob_line.contains("40"),
                       "Bottom 5 mob should not show value 40");

    // Verify bottom 5 population (should be 0s from unoccupied sectors)
    auto popn_line_start = output.find("Popn:");
    auto popn_line_end = output.find('\n', popn_line_start);
    auto popn_line =
        output.substr(popn_line_start, popn_line_end - popn_line_start);

    test::expect_contains(popn_line, " 0.(",
                          "Bottom 5 popn should show 0 values");
    // Should NOT show high population values
    test::expect_false(popn_line.contains("2000"),
                       "Bottom 5 popn should not show value 2000");
    test::expect_false(popn_line.contains("1000"),
                       "Bottom 5 popn should not show value 1000");
    test::expect_false(popn_line.contains("800"),
                       "Bottom 5 popn should not show value 800");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis filtered to ocean sectors only");
  {
    command_t argv = {"analysis", "."};  // . is sea
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    // Verify sector type filter is shown
    test::expect_contains(output, "Ocean",
                          "Sea filter (.) should show 'Ocean' in output");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis filtered to land sectors only");
  {
    command_t argv = {"analysis", "*"};  // * is land
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    // Verify sector type filter is shown
    test::expect_contains(output, "Land",
                          "Land filter (*) should show 'Land' in output");
    // Should only show land sector data
    test::expect_contains(output, "*( 0, 0)",
                          "Should show land sector coordinates");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis filtered to mountain sectors only");
  {
    command_t argv = {"analysis", "^"};  // ^ is mountain
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    test::expect_contains(
        output, "Mountain",
        "Mountain filter (^) should show 'Mountain' in output");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis filtered to desert sectors (special 'd')");
  {
    command_t argv = {"analysis", "d"};  // 'd' is desert (special case)
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    test::expect_contains(output, "Desert",
                          "Desert filter (d) should show 'Desert' in output");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Analysis with player filter");
  {
    command_t argv = {"analysis", "1"};  // Filter to player 1
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    test::expect_contains(
        output, "sectors owned by 1",
        "Player filter should show 'sectors owned by 1' in output");

    g.out.str("");
  }

  std::println(std::cout, "\n========================================\n");
  std::println(std::cout, "Combined sector type and player filter");
  {
    command_t argv = {"analysis", "*", "1"};  // Land sectors owned by player 1
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    std::println(std::cout, "\n--- Output ---");
    std::println(std::cout, "{}", output);

    test::expect_contains(output, "Land", "Should filter by land sectors");
    // Note: When both sector type and player are specified, the current
    // implementation shows the sector type but not the player in the header.
    // The filtering still happens correctly (only land sectors from player 1
    // are shown in the top 5 lists).

    g.out.str("");
  }

  std::println(std::cout, "\n========== Tests Complete ==========\n");
  std::println(std::cout, "All analysis command tests passed!");

  // PlayerFilter logic tests - tested indirectly through command behavior
  std::println(std::cout,
               "\n========== PlayerFilter Logic Tests (via command behavior) "
               "==========\n");

  // Test AllPlayers mode (default, no player filter)
  {
    std::println(std::cout, "Test: Default filter matches all owners");
    command_t argv = {"analysis"};
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    // Should see stats for all players (1, 2, and 0 for unoccupied)
    test::expect_contains(output, " sectors.\n",
                          "Default should show generic description");
    // Check that the table includes multiple players
    test::expect_contains(output, "Pl", "Should have player column");

    g.out.str("");
    std::println(std::cout, "✓ AllPlayers mode (default) works correctly");
  }

  // Test Unoccupied mode (player 0)
  {
    std::println(std::cout, "Test: Player 0 filter matches only unoccupied");
    command_t argv = {"analysis", "0"};
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    test::expect_contains(output, "unoccupied",
                          "Should show unoccupied description");

    g.out.str("");
    std::println(std::cout, "✓ Unoccupied mode (player 0) works correctly");
  }

  // Test SpecificPlayer mode
  {
    std::println(std::cout,
                 "Test: Specific player filter matches only that player");
    command_t argv = {"analysis", "1"};
    ctx.assert_dispatch_success(g, argv);

    std::string output = g.out.str();
    test::expect_contains(output, "owned by 1",
                          "Should show player 1 in description");

    g.out.str("");

    // Try player 2
    command_t argv2 = {"analysis", "2"};
    ctx.assert_dispatch_success(g, argv2);

    output = g.out.str();
    test::expect_contains(output, "owned by 2",
                          "Should show player 2 in description");

    g.out.str("");
    std::println(std::cout,
                 "✓ SpecificPlayer mode works correctly for different players");
  }

  std::println(std::cout,
               "\n========== PlayerFilter Logic Tests Complete ==========\n");

  return 0;
}
