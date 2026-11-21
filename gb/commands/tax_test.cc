// SPDX-License-Identifier: Apache-2.0

/// \file tax_test.cc
/// \brief Unit tests for tax command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Test querying and setting planetary tax rate successfully
void test_tax_happy_paths() {
  TestContext ctx;
  const auto races = ctx.with_races();
  ctx.em.mutate_planet(1, 1, [&](Planet& p) {
    p.info(races.empire).tax = 10;
    p.info(races.empire).newtax = 10;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_planet(1, 1);

  // 1. Query current tax rate
  ctx.assert_dispatch_success(g, {"tax"});
  test::expect_contains(g.out.str(), "Current tax rate: 10%");
  test::expect_contains(g.out.str(), "Target: 10%");

  // 2. Set new tax rate to 25%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "25"});
  test::expect_contains(g.out.str(), "Set.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).newtax, 25);

  // 3. Set new tax rate to 100% (max)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "100"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).newtax, 100);

  // 4. Set new tax rate to 0% (min)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "0"});
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).newtax, 0);

  ctx.verify_universe_invariants();
}

// Test tax command role and scope rejections
void test_tax_role_and_scope_rejections() {
  TestContext ctx;
  const auto races = ctx.with_races();

  // Make nomad a guest race with its own government ship
  const auto guest_gov_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                .owned_by(races.nomad, 1)
                                .landed_on(2, 1, Coordinates{0, 0})
                                .with_crew(100, 0)
                                .build();
  ctx.em.mutate_race(races.nomad, [guest_gov_id](Race& r) {
    r.Guest = true;
    r.Gov_ship = guest_gov_id;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, races.nomad, 1);
  g.set_scope_planet(1, 1);
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Star control rejection (Governor 2 on star assigned to Governor 1)
  ctx.em.mutate_star(1, [&](Star& s) {
    s.set_governor(races.empire, 1);  // Star assigned to Governor 1
  });
  g.out.str("");
  ctx.setup_game_obj(g, races.empire, 2);  // Empire, Governor 2
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system.");

  // 3. Scope rejection (ScopeLevel::LEVEL_UNIV)
  g.out.str("");
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_univ();
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_tax_domain_errors() {
  TestContext ctx;
  const auto races = ctx.with_races();
  ctx.em.mutate_planet(1, 1, [&](Planet& p) {
    p.info(races.empire).tax = 10;
    p.info(races.empire).newtax = 10;
  });

  // Ensure Gov_ship is std::nullopt (no government center active)
  ctx.em.mutate_race(races.empire, [](Race& r) { r.Gov_ship = std::nullopt; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_planet(1, 1);

  // 1. Domain error: No government center active
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "You have no government center active.");

  // 2. Domain error: Illegal value (>100 or <0)
  ctx.em.mutate_race(races.empire,
                     [&](Race& r) { r.Gov_ship = races.empire_gov_ship; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "150"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).newtax, 10);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "-10"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(races.empire).newtax, 10);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "abc"});
  test::expect_contains(g.out.str(), "Illegal value.");

  // 3. JSON UI mode for status and set
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax"});
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("tax":10)");
  test::expect_contains(g.out.str(), R"("target_tax":10)");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "35"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"("message":"Set.\n")");
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("target_tax":35)");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_tax_happy_paths();
  test_tax_role_and_scope_rejections();
  test_tax_domain_errors();

  std::println(std::cout, "✓ tax_test passed!");
  return 0;
}
