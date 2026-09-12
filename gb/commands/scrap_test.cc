// SPDX-License-Identifier: Apache-2.0

/// \file scrap_test.cc
/// \brief Unit tests for scrap command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe().with_populated_planet(0, 0, 1, 1000,
                                                     Coordinates{5, 5});

  auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                        .owned_by(player_t{1}, governor_t{0})
                        .named("Carrier")
                        .in_star_orbit(0)
                        .with_crew(10, 0)
                        .with_fuel(100.0)
                        .with_resource(100)
                        .build();

  auto fighter_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                        .owned_by(player_t{1}, governor_t{0})
                        .named("ToScrap")
                        .in_star_orbit(0)
                        .with_crew(5, 0)
                        .with_fuel(50.0)
                        .with_resource(20)
                        .with_destruct(10)
                        .build();

  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& s) { s.dock_with_ship(fighter_id); });
  ctx.em.mutate_ship(fighter_id, [&](Ship& s) {
    s.dock_with_ship(carrier_id);
    s.build_cost() = 100;
  });
}

void test_scrap_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Scrap docked fighter (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"scrap", "#2"}, 1);

  ctx.em.clear_cache();
  const auto* scrapped = ctx.em.peek_ship(2);
  test::expect_ne(scrapped, nullptr);
  test::expect_eq(scrapped->alive(), 0);

  const auto* carrier_after = ctx.em.peek_ship(1);
  test::expect_ne(carrier_after, nullptr);
  test::expect_gt(carrier_after->resource(), 100);
  test::expect_eq(carrier_after->docked(), 0);

  ctx.verify_universe_invariants();
}

void test_scrap_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "action points");
}

void test_scrap_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"scrap"});
  test::expect_contains(g.out.str(), "Syntax: scrap <ship>");

  // 2. Uncrewed ship rejection
  ctx.em.mutate_ship(2, [](Ship& s) { s.popn() = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"scrap", "#2"});
  test::expect_contains(g.out.str(), "no crew");
}

void test_scrap_toxic_waste_warning() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Create Toxic Waste Canister landed on planet
  auto tox_id = TestShipBuilder(ctx.em, ShipType::OTYPE_TOXWC)
                    .owned_by(player_t{1}, governor_t{0})
                    .named("HazMat")
                    .landed_on(0, 0, Coordinates{5, 5})
                    .with_crew(1, 0)
                    .with_special(WasteData{.toxic = 25})
                    .build();

  ctx.assert_dispatch_success(g, {"scrap", std::format("#{}", tox_id.value)},
                              1);
  test::expect_contains(g.out.str(),
                        "WARNING: This will release 25 toxin points");

  ctx.em.clear_cache();
  const auto* scrapped = ctx.em.peek_ship(tox_id);
  test::expect_eq(scrapped->alive(), 0);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_scrap_happy_paths();
  test_scrap_toxic_waste_warning();
  test_scrap_insufficient_ap();
  test_scrap_domain_errors();

  std::println(std::cout, "✓ scrap_test passed!");
  return 0;
}
