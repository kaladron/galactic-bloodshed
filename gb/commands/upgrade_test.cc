// SPDX-License-Identifier: Apache-2.0

/// \file upgrade_test.cc
/// \brief Unit tests for upgrade command and AP deduction.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_upgrade_command() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 500.0; });
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 10; });

  const auto type = ShipType::STYPE_FIGHTER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 1)
      .named("Upgradeable")
      .in_star_orbit(1, 100.0, 200.0)
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
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "2"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
  std::println(std::cout, "    ✓ Scope rejection at star level verified");

  // 3. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(1);
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
    g.set_snum(1);

    const auto* ship_before = ctx.em.peek_ship(1);
    test::expect_ne(ship_before, nullptr);
    int initial_armor = ship_before->armor();
    int target_armor = initial_armor + 2;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(1);
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
    const auto* star_after = ctx.em.peek_star(1);
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
    g.set_snum(1);

    const auto* ship_before = ctx.em.peek_ship(1);
    test::expect_ne(ship_before, nullptr);
    int initial_speed = ship_before->max_speed();
    int target_speed = initial_speed + 1;
    int initial_resource = ship_before->resource();
    const auto* star_before = ctx.em.peek_star(1);
    test::expect_eq(star_before->AP(1), 9);
    std::println(std::cout, "    Before: max_speed={}, resource={}, star AP={}",
                 initial_speed, initial_resource, star_before->AP(1));

    // upgrade speed target_speed
    ctx.assert_dispatch_success(
        g, {"upgrade", "speed", std::to_string(target_speed)}, 1);

    ctx.em.clear_cache();

    const auto* ship_after = ctx.em.peek_ship(1);
    test::expect_ne(ship_after, nullptr);
    const auto* star_after = ctx.em.peek_star(1);
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
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 500.0; });

  // Cruiser supports crew, cargo, hanger, fuel, destruct, primary, secondary,
  // cew, laser, jump, mount.
  const auto type = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 1)
      .named("CruiserOne")
      .in_star_orbit(1, 100.0, 200.0)
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
  g.set_snum(1);

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
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 25000.0; });

  const auto type = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, type, 1)
      .owned_by(1, 1)
      .named("CruiserWeapons")
      .in_star_orbit(1, 100.0, 200.0)
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
  g.set_snum(1);

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
  g.set_snum(1);

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
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 25000.0; });

  // Ship 1: Carrier in star orbit
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER, 1)
      .owned_by(1, 1)
      .named("FleetCarrier")
      .in_star_orbit(1, 100.0, 200.0)
      .with_resource(1000)
      .build();

  // Ship 2: Cruiser berthed inside Carrier (Ship 1)
  const auto ctype = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, ctype, 2)
      .owned_by(1, 1)
      .named("DockedCruiser")
      .with_resource(2000)
      .with_armor(ship_template(ctype).base_armor)
      .build();

  ctx.em.mutate_ship(2, [](Ship& s) {
    s.dock_into_carrier(1);
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
      .owned_by(1, 1)
      .named("Factory")
      .in_star_orbit(1, 100.0, 200.0)
      .build();

  // Ship 4: Spore Pod (not modifiable)
  TestShipBuilder(ctx.em, ShipType::STYPE_POD, 4)
      .owned_by(1, 1)
      .named("Pod")
      .in_star_orbit(1, 100.0, 200.0)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Factory rejection
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(3);
  g.set_snum(1);
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
  g.set_snum(1);
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
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "20"});
  test::expect_contains(g.out.str(),
                        "resources on board to make this modification.");

  std::println(std::cout, "    ✓ Preconditions and carrier hangar capacity "
                          "synchronization verified");
}

void test_upgrade_json_god_mode_and_edge_cases() {
  std::println(std::cout,
               "\nTest upgrade JSON output, God mode, and edge cases");
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.tech = 25000.0;
    r.discoveries.cew = true;
  });

  // Ship 1: Unarmed Cruiser owned by player 1
  const auto ctype = ShipType::STYPE_CRUISER;
  TestShipBuilder(ctx.em, ctype, 1)
      .owned_by(1, 1)
      .named("UnarmedCruiser")
      .in_star_orbit(1, 100.0, 200.0)
      .with_resource(5000)
      .with_armor(ship_template(ctype).base_armor)
      .build();

  ctx.em.mutate_ship(1, [](Ship& s) {
    s.max_resource() = 5000;
    s.set_primary_battery({.count = 0, .caliber = guntype_t::NONE});
    s.set_secondary_battery({.count = 0, .caliber = guntype_t::NONE});
    s.active_battery() = ActiveBattery::NONE;
    s.build_cost() = static_cast<resource_t>(cost(s));
    s.size() = s.calculate_size();
    s.to_struct().base_mass = s.base_mass();
    s.set_mass(s.base_mass() + s.resource() * MASS_RESOURCE);
  });

  // Ship 2: Cruiser owned by player 2 (non-commandable by player 1)
  TestShipBuilder(ctx.em, ctype, 2)
      .owned_by(2, 1)
      .named("EnemyCruiser")
      .in_star_orbit(1, 100.0, 200.0)
      .with_resource(5000)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // Direct handler call at non-SHIP scope
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.out.str("");
  test::expect_false(GB::commands::upgrade({"upgrade", "armor", "5"}, g));
  test::expect_contains(
      g.out.str(), "You have to change scope to the ship you wish to upgrade.");

  // Non-commandable ship rejection (owned by player 2)
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "armor", "5"});
  test::expect_contains(g.out.str(), "You don't own ship #2.");

  // Switch to Ship 1
  g.set_shipno(1);

  // 1. NoCaliberDefined when upgrading battery strength with NONE caliber
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "strength", "2"});
  test::expect_contains(g.out.str(), "No caliber defined.");

  // 2. InvalidGunCharacteristic when 3rd arg is unknown
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "primary", "range", "5"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  // 3. InvalidCewOption (< 4 args and unknown sub-option) & RidiculousSetting
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "cew", "strength"});
  test::expect_contains(g.out.str(), "No such option for CEWs.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "cew", "caliber", "10"});
  test::expect_contains(g.out.str(), "No such option for CEWs.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"upgrade", "cew", "strength", "-5"});
  test::expect_contains(g.out.str(), "That's a ridiculous setting.");

  // 4. Upgrading secondary battery when active_battery is NONE activates
  // SECONDARY
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.set_secondary_battery({.count = 1, .caliber = guntype_t::LIGHT});
    s.active_battery() = ActiveBattery::NONE;
    s.build_cost() = static_cast<resource_t>(cost(s));
  });
  ctx.assert_dispatch_success(g, {"upgrade", "secondary", "caliber", "medium"},
                              1);
  ctx.assert_dispatch_success(g, {"upgrade", "secondary", "strength", "2"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->active_battery(),
                  ActiveBattery::SECONDARY);

  // Reset to primary-only with active_battery NONE and verify PRIMARY
  // activation
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.set_primary_battery({.count = 1, .caliber = guntype_t::LIGHT});
    s.set_secondary_battery({.count = 0, .caliber = guntype_t::NONE});
    s.active_battery() = ActiveBattery::NONE;
    s.size() = s.calculate_size();
    s.to_struct().base_mass = s.base_mass();
    s.build_cost() = static_cast<resource_t>(cost(s));
  });
  const double base_mass_before = ctx.em.peek_ship(1)->to_struct().base_mass;
  ctx.assert_dispatch_success(g, {"upgrade", "primary", "caliber", "medium"},
                              1);
  ctx.assert_dispatch_success(g, {"upgrade", "primary", "strength", "2"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->active_battery(),
                  ActiveBattery::PRIMARY);
  test::expect_gt(ctx.em.peek_ship(1)->to_struct().base_mass, base_mass_before);

  // 5. "resource" alias for "cargo" and 2-arg no-op upgrade (net_cost = 1)
  const auto cur_res_cap = ctx.em.peek_ship(1)->max_resource();
  ctx.assert_dispatch_success(
      g, {"upgrade", "resource", std::to_string(cur_res_cap + 10)}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->max_resource(), cur_res_cap + 10);

  g.out.str("");
  ctx.assert_dispatch_success(g, {"upgrade", "armor"}, 1);
  test::expect_contains(g.out.str(),
                        "Characteristic modified at a cost of 1 resources.");

  // 6. God mode + UiMode::JSON: deity upgrades for free even with 0 tech
  ctx.em.mutate_race(1, [](Race& r) {
    r.God = true;
    r.tech = 0.0;
  });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.set_snum(1);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  const auto res_before_god = ctx.em.peek_ship(1)->resource();
  ctx.assert_dispatch_success(g, {"upgrade", "armor", "50"}, 1);
  test::expect_contains(g.out.str(), "\"type\":\"ship_upgrade\"");
  test::expect_contains(g.out.str(), "\"old_cost\":0");
  test::expect_contains(g.out.str(), "\"new_cost\":0");
  test::expect_contains(g.out.str(), "\"net_cost\":0");
  test::expect_eq(ctx.em.peek_ship(1)->armor(), 50);
  test::expect_eq(ctx.em.peek_ship(1)->resource(), res_before_god);

  std::println(std::cout,
               "    ✓ JSON output, God mode, battery activation, and edge "
               "cases verified");
}

}  // namespace

int main() {
  test_upgrade_command();
  test_upgrade_numeric_attributes_and_validations();
  test_upgrade_weapons_and_systems();
  test_upgrade_preconditions_and_carrier_hangar();
  test_upgrade_json_god_mode_and_edge_cases();
  std::println(std::cout, "\n✅ All upgrade tests passed!");
  return 0;
}
