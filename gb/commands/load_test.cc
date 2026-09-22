// SPDX-License-Identifier: Apache-2.0

/// \file load_test.cc
/// \brief Unit tests for load and unload commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "LoadTester";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;
  race.absorb = false;
  race.Metamorph = false;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "LoadStar";
  ss.coordinates = {100.0, 200.0};
  ss.pnames.emplace_back("LoadPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet with resources
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.dimensions = {10, 10};
  ps.info[player_t{1}].fuel = 1000;
  ps.info[player_t{1}].resource = 500;
  ps.info[player_t{1}].destruct = 200;
  ps.info[player_t{1}].crystals = 50;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a landed ship to load cargo onto
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO, 1)
      .owned_by(1, 0)
      .named("CargoHauler")
      .landed_on(0, 0, {5, 5})
      .with_fuel(100.0)
      .with_resource(0)
      .with_destruct(0)
      .with_crystals(0)
      .build();
}

void test_load_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println(std::cout, "Load fuel from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    double initial_ship_fuel = s_before->fuel();
    int initial_planet_fuel = p_before->info(player_t{1}).fuel;

    ctx.assert_dispatch_success(g, {"load", "#1", "f", "100"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->fuel(), initial_ship_fuel + 100);
    test::expect_eq(p_after->info(player_t{1}).fuel, initial_planet_fuel - 100);
    std::println(std::cout, "✓ Fuel loaded from planet to ship");
  }

  std::println(std::cout, "Load resources from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_resource = s_before->resource();
    int initial_planet_resource = p_before->info(player_t{1}).resource;

    ctx.assert_dispatch_success(g, {"load", "#1", "r", "200"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->resource(), initial_ship_resource + 200);
    test::expect_eq(p_after->info(player_t{1}).resource,
                    initial_planet_resource - 200);
    std::println(std::cout, "✓ Resources loaded from planet to ship");
  }

  std::println(std::cout, "Load destruct from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_destruct = s_before->destruct();
    int initial_planet_destruct = p_before->info(player_t{1}).destruct;

    ctx.assert_dispatch_success(g, {"load", "#1", "d", "50"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->destruct(), initial_ship_destruct + 50);
    test::expect_eq(p_after->info(player_t{1}).destruct,
                    initial_planet_destruct - 50);
    std::println(std::cout, "✓ Destruct loaded from planet to ship");
  }

  std::println(std::cout, "Load crystals from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_crystals = s_before->crystals();
    int initial_planet_crystals = p_before->info(player_t{1}).crystals;

    ctx.assert_dispatch_success(g, {"load", "#1", "x", "10"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(0, 0);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->crystals(), initial_ship_crystals + 10);
    test::expect_eq(p_after->info(player_t{1}).crystals,
                    initial_planet_crystals - 10);
    std::println(std::cout, "✓ Crystals loaded from planet to ship");
  }
}

void test_unload_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // First load resources, then unload
  ctx.assert_dispatch_success(g, {"load", "#1", "r", "200"});
  test::expect_eq(ctx.em.peek_ship(1)->resource(), 200);

  ctx.assert_dispatch_success(g, {"unload", "#1", "r", "50"});
  test::expect_eq(ctx.em.peek_ship(1)->resource(), 150);
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(player_t{1}).resource, 350);
  std::println(std::cout, "✓ Resources unloaded from ship to planet");
}

void test_load_syntax_and_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"load"});
  test::expect_contains(g.out.str(),
                        "Syntax: load <ship> <commodity> [<amount>]");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unload", "#1"});
  test::expect_contains(g.out.str(),
                        "Syntax: unload <ship> <commodity> [<amount>]");

  // 2. Unknown commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"load", "#1", "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");
}

void test_load_transporter() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Target receiver transporter ship 2
  const auto trans2_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 2)
                             .owned_by(1)
                             .named("TransporterReceiver")
                             .with_alive(true)
                             .with_active(true)
                             .with_on(true)
                             .landed_on(0, 0, {5, 5})
                             .with_max_resource(1000)
                             .build();

  // Source transmitter transporter ship 3
  const auto trans1_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 3)
                             .owned_by(1)
                             .named("TransporterSender")
                             .with_alive(true)
                             .with_active(true)
                             .with_on(true)
                             .landed_on(0, 0, {5, 5})
                             .with_max_resource(1000)
                             .with_special(TransportData{.target = trans2_id})
                             .build();

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", trans1_id.value), "r", "40"});
  test::expect_contains(g.out.str(), "Zap");
  test::expect_contains(g.out.str(), "40 resources transferred");

  ctx.em.clear_cache();
  // Sender should have 0 resources (transferred out)
  test::expect_eq(ctx.em.peek_ship(trans1_id)->resource(), 0);
  // Receiver should have 40 resources
  test::expect_eq(ctx.em.peek_ship(trans2_id)->resource(), 40);
  std::println(std::cout, "✓ Transporter automatic beam transfer succeeded");
}

void test_load_ship_to_ship() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Create carrier/mothership (s2) in star orbit
  shipnum_t s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                        .owned_by(1, 0)
                        .named("Mothership")
                        .in_star_orbit(0)
                        .with_fuel(200.0)
                        .with_max_fuel(500.0)
                        .with_resource(300)
                        .with_max_resource(1000)
                        .with_destruct(100)
                        .with_max_destruct(200)
                        .with_crystals(50)
                        .with_crew(20, 10)
                        .build();

  // Create tender/cargo ship (s1) docked to s2
  shipnum_t s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                        .owned_by(1, 0)
                        .named("Tender")
                        .docked_to(s2_id, 0)
                        .with_fuel(50.0)
                        .with_max_fuel(200.0)
                        .with_resource(50)
                        .with_max_resource(500)
                        .with_destruct(10)
                        .with_max_destruct(100)
                        .with_crystals(5)
                        .with_crew(5, 2)
                        .build();

  // Set mutual docking on carrier s2
  ctx.em.mutate_ship(s2_id, [&](Ship& s2) { s2.dock_with_ship(s1_id); });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // A. Load commodities from s2 into s1
  {
    double initial_mass =
        ctx.em.peek_ship(s1_id)->mass() + ctx.em.peek_ship(s2_id)->mass();
    int initial_res = ctx.em.peek_ship(s1_id)->resource() +
                      ctx.em.peek_ship(s2_id)->resource();

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "r", "50"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 100);
    test::expect_eq(ctx.em.peek_ship(s2_id)->resource(), 250);
    test::expect_eq(ctx.em.peek_ship(s1_id)->resource() +
                        ctx.em.peek_ship(s2_id)->resource(),
                    initial_res);
    test::expect_true(std::abs((ctx.em.peek_ship(s1_id)->mass() +
                                ctx.em.peek_ship(s2_id)->mass()) -
                               initial_mass) < 0.01);

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "f", "30"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->fuel(), 80.0);
    test::expect_eq(ctx.em.peek_ship(s2_id)->fuel(), 170.0);

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "d", "20"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->destruct(), 30);
    test::expect_eq(ctx.em.peek_ship(s2_id)->destruct(), 80);

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "x", "10"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->crystals(), 15);
    test::expect_eq(ctx.em.peek_ship(s2_id)->crystals(), 40);

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "c", "4"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->popn(), 9);
    test::expect_eq(ctx.em.peek_ship(s2_id)->popn(), 16);

    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", s1_id.value), "m", "2"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->troops(), 4);
    test::expect_eq(ctx.em.peek_ship(s2_id)->troops(), 8);
    std::println(std::cout,
                 "✓ Ship-to-ship load conserved commodities and physical mass");
  }

  // B. Unload commodities from s1 into s2 (verifies fix for silent commodity
  // destruction)
  {
    double initial_mass =
        ctx.em.peek_ship(s1_id)->mass() + ctx.em.peek_ship(s2_id)->mass();
    int initial_res = ctx.em.peek_ship(s1_id)->resource() +
                      ctx.em.peek_ship(s2_id)->resource();

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "r", "40"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 60);
    test::expect_eq(ctx.em.peek_ship(s2_id)->resource(), 290);
    test::expect_eq(ctx.em.peek_ship(s1_id)->resource() +
                        ctx.em.peek_ship(s2_id)->resource(),
                    initial_res);
    test::expect_true(std::abs((ctx.em.peek_ship(s1_id)->mass() +
                                ctx.em.peek_ship(s2_id)->mass()) -
                               initial_mass) < 0.01);

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "f", "20"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->fuel(), 60.0);
    test::expect_eq(ctx.em.peek_ship(s2_id)->fuel(), 190.0);

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "d", "15"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->destruct(), 15);
    test::expect_eq(ctx.em.peek_ship(s2_id)->destruct(), 95);

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "x", "5"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->crystals(), 10);
    test::expect_eq(ctx.em.peek_ship(s2_id)->crystals(), 45);

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "c", "3"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->popn(), 6);
    test::expect_eq(ctx.em.peek_ship(s2_id)->popn(), 19);

    ctx.assert_dispatch_success(
        g, {"unload", std::format("#{}", s1_id.value), "m", "1"});
    test::expect_eq(ctx.em.peek_ship(s1_id)->troops(), 3);
    test::expect_eq(ctx.em.peek_ship(s2_id)->troops(), 9);
    std::println(
        std::cout,
        "✓ Ship-to-ship unload conserved commodities without silent loss");

    // Verify database persistence via clear_cache
    ctx.em.clear_cache();
    test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 60);
    test::expect_eq(ctx.em.peek_ship(s2_id)->resource(), 290);
    test::expect_eq(ctx.em.peek_ship(s1_id)->fuel(), 60.0);
    test::expect_eq(ctx.em.peek_ship(s2_id)->fuel(), 190.0);
    test::expect_eq(ctx.em.peek_ship(s1_id)->destruct(), 15);
    test::expect_eq(ctx.em.peek_ship(s2_id)->destruct(), 95);
    test::expect_eq(ctx.em.peek_ship(s1_id)->crystals(), 10);
    test::expect_eq(ctx.em.peek_ship(s2_id)->crystals(), 45);
    test::expect_eq(ctx.em.peek_ship(s1_id)->popn(), 6);
    test::expect_eq(ctx.em.peek_ship(s2_id)->popn(), 19);
    test::expect_eq(ctx.em.peek_ship(s1_id)->troops(), 3);
    test::expect_eq(ctx.em.peek_ship(s2_id)->troops(), 9);
    std::println(std::cout, "✓ Ship-to-ship transfers persisted to SQLite");
  }

  // C. Alien ship transfer rules (can give goods to alien, cannot take)
  shipnum_t alien_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                           .owned_by(2, 0)
                           .named("KlingonFreighter")
                           .in_star_orbit(0)
                           .with_resource(100)
                           .with_max_resource(500)
                           .build();

  // Dock s1 with alien ship
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) { s1.destshipno() = alien_id; });
  ctx.em.mutate_ship(alien_id,
                     [&](Ship& alien) { alien.dock_with_ship(s1_id); });

  // Attempt to load from alien ship (must be rejected)
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", s1_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "you can only transfer between");
  test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 60);
  test::expect_eq(ctx.em.peek_ship(alien_id)->resource(), 100);

  // Unload to alien ship (must succeed and transfer goods to alien)
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "10 resources transferred");
  test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 50);
  test::expect_eq(ctx.em.peek_ship(alien_id)->resource(), 110);
  std::println(std::cout, "✓ Alien ship transfer constraints enforced");
}

void test_planet_crew_load_and_unload() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Create landed cargo ship with crew capacity and initial crew/troops
  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                          .owned_by(1, 0)
                          .named("ColonyShip")
                          .landed_on(0, 0, {5, 5})
                          .with_crew(50, 20)
                          .with_max_crew(100)
                          .build();

  // Ensure sector (5, 5) is empty land
  ctx.em.mutate_planet_and_sectors(0, 0, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({5, 5});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_owner(0);
    sect.set_popn_exact(0);
    sect.set_troops(0);
    p.sync_demographics(map);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Unload civilians onto empty sector -> sector COLONIZED
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "c", "10"});
  test::expect_contains(g.out.str(), "sector 5,5 COLONIZED");
  test::expect_eq(ctx.em.peek_ship(ship_id)->popn(), 40);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_popn(), 10);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_owner(), 1);
  ctx.verify_universe_invariants();

  // 2. Load civilians back until sector is empty -> sector evacuated
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", ship_id.value), "c", "10"});
  test::expect_contains(g.out.str(), "sector 5,5 evacuated");
  test::expect_eq(ctx.em.peek_ship(ship_id)->popn(), 50);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_popn(), 0);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_owner(), 0);
  ctx.verify_universe_invariants();

  // 3. Unload military onto empty sector -> sector OCCUPIED
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "m", "5"});
  test::expect_contains(g.out.str(), "sector 5,5 OCCUPIED");
  test::expect_eq(ctx.em.peek_ship(ship_id)->troops(), 15);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_troops(), 5);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_owner(), 1);
  ctx.verify_universe_invariants();

  // 4. Load military with omitted amount (default maximum available) ->
  // evacuated
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"load", std::format("#{}", ship_id.value), "m"});
  test::expect_contains(g.out.str(), "sector 5,5 evacuated");
  test::expect_eq(ctx.em.peek_ship(ship_id)->troops(), 20);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_troops(), 0);
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({5, 5}).get_owner(), 0);
  ctx.verify_universe_invariants();
  std::println(std::cout,
               "✓ Planetary crew colonization and evacuation verified");
}

void test_unload_onto_alien_sector() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Setup Player 1 (attacker) and Player 2 (defender)
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 10;
    r.absorb = true;  // Metamorph body absorption test on victory
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });

  // Create assault transport for Player 1 landed at (2, 2)
  shipnum_t assault_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(1, 0)
                             .named("DropShip")
                             .landed_on(0, 0, {2, 2})
                             .with_crew(100, 100)
                             .with_max_crew(200)
                             .build();

  // Populate sector (2, 2) with weak Player 2 forces
  ctx.em.mutate_planet_and_sectors(0, 0, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_owner(2);
    sect.set_popn_exact(2);
    sect.set_troops(1);
    p.sync_demographics(map);
  });
  ctx.verify_universe_invariants();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Victory assault with troops (and metamorph body absorption)
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "m", "50"});
  test::expect_contains(g.out.str(),
                        "That sector is already occupied by another player!");
  test::expect_contains(g.out.str(), "VICTORY! The sector is yours!");
  test::expect_contains(g.out.str(), "alien bodies absorbed");
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({2, 2}).get_owner(), 1);
  // Verify planet demographics were synced after ground assault casualties
  ctx.verify_universe_invariants();

  // 2. Defeat assault with military: Player 2 has overwhelming defense and
  // metamorph absorption
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 15;
    r.absorb = true;
  });
  ctx.em.mutate_planet_and_sectors(0, 0, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_owner(2);
    sect.set_popn_exact(100);
    sect.set_troops(500);
    p.sync_demographics(map);
  });
  ctx.verify_universe_invariants();

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "m", "5"});
  test::expect_contains(g.out.str(), "DEFEAT!  Your assault was repulsed.");
  test::expect_contains(g.out.str(), "Metamorphs have absorbed");
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({2, 2}).get_owner(), 2);
  ctx.verify_universe_invariants();

  // Also test civilian defeat branch
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "c", "5"});
  test::expect_contains(g.out.str(), "DEFEAT!  Your assault was repulsed.");
  ctx.verify_universe_invariants();

  // 3. Attempting to load (instead of unload) from an alien-occupied sector
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", assault_id.value), "c", "5"});
  test::expect_contains(g.out.str(),
                        "You have to unload to assault alien sectors.");

  // 4. Victory assault with civilians
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 15;
    r.absorb = true;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });
  ctx.em.mutate_planet_and_sectors(0, 0, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_owner(2);
    sect.set_popn_exact(1);
    sect.set_troops(0);
    p.sync_demographics(map);
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "c", "50"});
  test::expect_contains(g.out.str(), "VICTORY! The sector is yours!");
  test::expect_eq(ctx.em.peek_sectormap(0, 0)->get({2, 2}).get_owner(), 1);
  ctx.verify_universe_invariants();
  std::println(std::cout, "✓ Amphibious alien sector assaults (victory, "
                          "defeat, metamorph) verified");
}

void test_transporter_edge_cases() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Target receiver transporter owned by Player 2
  JsonStore store(ctx.db);
  Race r2{};
  r2.Playernum = 2;
  r2.name = "ReceiverRace";
  RaceRepository(store).save(r2);

  const auto recv_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 10)
                           .owned_by(2, 0)
                           .named("AlienReceiver")
                           .with_alive(true)
                           .with_active(true)
                           .with_on(true)
                           .landed_on(0, 0, {5, 5})
                           .with_fuel(0.0)
                           .with_max_fuel(500.0)
                           .with_resource(0)
                           .with_max_resource(500)
                           .with_destruct(0)
                           .with_max_destruct(200)
                           .with_crystals(0)
                           .with_crew(0, 0)
                           .with_max_crew(100)
                           .build();

  const auto send_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 11)
                           .owned_by(1, 0)
                           .named("SenderDevice")
                           .with_alive(true)
                           .with_active(true)
                           .with_on(true)
                           .landed_on(0, 0, {5, 5})
                           .with_fuel(0.0)
                           .with_max_fuel(500.0)
                           .with_resource(0)
                           .with_max_resource(500)
                           .with_destruct(0)
                           .with_max_destruct(200)
                           .with_crystals(0)
                           .with_crew(0, 0)
                           .with_max_crew(100)
                           .with_special(TransportData{.target = recv_id})
                           .build();

  // 1. Target device damaged
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.admin_override_damage(50); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Target device is damaged");

  // 2. Target device not receiving (off)
  ctx.em.mutate_ship(recv_id, [](Ship& s) {
    s.admin_override_damage(0);
    s.on() = false;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The target device is not receiving");

  // 3. Origin device damaged
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.on() = true; });
  ctx.em.mutate_ship(send_id, [](Ship& s) { s.admin_override_damage(25); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Origin device is damaged");

  // 4. Target device not landed
  ctx.em.mutate_ship(send_id, [](Ship& s) { s.admin_override_damage(0); });
  ctx.em.mutate_ship(
      recv_id, [](Ship& s) { s.launch_to_orbit(ScopeLevel::LEVEL_STAR); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Target ship not landed");

  // 5a. Hopper blocked (target ship == std::nullopt)
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.land_on_planet(); });
  ctx.em.mutate_ship(send_id, [](Ship& s) {
    static_cast<TransporterShip&>(s).transport().target = std::nullopt;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The hopper seems to be blocked");

  // 5b. Hopper blocked (target ship doesn't exist)
  ctx.em.mutate_ship(send_id, [](Ship& s) {
    static_cast<TransporterShip&>(s).transport().target = 9999;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The hopper seems to be blocked");

  // 6. Successful multi-commodity transfer to another player's receiver (sends
  // telegram)
  ctx.em.mutate_ship(send_id, [&](Ship& s) {
    static_cast<TransporterShip&>(s).transport().target = recv_id;
    s.add_fuel(30.0);
    s.destruct() = 15;
    s.add_crystals(5);
    s.popn() = 4;
    s.troops() = 2;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Zap");
  test::expect_eq(ctx.em.peek_ship(recv_id)->resource(), 70);
  test::expect_eq(ctx.em.peek_ship(recv_id)->fuel(), 30.0);
  test::expect_eq(ctx.em.peek_ship(recv_id)->destruct(), 15);
  test::expect_eq(ctx.em.peek_ship(recv_id)->crystals(), 5);
  test::expect_eq(ctx.em.peek_ship(recv_id)->popn(), 4);
  test::expect_eq(ctx.em.peek_ship(recv_id)->troops(), 2);
  std::println(std::cout,
               "✓ Transporter edge cases and cross-player telegrams verified");
}

void test_docking_and_validation_edge_cases() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Irradiated and inactive ship rejection
  shipnum_t rad_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                         .owned_by(1, 0)
                         .named("RadShip")
                         .landed_on(0, 0, {1, 1})
                         .with_active(false)
                         .build();
  ctx.em.mutate_ship(rad_id, [](Ship& s) { s.apply_radiation(10); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", rad_id.value), "r", "10"});

  // 2. Un-docked ship in orbit rejection
  shipnum_t orb_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                         .owned_by(1, 0)
                         .named("OrbitShip")
                         .in_star_orbit(0)
                         .build();
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", orb_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is not landed or docked");
  g.set_level(ScopeLevel::LEVEL_PLAN);

  // 3. Wrong planet scope rejection (attempting to load landed ship from star
  // scope)
  shipnum_t wrong_plan_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(1, 0)
                                .named("OtherPlanetShip")
                                .landed_on(0, 0, {1, 1})
                                .build();
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", wrong_plan_id.value), "r", "10"});
  test::expect_contains(g.out.str(),
                        "Change scope to the planet this ship is landed on");
  g.set_level(ScopeLevel::LEVEL_PLAN);

  // 4. Von Neumann machine unload rejection
  shipnum_t vn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_VN)
                        .owned_by(1, 0)
                        .named("VNProbe")
                        .landed_on(0, 0, {1, 1})
                        .with_resource(50)
                        .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", vn_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "You can't unload VNs");

  // 5. Invalid non-numeric amount argument and empty commodity
  shipnum_t cargo_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                           .owned_by(1, 0)
                           .named("GoodCargo")
                           .landed_on(0, 0, {1, 1})
                           .with_max_resource(500)
                           .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", cargo_id.value), "r", "notanumber"});
  test::expect_contains(g.out.str(), "Invalid amount");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", cargo_id.value), ""});
  test::expect_contains(g.out.str(), "Load what?");

  // 6. Out of bounds amount argument
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", cargo_id.value), "r", "999999"});
  test::expect_contains(g.out.str(), "you can only transfer between");

  // 7. Boobytrap message when loading/unloading destruct on robot ship
  // (max_crew == 0)
  shipnum_t robot_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                           .owned_by(1, 0)
                           .named("MineShip")
                           .landed_on(0, 0, {1, 1})
                           .with_max_crew(0)
                           .with_destruct(0)
                           .with_max_destruct(100)
                           .build();
  ctx.em.mutate_planet(0, 0,
                       [](Planet& p) { p.info(player_t{1}).destruct = 100; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", robot_id.value), "d", "10"});
  test::expect_contains(g.out.str(), "now boobytrapped");

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", robot_id.value), "d", "10"});
  test::expect_contains(g.out.str(), "no longer boobytrapped");

  // 8. Shuttle ship-to-ship resource load/unload (external hull strapping
  // beyond standard internal max_resource = 25)
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(1, 0)
                             .named("ShuttleCraft")
                             .in_star_orbit(0)
                             .with_resource(10)
                             .with_max_resource(25)
                             .build();
  shipnum_t carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(1, 0)
                             .named("CarrierShip")
                             .in_star_orbit(0)
                             .with_resource(100)
                             .with_max_resource(500)
                             .build();
  ctx.em.mutate_ship(shuttle_id,
                     [&](Ship& s) { s.dock_with_ship(carrier_id); });
  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.dock_with_ship(shuttle_id); });
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", shuttle_id.value), "r", "50"});
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 60);

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", shuttle_id.value), "r", "30"});
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 30);

  // Also test unloading from carrier to shuttle (lolim branch) and invalid
  // ship-to-ship commodity
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", carrier_id.value), "r", "10"});
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 40);

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", shuttle_id.value), "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");

  // 9. Overloaded destination ship, destshipno == 0, bogus destination ship,
  // and un-docked destination ship
  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_SHIP; });
  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.dock_with_ship(shuttle_id); });
  ctx.em.mutate_ship(shuttle_id, [&](Ship& s) {
    s.dock_with_ship(carrier_id);
    s.whatorbits() = ScopeLevel::LEVEL_SHIP;
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", carrier_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is overloaded!");

  ctx.em.mutate_ship(shuttle_id, [](Ship& s) {
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
    s.destshipno() = std::nullopt;
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", shuttle_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is not docked");

  ctx.em.mutate_ship(shuttle_id, [](Ship& s) { s.destshipno() = 9999; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", shuttle_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Destination ship is bogus");

  ctx.em.mutate_ship(shuttle_id, [&](Ship& s) {
    s.whatorbits() = ScopeLevel::LEVEL_STAR;
    s.destshipno() = carrier_id;
  });
  ctx.em.mutate_ship(carrier_id,
                     [](Ship& s) { s.destshipno() = std::nullopt; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", shuttle_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is not docked");

  std::println(std::cout,
               "✓ Docking, VN, boobytrap, and validation edge cases verified");
}

}  // namespace

int main() {
  test_load_happy_path();
  test_unload_happy_path();
  test_load_ship_to_ship();
  test_load_transporter();
  test_load_syntax_and_errors();
  test_planet_crew_load_and_unload();
  test_unload_onto_alien_sector();
  test_transporter_edge_cases();
  test_docking_and_validation_edge_cases();

  std::println(std::cout, "\n✅ All load command tests passed!");
  return 0;
}
