// SPDX-License-Identifier: Apache-2.0

/// \file load_test.cc
/// \brief Unit tests for the load command.

import commands;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

constexpr player_t trader_player = 2;
constexpr player_t alien_player = 3;

void setup_empty_cargo_galaxy(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Traders", 100.0, false, trader_player);
  world.add_race("Aliens", 100.0, false, alien_player);
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(trader_player);
    s.mark_inhabited_by(trader_player);
    s.AP(trader_player) = 50;
  });
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.mark_explored_by(trader_player); });
}

void setup_test_world(TestContext& ctx) {
  setup_empty_cargo_galaxy(ctx);

  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(trader_player).fuel = 1000;
    p.info(trader_player).resource = 500;
    p.info(trader_player).destruct = 200;
    p.info(trader_player).crystals = 50;
  });

  // Create a landed ship to load cargo onto
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(trader_player, 1)
      .named("CargoHauler")
      .landed_on(1, 1, {5, 5})
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
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  std::println(std::cout, "Load fuel from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    double initial_ship_fuel = s_before->fuel();
    int initial_planet_fuel = p_before->info(trader_player).fuel;

    ctx.assert_dispatch_success(g, {"load", "#1", "f", "100"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->fuel(), initial_ship_fuel + 100);
    test::expect_eq(p_after->info(trader_player).fuel,
                    initial_planet_fuel - 100);
    std::println(std::cout, "✓ Fuel loaded from planet to ship");
  }

  std::println(std::cout, "Load resources from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_resource = s_before->resource();
    int initial_planet_resource = p_before->info(trader_player).resource;

    ctx.assert_dispatch_success(g, {"load", "#1", "r", "200"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->resource(), initial_ship_resource + 200);
    test::expect_eq(p_after->info(trader_player).resource,
                    initial_planet_resource - 200);
    std::println(std::cout, "✓ Resources loaded from planet to ship");
  }

  std::println(std::cout, "Load destruct from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_destruct = s_before->destruct();
    int initial_planet_destruct = p_before->info(trader_player).destruct;

    ctx.assert_dispatch_success(g, {"load", "#1", "d", "50"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->destruct(), initial_ship_destruct + 50);
    test::expect_eq(p_after->info(trader_player).destruct,
                    initial_planet_destruct - 50);
    std::println(std::cout, "✓ Destruct loaded from planet to ship");
  }

  std::println(std::cout, "Load crystals from planet to ship");
  {
    const auto* s_before = ctx.em.peek_ship(1);
    const auto* p_before = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_before, nullptr);
    test::expect_ne(p_before, nullptr);
    int initial_ship_crystals = s_before->crystals();
    int initial_planet_crystals = p_before->info(trader_player).crystals;

    ctx.assert_dispatch_success(g, {"load", "#1", "x", "10"});

    const auto* s_after = ctx.em.peek_ship(1);
    const auto* p_after = ctx.em.peek_planet(1, 1);
    test::expect_ne(s_after, nullptr);
    test::expect_ne(p_after, nullptr);
    test::expect_eq(s_after->crystals(), initial_ship_crystals + 10);
    test::expect_eq(p_after->info(trader_player).crystals,
                    initial_planet_crystals - 10);
    std::println(std::cout, "✓ Crystals loaded from planet to ship");
  }
}

void test_load_syntax_and_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"load"});
  test::expect_contains(g.out.str(),
                        "Syntax: load <ship> <commodity> [<amount>]");

  // 2. Unknown commodity & enslaved planet stockpile smuggling blocked (while
  // unloading onto an enslaved planet remains permitted)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"load", "#1", "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.enslave_to(alien_player); });
  ctx.assert_dispatch_rejected(g, {"load", "#1", "r", "10"});
  test::expect_contains(g.out.str(), "enslaved to player 3");
  ctx.em.mutate_ship(1, [](Ship& s) { s.add_resource(10); });
  ctx.assert_dispatch_success(g, {"unload", "#1", "r", "5"});
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.free_slaves(); });
}

void test_load_transporter() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  // Target receiver transporter ship 2
  const auto trans2_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV)
                             .owned_by(trader_player)
                             .named("TransporterReceiver")
                             .with_alive(true)
                             .with_active(true)
                             .with_on(true)
                             .landed_on(1, 1, {5, 5})
                             .with_max_resource(1000)
                             .build();

  // Source transmitter transporter ship 3
  const auto trans1_id =
      TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV)
          .owned_by(trader_player)
          .named("TransporterSender")
          .with_alive(true)
          .with_active(true)
          .with_on(true)
          .landed_on(1, 1, {5, 5})
          .with_max_resource(1000)
          .with_special(TransportData{.target_ship = trans2_id})
          .build();

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", trans1_id.value), "r", "40"});
  test::expect_contains(g.out.str(), "Zap");
  test::expect_contains(g.out.str(), "40 resources transferred");

  ctx.em.clear_cache();
  // Sender should have 0 resources (transferred out)
  test::expect_eq(ctx.em.peek_ship(trans1_id)->resource(), 0);
  // Receiver should have 40 resources; same-owner crew/troops beam cleanly
  test::expect_eq(ctx.em.peek_ship(trans2_id)->resource(), 40);
  ctx.em.mutate_ship(trans1_id, [](Ship& s) {
    s.add_popn(3, 1.0);
    s.add_troops(2, 1.0);
  });
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", trans1_id.value), "r", "10"});
  test::expect_eq(ctx.em.peek_ship(trans2_id)->popn(), 3);
  test::expect_eq(ctx.em.peek_ship(trans2_id)->troops(), 2);
  std::println(std::cout, "✓ Transporter automatic beam transfer succeeded");
}

void test_load_ship_to_ship() {
  TestContext ctx;
  setup_empty_cargo_galaxy(ctx);

  // Create carrier/mothership (s2) in star orbit
  shipnum_t s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                        .owned_by(trader_player, 1)
                        .named("Mothership")
                        .in_star_orbit(1)
                        .with_fuel(200.0)
                        .with_max_fuel(500.0)
                        .with_resource(300)
                        .with_max_resource(1000)
                        .with_destruct(100)
                        .with_max_destruct(200)
                        .with_crystals(50)
                        .with_crew(20, 10)
                        .build();

  // Create tender/cargo ship (s1) moored to s2 in star orbit
  shipnum_t s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                        .owned_by(trader_player, 1)
                        .named("Tender")
                        .in_star_orbit(1)
                        .with_fuel(50.0)
                        .with_max_fuel(200.0)
                        .with_resource(50)
                        .with_max_resource(500)
                        .with_destruct(10)
                        .with_max_destruct(100)
                        .with_crystals(5)
                        .with_crew(5, 2)
                        .build();

  // Establish mutual peer mooring in star orbit
  test::expect_true(ctx.em.moor_ships(s1_id, s2_id).has_value());

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_star(1);

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

  // C. Carrier-to-berthed-craft load, unload, and jettison preserve ancestor
  //    carrier masses without drift
  {
    const shipnum_t super_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                   .owned_by(trader_player, 1)
                                   .named("SuperCarrier")
                                   .in_star_orbit(1)
                                   .with_max_hanger(2000)
                                   .build();
    const shipnum_t sub_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                 .owned_by(trader_player, 1)
                                 .named("EscortCarrier")
                                 .in_star_orbit(1)
                                 .with_max_hanger(500)
                                 .with_resource(100)
                                 .with_max_resource(500)
                                 .build();
    const shipnum_t craft_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                   .owned_by(trader_player, 1)
                                   .named("HangarShuttle")
                                   .in_star_orbit(1)
                                   .with_resource(10)
                                   .with_max_resource(100)
                                   .with_crew(2, 0)
                                   .build();
    test::expect_true(ctx.em.dock_carrier(sub_id, super_id).has_value());
    test::expect_true(ctx.em.dock_carrier(craft_id, sub_id).has_value());

    const double initial_super_mass = ctx.em.peek_ship(super_id)->mass();
    const double initial_sub_mass = ctx.em.peek_ship(sub_id)->mass();
    const double initial_craft_mass = ctx.em.peek_ship(craft_id)->mass();

    g.set_scope_ship(sub_id);
    ctx.assert_dispatch_success(
        g, {"load", std::format("#{}", craft_id.value), "r", "20"});
    test::expect_true(std::abs(ctx.em.peek_ship(super_id)->mass() -
                               initial_super_mass) < 1e-6);
    test::expect_true(
        std::abs(ctx.em.peek_ship(sub_id)->mass() - initial_sub_mass) < 1e-6);
    test::expect_true(std::abs(ctx.em.peek_ship(craft_id)->mass() -
                               (initial_craft_mass + 2.0)) < 1e-6);

    ctx.assert_dispatch_success(
        g, {"jettison", std::format("#{}", craft_id.value), "r", "10"});
    test::expect_true(std::abs(ctx.em.peek_ship(super_id)->mass() -
                               (initial_super_mass - 1.0)) < 1e-6);
    test::expect_true(std::abs(ctx.em.peek_ship(sub_id)->mass() -
                               (initial_sub_mass - 1.0)) < 1e-6);
    ctx.verify_universe_invariants();
  }

  // C. Alien ship transfer rules (can give goods to alien, cannot take)
  shipnum_t alien_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                           .owned_by(alien_player, 1)
                           .named("KlingonFreighter")
                           .in_star_orbit(1)
                           .with_resource(100)
                           .with_max_resource(500)
                           .build();

  // Dock s1 with alien ship
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) { s1.dock_with_ship(alien_id); });
  ctx.em.mutate_ship(alien_id,
                     [&](Ship& alien) { alien.dock_with_ship(s1_id); });
  g.set_scope_star(1);

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

  // D. Joint crew+troops berth capacity in ship-to-ship transfer + UiMode::JSON
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) {
    s1.dock_with_ship(s2_id);
    s1.popn() = 10;
    s1.troops() = 5;
  });
  ctx.em.mutate_ship(s2_id, [&](Ship& s2) {
    s2.dock_with_ship(s1_id);
    s2.popn() = 20;
    s2.troops() = 0;
  });
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"load", std::format("#{}", s1_id.value), "c"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), "docked with");
  test::expect_contains(g.out.str(), "crew complement");
  test::expect_eq(
      ctx.em.peek_ship(s1_id)->popn() + ctx.em.peek_ship(s2_id)->popn(), 30);
  std::println(std::cout, "✓ Alien ship transfer constraints enforced");
}

void test_planet_crew_load_and_unload() {
  TestContext ctx;
  setup_empty_cargo_galaxy(ctx);

  // Create landed cargo ship with crew capacity and initial crew/troops
  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                          .owned_by(trader_player, 1)
                          .named("ColonyShip")
                          .landed_on(1, 1, {5, 5})
                          .with_crew(50, 20)
                          .with_max_crew(100)
                          .build();

  // Ensure sector (5, 5) is empty land
  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({5, 5});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_owner(0);
    sect.set_popn_exact(0);
    sect.set_troops(0);
    p.sync_demographics(map);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  // 1. Unload civilians onto empty sector -> sector COLONIZED
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "c", "10"});
  test::expect_contains(g.out.str(), "sector 5,5 COLONIZED");
  test::expect_eq(ctx.em.peek_ship(ship_id)->popn(), 40);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_popn(), 10);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_owner(),
                  trader_player);
  ctx.verify_universe_invariants();

  // 2. Load civilians back until sector is empty -> sector evacuated
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", ship_id.value), "c", "10"});
  test::expect_contains(g.out.str(), "sector 5,5 evacuated");
  test::expect_eq(ctx.em.peek_ship(ship_id)->popn(), 50);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_popn(), 0);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_owner(), 0);
  ctx.verify_universe_invariants();

  // 3. Unload military onto empty sector -> sector OCCUPIED
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "m", "5"});
  test::expect_contains(g.out.str(), "sector 5,5 OCCUPIED");
  test::expect_eq(ctx.em.peek_ship(ship_id)->troops(), 15);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_troops(), 5);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_owner(),
                  trader_player);
  ctx.verify_universe_invariants();

  // 4. Load military with omitted amount (default maximum available) ->
  // evacuated
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"load", std::format("#{}", ship_id.value), "m"});
  test::expect_contains(g.out.str(), "sector 5,5 evacuated");
  test::expect_eq(ctx.em.peek_ship(ship_id)->troops(), 20);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_troops(), 0);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_owner(), 0);
  ctx.verify_universe_invariants();

  // 5. Joint crew capacity: ship has 50 popn + 20 troops (30 berths left out of
  // 100). Loading civilians from a sector with 50 popn must cap at 30 so 20
  // civilians remain on the sector instead of vanishing.
  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({5, 5});
    sect.set_owner(trader_player);
    sect.set_popn_exact(50);
    sect.set_troops(0);
    p.sync_demographics(map);
  });
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"load", std::format("#{}", ship_id.value), "c"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), "crew complement");
  test::expect_eq(ctx.em.peek_ship(ship_id)->popn(), 80);
  test::expect_eq(ctx.em.peek_ship(ship_id)->troops(), 20);
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({5, 5}).get_popn(), 20);
  ctx.verify_universe_invariants();

  for (const auto ev : all_planet_sector_events) {
    const PlanetCargoTransferResult sample{
        .commodity = ShipCargoType::Crew,
        .amount = 10,
        .ship_display = "c1ColonyShip [1]",
        .sector_coords = {5, 5},
        .sector_event = ev,
        .resulting_popn = 80,
    };
    test::expect_false(
        GB::presentation::render_for_mode(UiMode::ASCII, sample).empty());
    test::expect_contains(
        GB::presentation::render_for_mode(UiMode::JSON, sample),
        R"("type":"message")");
    test::expect_contains(
        GB::presentation::render_for_mode(UiMode::JSON, sample),
        "crew complement");
  }

  std::println(std::cout,
               "✓ Planetary crew colonization and evacuation verified");
}

void test_load_from_alien_sector_rejected() {
  TestContext ctx;
  setup_empty_cargo_galaxy(ctx);

  const shipnum_t assault_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                   .owned_by(trader_player, 1)
                                   .named("DropShip")
                                   .landed_on(1, 1, {2, 2})
                                   .with_crew(50, 50)
                                   .with_max_crew(200)
                                   .build();

  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_owner(alien_player);
    sect.set_popn_exact(10);
    sect.set_troops(5);
    p.sync_demographics(map);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", assault_id.value), "c", "5"});
  test::expect_contains(g.out.str(),
                        "You have to unload to assault alien sectors.");
  std::println(std::cout, "✓ Load from alien-occupied sector rejected");
}

void test_transporter_edge_cases() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  const auto recv_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV)
                           .owned_by(alien_player, 1)
                           .named("AlienReceiver")
                           .with_alive(true)
                           .with_active(true)
                           .with_on(true)
                           .landed_on(1, 1, {5, 5})
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

  const auto send_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV)
                           .owned_by(trader_player, 1)
                           .named("SenderDevice")
                           .with_alive(true)
                           .with_active(true)
                           .with_on(true)
                           .landed_on(1, 1, {5, 5})
                           .with_fuel(0.0)
                           .with_max_fuel(500.0)
                           .with_resource(0)
                           .with_max_resource(500)
                           .with_destruct(0)
                           .with_max_destruct(200)
                           .with_crystals(0)
                           .with_crew(0, 0)
                           .with_max_crew(100)
                           .with_special(TransportData{.target_ship = recv_id})
                           .build();

  // 1. Target device damaged
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.admin_override_damage(50); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "Target device is damaged");

  // 2. Target device not receiving (off and non-transporter type)
  ctx.em.mutate_ship(recv_id, [](Ship& s) {
    s.admin_override_damage(0);
    s.on() = false;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The target device is not receiving");

  const auto non_trans_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                .owned_by(trader_player, 1)
                                .landed_on(1, 2, {5, 5})
                                .with_on(true)
                                .build();
  ctx.em.mutate_as<TransporterShip>(
      send_id, [&](TransporterShip& t) { t.set_target_ship(non_trans_id); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The target device is not receiving");

  // 3. Origin device damaged, wrong planet scope, and not landed
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.on() = true; });
  ctx.em.mutate_as<TransporterShip>(
      send_id, [&](TransporterShip& t) { t.set_target_ship(recv_id); });
  ctx.em.mutate_ship(send_id, [&](Ship& s) {
    auto& trans = static_cast<TransporterShip&>(s);
    const auto wrong_star =
        execute_transporter_transfer(ctx.em, trans, 2, 1, 1.0, false);
    test::expect_false(wrong_star.has_value());
    test::expect_eq(wrong_star.error(), TransporterError::WrongPlanetScope);

    const auto wrong_plan =
        execute_transporter_transfer(ctx.em, trans, 1, 2, 1.0, false);
    test::expect_false(wrong_plan.has_value());
    test::expect_eq(wrong_plan.error(), TransporterError::WrongPlanetScope);

    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    const auto not_landed =
        execute_transporter_transfer(ctx.em, trans, 1, 1, 1.0, false);
    test::expect_false(not_landed.has_value());
    test::expect_eq(not_landed.error(), TransporterError::OriginNotLanded);

    s.land_on_planet(1, 1, {5, 5});
    s.admin_override_damage(25);
  });
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

  // 5. Hopper blocked (target ship == std::nullopt)
  ctx.em.mutate_ship(recv_id, [](Ship& s) { s.land_on_planet(1, 1); });
  ctx.em.mutate_ship(send_id, [](Ship& s) {
    static_cast<TransporterShip&>(s).set_target_ship(std::nullopt);
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "The hopper seems to be blocked");

  // 6. Successful multi-commodity transfer to another player's receiver (sends
  // telegram)
  ctx.em.mutate_ship(send_id, [&](Ship& s) {
    static_cast<TransporterShip&>(s).set_target_ship(recv_id);
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
  test::expect_eq(ctx.em.peek_ship(recv_id)->popn(), 0);
  test::expect_eq(ctx.em.peek_ship(recv_id)->troops(), 0);
  test::expect_eq(ctx.em.peek_ship(send_id)->popn(), 4);
  test::expect_eq(ctx.em.peek_ship(send_id)->troops(), 2);

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", send_id.value), "r", "5"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("type":"ship")");
  test::expect_contains(g.out.str(), "transferred");

  for (const auto err : all_transporter_errors) {
    test::expect_false(GB::presentation::format_transporter_error(err).empty());
  }
  for (const auto reason : all_ship_docking_error_reasons) {
    test::expect_false(GB::presentation::format_ship_docking_error(
                           ShipDockingError{.reason = reason})
                           .empty());
  }
  std::println(std::cout,
               "✓ Transporter edge cases and cross-player telegrams verified");
}

void test_docking_and_validation_edge_cases() {
  TestContext ctx;
  setup_empty_cargo_galaxy(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, trader_player, 1);
  g.set_scope_planet(1, 1);

  // 1. Irradiated and inactive ship rejection
  shipnum_t rad_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                         .owned_by(trader_player, 1)
                         .named("RadShip")
                         .landed_on(1, 1, {1, 1})
                         .with_active(false)
                         .build();
  ctx.em.mutate_ship(rad_id, [](Ship& s) { s.apply_radiation(10); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", rad_id.value), "r", "10"});

  // 2. Un-docked ship in orbit rejection
  shipnum_t orb_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                         .owned_by(trader_player, 1)
                         .named("OrbitShip")
                         .in_star_orbit(1)
                         .build();
  g.set_scope_star(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", orb_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is not landed or docked");
  g.set_scope_planet(1, 1);

  // 3. Wrong planet scope rejection (attempting to load landed ship from star
  // scope, wrong star, and wrong planet)
  shipnum_t wrong_plan_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(trader_player, 1)
                                .named("OtherPlanetShip")
                                .landed_on(1, 1, {1, 1})
                                .build();
  g.set_scope_star(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", wrong_plan_id.value), "r", "10"});
  test::expect_contains(g.out.str(),
                        "Change scope to the planet this ship is landed on");
  g.set_scope_planet(1, 1);
  ctx.em.mutate_ship(wrong_plan_id, [&](Ship& s) {
    const auto wrong_star = validate_ship_docking(ctx.em, s, trader_player,
                                                  ScopeLevel::LEVEL_PLAN, 2, 1);
    test::expect_false(wrong_star.has_value());
    test::expect_eq(wrong_star.error().reason,
                    ShipDockingErrorReason::WrongPlanetScope);

    const auto wrong_plan = validate_ship_docking(ctx.em, s, trader_player,
                                                  ScopeLevel::LEVEL_PLAN, 1, 2);
    test::expect_false(wrong_plan.has_value());
    test::expect_eq(wrong_plan.error().reason,
                    ShipDockingErrorReason::WrongPlanetScope);
  });

  // 4. Negative load on Von Neumann machine rejection
  shipnum_t vn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_VN)
                        .owned_by(trader_player, 1)
                        .named("VNProbe")
                        .landed_on(1, 1, {1, 1})
                        .with_resource(50)
                        .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", vn_id.value), "r", "-10"});
  test::expect_contains(g.out.str(), "You can't unload VNs");

  // 5. Invalid non-numeric amount argument and empty commodity
  shipnum_t cargo_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                           .owned_by(trader_player, 1)
                           .named("GoodCargo")
                           .landed_on(1, 1, {1, 1})
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

  // 6. Out of bounds amount argument (both > uplim and < lolim)
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", cargo_id.value), "r", "999999"});
  test::expect_contains(g.out.str(), "you can only transfer between");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", cargo_id.value), "r", "-999999"});
  test::expect_contains(g.out.str(), "you can only transfer between");

  // 7. Boobytrap message when loading destruct on robot ship (max_crew == 0)
  shipnum_t robot_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                           .owned_by(trader_player, 1)
                           .named("MineShip")
                           .landed_on(1, 1, {1, 1})
                           .with_max_crew(0)
                           .with_destruct(0)
                           .with_max_destruct(100)
                           .build();
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(trader_player).destruct = 100; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", robot_id.value), "d", "10"});
  test::expect_contains(g.out.str(), "now boobytrapped");

  // 8. Shuttle ship-to-ship resource load/unload (external hull strapping
  // beyond standard internal max_resource = 25)
  shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(trader_player, 1)
                             .named("ShuttleCraft")
                             .in_star_orbit(1)
                             .with_resource(10)
                             .with_max_resource(25)
                             .build();
  shipnum_t carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(trader_player, 1)
                             .named("CarrierShip")
                             .in_star_orbit(1)
                             .with_resource(100)
                             .with_max_resource(500)
                             .build();
  ctx.em.mutate_ship(shuttle_id,
                     [&](Ship& s) { s.dock_with_ship(carrier_id); });
  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.dock_with_ship(shuttle_id); });
  g.set_scope_star(1);
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", shuttle_id.value), "r", "50"});
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 60);

  // Overloaded shuttle moored in orbit (not inside carrier hangar) is allowed
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"load", std::format("#{}", carrier_id.value), "r", "10"});
  test::expect_eq(ctx.em.peek_ship(shuttle_id)->resource(), 50);

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", shuttle_id.value), "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");

  // 9. Overloaded destination ship in carrier, missing destshipno, dead
  // destination ship, and un-docked destination ship
  ctx.em.mutate_ship(shuttle_id,
                     [&](Ship& s) { s.dock_into_carrier(carrier_id); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", carrier_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is overloaded!");

  ctx.em.mutate_ship(carrier_id, [](Ship& s) { s.set_star_destination(1); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"load", std::format("#{}", carrier_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "is not docked");

  ctx.em.mutate_ship(shuttle_id, [&](Ship& s) {
    s.enter_star_orbit(1);
    s.dock_with_ship(carrier_id);
  });
  ctx.em.mutate_ship(carrier_id, [](Ship& s) { s.undock_from_ship(); });
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
  test_load_ship_to_ship();
  test_load_transporter();
  test_load_syntax_and_errors();
  test_planet_crew_load_and_unload();
  test_load_from_alien_sector_rejected();
  test_transporter_edge_cases();
  test_docking_and_validation_edge_cases();

  std::println(std::cout, "\n✅ All load command tests passed!");
  return 0;
}
