// SPDX-License-Identifier: Apache-2.0

/// \file race_repository_test.cc
/// \brief Unit tests for RaceRepository CRUD operations and SQLite JSON
/// persistence.

import dallib;
import gb.entities;
import gb.repositories;
import test;
import std;

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and RaceRepository
  JsonStore store(db);
  RaceRepository repo(store);

  // Create a test race
  Race test_race{};
  test_race.Playernum = 1;
  test_race.name = "Test Race";
  test_race.password = "secret123";
  test_race.info = "A test civilization";
  test_race.motto = "Testing is believing";
  test_race.absorb = true;
  test_race.collective_iq = false;
  test_race.pods = true;
  test_race.fighters = 75;
  test_race.IQ = 150;
  test_race.IQ_limit = 200;
  test_race.number_sexes = 2;
  test_race.fertilize = 10;
  test_race.adventurism = 0.5;
  test_race.birthrate = 0.15;
  test_race.mass = 1.0;
  test_race.metabolism = 1.0;
  test_race.dissolved = false;
  test_race.God = false;
  test_race.Guest = false;
  test_race.Metamorph = false;
  test_race.Gov_ship = 100;
  test_race.morale = 1000;
  test_race.controlled_planets = 5;
  test_race.victory_turns = 0;
  test_race.turn = 42;
  test_race.tech = 25.5;
  test_race.victory_score = 5000;
  test_race.votes = true;
  test_race.planet_points = 100;
  test_race.translate[player_t{1}] = 100;
  test_race.translate[player_t{2}] = 75;
  test_race.points[player_t{2}] = 350;
  test_race.discoveries.hyperdrive = true;
  test_race.discoveries.laser = true;
  test_race.discoveries.crystal = true;

  // Initialize conditions and sector compatibilities
  test_race.temp = 22;
  for (AtmosphereConditions c : all_atmosphere_conditions) {
    test_race.conditions[c] = 50 + static_cast<int>(c);
  }
  for (SectorType st : all_sector_types) {
    test_race.likes[st] = 0.5 + (static_cast<int>(st) * 0.1);
  }
  test_race.likesbest = SectorType::SEC_SEA;

  // Initialize governor data
  test_race.leader().name = "Governor One";
  test_race.leader().password = "gov1pass";
  test_race.leader().money = 10000;
  test_race.leader().income = 5000;
  test_race.leader().newspos = {
      .announce = 10, .combat = 20, .declaration = 30, .transfer = 40};

  // Save race
  std::println(std::cout, "Save race...");
  bool saved = repo.save(test_race);
  test::expect_true(saved, "Failed to save race");
  auto race_json = store.retrieve("tbl_race", 1);
  test::expect_true(race_json.has_value());
  test::expect_true(
      race_json->find("\"conditions\":{\"methane\":50,\"oxygen\":51") !=
          std::string::npos,
      "Race::conditions must serialize as a named ConditionValues JSON object");
  test::expect_true(race_json->find("\"likes\":{\"sea\":0.5,\"land\":0.6") !=
                        std::string::npos,
                    "Race::likes must serialize as a named "
                    "SectorCompatibilities JSON object");
  test::expect_true(
      race_json->find("\"newspos\":{\"announce\":10,\"combat\":20,"
                      "\"declaration\":30,\"transfer\":40}") !=
          std::string::npos,
      "Race::gov::newspos must serialize as a named NewsValues JSON object");
  std::println(std::cout, "  ✓ Race saved successfully");

  // Retrieve by player number
  std::println(std::cout, "Retrieve race by player number...");
  auto retrieved = repo.find_by_player(1);
  test::expect_true(retrieved.has_value(), "Failed to retrieve race");
  std::println(std::cout, "  ✓ Race retrieved successfully");

  // Verify data integrity
  std::println(std::cout, "Verify data integrity...");
  test::expect_eq(retrieved->Playernum, test_race.Playernum);
  test::expect_eq(retrieved->name, test_race.name);
  test::expect_eq(retrieved->password, test_race.password);
  test::expect_eq(retrieved->info, test_race.info);
  test::expect_eq(retrieved->motto, test_race.motto);
  test::expect_eq(retrieved->absorb, test_race.absorb);
  test::expect_eq(retrieved->collective_iq, test_race.collective_iq);
  test::expect_eq(retrieved->pods, test_race.pods);
  test::expect_eq(retrieved->fighters, test_race.fighters);
  test::expect_eq(retrieved->IQ, test_race.IQ);
  test::expect_eq(retrieved->tech, test_race.tech);
  test::expect_eq(retrieved->temp, test_race.temp);
  test::expect_eq(retrieved->discoveries, test_race.discoveries);
  test::expect_true(retrieved->conditions == test_race.conditions);
  test::expect_true(retrieved->likes == test_race.likes);
  test::expect_eq(retrieved->translate[player_t{1}],
                  test_race.translate[player_t{1}]);
  test::expect_eq(retrieved->translate[player_t{2}],
                  test_race.translate[player_t{2}]);
  test::expect_eq(retrieved->points[player_t{2}],
                  test_race.points[player_t{2}]);
  test::expect_eq(retrieved->leader().name, test_race.leader().name);
  test::expect_eq(retrieved->leader().money, test_race.leader().money);
  test::expect_true(retrieved->leader().newspos == test_race.leader().newspos);
  std::println(std::cout, "  ✓ All fields match original");

  // Update race
  std::println(std::cout, "Update race...");
  retrieved->tech = 50.0;
  retrieved->morale = 2000;
  saved = repo.save(*retrieved);
  test::expect_true(saved, "Failed to update race");
  std::println(std::cout, "  ✓ Race updated successfully");

  // Retrieve updated race
  std::println(std::cout, "Retrieve updated race...");
  auto updated = repo.find_by_player(1);
  test::expect_true(updated.has_value(), "Failed to retrieve updated race");
  test::expect_eq(updated->tech, 50.0);
  test::expect_eq(updated->morale, 2000);
  std::println(std::cout, "  ✓ Updated values verified");

  // Multiple races and list_ids
  std::println(std::cout, "Multiple races and listing IDs...");
  Race race2{};
  race2.Playernum = 2;
  race2.name = "Vulcans";
  race2.tech = 100.0;
  test::expect_true(repo.save(race2));

  Race race3{};
  race3.Playernum = 5;  // Sparse ID
  race3.name = "Andorians";
  race3.tech = 40.0;
  test::expect_true(repo.save(race3));

  auto player_ids = repo.list_ids();
  test::expect_eq(player_ids.size(), 3);
  test::expect_eq(player_ids[0], 1);
  test::expect_eq(player_ids[1], 2);
  test::expect_eq(player_ids[2], 5);
  std::println(std::cout, "  ✓ list_ids returns all player IDs in order");

  // Remove race
  std::println(std::cout, "Remove race...");
  test::expect_true(repo.remove(2));
  test::expect_false(repo.find_by_player(player_t{2}).has_value());
  auto remaining = repo.list_ids();
  test::expect_eq(remaining.size(), 2);
  test::expect_eq(remaining[0], 1);
  test::expect_eq(remaining[1], 5);
  std::println(std::cout, "  ✓ Race removal successfully deleted entity");

  std::println(std::cout, "\nAll RaceRepository tests passed!");
  return 0;
}
