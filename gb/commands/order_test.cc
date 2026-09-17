// SPDX-License-Identifier: Apache-2.0

/// \file order_test.cc
/// \brief Unit tests for order command and ship standing orders

import commands;
import dallib;
import gb.entities;
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
                .in_planet_orbit(0, 0)
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
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

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
    test::expect_true(ctx.em.peek_ship(1)->protect().self);
    test::expect_eq(ctx.em.peek_ship(1)->bombard(), 1);

    ctx.assert_dispatch_success(g, {"order", "#1", "retaliate", "off"});
    ctx.assert_dispatch_success(g, {"order", "#1", "bombard", "off"});
    ctx.em.clear_cache();
    test::expect_false(ctx.em.peek_ship(1)->protect().self);
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
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

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
                         .in_planet_orbit(0, 0)
                         .with_crew(20, 0)
                         .with_speed(5)
                         .build();
  ctx.assert_dispatch_success(g, {"order", "#1", "protect", "#2"});
  ctx.em.clear_cache();
  test::expect_true(ctx.em.peek_ship(1)->protect().on);
  test::expect_eq(ctx.em.peek_ship(1)->protect().ship, s2_id);

  // Follow ship destination
  ctx.assert_dispatch_success(g, {"order", "#1", "destination", "#2"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(1)->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(ctx.em.peek_ship(1)->destshipno(), s2_id);

  // Self-protect rejection
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#1", "protect", "#1"});
  test::expect_contains(g.out.str(), "You can't do that");

  // Clear protect
  ctx.assert_dispatch_success(g, {"order", "#1", "protect"});
  ctx.em.clear_cache();
  test::expect_false(ctx.em.peek_ship(1)->protect().on);

  std::println(std::cout, "    ✓ Combat and movement options verified");
}

void test_order_specialty_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Missile orders (impact and scatter)
  const auto missile_id = TestShipBuilder(ctx.em, ShipType::STYPE_MISSILE, 10)
                              .owned_by(1)
                              .named("Tomahawk")
                              .in_planet_orbit(0, 0)
                              .targeting_planet(0, 0)
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
                           .in_planet_orbit(0, 0)
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
                            .landed_on(0, 0, {1, 1})
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

  // 4. Space Mirror aim & intensity, plus Telescope survey
  const auto mirror_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR, 40)
                             .owned_by(1)
                             .named("Helios")
                             .in_planet_orbit(0, 0)
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

  const auto tele_id = TestShipBuilder(ctx.em, ShipType::OTYPE_STELE, 45)
                           .owned_by(1)
                           .named("Hubble")
                           .in_planet_orbit(0, 0)
                           .with_crew(2, 0)
                           .with_fuel(50.0)
                           .with_tech(200.0)
                           .build();
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "/Sol/Earth"});
  ctx.assert_dispatch_success(
      g, {"order", std::format("#{}", tele_id.value), "aim", "/Sol"});

  // 5. Terraformer move sequence (valid, cycling, truncation, invalid)
  const auto terra_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TERRA, 50)
                            .owned_by(1)
                            .named("TerraDev")
                            .in_planet_orbit(0, 0)
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
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Landed Factory on Earth activated via "order #60 on"
  auto f1 = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 60)
                .owned_by(1)
                .named("PlanetFact")
                .landed_on(0, 0, {0, 0})
                .with_crew(5, 0)
                .with_on(false)
                .build_handle();
  f1->build_cost() = 50;
  f1->deststar() = 0;
  f1->destpnum() = 0;

  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#60", "on"});
  test::expect_contains(g.out.str(),
                        "Factory activated at a cost of 100 resources");
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_ship(60)->on(), 1);

  // Cannot turn off an online factory
  g.out.str("");
  ctx.assert_dispatch_success(g, {"order", "#60", "off"});
  test::expect_contains(g.out.str(),
                        "You can't deactivate a factory once it's online");

  // 2. Factory inside a Habitat activated via "order #62 on"
  auto hab = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 61)
                 .owned_by(1)
                 .named("AlphaHab")
                 .in_planet_orbit(0, 0)
                 .with_crew(50, 0)
                 .with_resource(500)
                 .with_max_hanger(200)
                 .with_hanger(20)
                 .build();
  (void)hab;

  auto f2 = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY, 62)
                .owned_by(1)
                .named("HabFact")
                .docked_to(61, 0)
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
      .in_planet_orbit(0, 0)
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
      .in_planet_orbit(0, 0)
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

}  // namespace

int main() {
  test_order_happy_path();
  test_order_combat_and_movement_options();
  test_order_specialty_ships();
  test_order_factory_activation_and_errors();
  std::println(std::cout, "\n✅ All order tests passed!");
  return 0;
}
