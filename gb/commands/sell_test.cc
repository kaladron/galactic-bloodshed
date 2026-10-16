// SPDX-License-Identifier: Apache-2.0

/// \file sell_test.cc
/// \brief Unit tests for sell command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

shipnum_t setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.appoint_governor(2); });
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{1}).explored = true;
    p.info(player_t{1}).numsectsowned = 5;
    p.info(player_t{1}).resource = 1000;
    p.info(player_t{1}).fuel = 500;
    p.info(player_t{1}).destruct = 200;
    p.info(player_t{1}).crystals = 50;
  });

  return TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
      .owned_by(1, 1)
      .in_planet_orbit(1, 1)
      .build();
}

void test_sell_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Sell resources (min(20, 100) = 20 AP deducted)
  ctx.assert_dispatch_success(g, {"sell", "r", "100"}, 20);
  test::expect_contains(g.out.str(), "Lot #1 - 100 units of resources.");
  const auto* p1 = ctx.em.peek_planet(1, 1);
  test::expect_eq(p1->info(player_t{1}).resource, 900);
  const auto* lot1 = ctx.em.peek_commod(1);
  test::expect_ne(lot1, nullptr);
  test::expect_eq(lot1->star_from(), starnum_t{1});
  test::expect_eq(lot1->planet_from(), planetnum_t{1});
  test::expect_eq(lot1->star_to(), std::nullopt);
  test::expect_eq(lot1->planet_to(), std::nullopt);

  // 2. Sell fuel (min(20, 50) = 20 AP)
  ctx.assert_dispatch_success(g, {"sell", "f", "50"}, 20);
  const auto* p2 = ctx.em.peek_planet(1, 1);
  test::expect_eq(p2->info(player_t{1}).fuel, 450);

  // 3. Sell destruct (min(20, 25) = 20 AP)
  ctx.assert_dispatch_success(g, {"sell", "d", "25"}, 20);
  const auto* p3 = ctx.em.peek_planet(1, 1);
  test::expect_eq(p3->info(player_t{1}).destruct, 175);

  // 4. Sell crystals in JSON mode (min(20, 10) = 10 AP)
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"sell", "x", "10"}, 10);
  test::expect_contains(g.out.str(), "\"type\":\"sell\"");
  g.set_ui_mode(UiMode::ASCII);
  const auto* p4 = ctx.em.peek_planet(1, 1);
  test::expect_eq(p4->info(player_t{1}).crystals, 40);
}

void test_sell_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 5 (< 20 required for 100 units)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  test::expect_contains(g.out.str(), "action points");
}

void test_sell_role_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // 1. Guest race rejection
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 2, 1);  // Player 2 is guest
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(1);

    ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
    test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  }

  // 2. Star control rejection (Governor 2 when star is assigned to Governor 1)
  {
    auto& registry = get_test_session_registry();
    GameObj g(ctx.em, registry);
    ctx.setup_game_obj(g, 1, 2);  // Governor 2
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.set_snum(1);
    g.set_pnum(1);

    ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
    test::expect_contains(g.out.str(),
                          "You are not authorized to do that in this system.");
  }
}

void test_sell_domain_errors() {
  TestContext ctx;
  const shipnum_t port_ship = setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"sell", "r"});
  test::expect_contains(g.out.str(), "Syntax: sell <r|d|f|x> <amount>");

  // 2. Non-positive or non-numeric amount
  ctx.assert_dispatch_rejected(g, {"sell", "r", "0"});
  test::expect_contains(g.out.str(), "Try using positive values.");
  ctx.assert_dispatch_rejected(g, {"sell", "r", "-10"});
  test::expect_contains(g.out.str(), "Try using positive values.");
  ctx.assert_dispatch_rejected(g, {"sell", "r", "abc"});
  test::expect_contains(g.out.str(), "Try using positive values.");

  // 3. Invalid or empty commodity
  ctx.assert_dispatch_rejected(g, {"sell", "z", "100"});
  test::expect_contains(g.out.str(),
                        "Permitted commodities are r, d, f, and x");
  ctx.assert_dispatch_rejected(g, {"sell", "", "100"});
  test::expect_contains(g.out.str(),
                        "Permitted commodities are r, d, f, and x");

  // 4. Enslaved planet
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = player_t{2}; });
  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  test::expect_contains(g.out.str(), "This planet is enslaved to player 2.");
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = std::nullopt; });

  // 5. Starport conditions (damaged, non-starport, alien-owned, dead)
  TestShipBuilder(ctx.em, ShipType::STYPE_POD)
      .owned_by(1, 1)
      .in_planet_orbit(1, 1)
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1)
      .build();
  ctx.em.mutate_ship(port_ship, [](Ship& s) { s.admin_override_damage(50); });
  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  test::expect_contains(
      g.out.str(),
      "You don't have an undamaged space port or government center here.");
  ctx.em.mutate_ship(port_ship, [](Ship& s) { s.admin_override_damage(0); });

  // 6. Zero stockpile for r, d, f, x
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{1}).resource = 0;
    p.info(player_t{1}).destruct = 0;
    p.info(player_t{1}).fuel = 0;
    p.info(player_t{1}).crystals = 0;
  });
  ctx.assert_dispatch_rejected(g, {"sell", "r", "10"});
  test::expect_contains(g.out.str(),
                        "You don't have any resources here to sell!");
  ctx.assert_dispatch_rejected(g, {"sell", "d", "10"});
  test::expect_contains(g.out.str(),
                        "You don't have any destruct here to sell!");
  ctx.assert_dispatch_rejected(g, {"sell", "f", "10"});
  test::expect_contains(g.out.str(), "You don't have any fuel here to sell!");
  ctx.assert_dispatch_rejected(g, {"sell", "x", "10"});
  test::expect_contains(g.out.str(),
                        "You don't have any crystals here to sell!");

  // 7. Invalid scope (universe level)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"sell", "r", "100"});
  test::expect_contains(g.out.str(), "Invalid scope");
}

}  // namespace

int main() {
  test_sell_happy_paths();
  test_sell_insufficient_ap();
  test_sell_role_rejections();
  test_sell_domain_errors();

  std::println(std::cout, "✓ sell_test passed!");
  return 0;
}
