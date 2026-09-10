// SPDX-License-Identifier: Apache-2.0

/// \file ship_domain_test.cc
/// \brief Unit tests for Ship domain methods, bounded setters, and consumption
/// invariants.

import gb.entities;
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
  expect_near(getmass(ship), 17.0);

  // Dynamically reacts to structural changes without manual base_mass
  // assignment
  ship.armor() = 10;  // +5.0 mass
  expect_near(ship.base_mass(), 22.0);
  expect_near(getmass(ship), 22.0);

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
  test::expect_eq(retal_strength(ship), 8);  // limited by 8 primary guns

  ship.retaliate() = 5;
  test::expect_eq(retal_strength(ship), 5);  // limited by salvo order

  ship.guns() = ActiveBattery::NONE;
  test::expect_eq(retal_strength(ship), 0);  // offline weapons
}

void test_ship_continuous_coordinates() {
  ship_struct sdata{};
  sdata.xpos = -450.0;
  sdata.ypos = 1200.0;
  Ship ship{sdata};

  test::expect_eq(ship.coordinates(), UniverseCoordinates(-450.0, 1200.0));
  ship.set_coordinates(UniverseCoordinates(300.0, -800.0));
  expect_near(ship.xpos(), 300.0);
  expect_near(ship.ypos(), -800.0);
  test::expect_eq(ship.coordinates(), UniverseCoordinates(300.0, -800.0));
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
  sim.docked() = 1;
  sim.set_simulated_destination(ScopeLevel::LEVEL_PLAN, 3, 2, 0);
  test::expect_eq(sim.whatdest(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(sim.deststar(), 3);
  test::expect_eq(sim.destpnum(), 2);
  test::expect_eq(sim.docked(), 0);
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
  std::println(std::cout, "All Ship domain tests passed!");
  return 0;
}
