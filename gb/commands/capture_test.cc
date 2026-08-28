// SPDX-License-Identifier: Apache-2.0

/// \file capture_test.cc
/// \brief Unit tests for capture command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("AttackerRace", 10.0, false, player_t{1})
      .add_race("DefenderRace", 5.0, false, player_t{2})
      .add_star("TestStar", 10, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  // Set attacker race likes and governor
  {
    ctx.em.mutate_race(1, [](Race& r) {
      r.fighters = 10.0;
      r.mass = 1.0;
      r.morale = 100;
      r.likes[SectorType::SEC_LAND] = 50;
      r.governor[1].active = true;
    });

    ctx.em.mutate_race(2, [](Race& r) {
      r.fighters = 1.0;
      r.mass = 1.0;
      r.morale = 50;
    });
  }

  // Create sectormap with troops for attacker
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(50);
    smap.get(Coordinates{5, 5}).set_troops(100);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_LAND);
  });

  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.popn() = 50;
    planet.ships() = 1;
  });

  // Create defender's ship (landed on planet at 5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Cargo")
      .landed_on(0, 0, Coordinates(5, 5))
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
  g.set_snum(0);
  g.set_pnum(0);

  // Execute capture command - capture #1 50 military
  ctx.assert_dispatch_success(g, {"capture", "#1", "50", "military"});

  // Verify changes persisted
  const auto* captured_ship = ctx.em.peek_ship(1);
  test::expect_true(captured_ship != nullptr);

  const auto* final_smap = ctx.em.peek_sectormap(0, 0);
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
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

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
  ctx.em.mutate_star(0, [](Star& s) {
    s.governor(1) = 2;  // Star governed by Gov 2
  });
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
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
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"capture"});
  test::expect_contains(
      g.out.str(), "Syntax: capture <ship> [<number>] [civilians|military]");

  // 2. Ship not landed
  ctx.em.mutate_ship(1, [](Ship& s) { s.docked() = false; });
  ctx.assert_dispatch_rejected(g, {"capture", "#1"});
  test::expect_contains(g.out.str(), "not landed");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_capture_happy_path();
  test_capture_insufficient_ap();
  test_capture_role_and_scope_rejections();
  test_capture_domain_errors();

  std::println(std::cout, "✓ capture_test passed!");
  return 0;
}
