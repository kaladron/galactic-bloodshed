// SPDX-License-Identifier: Apache-2.0

/// \file capture_test.cc
/// \brief Unit tests for capture command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Set attacker race likes and governor
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 10.0;
    r.mass = 1.0;
    r.morale = 100;
    r.likes[SectorType::SEC_LAND] = 50;
    r.appoint_governor(1);
  });

  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1.0;
    r.mass = 1.0;
    r.morale = 50;
  });

  // Create sectormap with troops for attacker
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_owner(1);
    sect.set_popn_exact(50);
    sect.set_troops(100);
    sect.set_condition(SectorType::SEC_LAND);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned += 1;
    planet.popn() += 50;
    planet.troops() += 100;
  });

  // Create defender's ship (landed on planet at 5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Cargo")
      .landed_on(1, 1, Coordinates(5, 5))
      .with_crew(10, 5)
      .with_fuel(100.0)
      .build();
}

void test_capture_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Execute capture command - capture #1 50 military
  ctx.assert_dispatch_success(g, {"capture", "#1", "50", "military"});

  // Verify changes persisted
  const auto* captured_ship = ctx.em.peek_ship(1);
  test::expect_true(captured_ship != nullptr);

  const auto* final_smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(final_smap != nullptr);
  const auto& final_sector = final_smap->get(Coordinates{5, 5});
  test::expect_le(final_sector.get_troops(), 100);

  if (captured_ship->alive()) {
    test::expect_true(captured_ship->owner() == 1 ||
                      captured_ship->owner() == 2);
  }

  ctx.verify_universe_invariants();
}

void test_capture_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"capture", "#1", "50", "military"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_capture_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection
  ctx.em.mutate_star(1, [](Star& s) {
    s.governor(1) = 2;  // Star governed by Gov 2
  });
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  test::expect_contains(g.out.str(), "not authorized");

  ctx.verify_universe_invariants();
}

void test_capture_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"capture"});
  test::expect_contains(
      g.out.str(), "Syntax: capture <ship> [<number>] [civilians|military]");

  // 2. Enslaved planet check
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.enslave_to(2); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  test::expect_contains(g.out.str(), "enslaved");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.free_slaves(); });

  // 3. Ship not landed
  ctx.em.mutate_ship(1, [](Ship& s) { s.launch_to_orbit(); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  test::expect_contains(g.out.str(), "not landed");
  ctx.em.mutate_ship(1, [](Ship& s) { s.land_on_planet(); });

  // 4. Von Neumann machine rejection
  shipnum_t vn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_VN)
                        .owned_by(2, 0)
                        .landed_on(1, 1, Coordinates(5, 5))
                        .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", std::format("#{}", vn_id.value)});
  test::expect_contains(g.out.str(), "Von Neumann");

  // 5. Unowned landing sector
  ctx.em.mutate_sectormap(
      1, 1, [](SectorMap& smap) { smap.get(Coordinates{2, 2}).set_owner(0); });
  shipnum_t unowned_sect_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                    .owned_by(2, 0)
                                    .landed_on(1, 1, Coordinates(2, 2))
                                    .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"capture", std::format("#{}", unowned_sect_ship.value)});
  test::expect_contains(g.out.str(), "don't own the sector");

  // 6. Invalid population type
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", "#1", "10", "lasers"});
  test::expect_contains(g.out.str(), "Capture with what?");

  // 7. Illegal number of boarders (0)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capture", "#1", "0"});
  test::expect_contains(g.out.str(), "Illegal number of boarders");

  ctx.verify_universe_invariants();
}

void test_capture_civilian_victory() {
  TestContext ctx;
  setup_test_world(ctx);

  // Setup empty enemy cargo ship with no crew
  shipnum_t target_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 0)
                              .landed_on(1, 1, Coordinates(5, 5))
                              .with_crew(0, 0)
                              .with_destruct(0)
                              .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Capture with civilians
  ctx.assert_dispatch_success(
      g, {"capture", std::format("#{}", target_ship.value), "20", "civilians"});
  test::expect_contains(g.out.str(), "VICTORY! The ship is yours!");

  ctx.em.clear_cache();
  const auto* ship = ctx.em.peek_ship(target_ship);
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->owner(), 1);
  test::expect_eq(ship->popn(), 20);

  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_eq(smap->get(Coordinates{5, 5}).get_popn(), 30);

  ctx.verify_universe_invariants();
}

void test_capture_default_boarders_and_allied_ship() {
  TestContext ctx;
  setup_test_world(ctx);

  // Mark race 1 allied with race 2
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(player_t{2}); });

  shipnum_t target_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 0)
                              .landed_on(1, 1, Coordinates(5, 5))
                              .with_crew(0, 0)
                              .with_destruct(0)
                              .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Capture without specifying count or type (defaults to all available civs)
  ctx.assert_dispatch_success(
      g, {"capture", std::format("#{}", target_ship.value)});
  test::expect_contains(g.out.str(), "VICTORY! The ship is yours!");

  ctx.em.clear_cache();
  const auto* ship = ctx.em.peek_ship(target_ship);
  test::expect_eq(ship->owner(), 1);

  ctx.verify_universe_invariants();
}

void test_capture_booby_trap_robot_ship() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create booby-trapped robot cargo ship (destruct > 0, crew == 0)
  shipnum_t target_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 0)
                              .landed_on(1, 1, Coordinates(5, 5))
                              .with_crew(0, 0)
                              .with_destruct(5)
                              .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(
      g, {"capture", std::format("#{}", target_ship.value), "10", "military"});

  ctx.verify_universe_invariants();
}

void test_capture_boarders_wiped_and_ship_destroyed() {
  TestContext ctx;
  setup_test_world(ctx);

  // Strong defender ship
  shipnum_t dreadnought = TestShipBuilder(ctx.em, ShipType::STYPE_DREADNT)
                              .owned_by(2, 0)
                              .landed_on(1, 1, Coordinates(5, 5))
                              .with_crew(500, 200)
                              .with_armor(100)
                              .build();

  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 100.0;
    r.tech = 200.0;
    r.morale = 200;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Send 1 civilian against dreadnought -> wiped out
  ctx.assert_dispatch_success(
      g, {"capture", std::format("#{}", dreadnought.value), "1", "civilians"});
  test::expect_contains(g.out.str(), "killed your party to the last man");

  ctx.verify_universe_invariants();
}

void test_capture_command_matrix() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  TestCommandMatrix(ctx, "capture")
      .with_valid_argv({"capture", "#1", "5", "military"})
      .with_invalid_argv({"capture"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_capture_happy_path();
  test_capture_insufficient_ap();
  test_capture_role_and_scope_rejections();
  test_capture_domain_errors();
  test_capture_civilian_victory();
  test_capture_default_boarders_and_allied_ship();
  test_capture_booby_trap_robot_ship();
  test_capture_boarders_wiped_and_ship_destroyed();
  test_capture_command_matrix();

  std::println(std::cout, "✓ capture_test passed!");
  return 0;
}
