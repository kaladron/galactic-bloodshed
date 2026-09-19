// SPDX-License-Identifier: Apache-2.0

/// \file ship_domain_test.cc
/// \brief Unit tests for Ship domain methods, bounded setters, and consumption
/// invariants.

import gb.entities;
import gb.services;
import std;
import test;

namespace {

void expect_near(double actual, double expected, double eps = 1e-5) {
  test::expect_true(std::abs(actual - expected) <= eps,
                    std::format("Expected {} to be near {}, difference is {}",
                                actual, expected, std::abs(actual - expected)));
}

void test_hull_efficiency() {
  std::println(std::cout, "Testing Ship::hull_efficiency()...");
  Ship ship;

  ship.admin_override_damage(0);
  expect_near(ship.hull_efficiency(), 1.0);

  ship.admin_override_damage(25);
  expect_near(ship.hull_efficiency(), 0.75);

  ship.admin_override_damage(50);
  expect_near(ship.hull_efficiency(), 0.50);

  ship.admin_override_damage(100);
  expect_near(ship.hull_efficiency(), 0.0);

  // Clamped bounds
  ship.admin_override_damage(150);
  test::expect_eq(ship.damage(), 100);
  expect_near(ship.hull_efficiency(), 0.0);
}

void test_crew_ratio() {
  std::println(std::cout, "Testing Ship::crew_ratio()...");
  ship_struct sdata{
      .max_crew = 100,
      .popn = 0,
  };
  Ship ship{sdata};

  expect_near(ship.crew_ratio(), 0.0);

  ship.add_popn(50, 1.0);
  expect_near(ship.crew_ratio(), 0.5);

  ship.add_popn(50, 1.0);
  expect_near(ship.crew_ratio(), 1.0);

  // Zero-capacity ship returns 0.0 without division by zero
  ship_struct zero_crew_data{.max_crew = 0, .popn = 0};
  Ship zero_ship{zero_crew_data};
  expect_near(zero_ship.crew_ratio(), 0.0);
}

void test_fuel_predicates() {
  std::println(std::cout, "Testing Ship fuel predicates...");
  ship_struct sdata{
      .fuel = 0.0,
      .max_fuel = 200.0,
  };
  Ship ship{sdata};

  test::expect_false(ship.has_fuel());
  test::expect_false(ship.is_fully_fueled());

  // Negligible residual fuel below epsilon
  ship.admin_override_fuel(Ship::FUEL_EPSILON * 0.5);
  test::expect_false(ship.has_fuel());
  test::expect_false(ship.is_fully_fueled());

  // Normal fuel level
  ship.admin_override_fuel(100.0);
  test::expect_true(ship.has_fuel());
  test::expect_false(ship.is_fully_fueled());

  // Full fuel within epsilon
  ship.admin_override_fuel(200.0);
  test::expect_true(ship.has_fuel());
  test::expect_true(ship.is_fully_fueled());

  ship.admin_override_fuel(200.0 - Ship::FUEL_EPSILON * 0.5);
  test::expect_true(ship.is_fully_fueled());
}

void test_available_resource_capacity() {
  std::println(std::cout, "Testing Ship::available_resource_capacity()...");
  ship_struct sdata{
      .max_resource = 500,
      .resource = 0,
  };
  Ship ship{sdata};

  test::expect_eq(ship.available_resource_capacity(), 500);

  ship.add_resource(200);
  test::expect_eq(ship.available_resource_capacity(), 300);

  ship.add_resource(300);
  test::expect_eq(ship.available_resource_capacity(), 0);

  // If resource exceeds capacity, returns 0 rather than negative
  ship.resource() = 600;
  test::expect_eq(ship.available_resource_capacity(), 0);
}

void test_admin_overrides() {
  std::println(std::cout, "Testing Ship admin overrides and clear_crew...");
  ship_struct sdata{
      .max_crew = 50,
      .max_resource = 200,
      .max_destruct = 100,
      .max_fuel = 150.0,
  };
  Ship ship{sdata};

  // admin_override_damage
  ship.admin_override_damage(40);
  test::expect_eq(ship.damage(), 40);
  ship.admin_override_damage(120);
  test::expect_eq(ship.damage(), 100);

  // admin_override_fuel (with mass synchronization)
  const double base_mass = ship.base_mass();
  ship.admin_override_fuel(75.0, 1.0);
  expect_near(ship.fuel(), 75.0);
  expect_near(ship.mass(), base_mass + 75.0 * MASS_FUEL);
  ship.admin_override_fuel(250.0, 1.0);
  expect_near(ship.fuel(), 150.0);
  expect_near(ship.mass(), base_mass + 150.0 * MASS_FUEL);
  ship.admin_override_fuel(-10.0, 1.0);
  expect_near(ship.fuel(), 0.0);
  expect_near(ship.mass(), base_mass);

  // admin_override_resource (with mass synchronization)
  ship.admin_override_resource(120, 1.0);
  test::expect_eq(ship.resource(), 120);
  expect_near(ship.mass(), base_mass + 120 * MASS_RESOURCE);
  ship.admin_override_resource(350, 1.0);
  test::expect_eq(ship.resource(), 200);
  expect_near(ship.mass(), base_mass + 200 * MASS_RESOURCE);
  ship.admin_override_resource(-20, 1.0);
  test::expect_eq(ship.resource(), 0);
  expect_near(ship.mass(), base_mass);

  // admin_override_destruct (with mass synchronization)
  ship.admin_override_destruct(60, 1.0);
  test::expect_eq(ship.destruct(), 60);
  expect_near(ship.mass(), base_mass + 60 * MASS_DESTRUCT);
  ship.admin_override_destruct(180, 1.0);
  test::expect_eq(ship.destruct(), 100);
  expect_near(ship.mass(), base_mass + 100 * MASS_DESTRUCT);
  ship.admin_override_destruct(-10, 1.0);
  test::expect_eq(ship.destruct(), 0);
  expect_near(ship.mass(), base_mass);

  // admin_override_crystals
  ship.admin_override_crystals(50);
  test::expect_eq(ship.crystals(), 50U);
  ship.admin_override_crystals(200);
  test::expect_eq(ship.crystals(), 127U);

  // admin_override_max_fuel
  ship.admin_override_max_fuel(500.0);
  expect_near(ship.max_fuel_capacity(), 500.0);
  ship.admin_override_max_fuel(-50.0);
  expect_near(ship.max_fuel_capacity(), 0.0);

  // admin_resurrect and admin_destroy
  ship.admin_destroy();
  test::expect_false(ship.alive());
  test::expect_false(ship.active());
  test::expect_eq(ship.damage(), 100);
  ship.admin_resurrect();
  test::expect_true(ship.alive());
  test::expect_true(ship.active());
  test::expect_eq(ship.damage(), 0);

  // clear_crew (surrender / capture)
  ship.add_popn(30, 2.0);
  ship.add_troops(20, 2.0);
  test::expect_eq(ship.popn(), 30);
  test::expect_eq(ship.troops(), 20);
  const double crew_mass = ship.mass();
  ship.clear_crew(2.0);
  test::expect_eq(ship.popn(), 0);
  test::expect_eq(ship.troops(), 0);
  expect_near(ship.mass(), crew_mass - 50 * 2.0);
}

void test_fuel_consumption() {
  std::println(std::cout, "Testing Ship fuel consumption methods...");
  ship_struct sdata{
      .fuel = 50.0,
      .mass = 200.0,
      .max_fuel = 100.0,
  };
  Ship ship{sdata};

  // try_consume_fuel with non-positive cost
  test::expect_true(ship.try_consume_fuel(0.0));
  test::expect_true(ship.try_consume_fuel(-5.0));
  expect_near(ship.fuel(), 50.0);
  expect_near(ship.mass(), 200.0);

  // try_consume_fuel success
  test::expect_true(ship.try_consume_fuel(20.0));
  expect_near(ship.fuel(), 30.0);
  expect_near(ship.mass(), 200.0 - 20.0 * MASS_FUEL);

  // try_consume_fuel failure (insufficient fuel)
  test::expect_false(ship.try_consume_fuel(40.0));
  expect_near(ship.fuel(), 30.0);

  // try_consume_fuel with floating point epsilon (fuel is 30.0, cost
  // is 30.00005)
  test::expect_true(ship.try_consume_fuel(30.00005));
  expect_near(ship.fuel(), 0.0);

  // consume_up_to_fuel
  ship.admin_override_fuel(40.0);
  const double base_mass = ship.mass();
  expect_near(ship.consume_up_to_fuel(0.0), 0.0);
  expect_near(ship.consume_up_to_fuel(15.0), 15.0);
  expect_near(ship.fuel(), 25.0);
  expect_near(ship.mass(), base_mass - 15.0 * MASS_FUEL);

  // consume_up_to_fuel exceeding available fuel
  expect_near(ship.consume_up_to_fuel(100.0), 25.0);
  expect_near(ship.fuel(), 0.0);
}

void test_resource_consumption() {
  std::println(std::cout, "Testing Ship resource consumption methods...");
  ship_struct sdata{
      .mass = 500.0,
      .max_resource = 300,
      .resource = 150,
  };
  Ship ship{sdata};

  // try_consume_resource
  test::expect_true(ship.try_consume_resource(0));
  test::expect_true(ship.try_consume_resource(-10));
  test::expect_eq(ship.resource(), 150);

  test::expect_true(ship.try_consume_resource(50));
  test::expect_eq(ship.resource(), 100);
  expect_near(ship.mass(), 500.0 - 50.0 * MASS_RESOURCE);

  test::expect_false(ship.try_consume_resource(120));
  test::expect_eq(ship.resource(), 100);

  // consume_up_to_resource
  test::expect_eq(ship.consume_up_to_resource(40), 40);
  test::expect_eq(ship.resource(), 60);

  test::expect_eq(ship.consume_up_to_resource(100), 60);
  test::expect_eq(ship.resource(), 0);
}

void test_destruct_consumption() {
  std::println(std::cout, "Testing Ship destruct consumption methods...");
  ship_struct sdata{
      .mass = 1000.0,
      .max_destruct = 100000,
      .destruct = 70000,
  };
  Ship ship{sdata};

  // try_consume_destruct with value > 65535 to verify no 16-bit truncation
  test::expect_true(ship.try_consume_destruct(0));
  test::expect_true(ship.try_consume_destruct(66000));
  test::expect_eq(ship.destruct(), 4000);
  expect_near(ship.mass(), 1000.0 - 66000.0 * MASS_DESTRUCT);

  test::expect_false(ship.try_consume_destruct(5000));
  test::expect_eq(ship.destruct(), 4000);

  // consume_up_to_destruct
  test::expect_eq(ship.consume_up_to_destruct(10000), 4000);
  test::expect_eq(ship.destruct(), 0);
}

void test_clamped_add_and_consume() {
  std::println(std::cout, "Testing Ship clamped add_* and consume_*...");
  ship_struct sdata{
      .fuel = 20.0,
      .mass = 500.0,
      .max_crew = 100,
      .max_resource = 200,
      .max_destruct = 80000,
      .max_fuel = 100.0,
      .destruct = 10,
      .resource = 50,
      .popn = 40,
      .troops = 10,
      .damage = 90,
  };
  Ship ship{sdata};

  // apply_damage overflow safety and clamping
  const auto d_res1 = ship.apply_damage(20);
  test::expect_eq(d_res1.damage_applied, 10u);
  test::expect_eq(ship.damage(), 100);
  const auto d_res2 = ship.apply_damage(std::numeric_limits<damage_t>::max());
  test::expect_eq(d_res2.damage_applied, 0u);
  test::expect_eq(ship.damage(), 100);

  // add_fuel clamping to max capacity
  const double mass_before_fuel = ship.mass();
  ship.add_fuel(120.0);  // Can only take 80.0
  expect_near(ship.fuel(), 100.0);
  expect_near(ship.mass(), mass_before_fuel + 80.0 * MASS_FUEL);

  // consume_fuel clamping to 0
  const double mass_before_consume = ship.mass();
  ship.consume_fuel(150.0);  // Has only 100.0
  expect_near(ship.fuel(), 0.0);
  expect_near(ship.mass(), mass_before_consume - 100.0 * MASS_FUEL);

  // add_resource clamping to max capacity
  const double mass_before_res = ship.mass();
  ship.add_resource(300);  // Max 200, currently 50, takes 150
  test::expect_eq(ship.resource(), 200);
  expect_near(ship.mass(), mass_before_res + 150.0 * MASS_RESOURCE);

  // consume_resource clamping to 0
  const double mass_before_res_consume = ship.mass();
  ship.consume_resource(300);
  test::expect_eq(ship.resource(), 0);
  expect_near(ship.mass(), mass_before_res_consume - 200.0 * MASS_RESOURCE);

  // add_destruct clamping with 32-bit/64-bit value > 65535
  const double mass_before_des = ship.mass();
  ship.add_destruct(70000);
  test::expect_eq(ship.destruct(), 70010);
  expect_near(ship.mass(), mass_before_des + 70000.0 * MASS_DESTRUCT);

  // add_popn clamping to available joint crew capacity
  const double mass_before_popn = ship.mass();
  ship.add_popn(30, 2.0);  // Available 50 (100 - (40+10)), takes 30
  test::expect_eq(ship.popn(), 70);
  expect_near(ship.mass(), mass_before_popn + 30.0 * 2.0);

  // add_troops clamping to remaining joint crew capacity
  const double mass_before_troops = ship.mass();
  ship.add_troops(50, 2.0);  // Available 20 (100 - (70+10)), takes 20
  test::expect_eq(ship.troops(), 30);
  expect_near(ship.mass(), mass_before_troops + 20.0 * 2.0);

  // Further additions rejected when joint capacity is reached
  const double mass_full = ship.mass();
  ship.add_popn(10, 2.0);  // Available 0, takes 0
  test::expect_eq(ship.popn(), 70);
  expect_near(ship.mass(), mass_full);

  // apply_casualties deducting crew and troops and updating mass
  const double mass_before_cas = ship.mass();
  auto cas = ship.apply_casualties(20, 10, 2.0);
  test::expect_eq(cas.crew, 20);
  test::expect_eq(cas.troops, 10);
  test::expect_eq(ship.popn(), 50);
  test::expect_eq(ship.troops(), 20);
  expect_near(ship.mass(), mass_before_cas - 30.0 * 2.0);

  // remove_popn clamping to 0
  const double mass_before_rem_popn = ship.mass();
  ship.remove_popn(150, 2.0);  // Currently 50, removes all 50
  test::expect_eq(ship.popn(), 0);
  expect_near(ship.mass(), mass_before_rem_popn - 50.0 * 2.0);

  // remove_troops clamping to 0
  const double mass_before_rem_troops = ship.mass();
  ship.remove_troops(150, 2.0);  // Currently 20, removes all 20
  test::expect_eq(ship.troops(), 0);
  expect_near(ship.mass(), mass_before_rem_troops - 20.0 * 2.0);
}

void test_dynamic_base_mass() {
  std::println(std::cout, "Testing Ship::base_mass() dynamic calculation...");
  ship_struct sdata{
      .armor = 5,
      .size = 50,
      .base_mass = 9999.0,  // Stored legacy value should be completely ignored
      .primary_battery = GunBattery::create(4, guntype_t::MEDIUM),
      .secondary_battery = GunBattery::create(2, guntype_t::LIGHT),
      .max_hanger = 10,
  };
  Ship ship{sdata};

  // body = max(0, 50 - 10) = 40
  test::expect_eq(ship.shipbody(), 40u);
  test::expect_eq(ship.hanger_space(), 10u);

  // expected = 1.0 + 1.0 * 5 + 0.2 * 40 + 0.1 * 10 + 0.2 * 4 * 2 + 0.2 * 2 * 1
  //          = 1.0 + 5.0 + 8.0 + 1.0 + 1.6 + 0.4 = 17.0
  expect_near(ship.base_mass(), 17.0);

  // Dynamically reacts to structural changes without manual base_mass
  // assignment
  ship.armor() = 10;  // +5.0 mass
  expect_near(ship.base_mass(), 22.0);

  // Underflow protection: max_hanger > size clamps to 0
  ship.max_hanger() = 100;
  test::expect_eq(ship.shipbody(), 0u);
}

void test_gun_caliber_domain() {
  std::println(std::cout, "Testing guntype_t and gun_caliber()...");

  test::expect_eq(gun_caliber(guntype_t::NONE), 0u);
  test::expect_eq(gun_caliber(guntype_t::LIGHT), 1u);
  test::expect_eq(gun_caliber(guntype_t::MEDIUM), 2u);
  test::expect_eq(gun_caliber(guntype_t::HEAVY), 3u);

  test::expect_eq(caliber_char(guntype_t::NONE), ' ');
  test::expect_eq(caliber_char(guntype_t::LIGHT), 'L');
  test::expect_eq(caliber_char(guntype_t::MEDIUM), 'M');
  test::expect_eq(caliber_char(guntype_t::HEAVY), 'H');
}

void test_gun_battery_invariants_and_operations() {
  std::println(std::cout,
               "Testing GunBattery value object invariants and operations...");

  // Default value object
  GunBattery empty;
  test::expect_eq(empty.count, 0u);
  test::expect_eq(empty.caliber, guntype_t::NONE);
  test::expect_true(empty.is_empty());
  test::expect_false(empty.has_guns());
  test::expect_eq(empty.caliber_multiplier(), 0u);
  expect_near(empty.mass_contribution(), 0.0);

  // Normalization via factory
  auto valid = GunBattery::create(10, guntype_t::HEAVY);
  test::expect_eq(valid.count, 10u);
  test::expect_eq(valid.caliber, guntype_t::HEAVY);
  test::expect_true(valid.has_guns());
  test::expect_false(valid.is_empty());
  test::expect_eq(valid.caliber_multiplier(), 3u);
  expect_near(valid.mass_contribution(), 30.0);

  auto zero_count = GunBattery::create(0, guntype_t::HEAVY);
  test::expect_eq(zero_count.count, 0u);
  test::expect_eq(zero_count.caliber, guntype_t::NONE);
  test::expect_true(zero_count.is_empty());

  auto none_caliber = GunBattery::create(10, guntype_t::NONE);
  test::expect_eq(none_caliber.count, 0u);
  test::expect_eq(none_caliber.caliber, guntype_t::NONE);
  test::expect_true(none_caliber.is_empty());

  // Damage operations returning actual guns destroyed
  test::expect_eq(valid.damage(3), 3u);
  test::expect_eq(valid.count, 7u);
  test::expect_eq(valid.caliber, guntype_t::HEAVY);

  // Partial damage destroying remainder
  test::expect_eq(valid.damage(7), 7u);
  test::expect_eq(valid.count, 0u);
  test::expect_eq(valid.caliber, guntype_t::NONE);
  test::expect_true(valid.is_empty());

  // Overkill damage: reports actual clamped destroyed count
  auto overkill = GunBattery::create(5, guntype_t::MEDIUM);
  test::expect_eq(overkill.damage(10), 5u);
  test::expect_eq(overkill.count, 0u);
  test::expect_eq(overkill.caliber, guntype_t::NONE);
  test::expect_eq(overkill.damage(3), 0u);

  // Ship battery encapsulation and atomic setters
  Ship ship;
  ship.set_primary_battery(6, guntype_t::MEDIUM);
  test::expect_eq(ship.primary_battery().count, 6u);
  test::expect_eq(ship.primary_battery().caliber, guntype_t::MEDIUM);
  test::expect_true(ship.primary_battery().has_guns());

  ship.set_secondary_battery(4, guntype_t::LIGHT);
  test::expect_eq(ship.secondary_battery().count, 4u);
  test::expect_eq(ship.secondary_battery().caliber, guntype_t::LIGHT);
  test::expect_true(ship.secondary_battery().has_guns());

  // Damage via Ship domain methods returning actual guns lost
  test::expect_eq(ship.damage_primary_guns(2), 2u);
  test::expect_eq(ship.primary_battery().count, 4u);
  test::expect_eq(ship.primary_battery().caliber, guntype_t::MEDIUM);

  test::expect_eq(ship.damage_primary_guns(10), 4u);  // clamped to 4 remaining
  test::expect_eq(ship.primary_battery().count, 0u);
  test::expect_eq(ship.primary_battery().caliber, guntype_t::NONE);
  test::expect_false(ship.primary_battery().has_guns());

  test::expect_eq(ship.damage_secondary_guns(4), 4u);
  test::expect_eq(ship.secondary_battery().count, 0u);
  test::expect_eq(ship.secondary_battery().caliber, guntype_t::NONE);
  test::expect_false(ship.secondary_battery().has_guns());
}

void test_active_gun_battery_and_formatting() {
  // 1. GunBattery to_string formatting
  test::expect_eq(GunBattery{}.to_string(), "0 ");
  test::expect_eq(GunBattery::create(10, guntype_t::LIGHT).to_string(), "10L");
  test::expect_eq(GunBattery::create(5, guntype_t::MEDIUM).to_string(), "5M");
  test::expect_eq(GunBattery::create(2, guntype_t::HEAVY).to_string(), "2H");

  // 2. Active battery selection on Ship
  Ship ship;
  test::expect_eq(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::NONE);
  test::expect_eq(ship.active_guns(), 0u);
  test::expect_eq(ship.battery_summary(), "0 /0 ");

  // Mode set to PRIMARY, but battery has no guns
  ship.guns() = PRIMARY;
  test::expect_eq(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::NONE);
  test::expect_eq(ship.active_guns(), 0u);

  // Mount primary guns
  ship.set_primary_battery(8, guntype_t::MEDIUM);
  test::expect_ne(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_battery()->count, 8u);
  test::expect_eq(ship.active_gun_battery()->caliber, guntype_t::MEDIUM);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::MEDIUM);
  test::expect_eq(ship.active_guns(), 8u);
  test::expect_eq(ship.battery_summary(), "8M/0 ");

  // Switch to SECONDARY while empty
  ship.guns() = SECONDARY;
  test::expect_eq(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::NONE);
  test::expect_eq(ship.active_guns(), 0u);

  // Mount secondary guns
  ship.set_secondary_battery(4, guntype_t::LIGHT);
  test::expect_ne(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_battery()->count, 4u);
  test::expect_eq(ship.active_gun_battery()->caliber, guntype_t::LIGHT);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::LIGHT);
  test::expect_eq(ship.active_guns(), 4u);
  test::expect_eq(ship.battery_summary(), "8M/4L");

  // Destroy secondary guns; active battery returns nullptr safely
  test::expect_eq(ship.damage_secondary_guns(4), 4u);
  test::expect_eq(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::NONE);
  test::expect_eq(ship.active_guns(), 0u);
  test::expect_eq(ship.battery_summary(), "8M/0 ");

  // Switch back to PRIMARY with surviving guns
  ship.guns() = PRIMARY;
  test::expect_ne(ship.active_gun_battery(), nullptr);
  test::expect_eq(ship.active_guns(), 8u);
  test::expect_eq(ship.active_gun_caliber(), guntype_t::MEDIUM);

  // 3. Combat retaliation integration with active battery
  ship.alive() = true;
  ship.type() = ShipType::STYPE_BATTLE;
  ship.popn() = 100;
  ship.destruct() = 50;
  ship.retaliate() = 10;
  test::expect_eq(ship.retal_strength(), 8u);  // limited by 8 primary guns

  ship.retaliate() = 5;
  test::expect_eq(ship.retal_strength(), 5u);  // limited by salvo order

  ship.guns() = ActiveBattery::NONE;
  test::expect_eq(ship.retal_strength(), 0u);  // offline weapons
}

void test_ship_continuous_coordinates() {
  ship_struct sdata{};
  sdata.coordinates = UniverseCoordinates(-450.0, 1200.0);
  Ship ship{sdata};

  test::expect_eq(ship.coordinates(), UniverseCoordinates(-450.0, 1200.0));
  ship.set_coordinates(UniverseCoordinates(300.0, -800.0));
  test::expect_eq(ship.coordinates(), UniverseCoordinates(300.0, -800.0));
  test::expect_eq(ship.coordinates().x, 300.0);
  test::expect_eq(ship.coordinates().y, -800.0);
}

void test_crystals_domain() {
  ship_struct sdata{};
  Ship ship{sdata};

  test::expect_eq(ship.crystals(), 0U);
  test::expect_eq(ship.max_crystals_capacity(), 127);

  // admin_override_crystals with clamping
  ship.admin_override_crystals(50);
  test::expect_eq(ship.crystals(), 50U);
  ship.admin_override_crystals(200);
  test::expect_eq(ship.crystals(), 127U);

  // consume_crystals
  ship.admin_override_crystals(10);
  auto consumed = ship.consume_crystals(4);
  test::expect_eq(consumed, 4U);
  test::expect_eq(ship.crystals(), 6U);

  // consume more than available clamps to available
  consumed = ship.consume_crystals(20);
  test::expect_eq(consumed, 6U);
  test::expect_eq(ship.crystals(), 0U);

  // negative or zero consume is no-op
  consumed = ship.consume_crystals(-5);
  test::expect_eq(consumed, 0U);
  test::expect_eq(ship.crystals(), 0U);

  // add_crystals
  ship.add_crystals(15);
  test::expect_eq(ship.crystals(), 15U);

  // add_crystals clamps at max capacity
  ship.add_crystals(200);
  test::expect_eq(ship.crystals(), 127U);

  // add_crystals with negative delegates to consume
  ship.add_crystals(-27);
  test::expect_eq(ship.crystals(), 100U);
}

void test_local_mass_and_set_mass() {
  ship_struct sdata{
      .fuel = 200.0,
      .armor = 10,
      .size = 100,
      .destruct = 5,
      .resource = 50,
      .popn = 20,
      .troops = 10,
  };
  Ship ship{sdata};

  // set_mass updates data_.mass directly
  ship.set_mass(350.0);
  expect_near(ship.mass(), 350.0);

  // local_mass computes intrinsic mass:
  // base_mass() + fuel * MASS_FUEL + res * MASS_RESOURCE + des * MASS_DESTRUCT
  // + (popn + troops) * race_mass
  const double expected_local = ship.base_mass() + 200.0 * MASS_FUEL +
                                50.0 * MASS_RESOURCE + 5.0 * MASS_DESTRUCT +
                                (20.0 + 10.0) * 1.5;
  expect_near(ship.local_mass(1.5), expected_local);
}

void test_simulated_ship() {
  std::println(std::cout, "Testing SimulatedShip...");
  ship_struct sdata{
      .number = 42,
      .fuel = 50.0,
      .mass = 120.0,
      .max_fuel = 200.0,
      .base_mass = 100.0,
      .type = ShipType::STYPE_SHUTTLE,
  };
  Ship base_ship{sdata};
  test::expect_false(base_ship.is_simulation());
  test::expect_eq(base_ship.number(), 42);

  SimulatedShip sim{base_ship};
  test::expect_true(sim.is_simulation());
  test::expect_eq(sim.number(), 0);  // Identity neutralized

  // Test set_simulated_fuel and mass update
  sim.set_simulated_fuel(120.0, 1.0);
  expect_near(sim.fuel(), 120.0);
  expect_near(sim.mass(), sim.local_mass(1.0));

  // Clamping to max capacity
  sim.set_simulated_fuel(500.0, 1.0);
  expect_near(sim.fuel(), 200.0);

  // Test set_simulated_destination
  sim.land_on_planet();
  test::expect_true(sim.is_landed());
  sim.set_simulated_destination(ScopeLevel::LEVEL_PLAN, 3, 2, 0);
  test::expect_eq(sim.whatdest(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(sim.deststar(), 3);
  test::expect_eq(sim.destpnum(), 2);
  test::expect_eq(sim.dock_state(), DockState::Spaceborne);
  test::expect_false(sim.docked());
}

void test_ship_joint_crew_capacity() {
  std::println(std::cout,
               "Testing Ship joint crew capacity and casualty operations...");
  ship_struct sdata{
      .max_crew = 60,
      .popn = 0,
      .troops = 0,
  };
  Ship ship{sdata};

  test::expect_eq(ship.available_crew_capacity(), 60);

  // Add 40 civilian crew with race_mass = 1.5
  ship.add_popn(40, 1.5);
  test::expect_eq(ship.popn(), 40);
  test::expect_eq(ship.troops(), 0);
  test::expect_eq(ship.available_crew_capacity(), 20);
  expect_near(ship.mass(), 40 * 1.5);

  // Add 30 troops - only 20 berths remaining
  ship.add_troops(30, 1.5);
  test::expect_eq(ship.popn(), 40);
  test::expect_eq(ship.troops(), 20);
  test::expect_eq(ship.available_crew_capacity(), 0);
  expect_near(ship.mass(), 60 * 1.5);

  // Further additions fail gracefully when full
  ship.add_popn(10, 1.5);
  test::expect_eq(ship.popn(), 40);
  ship.add_troops(10, 1.5);
  test::expect_eq(ship.troops(), 20);

  // Negative delta delegates to remove_*
  ship.add_popn(-10, 1.5);
  test::expect_eq(ship.popn(), 30);
  test::expect_eq(ship.available_crew_capacity(), 10);
  expect_near(ship.mass(), 50 * 1.5);

  // Apply casualties
  auto cas = ship.apply_casualties(15, 10, 1.5);
  test::expect_eq(cas.crew, 15);
  test::expect_eq(cas.troops, 10);
  test::expect_eq(ship.popn(), 15);
  test::expect_eq(ship.troops(), 10);
  test::expect_eq(ship.available_crew_capacity(), 35);
  expect_near(ship.mass(), 25 * 1.5);

  // Over-casualties clamped to current counts
  cas = ship.apply_casualties(50, 50, 1.5);
  test::expect_eq(cas.crew, 15);
  test::expect_eq(cas.troops, 10);
  test::expect_eq(ship.popn(), 0);
  test::expect_eq(ship.troops(), 0);
  test::expect_eq(ship.available_crew_capacity(), 60);
  expect_near(ship.mass(), 0.0);
}

void test_damage_and_radiation_subsystem() {
  std::println(std::cout, "Testing Ship damage and radiation subsystem...");
  ship_struct sdata{
      .damage = 0,
      .rad = 0,
  };
  Ship ship{sdata};

  // 1. apply_damage returns DamageResult
  auto res = ship.apply_damage(30);
  test::expect_eq(res.damage_applied, 30u);
  test::expect_eq(res.new_damage, 30u);
  test::expect_false(res.destroyed);
  test::expect_eq(ship.damage(), 30u);

  // Incremental damage
  res = ship.apply_damage(20);
  test::expect_eq(res.damage_applied, 20u);
  test::expect_eq(res.new_damage, 50u);
  test::expect_false(res.destroyed);
  test::expect_eq(ship.damage(), 50u);

  // Lethal damage exactly reaching 100
  res = ship.apply_damage(50);
  test::expect_eq(res.damage_applied, 50u);
  test::expect_eq(res.new_damage, 100u);
  test::expect_true(res.destroyed);
  test::expect_eq(ship.damage(), 100u);

  // Damage to already destroyed ship
  res = ship.apply_damage(20);
  test::expect_eq(res.damage_applied, 0u);
  test::expect_eq(res.new_damage, 100u);
  test::expect_true(res.destroyed);
  test::expect_eq(ship.damage(), 100u);

  // 2. repair_damage with bounds safety
  ship.repair_damage(40);
  test::expect_eq(ship.damage(), 60u);

  // Over-repair clamped to 0 without underflow
  ship.repair_damage(100);
  test::expect_eq(ship.damage(), 0u);

  // Signed negative repair is ignored (does not wrap around to full heal)
  ship.admin_override_damage(50);
  ship.repair_damage(-20);
  test::expect_eq(ship.damage(), 50u);

  // 3. Signed negative apply_damage is ignored (does not wrap to instant
  // destruction)
  res = ship.apply_damage(-15);
  test::expect_eq(res.damage_applied, 0u);
  test::expect_eq(res.new_damage, 50u);
  test::expect_false(res.destroyed);
  test::expect_eq(ship.damage(), 50u);

  // 4. Overkill damage clamped to 100 without overflow
  res = ship.apply_damage(std::numeric_limits<damage_t>::max());
  test::expect_eq(res.damage_applied, 50u);
  test::expect_eq(res.new_damage, 100u);
  test::expect_true(res.destroyed);

  // 5. Floating point overload
  ship.admin_override_damage(10);
  res = ship.apply_damage(15.4);
  test::expect_eq(res.damage_applied, 15u);
  test::expect_eq(res.new_damage, 25u);

  // 6. apply_radiation peak-dose semantics
  ship.apply_radiation(30);
  test::expect_eq(ship.rad(), 30u);

  // Lower dosage does not reduce radiation
  ship.apply_radiation(20);
  test::expect_eq(ship.rad(), 30u);

  // Higher dosage sets new peak
  ship.apply_radiation(75);
  test::expect_eq(ship.rad(), 75u);

  // Clamped at 100%
  ship.apply_radiation(150);
  test::expect_eq(ship.rad(), 100u);

  // Signed negative dosage does not mutate
  ship.repair_radiation(60);
  test::expect_eq(ship.rad(), 40u);
  ship.apply_radiation(-10);
  test::expect_eq(ship.rad(), 40u);

  // 7. repair_radiation bounds safety
  ship.repair_radiation(15);
  test::expect_eq(ship.rad(), 25u);

  // Over-repair clamped to 0 without underflow
  ship.repair_radiation(50);
  test::expect_eq(ship.rad(), 0u);

  // Signed negative repair does not mutate
  ship.apply_radiation(20);
  ship.repair_radiation(-10);
  test::expect_eq(ship.rad(), 20u);

  // 8. Admin overrides with signedness protection
  ship.admin_override_damage(-10);
  test::expect_eq(ship.damage(), 0u);
  ship.admin_override_damage(150);
  test::expect_eq(ship.damage(), 100u);

  ship.admin_override_radiation(-10);
  test::expect_eq(ship.rad(), 0u);
  ship.admin_override_radiation(150);
  test::expect_eq(ship.rad(), 100u);
}

void test_dock_state_transitions() {
  std::println(std::cout,
               "Testing Ship DockState transitions and invariants...");
  ship_struct sdata{};
  Ship ship(sdata);

  // Default initial state is Spaceborne
  test::expect_eq(ship.dock_state(), DockState::Spaceborne);
  test::expect_true(ship.is_spaceborne());
  test::expect_false(ship.is_landed());
  test::expect_false(ship.is_docked());
  test::expect_false(ship.docked());
  test::expect_false(ship.carrier_id().has_value());

  // Transition to Landed
  ship.land_on_planet();
  test::expect_eq(ship.dock_state(), DockState::Landed);
  test::expect_false(ship.is_spaceborne());
  test::expect_true(ship.is_landed());
  test::expect_false(ship.is_docked());
  test::expect_true(ship.docked());
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship.whatdest(), ScopeLevel::LEVEL_PLAN);
  test::expect_false(ship.carrier_id().has_value());

  // Transition to Docked in carrier
  const shipnum_t carrier_no{42};
  ship.dock_into_carrier(carrier_no);
  test::expect_eq(ship.dock_state(), DockState::Docked);
  test::expect_false(ship.is_spaceborne());
  test::expect_false(ship.is_landed());
  test::expect_true(ship.is_docked());
  test::expect_true(ship.docked());
  test::expect_true(ship.carrier_id().has_value());
  test::expect_eq(*ship.carrier_id(), carrier_no);
  test::expect_eq(ship.destshipno(), carrier_no);
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(ship.whatdest(), ScopeLevel::LEVEL_SHIP);

  // Launch to star orbit clears carrier reference
  ship.launch_to_orbit(ScopeLevel::LEVEL_STAR);
  test::expect_eq(ship.dock_state(), DockState::Spaceborne);
  test::expect_true(ship.is_spaceborne());
  test::expect_false(ship.is_landed());
  test::expect_false(ship.is_docked());
  test::expect_false(ship.docked());
  test::expect_false(ship.carrier_id().has_value());
  test::expect_eq(ship.destshipno(), shipnum_t{0});
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_STAR);

  // Re-land and launch to planet orbit (default level)
  ship.land_on_planet();
  test::expect_true(ship.is_landed());
  ship.launch_to_orbit();
  test::expect_eq(ship.dock_state(), DockState::Spaceborne);
  test::expect_true(ship.is_spaceborne());
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_PLAN);

  // Transition to Docked with another spaceborne ship (ship-to-ship mooring)
  const shipnum_t other_ship_no{99};
  ship.dock_with_ship(other_ship_no);
  test::expect_eq(ship.dock_state(), DockState::Docked);
  test::expect_true(ship.is_docked());
  test::expect_false(ship.is_landed());
  test::expect_false(ship.is_spaceborne());
  test::expect_true(ship.docked());
  // whatorbits remains LEVEL_PLAN (orbital frame is preserved!)
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship.whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(ship.destshipno(), other_ship_no);
  // Not in a carrier hangar:
  test::expect_false(ship.carrier_id().has_value());
  test::expect_true(ship.moored_ship_id().has_value());
  test::expect_eq(*ship.moored_ship_id(), other_ship_no);

  // Undock from spaceborne ship
  ship.undock_from_ship();
  test::expect_eq(ship.dock_state(), DockState::Spaceborne);
  test::expect_true(ship.is_spaceborne());
  test::expect_false(ship.is_docked());
  test::expect_eq(ship.whatorbits(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ship.destshipno(), shipnum_t{0});
  test::expect_eq(ship.whatdest(), ScopeLevel::LEVEL_UNIV);
  test::expect_false(ship.moored_ship_id().has_value());
}

void test_carrier_craft_loading_and_unloading() {
  std::println(std::cout,
               "Testing Carrier craft loading and unloading encapsulation...");
  ship_struct carrier_data{
      .mass = 500.0,
      .base_mass = 500.0,
      .type = ShipType::STYPE_CARRIER,
      .hanger = 0,
      .max_hanger = 100,
  };
  Ship carrier{carrier_data};

  ship_struct child_data{
      .mass = 50.0,
      .size = 10,
      .base_mass = 40.0,
      .type = ShipType::STYPE_FIGHTER,
  };
  Ship child{child_data};

  // Load craft into carrier
  carrier.load_docked_craft(child);
  test::expect_eq(carrier.hanger(), 10);
  expect_near(carrier.mass(), 550.0);

  // Load second craft using size & mass directly
  carrier.load_docked_craft(15, 75.0);
  test::expect_eq(carrier.hanger(), 25);
  expect_near(carrier.mass(), 625.0);

  // Unload second craft
  carrier.unload_docked_craft(15, 75.0);
  test::expect_eq(carrier.hanger(), 10);
  expect_near(carrier.mass(), 550.0);

  // Unload child craft
  carrier.unload_docked_craft(child);
  test::expect_eq(carrier.hanger(), 0);
  expect_near(carrier.mass(), 500.0);

  // Underflow safety: unloading more mass than carried clamps cleanly to
  // base_mass()
  const double base = carrier.base_mass();
  carrier.unload_docked_craft(20, carrier.mass() + 100.0);
  test::expect_eq(carrier.hanger(), 0);
  expect_near(carrier.mass(), base);
}

void test_ship_cargo_transfer() {
  std::println(std::cout, "Testing Ship::transfer_cargo_to()...");

  // Test char_to_ship_cargo mapping
  test::expect_eq(char_to_ship_cargo('r'), ShipCargoType::Resource);
  test::expect_eq(char_to_ship_cargo('d'), ShipCargoType::Destruct);
  test::expect_eq(char_to_ship_cargo('f'), ShipCargoType::Fuel);
  test::expect_eq(char_to_ship_cargo('x'), ShipCargoType::Crystal);
  test::expect_eq(char_to_ship_cargo('&'), ShipCargoType::Crystal);
  test::expect_eq(char_to_ship_cargo('c'), ShipCargoType::Crew);
  test::expect_eq(char_to_ship_cargo('m'), ShipCargoType::Troops);
  test::expect_eq(char_to_ship_cargo('z'), std::nullopt);

  // Set up source and destination ships
  ship_struct src_data{
      .fuel = 150.0,
      .mass = 1000.0,
      .max_crew = 100,
      .max_resource = 500,
      .max_destruct = 200,
      .max_fuel = 300.0,
      .destruct = 80,
      .resource = 200,
      .popn = 50,
      .troops = 20,
      .crystals = 30,
  };
  Ship src{src_data};

  ship_struct dst_data{
      .fuel = 50.0,
      .mass = 800.0,
      .max_crew = 100,
      .max_resource = 500,
      .max_destruct = 200,
      .max_fuel = 300.0,
      .destruct = 20,
      .resource = 100,
      .popn = 10,
      .troops = 5,
      .crystals = 10,
  };
  Ship dst{dst_data};

  const double initial_aggregate_mass = src.mass() + dst.mass();

  // 1. Transfer Resource
  auto transferred = src.transfer_cargo_to(dst, ShipCargoType::Resource, 50);
  test::expect_eq(transferred, 50);
  test::expect_eq(src.resource(), 150);
  test::expect_eq(dst.resource(), 150);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 2. Transfer Destruct
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Destruct, 30);
  test::expect_eq(transferred, 30);
  test::expect_eq(src.destruct(), 50);
  test::expect_eq(dst.destruct(), 50);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 3. Transfer Fuel
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Fuel, 40);
  test::expect_eq(transferred, 40);
  expect_near(src.fuel(), 110.0);
  expect_near(dst.fuel(), 90.0);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 4. Transfer Crystal
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Crystal, 5);
  test::expect_eq(transferred, 5);
  test::expect_eq(src.crystals(), 25);
  test::expect_eq(dst.crystals(), 15);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 5. Transfer Crew
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Crew, 15, 2.0);
  test::expect_eq(transferred, 15);
  test::expect_eq(src.popn(), 35);
  test::expect_eq(dst.popn(), 25);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 6. Transfer Troops
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Troops, 10, 2.0);
  test::expect_eq(transferred, 10);
  test::expect_eq(src.troops(), 10);
  test::expect_eq(dst.troops(), 15);
  expect_near(src.mass() + dst.mass(), initial_aggregate_mass);

  // 7. Capacity clamping: destination capacity limit
  // dst has max_resource=500, currently 150 -> capacity is 350
  // src has resource=150 -> transfer request of 200 should be clamped to 150
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Resource, 200);
  test::expect_eq(transferred, 150);
  test::expect_eq(src.resource(), 0);
  test::expect_eq(dst.resource(), 300);

  // 8. Source exhaustion: transferring when source has 0 returns 0
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Resource, 50);
  test::expect_eq(transferred, 0);

  // 9. Negative or zero amount returns 0
  transferred = dst.transfer_cargo_to(src, ShipCargoType::Resource, 0);
  test::expect_eq(transferred, 0);
  transferred = dst.transfer_cargo_to(src, ShipCargoType::Resource, -10);
  test::expect_eq(transferred, 0);

  // 10. Destination capacity clamping
  dst.consume_resource(dst.resource());  // reset dst to 0
  dst.add_resource(480);                 // dst has 480 / 500 (20 remaining)
  src.add_resource(50);                  // src has 50
  transferred = src.transfer_cargo_to(dst, ShipCargoType::Resource, 50);
  test::expect_eq(transferred, 20);
  test::expect_eq(src.resource(), 30);
  test::expect_eq(dst.resource(), 500);
}

void test_ship_moor_together_and_commandability() {
  std::println(
      std::cout,
      "Testing Ship::moor_together() and commandability predicates...");
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                        .owned_by(1, 2)
                        .in_star_orbit(0)
                        .build();
  shipnum_t s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                        .owned_by(1, 0)
                        .in_star_orbit(0)
                        .build();

  // Symmetric mooring via moor_together
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    ctx.em.mutate_ship(s2_id, [&](Ship& s2) { s1.moor_together(s2); });
  });

  const auto* s1_peek = ctx.em.peek_ship(s1_id);
  const auto* s2_peek = ctx.em.peek_ship(s2_id);
  test::expect_true(s1_peek->is_docked());
  test::expect_true(s2_peek->is_docked());
  test::expect_eq(s1_peek->destshipno(), s2_id);
  test::expect_eq(s2_peek->destshipno(), s1_id);

  // Authorization checks: governor 0 or matching governor
  test::expect_true(s1_peek->is_authorized_for(0));
  test::expect_true(s1_peek->is_authorized_for(2));
  test::expect_false(s1_peek->is_authorized_for(1));

  // Commandability checks
  test::expect_true(s1_peek->is_commandable_by(1, 0));
  test::expect_true(s1_peek->is_commandable_by(1, 2));
  test::expect_false(s1_peek->is_commandable_by(2, 0));
  test::expect_false(s1_peek->is_commandable_by(1, 1));

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 2);
  test::expect_true(g.check_commandable(*s1_peek));

  // Unauthorized governor fails check_commandable and receives telegram via
  // notify_dont_own_ship
  ctx.setup_game_obj(g, 1, 1);
  test::expect_false(g.check_commandable(*s1_peek));
  auto t_p1g1 = ctx.em.get_telegrams(1, 1);
  test::expect_eq(t_p1g1.size(), 1u);
  test::expect_true(t_p1g1[0].message.find("don't own") != std::string::npos);

  // Wrong player fails check_commandable and receives telegram via
  // notify_dont_own_ship
  ctx.setup_game_obj(g, 2, 0);
  test::expect_false(g.check_commandable(*s1_peek));
  auto t_p2g0 = ctx.em.get_telegrams(2, 0);
  test::expect_eq(t_p2g0.size(), 1u);
  test::expect_true(t_p2g0[0].message.find("don't own") != std::string::npos);

  // Irradiated inactive ship fails check_commandable
  ctx.em.mutate_ship(s1_id, [](Ship& s1) {
    s1.active() = false;
    s1.apply_radiation(100);
  });
  ctx.setup_game_obj(g, 1, 0);
  g.out.str("");
  test::expect_false(g.check_commandable(*ctx.em.peek_ship(s1_id)));
  test::expect_true(g.out.str().find("irradiated") != std::string::npos);

  // Destroyed ship fails check_commandable
  ctx.em.mutate_ship(s1_id, [](Ship& s1) { s1.alive() = false; });
  g.out.str("");
  test::expect_false(g.check_commandable(*ctx.em.peek_ship(s1_id)));
  test::expect_true(g.out.str().find("destroyed") != std::string::npos);
}

void test_mirror_aim_and_formatting_helpers() {
  std::println(std::cout,
               "Testing SpaceMirrorShip aim_direction and location helpers...");
  TestContext ctx;
  ctx.with_standard_universe();

  const auto mirror_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR, 1)
                             .owned_by(1)
                             .named("SolarMirror")
                             .in_star_orbit(0)
                             .build();
  const auto target_ship_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 2)
          .owned_by(1)
          .named("TargetCraft")
          .in_planet_orbit(0, 0)
          .build();
  const auto child_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 3)
                                 .owned_by(1)
                                 .named("Parasite")
                                 .build();
  ctx.em.mutate_ship(child_ship_id, [&](Ship& child) {
    child.dock_into_carrier(target_ship_id);
  });

  // Test location display helpers across all ScopeLevels
  test::expect_contains(dispshiploc_brief(ctx.em, *ctx.em.peek_ship(mirror_id)),
                        "/Sol");
  test::expect_eq(dispshiploc(ctx.em, *ctx.em.peek_ship(mirror_id)), "/Sol");
  test::expect_eq(prin_ship_orbits(ctx.em, *ctx.em.peek_ship(mirror_id)),
                  "/Sol");

  test::expect_contains(
      dispshiploc_brief(ctx.em, *ctx.em.peek_ship(target_ship_id)),
      "/Sol/Eart");
  test::expect_eq(dispshiploc(ctx.em, *ctx.em.peek_ship(target_ship_id)),
                  "/Sol/Earth");
  test::expect_eq(prin_ship_orbits(ctx.em, *ctx.em.peek_ship(target_ship_id)),
                  "/Sol/Earth");
  test::expect_eq(prin_ship_orbits(ctx.em, *ctx.em.peek_ship(child_ship_id)),
                  "/Sol/Earth");
  test::expect_eq(dispshiploc_brief(ctx.em, *ctx.em.peek_ship(child_ship_id)),
                  "#2");
  test::expect_eq(dispshiploc(ctx.em, *ctx.em.peek_ship(child_ship_id)), "#2");

  ctx.em.mutate_ship(mirror_id, [](Ship& m) {
    m.whatorbits() = ScopeLevel::LEVEL_UNIV;
    m.set_coordinates({0.0, 0.0});
    m.whatdest() = ScopeLevel::LEVEL_STAR;
    m.deststar() = 1;
  });
  test::expect_eq(dispshiploc_brief(ctx.em, *ctx.em.peek_ship(mirror_id)), "/");
  test::expect_eq(dispshiploc(ctx.em, *ctx.em.peek_ship(mirror_id)), "/");
  test::expect_contains(prin_ship_orbits(ctx.em, *ctx.em.peek_ship(mirror_id)),
                        "/(");
  test::expect_eq(format_ship_dest(ctx.em, *ctx.em.peek_ship(mirror_id)),
                  "/Vega");

  // Test SpaceMirrorShip::aim_direction and EntityManager mirror resolution
  // across octants
  ctx.em.mutate_ship(mirror_id, [&](Ship& m) {
    auto* mirror = m.as<SpaceMirrorShip>();
    test::expect_ne(mirror, nullptr);

    // Unaimed (LEVEL_UNIV)
    mirror->aim() = {.level = ScopeLevel::LEVEL_UNIV};
    test::expect_false(
        ctx.em.resolve_mirror_target_coordinates(*mirror).has_value());
    test::expect_eq(ctx.em.resolve_mirror_aim_direction(*mirror), 0);

    // Aimed at star 0 and planet (0,0)
    mirror->aim() = {.snum = 0, .level = ScopeLevel::LEVEL_STAR};
    test::expect_true(
        ctx.em.resolve_mirror_target_coordinates(*mirror).has_value());
    mirror->aim() = {.snum = 0, .pnum = 0, .level = ScopeLevel::LEVEL_PLAN};
    test::expect_true(
        ctx.em.resolve_mirror_target_coordinates(*mirror).has_value());

    // Aimed at target_ship_id across all 8 compass headings and axes
    mirror->aim() = {.shipno = target_ship_id, .level = ScopeLevel::LEVEL_SHIP};
    const auto check_heading = [&](double tx, double ty) {
      ctx.em.mutate_ship(target_ship_id, [&](Ship& t) {
        t.set_coordinates(UniverseCoordinates{tx, ty});
      });
      return ctx.em.resolve_mirror_aim_direction(*mirror);
    };

    test::expect_eq(check_heading(0.0, -10.0), 0);  // North (xt == x, yt < y)
    test::expect_eq(check_heading(0.0, 10.0), 4);   // South (xt == x, yt > y)
    test::expect_eq(check_heading(10.0, 0.0), 2);   // East  (yt == y, xt > x)
    test::expect_eq(check_heading(-10.0, 0.0), 6);  // West  (yt == y, xt < x)
    test::expect_eq(check_heading(1.0, 10.0), 4);   // Positive dy, steep slope
    test::expect_eq(check_heading(10.0, 10.0), 3);  // SE (slope = 1.0)
    test::expect_eq(check_heading(10.0, 1.0), 2);   // E-SE (0 < slope < 0.414)
    test::expect_eq(check_heading(-10.0, 1.0), 6);  // W-SW (-0.414 < slope < 0)
    test::expect_eq(check_heading(-10.0, 10.0), 5);  // SW (slope = -1.0)
    test::expect_eq(check_heading(1.0, -10.0), 0);   // Negative dy, steep slope
    test::expect_eq(check_heading(-10.0, -10.0),
                    7);                              // NW (slope = 1.0, yt < y)
    test::expect_eq(check_heading(-10.0, -1.0), 6);  // W-NW (0 < slope < 0.414)
    test::expect_eq(check_heading(10.0, -1.0), 2);  // E-NE (-0.414 < slope < 0)
    test::expect_eq(check_heading(10.0, -10.0),
                    1);  // NE (slope = -1.0, yt < y)
  });

  // Verify kill_ship clears any SpaceMirrorShip aimed at the destroyed ship to
  // maintain foreign key referential integrity
  ctx.em.mutate_ship(target_ship_id,
                     [&](Ship& t) { ctx.em.kill_ship(player_t{1}, t); });
  const auto* mirror_after_kill =
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  test::expect_eq(mirror_after_kill->aimed_level(), ScopeLevel::LEVEL_UNIV);
  test::expect_false(
      ctx.em.resolve_mirror_target_coordinates(*mirror_after_kill).has_value());
}

void test_blueprint_complexity_defense_and_capture() {
  std::println(std::cout, "Testing set_factory_blueprint, complexity, "
                          "getdefense, capture_stuff...");
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // Blueprint and constructed state initialization for VN, Berserker, Mine,
  // Transdev, and specialty ships
  const auto* race1 = ctx.em.peek_race(1);
  Ship factory_ship;
  factory_ship.set_factory_blueprint(ShipType::STYPE_CRUISER, race1);
  test::expect_eq(factory_ship.build_type(), ShipType::STYPE_CRUISER);
  test::expect_true(factory_ship.calculate_size() > 0);
  test::expect_true(cost(factory_ship) > 0.0);
  expect_near(complexity(factory_ship), complexity(ShipType::STYPE_CRUISER));

  // Upgraded and downgraded stats in complexity()
  factory_ship.armor() += 10;
  factory_ship.max_speed() = 1;
  test::expect_true(complexity(factory_ship) > 0.0);

  for (const auto stype :
       {ShipType::OTYPE_VN, ShipType::OTYPE_BERS, ShipType::STYPE_MIRROR,
        ShipType::STYPE_POD, ShipType::OTYPE_CANIST, ShipType::STYPE_MISSILE,
        ShipType::STYPE_MINE, ShipType::OTYPE_TERRA, ShipType::OTYPE_TRANSDEV,
        ShipType::OTYPE_TOXWC}) {
    ship_struct sd{.build_type = stype, .type = stype};
    Ship special{sd};
    special.set_factory_blueprint(stype, race1);
    special.initialize_constructed_state(*race1, 0, 10.0, 5);
    test::expect_true(special.alive());

    const Ship& const_special = special;
    if (stype == ShipType::OTYPE_VN || stype == ShipType::OTYPE_BERS) {
      const auto* auto_ship = const_special.as<AutonomousShip>();
      test::expect_true(auto_ship != nullptr);
      test::expect_eq(auto_ship->progenitor(), player_t{1});
      test::expect_eq(auto_ship->generation(), 1u);
      test::expect_true(auto_ship->is_busy());
      test::expect_true(
          std::holds_alternative<MindData>(const_special.get_struct().special));
    } else if (stype == ShipType::STYPE_POD) {
      test::expect_true(
          std::holds_alternative<PodData>(const_special.get_struct().special));
    } else if (stype == ShipType::OTYPE_CANIST) {
      test::expect_true(std::holds_alternative<TimerData>(
          const_special.get_struct().special));
    } else if (stype == ShipType::STYPE_MISSILE) {
      test::expect_true(std::holds_alternative<ImpactData>(
          const_special.get_struct().special));
    } else if (stype == ShipType::STYPE_MINE) {
      test::expect_true(std::holds_alternative<TriggerData>(
          const_special.get_struct().special));
      test::expect_eq(const_special.as<MineShip>()->trigger_radius(), 100);
    } else if (stype == ShipType::OTYPE_TERRA) {
      test::expect_true(std::holds_alternative<TerraformData>(
          const_special.get_struct().special));
    } else if (stype == ShipType::OTYPE_TRANSDEV) {
      test::expect_true(std::holds_alternative<TransportData>(
          const_special.get_struct().special));
    } else if (stype == ShipType::OTYPE_TOXWC) {
      test::expect_true(std::holds_alternative<WasteData>(
          const_special.get_struct().special));
    }
  }

  // crash() fuel and damage checks
  ship_struct crash_sd{.fuel = 5.0, .damage = 100};
  Ship crash_ship{crash_sd};
  test::expect_true(std::get<0>(crash(crash_ship, 10.0)));
  test::expect_true(std::get<0>(crash(crash_ship, 1.0)));
  crash_ship.admin_override_damage(0);
  test::expect_false(std::get<0>(crash(crash_ship, 1.0)));

  // getdefense() spaceborne vs landed
  const auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER, 10)
                              .owned_by(1, 0)
                              .in_planet_orbit(0, 0)
                              .build();
  test::expect_eq(getdefense(ctx.em, *ctx.em.peek_ship(carrier_id)), 0);
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.set_land_coords({0, 0});
    c.land_on_planet();
  });
  test::expect_true(getdefense(ctx.em, *ctx.em.peek_ship(carrier_id)) >= 0);

  // capture_stuff() recursive capture of carried craft
  const auto f1_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER, 11)
                         .owned_by(2, 1)
                         .build();
  ctx.em.mutate_ship(f1_id, [&](Ship& f) { f.dock_into_carrier(carrier_id); });
  capture_stuff(*ctx.em.peek_ship(carrier_id), g);
  test::expect_eq(ctx.em.peek_ship(f1_id)->owner(), player_t{1});
}

void test_moveship_and_followable() {
  std::println(std::cout,
               "Testing moveship pipeline, followable, and do_merchant...");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.tech = 100.0; });
  const auto star0_coords = ctx.em.peek_star(0)->coordinates();
  const auto p0_coords =
      ctx.em.peek_planet(0, 0)->absolute_coordinates(*ctx.em.peek_star(0));

  // 1. followable() checks: alive, active, carrier-docked, same owner, range
  const auto s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 20)
                         .owned_by(1, 0)
                         .in_star_orbit(0)
                         .with_crew(50, 0)
                         .with_fuel(500.0)
                         .with_speed(9)
                         .with_tech(100.0)
                         .build();
  const auto s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 21)
                         .owned_by(2, 0)
                         .in_star_orbit(0)
                         .with_crew(50, 0)
                         .with_fuel(500.0)
                         .with_speed(9)
                         .build();

  test::expect_true(
      followable(ctx.em, *ctx.em.peek_ship(s1_id), *ctx.em.peek_ship(s2_id)));

  // Far away foreign non-allied ship is NOT followable
  ctx.em.mutate_ship(
      s2_id, [](Ship& s2) { s2.set_coordinates({999999.0, 999999.0}); });
  test::expect_false(
      followable(ctx.em, *ctx.em.peek_ship(s1_id), *ctx.em.peek_ship(s2_id)));

  // Dead target is NOT followable
  ctx.em.mutate_ship(s2_id, [&](Ship& s2) {
    s2.set_coordinates(star0_coords);
    s2.alive() = false;
  });
  test::expect_false(
      followable(ctx.em, *ctx.em.peek_ship(s1_id), *ctx.em.peek_ship(s2_id)));
  ctx.em.mutate_ship(s2_id, [](Ship& s2) { s2.alive() = true; });

  // 2. Hyperdrive charging (unmounted vs mounted), jump insufficient fuel, jump
  // arrival
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    s1.hyper_drive() = {.charge = 0, .on = true, .has = true};
    s1.mounted() = false;
    s1.deststar() = 1;
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.hyper_drive().charge, 1);

    s1.hyper_drive().charge = 0;
    s1.mounted() = true;
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(static_cast<int>(s1.hyper_drive().charge),
                    static_cast<int>(HYPER_DRIVE_READY_CHARGE));

    // Insufficient fuel disables hyperdrive
    s1.admin_override_fuel(0.1);
    moveship(ctx.em, s1, true, true, false);
    test::expect_false(s1.hyper_drive().on);

    // Sufficient fuel executes hyperdrive jump to star 1
    s1.admin_override_fuel(500.0);
    s1.hyper_drive().on = true;
    s1.hyper_drive().charge = HYPER_DRIVE_READY_CHARGE;
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.whatorbits(), ScopeLevel::LEVEL_STAR);
    test::expect_eq(s1.storbits(), starnum_t{1});
    test::expect_false(s1.hyper_drive().on);
  });

  // 3. Sublight navigation step and orbit breaking (PLAN -> STAR -> UNIV) + OOF
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    s1.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s1.storbits() = 0;
    s1.pnumorbits() = 0;
    s1.set_coordinates(p0_coords + SystemCoordinates{PLORBITSIZE + 5.0, 0.0});
    s1.navigate() = {.on = true, .turns = 1, .bearing = 90};
    moveship(ctx.em, s1, true, true, false);
    test::expect_false(s1.navigate().on);
    test::expect_eq(s1.whatorbits(), ScopeLevel::LEVEL_STAR);

    // Break star orbit into LEVEL_UNIV
    s1.set_coordinates(star0_coords +
                       SystemCoordinates{SYSTEMSIZE + 50.0, 0.0});
    s1.navigate() = {.on = true, .turns = 1, .bearing = 90};
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.whatorbits(), ScopeLevel::LEVEL_UNIV);

    // Sublight arrival at star 0
    s1.set_coordinates(star0_coords + SystemCoordinates{SYSTEMSIZE * 0.5, 0.0});
    s1.whatdest() = ScopeLevel::LEVEL_STAR;
    s1.deststar() = 0;
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.whatorbits(), ScopeLevel::LEVEL_STAR);
    test::expect_eq(s1.storbits(), starnum_t{0});
  });

  // 4. Sublight arrival at planet (0,0) + automated merchant route execution
  ctx.em.mutate_planet(0, 0, [](Planet& p) {
    auto& route = p.info(1).route_at(1);
    route.set = 1;
    route.dest_star = 1;
    route.dest_planet = 0;
    route.dest_coords = {0, 0};
    route.load = {
        .fuel = true, .destruct = true, .resources = true, .crystals = true};
    route.unload = {
        .fuel = false, .destruct = true, .resources = true, .crystals = true};
    p.info(1).fuel = 1000;
    p.info(1).resource = 1000;
    p.info(1).destruct = 1000;
    p.info(1).crystals = 500;  // Exceeds max_crystals_capacity (127)
  });

  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    s1.whatorbits() = ScopeLevel::LEVEL_STAR;
    s1.storbits() = 0;
    s1.pnumorbits() = 0;
    s1.set_coordinates(p0_coords + SystemCoordinates{DIST_TO_LAND * 0.5, 0.0});
    s1.whatdest() = ScopeLevel::LEVEL_PLAN;
    s1.deststar() = 0;
    s1.destpnum() = 0;
    s1.merchant() = 1;
    s1.admin_override_fuel(500.0);
    moveship(ctx.em, s1, true, true, false);
    // Merchant landed, loaded/unloaded, launched to orbit, and set jump orders
    test::expect_false(s1.is_landed());
    test::expect_eq(s1.deststar(), starnum_t{1});
    test::expect_true(s1.hyper_drive().on);
  });

  // 5. Sublight LEVEL_SHIP following and losing sight when out of range
  ctx.em.mutate_ship(s2_id, [&](Ship& s2) {
    s2.whatorbits() = ScopeLevel::LEVEL_STAR;
    s2.storbits() = 0;
    s2.set_coordinates(star0_coords + SystemCoordinates{1.0, 0.0});
  });
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    s1.hyper_drive().on = false;
    s1.whatorbits() = ScopeLevel::LEVEL_STAR;
    s1.storbits() = 0;
    s1.set_coordinates(star0_coords + SystemCoordinates{5.0, 0.0});
    s1.whatdest() = ScopeLevel::LEVEL_SHIP;
    s1.destshipno() = s2_id;
    s1.admin_override_fuel(500.0);
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.whatorbits(), ScopeLevel::LEVEL_STAR);
  });

  // Move target ship out of range so follower loses sight
  ctx.em.mutate_ship(
      s2_id, [](Ship& s2) { s2.set_coordinates({999999.0, 999999.0}); });
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    moveship(ctx.em, s1, true, true, false);
    test::expect_eq(s1.whatdest(), ScopeLevel::LEVEL_UNIV);
  });

  // 6. Deep space out-of-fuel loss for cheap / probe ship
  const auto probe_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, 22)
                            .owned_by(1, 0)
                            .with_fuel(0.0)
                            .with_speed(9)
                            .build();
  ctx.em.mutate_ship(probe_id, [&](Ship& probe) {
    probe.set_coordinates({50000.0, 50000.0});
    probe.whatorbits() = ScopeLevel::LEVEL_UNIV;
    probe.whatdest() = ScopeLevel::LEVEL_STAR;
    probe.deststar() = 0;
    moveship(ctx.em, probe, true, true, false);
    test::expect_false(probe.alive());
  });
}

}  // namespace

int main() {
  test_hull_efficiency();
  test_crew_ratio();
  test_fuel_predicates();
  test_available_resource_capacity();
  test_admin_overrides();
  test_fuel_consumption();
  test_resource_consumption();
  test_destruct_consumption();
  test_clamped_add_and_consume();
  test_ship_joint_crew_capacity();
  test_dynamic_base_mass();
  test_gun_caliber_domain();
  test_gun_battery_invariants_and_operations();
  test_active_gun_battery_and_formatting();
  test_ship_continuous_coordinates();
  test_crystals_domain();
  test_local_mass_and_set_mass();
  test_simulated_ship();
  test_damage_and_radiation_subsystem();
  test_dock_state_transitions();
  test_carrier_craft_loading_and_unloading();
  test_ship_cargo_transfer();
  test_ship_moor_together_and_commandability();
  test_mirror_aim_and_formatting_helpers();
  test_blueprint_complexity_defense_and_capture();
  test_moveship_and_followable();
  std::println(std::cout, "All Ship domain tests passed!");
  return 0;
}
