// SPDX-License-Identifier: Apache-2.0

/// \file gblib_test.cc
/// \brief Unit tests for bit setting operations across integer widths.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  std::uint8_t test8 = 0;
  std::uint32_t test32 = 0;
  std::uint64_t test64 = 0;

  setbit(test8, 4U);
  test::expect_eq(test8, 16);

  setbit(test32, 22U);
  test::expect_eq(test32, 4194304);

  setbit(test64, 48U);
  test::expect_eq(test64, static_cast<std::uint64_t>(std::exp2(48)));

  // Test bool_rand
  test::expect_false(bool_rand(0.0));
  test::expect_true(bool_rand(1.0));

  // Test Ship member methods and predicates
  {
    ship_struct data{};
    data.type = ShipType::STYPE_BATTLE;
    data.armor = 10;
    data.damage = 20;
    data.size = 100;
    data.max_hanger = 20;
    data.hanger = 5;
    data.guns = PRIMARY;
    data.primary = 40;
    data.secondary = 20;
    data.max_crew = 100;
    data.popn = 40;
    data.troops = 30;
    data.max_resource = 500;
    data.resource = 200;
    data.max_fuel = 1000;
    data.fuel = 500.0;
    data.max_destruct = 50;
    data.destruct = 20;
    data.max_speed = 6;
    data.speed = 4;
    data.laser = true;
    data.fire_laser = true;
    data.hyper_drive = {.charge = 10, .on = false, .has = true};
    data.docked = true;
    data.whatdest = ScopeLevel::LEVEL_SHIP;

    Ship ship(data);

    // Docked / Landed predicates
    test::expect_true(ship.is_docked());
    test::expect_false(ship.is_landed());

    ship.whatdest() = ScopeLevel::LEVEL_PLAN;
    test::expect_false(ship.is_docked());
    test::expect_true(ship.is_landed());

    ship.docked() = false;
    test::expect_false(ship.is_docked());
    test::expect_false(ship.is_landed());

    // Laser & Hyperdrive
    test::expect_true(ship.is_laser_on());
    test::expect_true(ship.is_hyper_drive_ready());
    ship.fire_laser() = false;
    test::expect_false(ship.is_laser_on());

    // Capabilities
    test::expect_true(ship.can_bombard());
    test::expect_true(ship.can_navigate());
    test::expect_false(ship.can_aim());
    test::expect_true(ship.has_sight());

    // Body, hangar & armor
    test::expect_eq(ship.shipbody(), 80);
    test::expect_eq(ship.hanger_space(), 15U);
    test::expect_eq(ship.effective_armor(), 8U);  // 10 * (100 - 20) / 100 = 8
    test::expect_eq(ship.active_guns(), 40U);

    // Capacities & Overload
    test::expect_eq(ship.available_crew(), 70U);  // 100 - 30
    test::expect_eq(ship.available_mil(), 60U);   // 100 - 40
    test::expect_false(ship.is_overloaded());

    ship.resource() = 600;
    test::expect_true(ship.is_overloaded());
  }

  // Test Factory specialization methods
  {
    ship_struct fact_data{};
    fact_data.type = ShipType::OTYPE_FACTORY;
    fact_data.build_cost = 50;
    fact_data.on = true;

    Ship fact(fact_data);
    test::expect_true(fact.has_switch());
    test::expect_eq(fact.repair_capacity(), 1L);
    test::expect_eq(fact.max_crew_capacity(),
                    static_cast<population_t>(
                        Shipdata[ShipType::OTYPE_FACTORY][ABIL_MAXCREW]));
    test::expect_eq(
        fact.max_resource_capacity(),
        static_cast<resource_t>(Shipdata[ShipType::OTYPE_FACTORY][ABIL_CARGO]));
  }

  std::println(std::cout, "✓ gblib_test passed!");
  return 0;
}
