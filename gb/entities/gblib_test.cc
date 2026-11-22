// SPDX-License-Identifier: Apache-2.0

/// \file gblib_test.cc
/// \brief Unit tests for bit setting operations across integer widths.

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

int main() {
  test::expect_false(GB_VERSION.empty());
  test::expect_ne(GB_VERSION, "Standard GB 5.0");

  // Test bool_rand
  test::expect_false(bool_rand(0.0));
  test::expect_true(bool_rand(1.0));

  // Test Ship member methods and predicates
  {
    auto ship_ptr =
        ShipFactory::create_from_template(ShipType::STYPE_BATTLE, 1);
    Ship& ship = *ship_ptr;
    ship.armor() = 10;
    ship.admin_override_damage(20);
    ship.size() = 100;
    ship.max_hanger() = 20;
    ship.hanger() = 5;
    ship.guns() = PRIMARY;
    ship.set_primary_battery(40, guntype_t::MEDIUM);
    ship.set_secondary_battery(20, guntype_t::LIGHT);
    ship.max_crew() = 100;
    ship.popn() = 40;
    ship.troops() = 30;
    ship.max_resource() = 500;
    ship.resource() = 200;
    ship.max_fuel() = 1000;
    ship.add_fuel(500.0);
    ship.max_destruct() = 50;
    ship.destruct() = 20;
    ship.max_speed() = 6;
    ship.speed() = 4;
    ship.laser() = true;
    ship.fire_laser() = 10;
    ship.hyper_drive() = {.charge = 10, .on = false, .has = true};
    ship.dock_into_carrier(shipnum_t{2});

    // Docked / Landed predicates
    test::expect_true(ship.is_docked());
    test::expect_false(ship.is_landed());
    test::expect_false(ship.is_spaceborne());

    ship.land_on_planet();
    test::expect_false(ship.is_docked());
    test::expect_true(ship.is_landed());
    test::expect_false(ship.is_spaceborne());

    ship.launch_to_orbit();
    test::expect_false(ship.is_docked());
    test::expect_false(ship.is_landed());
    test::expect_true(ship.is_spaceborne());

    // Laser & Hyperdrive
    test::expect_true(ship.is_laser_on());
    test::expect_true(ship.is_hyper_drive_ready());
    ship.fire_laser() = 0;
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
    test::expect_eq(ship.available_crew(), 70U);               // 100 - 30
    test::expect_eq(ship.available_mil(), 60U);                // 100 - 40
    test::expect_eq(ship.available_destruct_capacity(), 30L);  // 50 - 20
    test::expect_eq(ship.destruct_power(), 20U);
    test::expect_eq(ship.max_crystals_capacity(), MAX_CRYSTALS);
    test::expect_eq(ship.available_crystals_capacity(), MAX_CRYSTALS);
    test::expect_false(ship.is_overloaded());

    ship.resource() = 600;
    test::expect_true(ship.is_overloaded());
  }

  // Test Factory specialization methods
  {
    auto fact = ShipFactory::create_from_template(ShipType::OTYPE_FACTORY, 1);
    fact->as<FactoryShip>()->configure_blueprint(ShipType::STYPE_CRUISER);
    fact->on() = true;

    test::expect_true(fact->has_switch());
    test::expect_eq(fact->repair_capacity(), 1L);
    test::expect_eq(fact->capture_morale_value(), 0);
    const auto& fact_tmpl = ship_template(ShipType::OTYPE_FACTORY);
    test::expect_eq(fact->max_crew_capacity(), fact_tmpl.max_crew);
    test::expect_eq(fact->max_resource_capacity(), fact_tmpl.max_resource);
  }

  std::println(std::cout, "✓ gblib_test passed!");
  return 0;
}
