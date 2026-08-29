// SPDX-License-Identifier: Apache-2.0

/// \file misc_repositories_test.cc
/// \brief Unit tests for miscellaneous repositories (Commod, Block, Power,
/// Universe, ServerState, ShipExam).

import dallib;
import gb.entities;
import gb.repositories;
import test;
import std;

// Test file for miscellaneous repositories:
// CommodRepository, BlockRepository, PowerRepository, UniverseRepository,
// and ServerStateRepository

void test_commod_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  CommodRepository repo(store);

  // Test data
  Commod c1{};
  c1.id = 1;
  c1.owner = 1;
  c1.governor = 2;
  c1.type = CommodType::FUEL;
  c1.amount = 100;
  c1.deliver = true;
  c1.bid = 75;
  c1.bidder = 4;
  c1.bidder_gov = 5;
  c1.star_from = 6;
  c1.planet_from = 7;
  c1.star_to = 8;
  c1.planet_to = 9;

  // Save and retrieve
  test::expect_true(repo.save(c1));
  auto retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->owner, 1);
  test::expect_eq(retrieved->governor, 2);
  test::expect_eq(retrieved->amount, 100);
  test::expect_eq(retrieved->bid, 75);

  // Update
  c1.amount = 200;
  test::expect_true(repo.save(c1));
  retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->amount, 200);

  // Multiple commods
  Commod c2{};
  c2.id = 5;
  c2.owner = 10;
  c2.amount = 500;
  test::expect_true(repo.save(c2));
  test::expect_true(repo.find_by_id(1).has_value());
  test::expect_true(repo.find_by_id(5).has_value());

  // Non-existent commod
  auto none = repo.find_by_id(999);
  test::expect_false(none.has_value());

  // Remove
  test::expect_true(repo.remove(1));
  test::expect_false(repo.find_by_id(1).has_value());
  test::expect_true(repo.find_by_id(5).has_value());  // Other still exists

  std::println(std::cout, "✓ All CommodRepository tests passed");
}

void test_block_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  BlockRepository repo(store);

  // Test data
  block b1{};
  b1.Playernum = 1;
  b1.name = "Alliance Alpha";
  b1.motto = "United we stand";
  b1.invite = 1;
  b1.pledge = 1;
  b1.atwar = 2;
  b1.allied = 3;
  b1.next = 0;
  b1.systems_owned = 10;
  b1.VPs = 1000;
  b1.money = 5000;

  // Save and retrieve
  test::expect_true(repo.save(b1));
  auto retrieved = repo.find_by_id(blocknum_t{1});
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->Playernum, 1);
  test::expect_eq(retrieved->name, "Alliance Alpha");
  test::expect_eq(retrieved->systems_owned, 10);
  test::expect_eq(retrieved->VPs, 1000);

  // Update
  b1.VPs = 2000;
  b1.money = 10000;
  test::expect_true(repo.save(b1));
  retrieved = repo.find_by_id(blocknum_t{1});
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->VPs, 2000);
  test::expect_eq(retrieved->money, 10000);

  // Multiple blocks
  block b2{};
  b2.Playernum = 3;
  b2.name = "Beta Coalition";
  b2.VPs = 500;
  test::expect_true(repo.save(b2));
  test::expect_true(repo.find_by_id(blocknum_t{1}).has_value());
  test::expect_true(repo.find_by_id(blocknum_t{3}).has_value());

  // Remove
  test::expect_true(repo.remove(blocknum_t{3}));
  test::expect_false(repo.find_by_id(blocknum_t{3}).has_value());

  std::println(std::cout, "✓ All BlockRepository tests passed");
}

void test_power_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  PowerRepository repo(store);

  // Test data
  power p1{};
  p1.troops = 1000;
  p1.popn = 5000;
  p1.resource = 2000;
  p1.fuel = 500;
  p1.destruct = 100;
  p1.ships_owned = 25;
  p1.planets_owned = 5;
  p1.sectors_owned = 100;
  p1.money = 10000;
  p1.sum_mob = 75;
  p1.sum_eff = 85;
  p1.id = 1;

  // Save and retrieve
  test::expect_true(repo.save(p1));
  auto retrieved = repo.find_by_id(powernum_t{1});
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->troops, 1000);
  test::expect_eq(retrieved->popn, 5000);
  test::expect_eq(retrieved->ships_owned, 25);
  test::expect_eq(retrieved->money, 10000);

  // Update
  p1.troops = 2000;
  p1.ships_owned = 30;
  test::expect_true(repo.save(p1));
  retrieved = repo.find_by_id(powernum_t{1});
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->troops, 2000);
  test::expect_eq(retrieved->ships_owned, 30);

  // Multiple power entries (one per player)
  power p2{};
  p2.id = 2;
  p2.troops = 500;
  p2.popn = 2000;
  p2.ships_owned = 10;
  test::expect_true(repo.save(p2));
  test::expect_true(repo.find_by_id(powernum_t{1}).has_value());
  test::expect_true(repo.find_by_id(powernum_t{2}).has_value());

  // Gap finding
  p1.id = 5;
  test::expect_true(repo.save(p1));
  int next_id = repo.next_available_id();
  test::expect_eq(next_id, 3);  // Should find gap at 3

  std::println(std::cout, "✓ All PowerRepository tests passed");
}

void test_universe_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  UniverseRepository repo(store);

  // Test data - universe_struct is typically a singleton
  universe_struct sd{};
  sd.id = 1;  // Stardata is a singleton with id=1
  sd.numstars = 50;
  sd.ships = 100;
  ap_t ap_val = 0;
  for (auto& ap : sd.AP) {
    ap = ap_val;
    ap_val += 10;
  }
  sd.VN_hitlist[player_t{1}] = 1;
  sd.VN_hitlist[player_t{2}] = 2;
  // VN_index arrays are int arrays for VN tracking
  sd.VN_index1[player_t{1}] = 5;
  sd.VN_index1[player_t{2}] =
      -3;  // Test negative values (comment says negative values are used)
  sd.VN_index2[player_t{1}] = 10;
  sd.VN_index2[player_t{2}] = 15;

  // Save and retrieve global data
  test::expect_true(repo.save(sd));
  auto retrieved = repo.get_global_data();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->numstars, 50);
  test::expect_eq(retrieved->ships, 100);
  test::expect_eq(retrieved->AP[player_t{1}], 0);
  test::expect_eq(retrieved->AP[player_t{6}], 50);
  test::expect_eq(retrieved->VN_index1[player_t{1}], 5);

  // Update global data
  sd.numstars = 75;
  sd.ships = 200;
  test::expect_true(repo.save(sd));
  retrieved = repo.get_global_data();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->numstars, 75);
  test::expect_eq(retrieved->ships, 200);

  // Array preservation
  ap_t expected_ap = 0;
  for (const auto& ap : retrieved->AP) {
    test::expect_eq(ap, expected_ap);
    expected_ap += 10;
  }

  // VN arrays preserved
  test::expect_eq(retrieved->VN_hitlist[player_t{1}], 1);
  test::expect_eq(retrieved->VN_hitlist[player_t{2}], 2);
  // Check VN_index values match what we set (including negative values)
  test::expect_eq(retrieved->VN_index1[player_t{1}], 5);
  test::expect_eq(retrieved->VN_index1[player_t{2}], -3);
  test::expect_eq(retrieved->VN_index2[player_t{1}], 10);
  test::expect_eq(retrieved->VN_index2[player_t{2}], 15);

  std::println(std::cout, "✓ All UniverseRepository tests passed");
}

void test_server_state_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  ServerStateRepository repo(store);

  // Test data - ServerState is a singleton with id=1
  ServerState state{};
  state.id = 1;
  state.segments = 10;
  state.next_update_time = 1735000000;   // Some future timestamp
  state.next_segment_time = 1734900000;  // Earlier timestamp
  state.update_time_minutes = 60;
  state.nsegments_done = 3;
  state.welcome_message = "Welcome to Galactic Bloodshed!";

  // Save and retrieve server state
  test::expect_true(repo.save(state));
  auto retrieved = repo.get_state();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->id, 1);
  test::expect_eq(retrieved->segments, 10);
  test::expect_eq(retrieved->next_update_time, 1735000000);
  test::expect_eq(retrieved->next_segment_time, 1734900000);
  test::expect_eq(retrieved->update_time_minutes, 60);
  test::expect_eq(retrieved->nsegments_done, 3);
  test::expect_eq(retrieved->welcome_message, "Welcome to Galactic Bloodshed!");

  // Update server state
  state.segments = 15;
  state.nsegments_done = 7;
  state.update_time_minutes = 120;
  state.welcome_message = "Updated welcome message!";
  test::expect_true(repo.save(state));
  retrieved = repo.get_state();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->segments, 15);
  test::expect_eq(retrieved->nsegments_done, 7);
  test::expect_eq(retrieved->update_time_minutes, 120);
  test::expect_eq(retrieved->welcome_message, "Updated welcome message!");

  // Timestamps are preserved
  test::expect_eq(retrieved->next_update_time, 1735000000);
  test::expect_eq(retrieved->next_segment_time, 1734900000);

  // ID remains 1 (singleton)
  test::expect_eq(retrieved->id, 1);

  std::println(std::cout, "✓ All ServerStateRepository tests passed");
}

void test_ship_exam_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  ShipExamRepository repo(store);

  // Test data
  ShipExam exam1{.ship_type = ShipType::STYPE_POD,
                 .name = "Spore pod",
                 .description =
                     "A small seed pod grown to colonize other planets."};

  // Save and retrieve
  test::expect_true(repo.save(exam1));
  auto retrieved = repo.find_by_type(ShipType::STYPE_POD);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->ship_type, ShipType::STYPE_POD);
  test::expect_eq(retrieved->name, "Spore pod");
  test::expect_eq(retrieved->description,
                  "A small seed pod grown to colonize other planets.");

  // Update
  exam1.description = "Updated spore pod description.";
  test::expect_true(repo.save(exam1));
  retrieved = repo.find_by_type(ShipType::STYPE_POD);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->description, "Updated spore pod description.");

  // Multiple ship exams
  ShipExam exam2{.ship_type = ShipType::STYPE_SHUTTLE,
                 .name = "Shuttle",
                 .description = "Short range transport craft."};
  test::expect_true(repo.save(exam2));
  test::expect_true(repo.find_by_type(ShipType::STYPE_POD).has_value());
  test::expect_true(repo.find_by_type(ShipType::STYPE_SHUTTLE).has_value());

  // Non-existent ship exam
  auto none = repo.find_by_type(static_cast<ShipType>(999));
  test::expect_false(none.has_value());

  // Remove
  test::expect_true(repo.remove(std::to_underlying(ShipType::STYPE_POD)));
  test::expect_false(repo.find_by_type(ShipType::STYPE_POD).has_value());
  test::expect_true(repo.find_by_type(ShipType::STYPE_SHUTTLE).has_value());

  // Test seed_from_file using PKGDATADIR exam.dat
  {
    Database seed_db(":memory:");
    initialize_schema(seed_db);
    JsonStore seed_store(seed_db);
    ShipExamRepository seed_repo(seed_store);
    bool seeded = seed_repo.seed_from_file(PKGDATADIR "exam.dat");
    if (seeded) {
      auto pod_exam = seed_repo.find_by_type(ShipType::STYPE_POD);
      test::expect_true(pod_exam.has_value());
      test::expect_contains(pod_exam->description, "Spore Pod");
    }
  }

  std::println(std::cout, "✓ All ShipExamRepository tests passed");
}

int main() {
  std::println(std::cout, "Running miscellaneous repository tests...\n");

  test_commod_repository();
  test_block_repository();
  test_power_repository();
  test_universe_repository();
  test_server_state_repository();
  test_ship_exam_repository();

  std::println(std::cout, "\n✅ All miscellaneous repository tests passed!");
  return 0;
}
