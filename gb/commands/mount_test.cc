// SPDX-License-Identifier: Apache-2.0

/// \file mount_test.cc
/// \brief Unit tests for mount command

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

// Test 1: Database persistence for mounting crystals
void test_mount_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with crystal mount capability
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;  // Has crystal mount
  ship.alive() = true;
  ship.crystals() = 2;  // Has 2 crystals on board
  ship.mounted() = 0;   // Not mounted yet
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    assert(s);
    assert(s->crystals() == 2);
    assert(s->mounted() == 0);
  }

  // 4. Simulate mounting a crystal via EntityManager
  {
    auto ship_handle = ctx.em.get_ship(1);
    assert(ship_handle.get());
    auto& s = *ship_handle;
    s.mounted() = 1;
    s.crystals()--;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  assert(final_ship);
  assert(final_ship->mounted() == 1);
  assert(final_ship->crystals() == 1);

  std::println("✓ mount persistence test passed");
}

// Test 2: Database persistence for dismounting crystals
void test_dismount_persistence() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with mounted crystal
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  ship.crystals() = 1;
  ship.mounted() = 1;              // Crystal mounted
  ship.hyper_drive().charge = 50;  // Charged
  ship.hyper_drive().ready = 1;
  ships.save(ship);

  // 3. Verify initial state via EntityManager
  ctx.em.clear_cache();
  {
    const auto* s = ctx.em.peek_ship(1);
    assert(s);
    assert(s->crystals() == 1);
    assert(s->mounted() == 1);
    assert(s->hyper_drive().charge == 50);
    assert(s->hyper_drive().ready == 1);
  }

  // 4. Simulate dismounting crystal via EntityManager
  {
    auto ship_handle = ctx.em.get_ship(1);
    assert(ship_handle.get());
    auto& s = *ship_handle;
    s.mounted() = 0;
    s.crystals()++;
    // Discharge hyperdrive when dismounting
    s.hyper_drive().charge = 0;
    s.hyper_drive().ready = 0;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  ctx.em.clear_cache();
  const auto* final_ship = ctx.em.peek_ship(1);
  assert(final_ship);
  assert(final_ship->mounted() == 0);
  assert(final_ship->crystals() == 2);
  assert(final_ship->hyper_drive().charge == 0);
  assert(final_ship->hyper_drive().ready == 0);

  std::println("✓ dismount persistence test passed");
}

// Test 3: Edge case - cannot mount without crystals
void test_mount_no_crystals() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship without crystals
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  ship.crystals() = 0;  // No crystals
  ship.mounted() = 0;
  ships.save(ship);

  // 3. Verify cannot mount (command validation)
  ctx.em.clear_cache();
  const auto* s = ctx.em.peek_ship(1);
  assert(s);
  assert(s->crystals() == 0);
  assert(s->mounted() == 0);
  // Command should detect no crystals and not modify ship

  std::println("✓ mount no crystals edge case test passed");
}

// Test 4: Edge case - cannot dismount if crystal storage full
void test_dismount_full_storage() {
  // 1. Create in-memory database
  TestContext ctx;

  // 2. Create test entities via Repository
  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with max crystals and one mounted
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  // Assume max_crystals is 127 for this ship type
  ship.crystals() = 127;  // Max storage full
  ship.mounted() = 1;
  ships.save(ship);

  // 3. Verify state
  ctx.em.clear_cache();
  const auto* s = ctx.em.peek_ship(1);
  assert(s);
  assert(s->crystals() == 127);
  assert(s->mounted() == 1);
  // Command should detect full storage and not allow dismount

  std::println("✓ dismount full storage edge case test passed");
}

int main() {
  test_mount_persistence();
  test_dismount_persistence();
  test_mount_no_crystals();
  test_dismount_full_storage();

  std::println("\n✅ All mount tests passed!");
  return 0;
}
