// SPDX-License-Identifier: Apache-2.0

/// \file order_test.cc
/// \brief Unit tests for order command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test ship with default orders - use battleship which can bombard
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_BATTLE;  // Battleship can bombard
  ship.name() = "TestShip";
  ship.speed() = 5;
  ship.max_speed() = 9;
  ship.popn() = 100;  // Crew

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);
}

void test_order_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println(std::cout, "Set ship defense order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "on"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense order was set
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->protect().planet, 1);
    std::println(std::cout, "    ✓ Defense order set: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 2: Turn defense order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "off"});

    // Force reload from database
    ctx.em.clear_cache();

    // Verify defense was turned off
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->protect().planet, 0);
    std::println(std::cout, "    ✓ Defense order turned off: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 3: Display all orders (no modifications)");
  {
    ctx.assert_dispatch_success(g, {"order"});

    // Verify ship state unchanged
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_eq(saved_ship->protect().planet,
                    0);  // Still off from previous test
    std::println(std::cout, "    ✓ Display orders works without modification");
  }
}

}  // namespace

int main() {
  test_order_happy_path();

  std::println(std::cout, "All order tests passed!");
  return 0;
}
