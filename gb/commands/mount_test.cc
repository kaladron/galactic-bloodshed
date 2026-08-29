// SPDX-License-Identifier: Apache-2.0

/// \file mount_test.cc
/// \brief Unit tests for mount and dismount commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_race(TestContext& ctx) {
  JsonStore store(ctx.db);
  Race race{};
  race.Playernum = 1;
  race.name = "Crystallines";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);
}

// Database persistence for mounting crystals
void test_mount_persistence() {
  TestContext ctx;
  setup_test_race(ctx);

  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with crystal mount capability
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;  // Has crystal mount
  ship.alive() = true;
  ship.active() = true;
  ship.mount() = 1;
  ship.crystals() = 2;  // Has 2 crystals on board
  ship.mounted() = 0;   // Not mounted yet
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  ctx.assert_dispatch_success(g, {"mount", "#1"});

  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->mounted(), 1);
  test::expect_eq(final_ship->crystals(), 1);
  test::expect_contains(g.out.str(), "Mounted.");

  std::println(std::cout, "✓ mount persistence test passed");
}

// Database persistence for dismounting crystals
void test_dismount_persistence() {
  TestContext ctx;
  setup_test_race(ctx);

  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with mounted crystal
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  ship.active() = true;
  ship.mount() = 1;
  ship.crystals() = 1;
  ship.mounted() = 1;              // Crystal mounted
  ship.hyper_drive().charge = 50;  // Charged
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  ctx.assert_dispatch_success(g, {"dismount", "#1"});

  const auto* final_ship = ctx.em.peek_ship(1);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->mounted(), 0);
  test::expect_eq(final_ship->crystals(), 2);
  test::expect_eq(final_ship->hyper_drive().charge, 0U);
  test::expect_false(final_ship->hyper_drive().is_ready());
  test::expect_contains(g.out.str(), "Dismounted.");
  test::expect_contains(g.out.str(), "Discharged.");

  std::println(std::cout, "✓ dismount persistence test passed");
}

// Edge case - cannot mount without crystals
void test_mount_no_crystals() {
  TestContext ctx;
  setup_test_race(ctx);

  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship without crystals
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  ship.active() = true;
  ship.mount() = 1;
  ship.crystals() = 0;  // No crystals
  ship.mounted() = 0;
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  ctx.assert_dispatch_rejected(g, {"mount", "#1"});
  test::expect_contains(g.out.str(), "You have no crystals on board.");

  const auto* s = ctx.em.peek_ship(1);
  test::expect_ne(s, nullptr);
  test::expect_eq(s->crystals(), 0);
  test::expect_eq(s->mounted(), 0);

  std::println(std::cout, "✓ mount no crystals edge case test passed");
}

// Edge case - cannot dismount if crystal storage full
void test_dismount_full_storage() {
  TestContext ctx;
  setup_test_race(ctx);

  JsonStore store(ctx.db);
  ShipRepository ships(store);

  // Create a ship with max crystals and one mounted
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.type() = ShipType::STYPE_HABITAT;
  ship.alive() = true;
  ship.active() = true;
  ship.mount() = 1;
  ship.crystals() = ship.max_crystals_capacity();  // Max storage full
  ship.mounted() = 1;
  ships.save(ship);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  ctx.assert_dispatch_rejected(g, {"dismount", "#1"});
  test::expect_contains(
      g.out.str(),
      "You can't dismount the crystal. Max allowed already on board.");

  const auto* s = ctx.em.peek_ship(1);
  test::expect_ne(s, nullptr);
  test::expect_eq(s->mounted(), 1);

  std::println(std::cout, "✓ dismount full storage edge case test passed");
}

void test_mount_syntax_and_errors() {
  TestContext ctx;
  setup_test_race(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"mount"});
  test::expect_contains(g.out.str(), "Syntax: mount <ship>");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dismount"});
  test::expect_contains(g.out.str(), "Syntax: dismount <ship>");
}

}  // namespace

int main() {
  test_mount_persistence();
  test_dismount_persistence();
  test_mount_no_crystals();
  test_dismount_full_storage();
  test_mount_syntax_and_errors();

  std::println(std::cout, "\n✅ All mount tests passed!");
  return 0;
}
