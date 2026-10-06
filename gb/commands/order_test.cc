// SPDX-License-Identifier: Apache-2.0

/// \file order_test.cc
/// \brief Unit tests for order command and ship standing orders

import commands;
import dallib;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Ship 1: Player 1 Battleship (can bombard, has hyperdrive, lasers, primary &
  // secondary guns)
  auto s1 = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE, 1)
                .owned_by(1)
                .named("TestShip")
                .in_planet_orbit(1, 1)
                .with_guns(guntype_t::HEAVY, 4)
                .with_crew(100, 0)
                .with_speed(5)
                .with_max_speed(9)
                .with_fuel(200.0)
                .with_max_fuel(500.0)
                .with_mount(1)
                .with_crystals(1)
                .build_handle();
  s1->hyper_drive().has = 1;
  s1->laser() = 1;
  s1->mounted() = true;
  s1->set_secondary_battery(2, guntype_t::LIGHT);
}

void test_order_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  std::println(std::cout, "Set ship defense order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "on"});
    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_true(saved_ship->protect().planet);
    test::expect_contains(g.out.str(), "/defense");
  }

  std::println(std::cout, "\nTest 2: Turn defense order off");
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"order", "#1", "defense", "off"});
    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_ne(saved_ship, nullptr);
    test::expect_false(saved_ship->protect().planet);
  }

  std::println(std::cout, "\nTest 3: Set navigation order");
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "270", "4"});
    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_true(saved_ship->navigate().on);
    test::expect_eq(saved_ship->navigate().bearing, 270U);
    test::expect_eq(saved_ship->navigate().turns, 4U);
    test::expect_contains(g.out.str(), "/nav 270 (4)");
  }

  std::println(std::cout, "\nTest 4: Turn navigation order off");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "off"});
    ctx.em.clear_cache();
    const auto* saved_ship = ctx.em.peek_ship(1);
    test::expect_false(saved_ship->navigate().on);
  }

  std::println(std::cout, "\nTest 5: Set evasion order");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "evade", "on"});
    ctx.em.clear_cache();
    test::expect_true(ctx.em.peek_ship(1)->protect().evade);

    ctx.assert_dispatch_success(g, {"order", "#1", "evade", "off"});
    ctx.em.clear_cache();
    test::expect_false(ctx.em.peek_ship(1)->protect().evade);
  }

  std::println(std::cout, "\nTest 6: Set retaliation and bombard orders");
  {
    ctx.assert_dispatch_success(g, {"order", "#1", "retaliate", "on"});
    ctx.assert_dispatch_success(g, {"order", "#1", "bombard", "on"});
    ctx.em.clear_cache();
    test::expect_true(ctx.em.peek_ship(1)->protect().retaliate);
    test::expect_eq(ctx.em.peek_ship(1)->bombard(), 1);

    ctx.assert_dispatch_success(g, {"order", "#1", "retaliate", "off"});
    ctx.assert_dispatch_success(g, {"order", "#1", "bombard", "off"});
    ctx.em.clear_cache();
    test::expect_false(ctx.em.peek_ship(1)->protect().retaliate);
    test::expect_eq(ctx.em.peek_ship(1)->bombard(), 0);
  }

  std::println(std::cout, "\nTest 7: Display all orders to g.out");
  {
    g.out.str("");
    ctx.assert_dispatch_success(g, {"order"});
    test::expect_contains(g.out.str(), "TestShip");
  }
}

void test_order_combat_and_movement_options() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Speed, primary, secondary, salvo, laser, focus, merchant
  ctx.assert_dispatch_success(g, {"order", "#1", "speed", "7"});
  ctx.assert_dispatch_success(g, {"order", "#1", "primary", "3"});
  ctx.assert_dispatch_success(g, {"order", "#1", "salvo", "2"});
  ctx.assert_dispatch_success(g, {"order", "#1", "laser", "on", "5"});
  ctx.assert_dispatch_success(g, {"order", "#1", "focus", "on"});
  ctx.assert_dispatch_success(g, {"order", "#1", "merchant", "2"});

  ctx.em.clear_cache();
  const auto* s1 = ctx.em.peek_ship(1);
  test::expect_eq(s1->speed(), 7);
  test::expect_eq(s1->guns(), PRIMARY);
  test::expect_eq(s1->retaliate(), 2U);
  test::expect_eq(s1->fire_laser(), 5U);
  test::expect_eq(s1->focus(), 1);
  test::expect_eq(s1->merchant(), 2);

  // Secondary battery without explicit gun count
  ctx.assert_dispatch_success(g, {"order", "#1", "secondary"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->guns(), SECONDARY);

  // Destination and hyperdrive jump
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "/Vega"});
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "on"});
  test::expect_contains(g.out.str(), "/jump ready");
  test::expect_contains(g.out.str(), "jump will cost");

  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "off"});
  ctx.assert_dispatch_success(g, {"order", "#1", "laser", "off"});
  ctx.assert_dispatch_success(g, {"order", "#1", "focus", "off"});
  ctx.assert_dispatch_success(g, {"order", "#1", "merchant", "off"});

  // Protect another ship vs self-protect rejection
  const auto s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 2)
                         .owned_by(1)
                         .named("EscortTarget")
                         .in_planet_orbit(1, 1)
                         .with_crew(20, 0)
                         .with_speed(5)
                         .build();
  ctx.assert_dispatch_success(g, {"order", "#1", "protect", "#2"});
  ctx.em.clear_cache();
  test::expect_true(ctx.em.peek_ship(1)->protect().on);
  test::expect_eq(ctx.em.peek_ship(1)->protect().ship, s2_id);

  // Follow ship destination, then redirect to a star or planet and verify
  // destshipno is cleared
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "#2"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(ctx.em.peek_ship(1)->destshipno(), s2_id);

  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "/Sol/Earth"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(ctx.em.peek_ship(1)->deststar(), starnum_t{1});
  test::expect_eq(ctx.em.peek_ship(1)->destpnum(), planetnum_t{1});
  test::expect_eq(ctx.em.peek_ship(1)->destshipno(), std::nullopt);

  // Enable jump toward /Vega, then switch destination to follow #2 and
  // verify jump is deactivated
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "/Vega"});
  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "on"});
  test::expect_true(ctx.em.peek_ship(1)->hyper_drive().on);
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "#2"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(ctx.em.peek_ship(1)->destshipno(), s2_id);
  test::expect_false(ctx.em.peek_ship(1)->hyper_drive().on);

  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "/Vega"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(ctx.em.peek_ship(1)->deststar(), starnum_t{2});
  test::expect_eq(ctx.em.peek_ship(1)->destpnum(), planetnum_t{0});
  test::expect_eq(ctx.em.peek_ship(1)->destshipno(), std::nullopt);

  // Enable jump toward /Vega, then clear destination ("-") and verify jump is
  // deactivated and display_orders does not look up star 0
  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "on"});
  test::expect_true(ctx.em.peek_ship(1)->hyper_drive().on);
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "-"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_UNIV);
  test::expect_false(ctx.em.peek_ship(1)->hyper_drive().on);

  // Self-protect rejection
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "protect", "#1"});
  test::expect_contains(g.out.str(), "You can't do that");

  // Clear protect
  ctx.assert_dispatch_success(g, {"order", "#1", "protect"});
  ctx.em.clear_cache();
  test::expect_false(ctx.em.peek_ship(1)->protect().on);
  test::expect_eq(ctx.em.peek_ship(1)->protect().ship, std::nullopt);

  std::println(std::cout, "    ✓ Combat and movement options verified");
}

void test_order_specialty_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Missile orders (impact and scatter)
  const auto missile_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE, 10)
                              .owned_by(1)
                              .named("Tomahawk")
                              .in_planet_orbit(1, 1)
                              .targeting_planet(1, 1)
                              .build();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", missile_id.value), "impact", "12,34"});
  ctx.em.clear_cache();
  const auto* missile = ctx.em.peek_ship(missile_id)->as<MissileShip>();
  test::expect_ne(missile, nullptr);
  test::expect_eq(missile->impact_coords(), (Coordinates{12, 34}));
  test::expect_false(missile->is_scatter());

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", missile_id.value), "scatter"});
  ctx.em.clear_cache();
  missile = ctx.em.peek_ship(missile_id)->as<MissileShip>();
  test::expect_ne(missile, nullptr);
  test::expect_true(missile->is_scatter());

  // 2. Mine orders (trigger radius, explosive, radiative, switch)
  const auto mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE, 20)
                           .owned_by(1)
                           .named("ProximityMine")
                           .in_planet_orbit(1, 1)
                           .with_on(false)
                           .build();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "trigger", "15"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "radiative"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "switch"});
  ctx.em.clear_cache();
  const auto* mine = ctx.em.peek_ship(mine_id)->as<MineShip>();
  test::expect_ne(mine, nullptr);
  test::expect_eq(mine->trigger_radius(), 15U);
  test::expect_true(mine->is_radiative());
  test::expect_eq(mine->on(), 1);

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "explosive"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id.value), "switch"});
  ctx.em.clear_cache();
  mine = ctx.em.peek_ship(mine_id)->as<MineShip>();
  test::expect_false(mine->is_radiative());
  test::expect_eq(mine->on(), 0);

  // 3. Transporter orders (target ship, self-target rejection, switch)
  const auto trans_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 30)
                            .owned_by(1)
                            .named("Transporter")
                            .landed_on(1, 1, {1, 1})
                            .build();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id.value), "transport", "1"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id.value), "switch"});
  ctx.em.clear_cache();
  const auto* trans = ctx.em.peek_ship(trans_id)->as<TransporterShip>();
  test::expect_eq(trans->target_ship(), shipnum_t{1});

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id.value), "transport", "30"});
  test::expect_contains(g.out.str(), "cannot transport to itself");
  ctx.em.clear_cache();
  test::expect_eq(
      ctx.em.peek_ship(trans_id)->as<TransporterShip>()->target_ship(),
      std::nullopt);

  // 4. Space Mirror aim & intensity, plus Telescope survey
  const auto mirror_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR, 40)
                             .owned_by(1)
                             .named("Helios")
                             .in_planet_orbit(1, 1)
                             .with_crew(10, 0)
                             .with_fuel(50.0)
                             .build();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id.value), "aim", "/Sol/Earth"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id.value), "intensity", "85"});
  ctx.em.clear_cache();
  const auto* mirror = ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  test::expect_eq(mirror->intensity(), 85);
  test::expect_eq(mirror->aimed_level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(mirror->aimed_star(), starnum_t{1});
  test::expect_eq(mirror->aimed_planet(), planetnum_t{1});
  test::expect_eq(mirror->aimed_ship(), std::nullopt);

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id.value), "aim", "#1"});
  ctx.em.clear_cache();
  const auto* mirror_ship_aim =
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>();
  test::expect_eq(mirror_ship_aim->aimed_level(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(mirror_ship_aim->aimed_ship(), shipnum_t{1});
  test::expect_eq(mirror_ship_aim->aimed_star(), std::nullopt);
  test::expect_eq(mirror_ship_aim->aimed_planet(), std::nullopt);

  const auto tele_id = TestShipBuilder(ctx.em, ShipType::OTYPE_STELE, 45)
                           .owned_by(1)
                           .named("Hubble")
                           .in_planet_orbit(1, 1)
                           .with_crew(2, 0)
                           .with_fuel(50.0)
                           .with_tech(200.0)
                           .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "/Sol/Earth"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "/Sol"});
  // Aim telescope at ship and universe while in UNIV scope (snum == 0)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "#1"});
  test::expect_contains(g.out.str(), "You can't see anything of use there.");
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "/"});
  test::expect_contains(g.out.str(), "There is nothing out here to aim at.");
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 5. Terraformer move sequence (valid, cycling, truncation, invalid)
  const auto terra_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TERRA, 50)
                            .owned_by(1)
                            .named("TerraDev")
                            .in_planet_orbit(1, 1)
                            .with_crew(10, 0)
                            .build();

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id.value), "move", "1234c"});
  ctx.em.clear_cache();
  const auto* terra = ctx.em.peek_ship(terra_id)->as<TerraformerShip>();
  test::expect_eq(terra->shipclass(), "1234c");
  test::expect_eq(terra->index(), 0U);

  // Move truncation after 'c' and invalid move characters
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id.value), "move", "12c34"});
  test::expect_contains(g.out.str(), "should be the last character");

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id.value), "move", "c"});
  test::expect_contains(g.out.str(), "Cycling move orders can not be empty");

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id.value), "move", "12x"});
  test::expect_contains(g.out.str(), "is not a valid move direction");

  std::println(std::cout, "    ✓ Specialty ship orders verified");
}

void test_order_factory_activation_and_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Landed Factory on Earth activated via "order #60 on"
  auto f1 = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 60)
                .owned_by(1)
                .named("PlanetFact")
                .landed_on(1, 1, {0, 0})
                .with_crew(5, 0)
                .with_on(false)
                .build_handle();
  f1->build_cost() = 50;

  const auto initial_res = ctx.em.peek_planet(1, 1)->info(1).resource;
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#60", "on"});
  test::expect_contains(g.out.str(),
                        "Factory activated at a cost of 100 resources");
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(60)->on(), 1);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(1).resource,
                  initial_res - 100);

  // Cannot turn off an online factory
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#60", "off"});
  test::expect_contains(g.out.str(),
                        "You can't deactivate a factory once it's online");

  // 2. Factory inside a Habitat activated via "order #62 on"
  auto hab = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 61)
                 .owned_by(1)
                 .named("AlphaHab")
                 .in_planet_orbit(1, 1)
                 .with_crew(50, 0)
                 .with_resource(500)
                 .with_max_hanger(200)
                 .with_hanger(20)
                 .build();
  (void)hab;

  auto f2 = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 62)
                .owned_by(1)
                .named("HabFact")
                .docked_to(61, 1)
                .with_crew(5, 0)
                .with_size(20)
                .with_on(false)
                .build_handle();
  f2->build_cost() = 20;

  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(61);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#62", "on"});
  test::expect_contains(g.out.str(), "Factory activated");
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(62)->on(), 1);

  // 3. Irradiated ship cannot be given orders
  TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 63)
      .owned_by(1)
      .named("RadShip")
      .in_planet_orbit(1, 1)
      .with_active(false)
      .with_radiation(75)
      .build();
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#63", "evade", "on"});
  test::expect_contains(g.out.str(), "is irradiated");

  // 4. Crewless ship (non-robotic) cannot be given orders
  TestShipBuilder(ctx.em, ShipType::STYPE_DESTROYER, 64)
      .owned_by(1)
      .named("GhostShip")
      .in_planet_orbit(1, 1)
      .with_crew(0, 0)
      .build();
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#64", "evade", "on"});
  test::expect_contains(g.out.str(), "has no crew");

  // 5. Capability & validation rejection paths on a Factory (#60) and Missile
  // (#10)
  ctx.assert_dispatch_success(g, {"order", "#60", "defense", "on"});
  ctx.assert_dispatch_success(g, {"order", "#60", "scatter"});
  ctx.assert_dispatch_success(g, {"order", "#60", "impact", "1,1"});
  ctx.assert_dispatch_success(g, {"order", "#10", "impact", "badcoords"});
  ctx.assert_dispatch_success(g, {"order", "#60", "jump", "on"});
  ctx.assert_dispatch_success(g, {"order", "#60", "protect", "#1"});
  ctx.assert_dispatch_success(g, {"order", "#60", "switch"});
  ctx.assert_dispatch_success(g, {"order", "#60", "destination", "/Sol"});
  ctx.assert_dispatch_success(g, {"order", "#60", "bombard", "on"});
  ctx.assert_dispatch_success(g, {"order", "#60", "retaliate", "on"});
  ctx.assert_dispatch_success(g, {"order", "#60", "focus", "on"});
  ctx.assert_dispatch_success(g, {"order", "#60", "laser", "on", "5"});
  ctx.assert_dispatch_success(g, {"order", "#1", "merchant", "99"});
  ctx.assert_dispatch_success(g, {"order", "#60", "speed", "5"});
  ctx.assert_dispatch_success(g, {"order", "#1", "speed", "abc"});
  ctx.assert_dispatch_success(g, {"order", "#60", "salvo", "1"});
  ctx.assert_dispatch_success(g, {"order", "#1", "salvo", "abc"});
  ctx.assert_dispatch_success(g, {"order", "#60", "primary"});
  ctx.assert_dispatch_success(g, {"order", "#1", "primary", "abc"});
  ctx.assert_dispatch_success(g, {"order", "#60", "move", "12"});
  ctx.assert_dispatch_success(g, {"order", "#60", "trigger", "5"});
  ctx.assert_dispatch_success(g, {"order", "#60", "transport", "1"});
  ctx.assert_dispatch_success(g, {"order", "#60", "aim", "/Sol"});

  // Telescope (#45) aimed at ship (#1) and out-of-range star (/Vega)
  ctx.assert_dispatch_success(g, {"order", "#45", "aim", "#1"});
  ctx.assert_dispatch_success(g, {"order", "#45", "aim", "/Vega"});
  ctx.assert_dispatch_success(g, {"order", "#45", "aim", "/Vega/Vega Prime"});

  std::println(std::cout,
               "    ✓ Factory activation and error conditions verified");
}

void test_navigation_and_combat_order_mcdc_and_json() {
  static_assert(std::is_aggregate_v<ShipOrdersHeader>);
  static_assert(std::is_aggregate_v<ShipOrderStatus>);
  static_assert(std::is_aggregate_v<OrderError>);
  static_assert(std::is_aggregate_v<OrderUpdate>);

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Navigate with turns == 0 disables navigation rather than leaving on=true
  // with turns=0; bearing >= 360 wraps modulo 360
  ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "450", "3"});
  ctx.em.clear_cache();
  test::expect_true(ctx.em.peek_ship(1)->navigate().on);
  test::expect_eq(ctx.em.peek_ship(1)->navigate().bearing, 90U);
  test::expect_eq(ctx.em.peek_ship(1)->navigate().turns, 3U);

  ctx.assert_dispatch_success(g, {"order", "#1", "navigate", "90", "0"});
  ctx.em.clear_cache();
  test::expect_false(ctx.em.peek_ship(1)->navigate().on);
  test::expect_eq(ctx.em.peek_ship(1)->navigate().turns, 0U);

  // 2. Destination edge cases: unexplored system planet, invalid place,
  // out-of-range target ship, docked ship, missing destination arg
  ctx.em.mutate_star(2, [](Star& vega) { vega.clear_all_explored(); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", "#1", "destination", "/Vega/Vega Prime"});
  test::expect_contains(g.out.str(), "You haven't explored this system.");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "/NoSuchStar"});
  test::expect_contains(g.out.str(), "No such star");

  const auto far_enemy_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER, 70)
                                .owned_by(2, 1)
                                .in_star_orbit(2)
                                .with_crew(50, 0)
                                .build();
  ctx.em.mutate_ship(far_enemy_id,
                     [](Ship& s) { s.set_coordinates({999999.0, 999999.0}); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", "#1", "destination", std::format("#{}", far_enemy_id)});
  test::expect_contains(g.out.str(), "Warning: that ship is out of range.");

  ctx.assert_dispatch_success(g, {"order", "#1", "destination"});

  // 3. Jump edge cases: no hyperdrive, destination not star/planet, unmounted
  // charging display, and insufficient max_fuel_capacity warning
  const auto no_hd_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE, 71)
                            .owned_by(1, 1)
                            .in_planet_orbit(1, 1)
                            .with_crew(5, 0)
                            .with_speed(3)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", no_hd_id), "jump", "on"});
  test::expect_contains(g.out.str(),
                        "This ship does not have hyper drive capability.");

  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "-"});
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "on"});
  test::expect_contains(g.out.str(), "Destination must be star or planet.");

  ctx.em.mutate_ship(1, [](Ship& s) {
    s.mounted() = false;
    s.hyper_drive().charge = 0;
    s.set_mass(500.0);
    s.max_fuel() = 0.0;
    s.set_star_destination(2);
  });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "jump", "on"});
  test::expect_contains(g.out.str(), "/jump charging 0");
  test::expect_contains(g.out.str(),
                        "Your ship cannot carry enough fuel to do this jump.");
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.mounted() = true;
    s.max_fuel() = 500.0;
    s.hyper_drive().on = false;
  });

  // 4. Laser without crystal mounted, laser on without power arg, secondary
  // guns missing, speed/salvo missing arg, and OMCL bombard/retaliate no-op
  ctx.em.mutate_ship(1, [](Ship& s) { s.mounted() = false; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "laser", "on", "5"});
  test::expect_contains(g.out.str(), "You do not have a crystal mounted.");
  ctx.em.mutate_ship(1, [](Ship& s) { s.mounted() = true; });
  ctx.assert_dispatch_success(g, {"order", "#1", "laser", "on"});

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", no_hd_id), "secondary"});
  test::expect_contains(g.out.str(), "This ship does not have secondary guns.");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "speed"});
  test::expect_contains(g.out.str(), "Specify a positive speed.");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "salvo"});
  test::expect_contains(g.out.str(), "Specify a positive number of guns.");

  const auto omcl_id = TestShipBuilder(ctx.em, ShipType::OTYPE_OMCL, 72)
                           .owned_by(1, 1)
                           .in_planet_orbit(1, 1)
                           .with_crew(10, 0)
                           .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", omcl_id), "bombard", "on"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", omcl_id), "retaliate", "on"});

  // 5. UiMode::JSON rendering of order query
  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"order", "#1"});
  test::expect_contains(g.out.str(), "\"type\":\"ship_order_status\"");
  test::expect_contains(g.out.str(), "\"name\":\"TestShip\"");
  g.set_ui_mode(UiMode::ASCII);

  std::println(std::cout,
               "    ✓ Navigation & combat order MC/DC and JSON verified");
}

void test_special_ship_order_mcdc_and_notices() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. SpaceMirrorShip preserves configured intensity across aim updates, and
  // tests fuel shortage, docked mirror rejection, missing/invalid aim arg,
  // OTYPE_GTELE / OTYPE_TRACT zero-fuel aiming, and intensity edge cases
  const auto mirror_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR, 80)
                             .owned_by(1, 1)
                             .named("MirrorTest")
                             .in_planet_orbit(1, 1)
                             .with_crew(10, 0)
                             .with_fuel(50.0)
                             .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "intensity", "75"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "aim", "/Sol/Earth"});
  ctx.em.clear_cache();
  test::expect_eq(
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>()->intensity(), 75);

  // Intensity with missing/invalid args and on non-mirror ship
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "intensity"});
  ctx.em.clear_cache();
  test::expect_eq(
      ctx.em.peek_ship(mirror_id)->as<SpaceMirrorShip>()->intensity(), 0);
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "intensity", "bad"});
  ctx.assert_dispatch_success(g, {"order", "#1", "intensity", "50"});

  // Aim errors: missing arg, invalid place, docked mirror, insufficient fuel
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"order", std::format("#{}", mirror_id), "aim"});
  test::expect_contains(g.out.str(), "Error in destination.");

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "aim", "/NoSuchStar"});
  test::expect_contains(g.out.str(), "Error in destination.");

  ctx.em.mutate_ship(mirror_id,
                     [](Ship& s) { s.land_on_planet(1, 1, {0, 0}); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "aim", "/Sol"});
  test::expect_contains(g.out.str(), "docked; use undock or launch first.");

  ctx.em.mutate_ship(mirror_id, [](Ship& s) {
    s.enter_planet_orbit(1, 1);
    s.consume_fuel(s.fuel());
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mirror_id), "aim", "/Sol"});
  test::expect_contains(g.out.str(), "Not enough maneuvering fuel");

  // Ground telescope (OTYPE_GTELE) and Tractor (OTYPE_TRACT) aim without fuel
  const auto gtele_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GTELE, 81)
                            .owned_by(1, 1)
                            .landed_on(1, 1, {0, 0})
                            .with_crew(2, 0)
                            .with_fuel(0.0)
                            .with_tech(200.0)
                            .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", gtele_id), "aim", "/Sol/Earth"});

  const auto tract_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRACT, 82)
                            .owned_by(1, 1)
                            .in_planet_orbit(1, 1)
                            .with_crew(5, 0)
                            .with_fuel(0.0)
                            .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tract_id), "aim", "#1"});

  // 2. Failed orders preserve ship.notified(), valid orders clear notified()=0
  ctx.em.mutate_ship(1, [](Ship& s) { s.notified() = 1; });
  ctx.assert_dispatch_success(g, {"order", "#1", "scatter"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->notified(), 1);
  ctx.assert_dispatch_success(g, {"order", "#1", "evade", "on"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->notified(), 0);

  // 3. OTYPE_GR (Gamma Ray Laser) explosive/radiative mode and display
  const auto gr_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GR, 83)
                         .owned_by(1, 1)
                         .landed_on(1, 1, {1, 1})
                         .with_crew(5, 0)
                         .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", gr_id), "radiative"});
  test::expect_contains(g.out.str(), "/radiate");
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", gr_id), "explosive"});
  test::expect_contains(g.out.str(), "/explode");

  // 4. Terraformer move sequence length truncation, default move, and cycling
  // display when index > 0
  const auto terra_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TERRA, 84)
                            .owned_by(1, 1)
                            .in_planet_orbit(1, 1)
                            .with_crew(10, 0)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", terra_id), "move",
                                  "12345678912345678912"});
  test::expect_contains(g.out.str(), "These move orders have been truncated.");

  ctx.assert_dispatch_success(g,
                              {"order", std::format("#{}", terra_id), "move"});
  ctx.em.clear_cache();
  test::expect_eq(
      ctx.em.peek_ship(terra_id)->as<TerraformerShip>()->shipclass(), "5");

  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", terra_id), "move", "1234c"});
  ctx.em.mutate_ship(terra_id,
                     [](Ship& s) { s.as<TerraformerShip>()->set_index(2); });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", terra_id)});
  test::expect_contains(g.out.str(), "/move 34c12c");

  // 5. Switch & on/off edge cases: no switch, transported ship, transporter
  // toggle off, damaged ship activation, already active ship, and factory
  // habitat/planet resource/hangar errors
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "switch"});
  test::expect_contains(g.out.str(),
                        "That ship does not have an on/off setting.");
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "on"});
  test::expect_contains(g.out.str(),
                        "This ship does not have an on/off setting.");

  const auto trans_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TRANSDEV, 85)
                            .owned_by(1, 1)
                            .landed_on(1, 1, {2, 2})
                            .with_on(true)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id), "switch"});
  test::expect_contains(g.out.str(), "No longer receiving.");
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", trans_id), "transport"});

  const auto mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE, 86)
                           .owned_by(1, 1)
                           .in_planet_orbit(1, 1)
                           .with_damage(20)
                           .with_on(false)
                           .build();
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", mine_id), "on"});
  test::expect_contains(g.out.str(), "Damaged ships cannot be activated.");

  ctx.em.mutate_ship(mine_id, [](Ship& s) { s.repair_damage(20); });
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", mine_id), "on"});
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", mine_id), "on"});
  test::expect_contains(g.out.str(), "This ship is already activated.");
  ctx.assert_dispatch_success(g, {"order", std::format("#{}", mine_id), "off"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id), "trigger"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", mine_id), "trigger", "bad"});

  // Transported mine cannot use switch
  ctx.em.mutate_ship(mine_id, [](Ship& s) { s.dock_into_carrier(1); });
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"order", std::format("#{}", mine_id), "switch"});
  test::expect_contains(g.out.str(), "That ship is being transported.");

  // Factory activation errors: transported inside non-Habitat, Habitat lacking
  // resources, Habitat lacking hangar space, orbiting factory, planet lacking
  // resources
  auto f_err = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 87)
                   .owned_by(1, 1)
                   .docked_to(1, 1)
                   .with_crew(5, 0)
                   .with_max_hanger(100)
                   .with_size(1)
                   .with_on(false)
                   .build_handle();
  f_err->build_cost() = 50;
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(),
                        "The factory is currently being transported.");

  const auto hab_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 88)
                          .owned_by(1, 1)
                          .in_planet_orbit(1, 1)
                          .with_crew(50, 0)
                          .with_resource(10)
                          .with_max_hanger(20)
                          .with_hanger(20)
                          .build();
  ctx.em.mutate_ship(87, [&](Ship& s) { s.dock_into_carrier(hab_id); });
  g.set_shipno(hab_id);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(), "You don't have");

  ctx.em.mutate_ship(hab_id, [](Ship& s) { s.resource() = 1000; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(), "Not enough hanger space free on Habitat");

  g.set_level(ScopeLevel::LEVEL_PLAN);
  ctx.em.mutate_ship(87, [](Ship& s) { s.enter_planet_orbit(1, 1); });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(), "You cannot activate the factory here.");

  ctx.em.mutate_ship(87, [](Ship& s) { s.land_on_planet(1, 1, {0, 0}); });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(1).resource = 0; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(),
                        "resources on the planet to activate this factory");

  // 6. UiMode::JSON rendering of OrderUpdate notice
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(1).resource = 500; });
  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"order", "#87", "on"});
  test::expect_contains(g.out.str(), "\"type\":\"order_update\"");
  test::expect_contains(g.out.str(), "\"factory_activation_cost\":100");
  g.set_ui_mode(UiMode::ASCII);

  std::println(std::cout,
               "    ✓ Special ship order MC/DC and notices verified");
}

}  // namespace

int main() {
  test_order_happy_path();
  test_order_combat_and_movement_options();
  test_order_specialty_ships();
  test_order_factory_activation_and_errors();
  test_navigation_and_combat_order_mcdc_and_json();
  test_special_ship_order_mcdc_and_notices();
  std::println(std::cout, "\n✅ All order tests passed!");
  return 0;
}
