// SPDX-License-Identifier: Apache-2.0

// Test file for miscellaneous repositories:
// CommodRepository, BlockRepository, PowerRepository, UniverseRepository,
// and ServerStateRepository

import dallib;
import gblib;
import dallib;
import std.compat;

#include <cassert>

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

  // Test 1: Save and retrieve
  assert(repo.save(c1));
  auto retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->owner == 1);
  assert(retrieved->governor == 2);
  assert(retrieved->amount == 100);
  assert(retrieved->bid == 75);

  // Test 2: Update
  c1.amount = 200;
  assert(repo.save(c1));
  retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->amount == 200);

  // Test 3: Multiple commods
  Commod c2{};
  c2.id = 5;
  c2.owner = 10;
  c2.amount = 500;
  assert(repo.save(c2));
  assert(repo.find_by_id(1).has_value());
  assert(repo.find_by_id(5).has_value());

  // Test 4: Non-existent commod
  auto none = repo.find_by_id(999);
  assert(!none.has_value());

  // Test 5: Remove
  assert(repo.remove(1));
  assert(!repo.find_by_id(1).has_value());
  assert(repo.find_by_id(5).has_value());  // Other still exists

  std::println("✓ All CommodRepository tests passed");
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

  // Test 1: Save and retrieve
  assert(repo.save(b1));
  auto retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->Playernum == 1);
  assert(retrieved->name == "Alliance Alpha");
  assert(retrieved->systems_owned == 10);
  assert(retrieved->VPs == 1000);

  // Test 2: Update
  b1.VPs = 2000;
  b1.money = 10000;
  assert(repo.save(b1));
  retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->VPs == 2000);
  assert(retrieved->money == 10000);

  // Test 3: Multiple blocks
  block b2{};
  b2.Playernum = 3;
  b2.name = "Beta Coalition";
  b2.VPs = 500;
  assert(repo.save(b2));
  assert(repo.find_by_id(1).has_value());
  assert(repo.find_by_id(3).has_value());

  // Test 4: Remove
  assert(repo.remove(3));
  assert(!repo.find_by_id(3).has_value());

  std::println("✓ All BlockRepository tests passed");
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

  // Test 1: Save and retrieve
  assert(repo.save(p1));
  auto retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->troops == 1000);
  assert(retrieved->popn == 5000);
  assert(retrieved->ships_owned == 25);
  assert(retrieved->money == 10000);

  // Test 2: Update
  p1.troops = 2000;
  p1.ships_owned = 30;
  assert(repo.save(p1));
  retrieved = repo.find_by_id(1);
  assert(retrieved.has_value());
  assert(retrieved->troops == 2000);
  assert(retrieved->ships_owned == 30);

  // Test 3: Multiple power entries (one per player)
  power p2{};
  p2.id = 2;
  p2.troops = 500;
  p2.popn = 2000;
  p2.ships_owned = 10;
  assert(repo.save(p2));
  assert(repo.find_by_id(1).has_value());
  assert(repo.find_by_id(2).has_value());

  // Test 4: Gap finding
  p1.id = 5;
  assert(repo.save(p1));
  int next_id = repo.next_available_id();
  assert(next_id == 3);  // Should find gap at 3

  std::println("✓ All PowerRepository tests passed");
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
  for (int i = 0; i < MAXPLAYERS; i++) {
    sd.AP[i] = i * 10;
  }
  sd.VN_hitlist[0] = 1;
  sd.VN_hitlist[1] = 2;
  // VN_index arrays are int arrays for VN tracking
  sd.VN_index1[0] = 5;
  sd.VN_index1[1] =
      -3;  // Test negative values (comment says negative values are used)
  sd.VN_index2[0] = 10;
  sd.VN_index2[1] = 15;

  // Test 1: Save and retrieve global data
  assert(repo.save(sd));
  auto retrieved = repo.get_global_data();
  assert(retrieved.has_value());
  assert(retrieved->numstars == 50);
  assert(retrieved->ships == 100);
  assert(retrieved->AP[0] == 0);
  assert(retrieved->AP[5] == 50);
  assert(retrieved->VN_index1[0] == 5);

  // Test 2: Update global data
  sd.numstars = 75;
  sd.ships = 200;
  assert(repo.save(sd));
  retrieved = repo.get_global_data();
  assert(retrieved.has_value());
  assert(retrieved->numstars == 75);
  assert(retrieved->ships == 200);

  // Test 3: Array preservation
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved->AP[i] == i * 10);
  }

  // Test 4: VN arrays preserved
  assert(retrieved->VN_hitlist[0] == 1);
  assert(retrieved->VN_hitlist[1] == 2);
  // Check VN_index values match what we set (including negative values)
  assert(retrieved->VN_index1[0] == 5);
  assert(retrieved->VN_index1[1] == -3);
  assert(retrieved->VN_index2[0] == 10);
  assert(retrieved->VN_index2[1] == 15);

  std::println("✓ All UniverseRepository tests passed");
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

  // Test 1: Save and retrieve server state
  assert(repo.save(state));
  auto retrieved = repo.get_state();
  assert(retrieved.has_value());
  assert(retrieved->id == 1);
  assert(retrieved->segments == 10);
  assert(retrieved->next_update_time == 1735000000);
  assert(retrieved->next_segment_time == 1734900000);
  assert(retrieved->update_time_minutes == 60);
  assert(retrieved->nsegments_done == 3);

  // Test 2: Update server state
  state.segments = 15;
  state.nsegments_done = 7;
  state.update_time_minutes = 120;
  assert(repo.save(state));
  retrieved = repo.get_state();
  assert(retrieved.has_value());
  assert(retrieved->segments == 15);
  assert(retrieved->nsegments_done == 7);
  assert(retrieved->update_time_minutes == 120);

  // Test 3: Timestamps are preserved
  assert(retrieved->next_update_time == 1735000000);
  assert(retrieved->next_segment_time == 1734900000);

  // Test 4: ID remains 1 (singleton)
  assert(retrieved->id == 1);

  std::println("✓ All ServerStateRepository tests passed");
}

int main() {
  std::println("Running miscellaneous repository tests...\n");

  test_commod_repository();
  test_block_repository();
  test_power_repository();
  test_universe_repository();
  test_server_state_repository();

  std::println("\n✅ All miscellaneous repository tests passed!");
  return 0;
}
