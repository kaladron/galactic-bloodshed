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

  ship.set_damage(0);
  expect_near(ship.hull_efficiency(), 1.0);

  ship.set_damage(25);
  expect_near(ship.hull_efficiency(), 0.75);

  ship.set_damage(50);
  expect_near(ship.hull_efficiency(), 0.50);

  ship.set_damage(100);
  expect_near(ship.hull_efficiency(), 0.0);

  // Clamped bounds
  ship.set_damage(150);
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

  ship.set_popn(50);
  expect_near(ship.crew_ratio(), 0.5);

  ship.set_popn(100);
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
  ship.fuel() = 5e-5;
  test::expect_false(ship.has_fuel());
  test::expect_false(ship.is_fully_fueled());

  // Normal fuel level
  ship.set_fuel(100.0);
  test::expect_true(ship.has_fuel());
  test::expect_false(ship.is_fully_fueled());

  // Full fuel within epsilon
  ship.set_fuel(200.0);
  test::expect_true(ship.has_fuel());
  test::expect_true(ship.is_fully_fueled());

  ship.fuel() = 200.0 - 5e-5;
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

  ship.set_resource(200);
  test::expect_eq(ship.available_resource_capacity(), 300);

  ship.set_resource(500);
  test::expect_eq(ship.available_resource_capacity(), 0);

  // If resource exceeds capacity, returns 0 rather than negative
  ship.resource() = 600;
  test::expect_eq(ship.available_resource_capacity(), 0);
}

void test_bounded_setters() {
  std::println(std::cout, "Testing Ship bounded setters...");
  ship_struct sdata{
      .max_crew = 50,
      .max_resource = 200,
      .max_destruct = 100,
      .max_fuel = 150.0,
  };
  Ship ship{sdata};

  // set_damage
  ship.set_damage(40);
  test::expect_eq(ship.damage(), 40);
  ship.set_damage(120);
  test::expect_eq(ship.damage(), 100);

  // set_fuel
  ship.set_fuel(75.0);
  expect_near(ship.fuel(), 75.0);
  ship.set_fuel(250.0);
  expect_near(ship.fuel(), 150.0);
  ship.set_fuel(-10.0);
  expect_near(ship.fuel(), 0.0);

  // set_popn
  ship.set_popn(30);
  test::expect_eq(ship.popn(), 30);
  ship.set_popn(80);
  test::expect_eq(ship.popn(), 50);
  ship.set_popn(-5);
  test::expect_eq(ship.popn(), 0);

  // set_resource
  ship.set_resource(120);
  test::expect_eq(ship.resource(), 120);
  ship.set_resource(350);
  test::expect_eq(ship.resource(), 200);
  ship.set_resource(-20);
  test::expect_eq(ship.resource(), 0);

  // set_destruct
  ship.set_destruct(60);
  test::expect_eq(ship.destruct(), 60);
  ship.set_destruct(180);
  test::expect_eq(ship.destruct(), 100);
  ship.set_destruct(-10);
  test::expect_eq(ship.destruct(), 0);
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
  ship.set_fuel(40.0);
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
  ship.apply_damage(20);
  test::expect_eq(ship.damage(), 100);
  ship.apply_damage(std::numeric_limits<damage_t>::max());
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

  // add_popn clamping to max crew capacity
  const double mass_before_popn = ship.mass();
  ship.add_popn(100, 2.0);  // Max 100, currently 40, takes 60
  test::expect_eq(ship.popn(), 100);
  expect_near(ship.mass(), mass_before_popn + 60.0 * 2.0);

  // add_troops clamping to max crew capacity
  const double mass_before_troops = ship.mass();
  ship.add_troops(120, 2.0);  // Max 100, currently 10, takes 90
  test::expect_eq(ship.troops(), 100);
  expect_near(ship.mass(), mass_before_troops + 90.0 * 2.0);
}

void test_dynamic_base_mass() {
  std::println(std::cout, "Testing Ship::base_mass() dynamic calculation...");
  ship_struct sdata{
      .armor = 5,
      .size = 50,
      .base_mass = 9999.0,  // Stored legacy value should be completely ignored
      .primary = 4,
      .primtype = guntype_t::MEDIUM,
      .secondary = 2,
      .sectype = guntype_t::LIGHT,
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

}  // namespace

int main() {
  test_hull_efficiency();
  test_crew_ratio();
  test_fuel_predicates();
  test_available_resource_capacity();
  test_bounded_setters();
  test_fuel_consumption();
  test_resource_consumption();
  test_destruct_consumption();
  test_clamped_add_and_consume();
  test_dynamic_base_mass();
  test_gun_caliber_domain();
  std::println(std::cout, "All Ship domain tests passed!");
  return 0;
}
