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
  TestContext ctx;
  ctx.with_standard_universe();
  JsonStore store(ctx.db);
  CommodRepository repo(store);

  // Test data
  Commod c1{{
      .id = 1,
      .owner = 1,
      .governor = 2,
      .type = CommodType::FUEL,
      .amount = 100,
      .deliver = true,
      .bid = 75,
      .bidder = 4,
      .bidder_gov = 5,
      .star_from = 1,
      .planet_from = 1,
      .star_to = 2,
      .planet_to = 1,
  }};

  // Save and retrieve
  test::expect_true(repo.save(c1));
  auto retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->owner(), 1);
  test::expect_eq(retrieved->governor(), 2);
  test::expect_eq(retrieved->amount(), 100);
  test::expect_eq(retrieved->bid(), 75);

  // Update
  c1.set_amount(200);
  test::expect_true(repo.save(c1));
  retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->amount(), 200);

  // Multiple commods
  Commod c2{{
      .id = 5,
      .owner = 2,
      .amount = 500,
      .star_from = 3,
      .planet_from = 1,
  }};
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
  RaceRepository races(store);
  for (player_t p : {player_t{1}, player_t{3}}) {
    Race r{};
    r.Playernum = p;
    races.save(r);
  }
  BlockRepository repo(store);

  // Test data
  Block b1{{
      .Playernum = 1,
      .name = "Alliance Alpha",
      .motto = "United we stand",
      .invited = {player_t{1}},
      .pledged = {player_t{1}},
      .systems_owned = 10,
      .VPs = 1000,
      .money = 5000,
  }};

  // Save and retrieve
  test::expect_true(repo.save(b1));
  auto retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->Playernum(), 1);
  test::expect_eq(retrieved->name(), "Alliance Alpha");
  test::expect_eq(retrieved->systems_owned(), 10);
  test::expect_eq(retrieved->VPs(), 1000);

  // Update
  b1.VPs() = 2000;
  b1.money() = 10000;
  test::expect_true(repo.save(b1));
  retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->VPs(), 2000);
  test::expect_eq(retrieved->money(), 10000);

  // Multiple blocks
  Block b2{{
      .Playernum = 3,
      .name = "Beta Coalition",
      .VPs = 500,
  }};
  test::expect_true(repo.save(b2));
  test::expect_true(repo.find_by_id(1).has_value());
  test::expect_true(repo.find_by_id(3).has_value());

  // Remove
  test::expect_true(repo.remove(3));
  test::expect_false(repo.find_by_id(3).has_value());

  std::println(std::cout, "✓ All BlockRepository tests passed");
}

void test_power_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  RaceRepository races(store);
  for (player_t p : {player_t{1}, player_t{2}, player_t{5}}) {
    Race r{};
    r.Playernum = p;
    races.save(r);
  }
  PowerRepository repo(store);

  // Test data
  Power p1{{
      .id = 1,
      .troops = 1000,
      .popn = 5000,
      .resource = 2000,
      .fuel = 500,
      .destruct = 100,
      .ships_owned = 25,
      .planets_owned = 5,
      .money = 10000,
  }};

  // Save and retrieve
  test::expect_true(repo.save(p1));
  auto retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->troops(), 1000);
  test::expect_eq(retrieved->popn(), 5000);
  test::expect_eq(retrieved->ships_owned(), 25);
  test::expect_eq(retrieved->money(), 10000);

  // Update
  p1.troops() = 2000;
  p1.ships_owned() = 30;
  test::expect_true(repo.save(p1));
  retrieved = repo.find_by_id(1);
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->troops(), 2000);
  test::expect_eq(retrieved->ships_owned(), 30);

  // Multiple power entries (one per player)
  Power p2{{
      .id = 2,
      .troops = 500,
      .popn = 2000,
      .ships_owned = 10,
  }};
  test::expect_true(repo.save(p2));
  test::expect_true(repo.find_by_id(1).has_value());
  test::expect_true(repo.find_by_id(2).has_value());

  // Monotonic next available ID
  p1.set_id(5);
  test::expect_true(repo.save(p1));
  int next_id = repo.next_available_id();
  test::expect_eq(next_id, 6);  // Should return max + 1 (6)

  std::println(std::cout, "✓ All PowerRepository tests passed");
}

void test_universe_repository() {
  // Setup
  Database db(":memory:");
  initialize_schema(db);
  JsonStore store(db);
  UniverseRepository repo(store);

  // Test data - Universe is typically a singleton
  Universe sd{};
  for (player_t p : all_players()) {
    sd.set_AP(p, static_cast<ap_t>((p.value - 1) * 10));
  }
  sd.set_vn_target(1, VnTargetRecord{.hits = 1,
                                     .primary_star = starnum_t{5},
                                     .secondary_star = starnum_t{10}});
  sd.set_vn_target(2, VnTargetRecord{.hits = 2,
                                     .primary_star = std::nullopt,
                                     .secondary_star = starnum_t{15}});

  // Save and retrieve global data
  test::expect_true(repo.save(sd));
  auto retrieved = repo.get_global_data();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->get_AP(1), 0);
  test::expect_eq(retrieved->get_AP(6), 50);
  test::expect_eq(retrieved->vn_primary_star(1), starnum_t{5});

  // Update global data
  sd.set_vn_hits(1, 9);
  test::expect_true(repo.save(sd));
  retrieved = repo.get_global_data();
  test::expect_true(retrieved.has_value());
  test::expect_eq(retrieved->vn_hits(1), 9U);

  // Map preservation
  for (player_t p : all_players()) {
    test::expect_eq(retrieved->get_AP(p),
                    static_cast<ap_t>((p.value - 1) * 10));
  }

  // VN target records preserved
  test::expect_eq(retrieved->vn_hits(2), 2U);
  test::expect_eq(retrieved->vn_primary_star(1), starnum_t{5});
  test::expect_eq(retrieved->vn_primary_star(2), std::nullopt);
  test::expect_eq(retrieved->vn_secondary_star(1), starnum_t{10});
  test::expect_eq(retrieved->vn_secondary_star(2), starnum_t{15});

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
