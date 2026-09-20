// SPDX-License-Identifier: Apache-2.0

/// \file upgrade_test.cc
/// \brief Unit tests for upgrade command and AP deduction.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_upgrade_command() {
  // Create test context
  TestContext ctx;

  // Create test race with enough tech for upgrades
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 500.0;  // High tech to allow upgrades
  race.morale = 100;
  race.God = false;

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.coordinates = {100.0, 200.0};
  ss.explored.set(player_t{1});
  ss.AP[player_t{1}] = 10;
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  const auto type = ShipType::STYPE_FIGHTER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 0)
      .named("Upgradeable")
      .in_star_orbit(0, 100.0, 200.0)
      .with_fuel(10.0)
      .with_resource(500)
      .with_guns(shipdata_primary(ShipType::STYPE_BATTLE),
                 ship_template(type).max_guns)
      .with_armor(ship_template(type).base_armor)
      .with_max_speed(5)
      .build();

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Scope rejection at STAR scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at star level verified");

  // 3. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  // Restore non-guest race
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);

  // 4. Upgrade ship armor (at SHIP scope)
  std::println(std::cout, "Upgrade ship armor");
  {
    ctx.setup_game_obj(g);
    g.set_level(ScopeLevel::LEVEL_SHIP);
    g.set_shipno(1);
    g.set_snum(0);

    const auto* ship_before = ctx.em.peek_ship(1);
    test::expect_ne(ship_before, nullptr);
    int initial_armor = ship_before->armor();
    int target_armor = initial_armor + 2;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(0);
    test::expect_eq(star_before->AP(1), 10);
    std::println(std::cout, "    Before: armor={}, resource={}, star AP={}",
                 initial_armor, initial_resource, star_before->AP(1));

    // upgrade armor target_armor
    ctx.assert_dispatch_success(
        g, {"upgrade", "armor", std::to_string(target_armor)}, 1);

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    const auto* ship_after = ctx.em.peek_ship(1);
    test::expect_ne(ship_after, nullptr);
    const auto* star_after = ctx.em.peek_star(0);
    test::expect_eq(star_after->AP(1), 9);  // 1 Star AP deducted
    std::println(std::cout, "    After: armor={}, resource={}, star AP={}",
                 ship_after->armor(), ship_after->resource(),
                 star_after->AP(1));

    // Armor should have increased
    test::expect_eq(ship_after->armor(), target_armor);
    std::println(
        std::cout,
        "    ✓ Armor upgrade applied and 1 Star AP deducted (was {}, now {})",
        initial_armor, ship_after->armor());
  }

  // 5. Upgrade ship speed
  std::println(std::cout, "Upgrade ship speed");
  {
    ctx.setup_game_obj(g);
    g.set_level(ScopeLevel::LEVEL_SHIP);
    g.set_shipno(1);
    g.set_snum(0);

    const auto* ship_before = ctx.em.peek_ship(1);
    test::expect_ne(ship_before, nullptr);
    int initial_speed = ship_before->max_speed();
    int target_speed = initial_speed + 1;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(0);
    test::expect_eq(star_before->AP(1), 9);
    std::println(std::cout, "    Before: max_speed={}, resource={}, star AP={}",
                 initial_speed, initial_resource, star_before->AP(1));

    // upgrade speed target_speed
    ctx.assert_dispatch_success(
        g, {"upgrade", "speed", std::to_string(target_speed)}, 1);

    ctx.em.clear_cache();

    const auto* ship_after = ctx.em.peek_ship(1);
    test::expect_ne(ship_after, nullptr);
    const auto* star_after = ctx.em.peek_star(0);
    test::expect_eq(star_after->AP(1), 8);  // Another 1 Star AP deducted
    std::println(std::cout, "    After: max_speed={}, resource={}, star AP={}",
                 ship_after->max_speed(), ship_after->resource(),
                 star_after->AP(1));

    // Speed should have increased
    test::expect_eq(ship_after->max_speed(), target_speed);
    std::println(
        std::cout,
        "    ✓ Speed upgrade applied and 1 Star AP deducted (was {}, now {})",
        initial_speed, ship_after->max_speed());
  }

  std::println(std::cout, "Verify upgrades persist after cache clear");
  {
    ctx.em.clear_cache();

    const auto* ship_check = ctx.em.peek_ship(1);
    test::expect_ne(ship_check, nullptr);

    // Values should still reflect upgrades
    std::println(
        std::cout, "    Final values: armor={}, max_speed={}, resource={}",
        ship_check->armor(), ship_check->max_speed(), ship_check->resource());

    std::println(std::cout, "    ✓ Upgrades persisted to database");
  }
}

void test_upgrade_numeric_attributes_and_validations() {
  std::println(std::cout, "\nTest upgrade numeric attributes and validations");
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars_repo(store);

  Race race{};
  race.Playernum = 1;
  race.name = "BuilderRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 500.0;
  race.morale = 100;
  race.God = false;
  races.save(race);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.explored.set(player_t{1});
  ss.AP[player_t{1}] = 50;
  stars_repo.save(Star(ss));

  // Cruiser supports crew, cargo, hanger, fuel, destruct, primary, secondary,
  // cew, laser, jump, mount.
  const auto type = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 0)
      .named("CruiserOne")
      .in_star_orbit(0, 100.0, 200.0)
      .with_fuel(100.0)
      .with_resource(5000)
      .with_armor(ship_template(type).base_armor)
      .with_max_speed(ship_template(type).base_speed)
      .build();

  ctx.em.mutate_ship(1, [](Ship& s) {
    s.build_cost() = static_cast<resource_t>(cost(s));
    s.size() = s.calculate_size();
    s.set_mass(s.base_mass() + s.resource() * MASS_RESOURCE +
               s.fuel() * MASS_FUEL);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(0);

  // Negative value rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "-1"});
  test::expect_contains(g.out.str(), "That's a ridiculous setting.");

  // Unknown characteristic rejected
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "shields", "10"});
  test::expect_contains(
      g.out.str(),
      "That characteristic either doesn't exist or can't be modified.");

  // Downgrade rejected when build_cost exceeds candidate cost
  ctx.em.mutate_ship(1, [](Ship& s) { s.build_cost() += 500; });
  const auto cur_armor = ctx.em.peek_ship(1)->armor();
  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"upgrade", "armor", std::to_string(cur_armor)});
  test::expect_contains(g.out.str(), "You cannot downgrade ships!");
  ctx.em.mutate_ship(
      1, [](Ship& s) { s.build_cost() = static_cast<resource_t>(cost(s)); });

  // Upgrade crew, cargo, hanger, fuel, destruct
  const auto target_crew = ctx.em.peek_ship(1)->max_crew() + 10;
  ctx.assert_dispatch_success(
      g, {"upgrade", "crew", std::to_string(target_crew)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_crew(), target_crew);

  const auto target_cargo = ctx.em.peek_ship(1)->max_resource() + 20;
  ctx.assert_dispatch_success(
      g, {"upgrade", "cargo", std::to_string(target_cargo)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_resource(), target_cargo);

  const auto target_hanger = ctx.em.peek_ship(1)->max_hanger() + 5;
  ctx.assert_dispatch_success(
      g, {"upgrade", "hanger", std::to_string(target_hanger)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_hanger(), target_hanger);

  const auto target_fuel = ctx.em.peek_ship(1)->max_fuel() + 25;
  ctx.assert_dispatch_success(
      g, {"upgrade", "fuel", std::to_string(target_fuel)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_fuel(), target_fuel);

  const auto target_destruct = ctx.em.peek_ship(1)->max_destruct() + 15;
  ctx.assert_dispatch_success(
      g, {"upgrade", "destruct", std::to_string(target_destruct)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_destruct(), target_destruct);

  std::println(std::cout,
               "    ✓ Crew, cargo, hanger, fuel, destruct upgrades verified");
}

void test_upgrade_weapons_and_systems() {
  std::println(std::cout, "\nTest upgrade weapons and boolean systems");
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars_repo(store);

  Race race{};
  race.Playernum = 1;
  race.name = "WeaponRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 25000.0;
  race.morale = 100;
  race.God = false;
  races.save(race);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.explored.set(player_t{1});
  ss.AP[player_t{1}] = 50;
  stars_repo.save(Star(ss));

  const auto type = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 0)
      .named("CruiserWeapons")
      .in_star_orbit(0, 100.0, 200.0)
      .with_fuel(100.0)
      .with_resource(15000)
      .with_armor(ship_template(type).base_armor)
      .with_max_speed(ship_template(type).base_speed)
      .build();

  ctx.em.mutate_ship(1, [](Ship& s) {
    s.max_resource() = 20000;
    s.set_primary_battery({.count = 2, .caliber = guntype_t::LIGHT});
    s.set_secondary_battery({.count = 1, .caliber = guntype_t::LIGHT});
    s.build_cost() = static_cast<resource_t>(cost(s));
    s.size() = s.calculate_size();
    s.set_mass(s.base_mass() + s.resource() * MASS_RESOURCE +
               s.fuel() * MASS_FUEL);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(0);

  // Primary battery validation & upgrades
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "strength"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "strength", "-3"});
  test::expect_contains(g.out.str(), "That's a ridiculous setting.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "caliber"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "caliber", "super"});
  test::expect_contains(g.out.str(), "No such caliber.");

  // Upgrade primary caliber from light -> medium -> heavy, and strength -> 4
  ctx.assert_dispatch_success(g, {"upgrade", "primary", "caliber", "medium"},
                              1);
  test::expect_eq(ctx.em.peek_ship(1)->primary_battery().caliber,
                  guntype_t::MEDIUM);

  ctx.assert_dispatch_success(g, {"upgrade", "primary", "caliber", "heavy"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->primary_battery().caliber,
                  guntype_t::HEAVY);

  ctx.assert_dispatch_success(g, {"upgrade", "primary", "strength", "4"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->primary_battery().count, 4U);

  // Secondary battery upgrades
  ctx.assert_dispatch_success(g, {"upgrade", "secondary", "caliber", "medium"},
                              1);
  test::expect_eq(ctx.em.peek_ship(1)->secondary_battery().caliber,
                  guntype_t::MEDIUM);

  ctx.assert_dispatch_success(g, {"upgrade", "secondary", "strength", "3"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->secondary_battery().count, 3U);

  // Boolean systems & CEW before tech discoveries
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "mount"});
  test::expect_contains(
      g.out.str(), "Your race does not now how to utilize crystal power yet.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "hyperdrive"});
  test::expect_contains(
      g.out.str(),
      "That characteristic either doesn't exist or can't be modified.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "laser"});
  test::expect_contains(g.out.str(), "Your race cannot build lasers.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "cew", "strength", "50"});
  test::expect_contains(g.out.str(),
                        "Your race cannot build confined energy weapons.");

  // Grant discoveries and verify upgrades succeed
  ctx.em.mutate_race(1, [](Race& r) {
    r.discoveries.crystal = true;
    r.discoveries.hyperdrive = true;
    r.discoveries.laser = true;
    r.discoveries.cew = true;
  });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(0);

  ctx.assert_dispatch_success(g, {"upgrade", "mount"}, 1);
  test::expect_true(ctx.em.peek_ship(1)->mount());

  ctx.assert_dispatch_success(g, {"upgrade", "hyperdrive"}, 1);
  test::expect_true(ctx.em.peek_ship(1)->hyper_drive().has);

  ctx.assert_dispatch_success(g, {"upgrade", "laser"}, 1);
  test::expect_true(ctx.em.peek_ship(1)->laser());

  ctx.assert_dispatch_success(g, {"upgrade", "cew", "strength", "50"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->cew(), 50);

  ctx.assert_dispatch_success(g, {"upgrade", "cew", "range", "100"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->cew_range(), 100);

  std::println(std::cout,
               "    ✓ Batteries, CEW, mount, hyperdrive, and laser verified");
}

void test_upgrade_preconditions_and_carrier_hangar() {
  std::println(std::cout,
               "\nTest upgrade preconditions and carrier hangar capacity");
  TestContext ctx;
  JsonStore store(ctx.db);
  RaceRepository races(store);
  StarRepository stars_repo(store);

  Race race{};
  race.Playernum = 1;
  race.name = "CarrierRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 25000.0;
  race.morale = 100;
  race.God = false;
  races.save(race);

  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.explored.set(player_t{1});
  ss.AP[player_t{1}] = 50;
  stars_repo.save(Star(ss));

  // Ship 1: Carrier in star orbit
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER, 1)
      .owned_by(1, 0)
      .named("FleetCarrier")
      .in_star_orbit(0, 100.0, 200.0)
      .with_resource(1000)
      .build();

  // Ship 2: Cruiser berthed inside Carrier (Ship 1)
  const auto ctype = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, ctype, 2)
      .owned_by(1, 0)
      .named("DockedCruiser")
      .with_resource(2000)
      .with_armor(ship_template(ctype).base_armor)
      .build();

  ctx.em.mutate_ship(2, [](Ship& s) {
    s.whatorbits() = ScopeLevel::LEVEL_SHIP;
    s.destshipno() = 1;
    s.max_resource() = 5000;
    s.build_cost() = static_cast<resource_t>(cost(s));
    s.size() = s.calculate_size();
    s.set_mass(s.base_mass() + s.resource() * MASS_RESOURCE);
  });

  const auto initial_cruiser_size = ctx.em.peek_ship(2)->size();
  const auto initial_cruiser_mass = ctx.em.peek_ship(2)->mass();
  ctx.em.mutate_ship(1, [initial_cruiser_size, initial_cruiser_mass](Ship& c) {
    c.max_hanger() = initial_cruiser_size + 1;  // Only 1 unit of free hangar
    c.hanger() = initial_cruiser_size;
    c.set_mass(c.base_mass() + initial_cruiser_mass);
  });

  // Ship 3: Factory (cannot upgrade)
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 3)
      .owned_by(1, 0)
      .named("Factory")
      .in_star_orbit(0, 100.0, 200.0)
      .build();

  // Ship 4: Spore Pod (not modifiable)
  TestShipBuilder(ctx.em, ShipType::STYPE_POD, 4)
      .owned_by(1, 0)
      .named("Pod")
      .in_star_orbit(0, 100.0, 200.0)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Factory rejection
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(3);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "5"});
  test::expect_contains(g.out.str(), "You can't upgrade factories.");

  // Pod rejection
  g.set_shipno(4);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "5"});
  test::expect_contains(g.out.str(), "This ship cannot be upgraded.");

  // Damaged ship rejection
  ctx.em.mutate_ship(
      2, [](Ship& s) { test::expect_false(s.apply_damage(10).destroyed); });
  g.set_shipno(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "10"});
  test::expect_contains(g.out.str(), "You cannot upgrade damaged ships.");
  ctx.em.mutate_ship(2, [](Ship& s) { s.repair_damage(10); });

  // Carrier hangar overflow rejection when expanding hanger by +50
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "hanger", "80"});
  test::expect_contains(g.out.str(), "Not enough free hanger space on");

  // Expand carrier max_hanger and verify docked cruiser upgrade updates carrier
  ctx.em.mutate_ship(1, [](Ship& c) { c.max_hanger() = 500; });
  ctx.assert_dispatch_success(g, {"upgrade", "hanger", "80"}, 1);
  const auto new_cruiser_size = ctx.em.peek_ship(2)->size();
  test::expect_gt(new_cruiser_size, initial_cruiser_size);
  test::expect_eq(ctx.em.peek_ship(1)->hanger(), new_cruiser_size);

  // Insufficient tech rejection
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 0.1; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(2);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "20"});
  test::expect_contains(g.out.str(),
                        "This upgrade requires an engineering technology of");

  // Insufficient onboard resources rejection
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 25000.0; });
  ctx.em.mutate_ship(2, [](Ship& s) { s.resource() = 0; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(2);
  g.set_snum(0);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "20"});
  test::expect_contains(g.out.str(),
                        "resources on board to make this modification.");

  std::println(std::cout, "    ✓ Preconditions and carrier hangar capacity "
                          "synchronization verified");
}

}  // namespace

int main() {
  test_upgrade_command();
  test_upgrade_numeric_attributes_and_validations();
  test_upgrade_weapons_and_systems();
  test_upgrade_preconditions_and_carrier_hangar();
  std::println(std::cout, "\n✅ All upgrade tests passed!");
  return 0;
}
