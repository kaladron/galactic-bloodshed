// SPDX-License-Identifier: Apache-2.0

/// \file fire_test.cc
/// \brief Unit tests for fire and cew commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Attacker", 100.0, false, player_t{1})
      .add_race("Defender", 100.0, false, player_t{2})
      .add_star("CombatStar", 100, starnum_t{0});

  // Create attacker ship - armed with guns
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 0)
      .named("Battleship")
      .in_star_orbit(0, 100.0, 200.0)
      .with_guns(GTYPE_LIGHT, 10)
      .with_destruct(100)
      .with_crew(10, 10)
      .with_fuel(1000.0)
      .build();

  // Create target ship
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Target")
      .in_star_orbit(0, 110.0, 210.0)
      .with_armor(10)
      .with_crew(10, 0)
      .build();

  // Create CEW equipped ship
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 0)
      .named("CEWBattleship")
      .in_star_orbit(0, 100.0, 200.0)
      .with_cew(20, 1000)
      .with_crew(10, 0)
      .with_fuel(1000.0)
      .build();
}

void test_fire_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Execute fire command: Ship #1 attacks Ship #2 with strength 10
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "10"}, 1);

  // Verify ships still exist in database (persisted via EntityManager)
  const auto* ship1 = ctx.em.peek_ship(1);
  test::expect_true(ship1 != nullptr);
  test::expect_eq(ship1->number(), 1);
  test::expect_lt(ship1->destruct(), 100);

  const auto* ship2 = ctx.em.peek_ship(2);
  test::expect_true(ship2 != nullptr);
  test::expect_eq(ship2->number(), 2);

  // 2. Execute cew command: Ship #3 attacks Ship #2 with CEWs
  ctx.assert_dispatch_success(g, {"cew", "#3", "#2"}, 1);
  const auto* ship3 = ctx.em.peek_ship(3);
  test::expect_true(ship3 != nullptr);
  test::expect_lt(ship3->fuel(), 1000.0);

  ctx.verify_universe_invariants();
}

void test_fire_universe_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Move attacker ship to Universe scope and target at Star scope
  ctx.em.mutate_ship(
      1, [](Ship& s1) { s1.whatorbits() = ScopeLevel::LEVEL_UNIV; });
  ctx.em.mutate_ship(
      2, [](Ship& s2) { s2.whatorbits() = ScopeLevel::LEVEL_STAR; });

  // Set universe AP
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[0] = 50; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  bool ok = ctx.dispatch(g, {"fire", "#1", "#2", "10"});
  test::expect_true(ok);
  test::expect_eq(ctx.em.peek_universe()->AP[0], 49);

  ctx.verify_universe_invariants();
}

void test_fire_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_fire_role_and_guest_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  Race guest_race{};
  guest_race.Playernum = 3;
  guest_race.name = "GuestAttacker";
  guest_race.Guest = true;
  guest_race.governor[0].active = true;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest_race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Guest race rejection for fire
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Guest race rejection for cew
  ctx.assert_dispatch_rejected(g, {"cew", "#3", "#2"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_fire_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"fire", "#1"});
  test::expect_contains(g.out.str(),
                        "Syntax: fire <ship> <target> [<strength>]");

  // 2. Target self
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#1", "10"});
  test::expect_contains(g.out.str(), "Get real.");

  // 3. Inactive ship
  ctx.em.mutate_ship(1, [](Ship& s) { s.active() = false; });
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "inactive");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_fire_happy_paths();
  test_fire_universe_ap();
  test_fire_insufficient_ap();
  test_fire_role_and_guest_rejections();
  test_fire_domain_errors();

  std::println(std::cout, "✓ fire_test passed!");
  return 0;
}
