// SPDX-License-Identifier: Apache-2.0
// Extended test coverage for build command

import dallib;
import gb.entities;
import gb.mechanics;
import gb.services;
import test;
import commands;
import std;

// Helper to create test infrastructure
struct BuildTestFixture {
  TestContext ctx;
  Database& db = ctx.db;
  EntityManager& em = ctx.em;
  JsonStore store;
  starnum_t star_id{1};
  planetnum_t planet_id{1};

  BuildTestFixture() : store(ctx.db) {
    ctx.with_standard_universe();
    ctx.em.mutate_race(1, [](Race& r) { r.tech = 500.0; });
    ctx.em.mutate_planet(star_id, planet_id, [](Planet& p) {
      p.info(player_t{1}).resource = 50000;
      p.info(player_t{1}).fuel = 10000;
    });
    ctx.em.mutate_sectormap(star_id, planet_id, [](SectorMap& smap) {
      smap.get(Coordinates{5, 5}).set_owner(1);
      smap.get(Coordinates{5, 5}).set_popn_exact(100);
      smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
    });
  }

  void init_game_obj(GameObj& g, ScopeLevel level = ScopeLevel::LEVEL_PLAN,
                     shipnum_t shipno = 0) {
    g.set_player(1);
    g.set_governor(1);
    g.race = em.peek_race(1);
    g.set_level(level);
    g.set_snum(star_id);
    g.set_pnum(planet_id);
    g.set_shipno(shipno);
  }

  shipnum_t count_ships() {
    return em.num_ships();
  }

  const Ship* get_ship(shipnum_t num) {
    return em.peek_ship(num);
  }

  const Planet* get_planet() {
    return em.peek_planet(star_id, planet_id);
  }
};

// Test: Multiple ship builds on planet (count > 1)
// CRITICAL: Tests x,y coordinate persistence across loop iterations
void test_planet_multiple_builds() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g);

  shipnum_t initial_ships = fixture.count_ships();
  resource_t initial_resource =
      fixture.get_planet()->info(player_t{1}).resource;

  // Build 5 probes at sector 5,5
  command_t argv = {"build", ":", "5,5", "5"};
  GB::commands::build(argv, g);

  // Clear cache to ensure fresh read from DB
  fixture.em.clear_cache();

  // Verify 5 ships were created
  shipnum_t ship_count = fixture.count_ships();
  test::expect_eq(ship_count, shipnum_t{initial_ships.value + 5});

  // Verify all ships at same location (assigned lowest available IDs 1..5)
  for (shipnum_t i = 1; i <= 5; i++) {
    const auto* ship = fixture.get_ship(i);
    test::expect_ne(ship, nullptr);
    test::expect_eq(ship->land_coords(), Coordinates(5, 5));
    test::expect_eq(ship->whatorbits(), ScopeLevel::LEVEL_PLAN);
  }

  // Verify resources deducted for all 5 ships
  const auto* planet = fixture.get_planet();
  const auto* race = fixture.em.peek_race(1);
  resource_t cost_per_probe = Shipcost(ShipType::OTYPE_PROBE, *race);
  resource_t expected_resource = initial_resource - (5 * cost_per_probe);
  test::expect_eq(planet->info(player_t{1}).resource, expected_resource);

  std::println(std::cout, "✓ Planet multiple builds test passed");
}

// Test: Factory building multiple ships
void test_factory_multiple_builds() {
  BuildTestFixture fixture;
  shipnum_t initial_ships = fixture.count_ships();

  // Create a factory ship landed at 5,5 configured to build probes
  shipnum_t factory_num =
      TestShipBuilder(fixture.em, ShipType::OTYPE_FACTORY)
          .owned_by(1, 1)
          .landed_on(fixture.star_id, fixture.planet_id, {5, 5})
          .with_resource(10000)
          .with_crew(100, 0)
          .with_on(true)
          .with_build_type(ShipType::OTYPE_PROBE)
          .build();

  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, factory_num);

  // Factory builds 3 ships
  command_t argv = {"build", "x", "3"};
  GB::commands::build(argv, g);

  fixture.em.clear_cache();

  // Verify ships created (1 factory + 3 built ships)
  shipnum_t ship_count = fixture.count_ships();
  test::expect_eq(ship_count, shipnum_t{initial_ships.value + 4});

  // Verify all built ships at factory's landed coordinates (5,5)
  for (shipnum_t i{factory_num.value + 1};
       i <= shipnum_t{factory_num.value + 3}; i++) {
    const auto* ship = fixture.get_ship(i);
    test::expect_ne(ship, nullptr);
    test::expect_eq(ship->land_coords(), Coordinates(5, 5));
  }

  std::println(std::cout, "✓ Factory multiple builds test passed");
}

// Test: Invalid sector coordinates
void test_invalid_coordinates() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g);

  // Out of bounds - negative
  {
    command_t argv = {"build", ":", "-1,5", "1"};
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    test::expect_contains(output, "Illegal sector");
    g.out.str("");
  }

  // Out of bounds - too large
  {
    command_t argv = {"build", ":", "25,25", "1"};  // Planet is 20x20
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    test::expect_contains(output, "Illegal sector");
    g.out.str("");
  }

  // Invalid format
  {
    command_t argv = {"build", ":", "abc", "1"};
    GB::commands::build(argv, g);
    std::string output = g.out.str();
    test::expect_contains(output, "Invalid sector format");
    g.out.str("");
  }

  std::println(std::cout, "✓ Invalid coordinates test passed");
}

// Test: Wrong scope level
void test_wrong_scope() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_UNIV);

  command_t argv = {"build", ":", "5,5", "1"};
  GB::commands::build(argv, g);

  std::string output = g.out.str();
  test::expect_contains(output, "change scope");

  std::println(std::cout, "✓ Wrong scope test passed");
}

// Test: Ship cannot be built by planet and planet error conditions
void test_invalid_ship_for_planet() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g);

  // 1. Unknown ship type
  {
    g.out.str("");
    GB::commands::build({"build", "9", "5,5", "1"}, g);
    test::expect_contains(g.out.str(), "No such ship type.");
  }

  // 2. Dreadnought ('D') cannot be built on planets (requires Factory)
  {
    g.out.str("");
    GB::commands::build({"build", "D", "5,5", "1"}, g);
    test::expect_contains(g.out.str(),
                          "This ship cannot be built by a planet.");
  }

  // 3. Zero builds count
  {
    g.out.str("");
    GB::commands::build({"build", ":", "5,5", "0"}, g);
    test::expect_contains(g.out.str(), "Give a positive number of builds.");
  }

  // 4. Autoloading enabled on planet build
  {
    fixture.em.mutate_race(1,
                           [](Race& r) { r.leader().toggle.autoload = true; });
    fixture.init_game_obj(g);
    g.out.str("");
    GB::commands::build({"build", ":", "5,5", "1"}, g);
    test::expect_contains(g.out.str(), "Loaded with");
  }

  std::println(std::cout, "✓ Invalid ship for planet and autoload test passed");
}

// Test: Insufficient hanger space
void test_insufficient_hanger_space() {
  BuildTestFixture fixture;

  // Create a small ship with limited hanger space
  shipnum_t builder_num =
      TestShipBuilder(fixture.em, ShipType::STYPE_HABITAT, 1)
          .owned_by(1, 1)
          .in_star_orbit(fixture.star_id)
          .with_resource(10000)
          .with_max_hanger(1)
          .with_hanger(1)
          .with_crew(100, 0)
          .build();

  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, builder_num);

  // Try to build a probe that won't fit in the hangar
  GB::commands::build({"build", ":", "1"}, g);
  test::expect_contains(g.out.str(), "Not enough hanger space.");

  std::println(std::cout, "✓ Insufficient hanger space test passed");
}

void test_ship_build_error_paths() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);

  // 1. Unlanded factory cannot build
  shipnum_t factory_id =
      TestShipBuilder(fixture.em, ShipType::OTYPE_FACTORY, 10)
          .owned_by(1, 1)
          .in_star_orbit(fixture.star_id)
          .with_resource(1000)
          .with_crew(100, 0)
          .with_on(true)
          .with_build_type(ShipType::OTYPE_PROBE)
          .build();

  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, factory_id);
  g.out.str("");
  GB::commands::build({"build", "1"}, g);
  test::expect_contains(g.out.str(), "Factories must be landed on a planet.");

  // 2. Landed shuttle cannot build
  shipnum_t shuttle_id =
      TestShipBuilder(fixture.em, ShipType::STYPE_SHUTTLE, 11)
          .owned_by(1, 1)
          .landed_on(fixture.star_id, fixture.planet_id, {5, 5})
          .with_resource(1000)
          .with_crew(50, 0)
          .build();

  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, shuttle_id);
  g.out.str("");
  GB::commands::build({"build", "H", "1"}, g);
  test::expect_contains(g.out.str(), "This ships cannot build when landed.");

  // 3. Habitat ship build with autoload enabled and low tech error
  shipnum_t hab_id = TestShipBuilder(fixture.em, ShipType::STYPE_HABITAT, 12)
                         .owned_by(1, 1)
                         .in_star_orbit(fixture.star_id)
                         .with_resource(50000)
                         .with_max_hanger(500)
                         .with_hanger(0)
                         .with_crew(100, 0)
                         .with_fuel(500.0)
                         .build();

  fixture.em.mutate_race(1, [](Race& r) { r.leader().toggle.autoload = true; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, hab_id);

  // Autoloaded build of Space Probe (':') inside Habitat
  g.out.str("");
  GB::commands::build({"build", ":", "1"}, g);
  test::expect_contains(g.out.str(), "Loaded with");

  // Insufficient tech error when race tech is lowered
  fixture.em.mutate_race(1, [](Race& r) { r.tech = 1.0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, hab_id);
  g.out.str("");
  GB::commands::build({"build", ":", "1"}, g);
  test::expect_contains(g.out.str(), "engineering technology needed");

  std::println(std::cout, "✓ Ship build error and autoload paths test passed");
}

void test_build_info_and_json_presentation() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);
  fixture.init_game_obj(g);

  // 1. "build ?" in ASCII and JSON modes for mortal without pods, mortal with
  // pods, and God
  fixture.em.mutate_race(1, [](Race& r) {
    r.God = false;
    r.pods = false;
  });
  fixture.init_game_obj(g);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?"}, g));
  test::expect_contains(g.out.str(), "Default ship parameters");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?"}, g));
  test::expect_contains(g.out.str(), "\"type\":\"ship_specs_table\"");

  fixture.em.mutate_race(1, [](Race& r) {
    r.God = true;
    r.pods = true;
  });
  fixture.init_game_obj(g);
  g.set_ui_mode(UiMode::ASCII);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?"}, g));
  test::expect_contains(g.out.str(), "Spore pod");

  // 2. "build ? <type>" for unprogrammed ship ('-'), 0 builders ('F'), 1
  // builder ('D'), and multiple builders (':') with custom ShipExam description
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "?", "-"}, g));
  test::expect_contains(g.out.str(),
                        "This ship type has not been programmed.\n");

  ShipExamRepository exams(fixture.store);
  exams.save(ShipExam{.ship_type = ShipType::OTYPE_FACTORY,
                      .name = "Factory",
                      .description = "Custom factory description no newline"});
  exams.save(ShipExam{.ship_type = ShipType::STYPE_DREADNT,
                      .name = "Dreadnaught",
                      .description = "Custom dread description\n"});
  fixture.em.clear_cache();
  fixture.init_game_obj(g);

  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?", "F"}, g));
  test::expect_contains(g.out.str(), "Custom factory description no newline\n");
  test::expect_contains(g.out.str(), "Can be constructed on planet.");

  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?", "D"}, g));
  test::expect_contains(g.out.str(), "Custom dread description\n");
  test::expect_contains(g.out.str(), "Can be built by F type ships.\n");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "?", ":"}, g));
  test::expect_contains(g.out.str(), "\"type\":\"ship_spec_detail\"");

  std::println(std::cout, "✓ Build info and JSON presentation test passed");
}

void test_planet_and_factory_mcdc_branches() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);

  // 1. Non-numeric build count handled safely via scn::scan
  fixture.init_game_obj(g);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "5,5", "abc"}, g));
  test::expect_contains(g.out.str(), "Give a positive number of builds.\n");

  // 2. Planet build on wasted sector rejected for mortal, allowed for God
  fixture.em.mutate_sectormap(
      fixture.star_id, fixture.planet_id, [](SectorMap& smap) {
        smap.get(Coordinates{6, 6}).set_owner(1);
        smap.get(Coordinates{6, 6}).set_popn_exact(50);
        smap.get(Coordinates{6, 6}).set_condition(SectorType::SEC_WASTED);
      });
  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "6,6", "1"}, g));
  test::expect_contains(g.out.str(), "You can't build on wasted sectors.\n");

  // God bypasses can_build_this, can_build_on_planet, can_build_at_planet, and
  // can_build_on_sector, and skips autoloading even when autoload toggle is on
  fixture.em.mutate_race(1, [](Race& r) {
    r.God = true;
    r.pods = false;
    r.governor(1).toggle.autoload = true;
  });
  fixture.init_game_obj(g);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "p", "6,6", "1"}, g));
  test::expect_contains(g.out.str(), "\"type\":\"created_ship\"");
  test::expect_contains(g.out.str(), "\"type\":\"initialized_ship\"");
  g.set_ui_mode(UiMode::ASCII);

  // Mortal with AVPM building OTYPE_TRANSDEV ('[') skips autoloading
  fixture.em.mutate_race(1, [](Race& r) {
    r.God = false;
    r.discoveries.avpm = true;
    r.governor(1).toggle.autoload = true;
  });
  fixture.init_game_obj(g);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "[", "5,5", "1"}, g));
  test::expect_contains(g.out.str(), "Loaded with 0 crew and 0.0 fuel.\n");

  // 3. Factory validation and execution branches
  shipnum_t undesignated_fac =
      TestShipBuilder(fixture.em, ShipType::OTYPE_FACTORY, 20)
          .owned_by(1, 1)
          .landed_on(fixture.star_id, fixture.planet_id, {5, 5})
          .with_resource(1000)
          .with_crew(100, 0)
          .with_on(true)
          .build();
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, undesignated_fac);

  // Negative count ("build -1") and zero count ("build x 0") on factory
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "-1"}, g));
  test::expect_contains(g.out.str(), "Give a positive number of builds.\n");

  g.out.str("");
  test::expect_false(GB::commands::build({"build", "x", "0"}, g));
  test::expect_contains(g.out.str(), "Give a positive number of builds.\n");

  // Undesignated factory
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(
      g.out.str(),
      "This factory has not been designated to build a ship type.\n");

  // Designated factory: tech check, planet check, sector check, resource check,
  // AP check, and autoloading
  shipnum_t fac_id = TestShipBuilder(fixture.em, ShipType::OTYPE_FACTORY, 21)
                         .owned_by(1, 1)
                         .landed_on(fixture.star_id, fixture.planet_id, {5, 5})
                         .with_resource(1000)
                         .with_crew(100, 0)
                         .with_on(true)
                         .with_build_type(ShipType::OTYPE_PROBE)
                         .build();

  fixture.em.mutate_race(1, [](Race& r) { r.tech = 1.0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "engineering technology needed");

  // God bypasses tech check on factory
  fixture.em.mutate_race(1, [](Race& r) { r.God = true; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "1"}, g));

  // Restore mortal with high tech
  fixture.em.mutate_race(1, [](Race& r) {
    r.God = false;
    r.tech = 500.0;
    r.governor(1).toggle.autoload = true;
  });

  // Enslaved planet blocks factory build
  fixture.em.mutate_planet(fixture.star_id, fixture.planet_id,
                           [](Planet& p) { p.enslave_to(2); });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "You can't build that here.\n");
  fixture.em.mutate_planet(fixture.star_id, fixture.planet_id,
                           [](Planet& p) { p.free_slaves(); });

  // Wasted sector blocks factory build
  fixture.em.mutate_sectormap(
      fixture.star_id, fixture.planet_id, [](SectorMap& smap) {
        smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_WASTED);
      });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "You can't build on wasted sectors.\n");
  fixture.em.mutate_sectormap(
      fixture.star_id, fixture.planet_id, [](SectorMap& smap) {
        smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
      });

  // Insufficient planet resources on factory build
  fixture.em.mutate_planet(fixture.star_id, fixture.planet_id,
                           [](Planet& p) { p.info(player_t{1}).resource = 0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "You need");
  fixture.em.mutate_planet(fixture.star_id, fixture.planet_id, [](Planet& p) {
    p.info(player_t{1}).resource = 50000;
  });

  // Insufficient star AP on factory build
  fixture.em.mutate_star(fixture.star_id, [](Star& s) { s.AP(1) = 0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "You don't have 1 action points there.\n");
  fixture.em.mutate_star(fixture.star_id, [](Star& s) { s.AP(1) = 50; });

  // Autoload on factory build
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, fac_id);
  g.out.str("");
  test::expect_true(GB::commands::build({"build", "1"}, g));
  test::expect_contains(g.out.str(), "Loaded with");

  std::println(std::cout, "✓ Planet and factory MC/DC branches test passed");
}

void test_non_factory_ship_mcdc_and_hangar_accounting() {
  BuildTestFixture fixture;
  auto& registry = get_test_session_registry();
  GameObj g(fixture.em, registry);

  // 1. Cargo ship landed vs in orbit
  shipnum_t cargo_landed =
      TestShipBuilder(fixture.em, ShipType::STYPE_CARGO, 30)
          .owned_by(1, 1)
          .landed_on(fixture.star_id, fixture.planet_id, {5, 5})
          .with_resource(50000)
          .with_crew(50, 0)
          .build();
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, cargo_landed);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", "H", "1"}, g));
  test::expect_contains(g.out.str(), "This ships cannot build when landed.\n");

  // 2. Non-factory ship validation errors: missing arg, invalid type, cannot
  // build on ship, zero count (3 args and 4 args)
  shipnum_t hab_id = TestShipBuilder(fixture.em, ShipType::STYPE_HABITAT, 31)
                         .owned_by(1, 1)
                         .in_star_orbit(fixture.star_id)
                         .with_resource(50000)
                         .with_max_hanger(500)
                         .with_hanger(0)
                         .with_crew(100, 0)
                         .build();
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, hab_id);

  g.out.str("");
  test::expect_false(GB::commands::build({"build"}, g));
  test::expect_contains(g.out.str(), "Build what?\n");

  g.out.str("");
  test::expect_false(GB::commands::build({"build", "9"}, g));
  test::expect_contains(g.out.str(), "No such ship type.\n");

  g.out.str("");
  test::expect_false(GB::commands::build({"build", "D"}, g));
  test::expect_contains(g.out.str(), "This ship type cannot be built by a");

  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "0"}, g));
  test::expect_contains(g.out.str(), "Give a positive number of builds.\n");

  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "0,0", "0"}, g));
  test::expect_contains(g.out.str(), "Give a positive number of builds.\n");

  // 3. Insufficient ship resources, insufficient star AP, insufficient univ AP
  fixture.em.mutate_ship(hab_id, [](Ship& s) { s.resource() = 0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, hab_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "1"}, g));
  test::expect_contains(g.out.str(), "to construct the ship.\n");
  fixture.em.mutate_ship(hab_id, [](Ship& s) { s.resource() = 50000; });

  fixture.em.mutate_star(fixture.star_id, [](Star& s) { s.AP(1) = 0; });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, hab_id);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "1"}, g));
  test::expect_contains(g.out.str(), "You don't have 1 action points there.\n");
  fixture.em.mutate_star(fixture.star_id, [](Star& s) { s.AP(1) = 50; });

  shipnum_t univ_hab = TestShipBuilder(fixture.em, ShipType::STYPE_HABITAT, 32)
                           .owned_by(1, 1)
                           .in_deep_space()
                           .with_resource(50000)
                           .with_max_hanger(500)
                           .with_hanger(0)
                           .with_crew(100, 0)
                           .build();
  fixture.em.mutate_universe([](Universe& u) { u.set_AP(1, 0); });
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, univ_hab);
  g.out.str("");
  test::expect_false(GB::commands::build({"build", ":", "1"}, g));
  test::expect_contains(g.out.str(), "You need 1 universe action point.\n");

  // 4. Hangar space accounting across multi-ship build loop
  const auto probe_size =
      getship(ShipType::OTYPE_PROBE, *fixture.em.peek_race(1))->size();
  shipnum_t tight_hab = TestShipBuilder(fixture.em, ShipType::STYPE_HABITAT, 33)
                            .owned_by(1, 1)
                            .in_star_orbit(fixture.star_id)
                            .with_resource(50000)
                            .with_max_hanger(probe_size)
                            .with_hanger(0)
                            .with_crew(100, 0)
                            .build();
  fixture.init_game_obj(g, ScopeLevel::LEVEL_SHIP, tight_hab);
  g.out.str("");
  // Request 2 probes when hangar only fits 1: first succeeds, second stops with
  // "Not enough hanger space."
  test::expect_true(GB::commands::build({"build", ":", "0,0", "2"}, g));
  test::expect_contains(g.out.str(), "built at a cost of");
  test::expect_contains(g.out.str(), "Not enough hanger space.\n");
  test::expect_eq(fixture.get_ship(tight_hab)->hanger(), probe_size);

  std::println(std::cout,
               "✓ Non-factory ship MC/DC and hangar accounting test passed");
}

int main() {
  try {
    std::println(std::cout, "Running extended build command tests...\n");

    // Phase 1: Critical multi-build tests
    test_planet_multiple_builds();
    test_factory_multiple_builds();

    // Phase 2: Error conditions and autoload paths
    test_invalid_coordinates();
    test_wrong_scope();
    test_invalid_ship_for_planet();
    test_insufficient_hanger_space();
    test_ship_build_error_paths();

    // Phase 3: Presentation and MC/DC coverage
    test_build_info_and_json_presentation();
    test_planet_and_factory_mcdc_branches();
    test_non_factory_ship_mcdc_and_hangar_accounting();

    std::println(std::cout, "\n✅ All extended build tests passed!");
    return 0;
  } catch (const std::exception& e) {
    std::println(std::cerr, "Exception: {}", e.what());
    return 1;
  }
}
