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
  ss.xpos = 100.0;
  ss.ypos = 200.0;
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
  const auto trans1_id =
      TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 3)
          .owned_by(1)
          .named("TransporterSender")
          .with_alive(true)
          .with_active(true)
          .with_on(true)
          .landed_on(0, 0, {5, 5})
          .with_max_resource(1000)
          .with_special(TransportData{
              .target = static_cast<unsigned short>(trans2_id.value)})
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

}  // namespace

int main() {
  test_load_happy_path();
  test_unload_happy_path();
  test_load_ship_to_ship();
  test_load_transporter();
  test_load_syntax_and_errors();

  std::println(std::cout, "\n✅ All load command tests passed!");
  return 0;
}
