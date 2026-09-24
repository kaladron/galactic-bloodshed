// SPDX-License-Identifier: Apache-2.0

/// \file cs_test.cc
/// \brief Unit tests for cs command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().deflevel = ScopeLevel::LEVEL_STAR;
    r.leader().defsystem = 1;
    r.leader().defplanetnum = 1;
  });
}

void test_cs_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Switch to universe scope (free AP)
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.assert_dispatch_success(g, {"cs", "/"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_UNIV);

  // 2. Switch to star Vega by name
  ctx.assert_dispatch_success(g, {"cs", "Vega"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.snum(), 2);

  // 3. Switch to planet Earth via full path
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.snum(), 1);
  test::expect_eq(g.pnum(), 1);

  // 4. Default cs without arguments
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.snum(), 1);

  // 5. Change default system with -d
  ctx.assert_dispatch_success(g, {"cs", "-d", "/"}, 0);
  test::expect_contains(g.out.str(), "New home system");

  // 6. Default cs clamps out-of-bounds defsystem/defplanetnum to last star and
  // planet
  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().deflevel = ScopeLevel::LEVEL_PLAN;
    r.leader().defsystem = 99;
    r.leader().defplanetnum = 5;
  });
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.snum(), 3);
  test::expect_eq(g.pnum(), 1);
}

void test_cs_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Invalid star name
  ctx.assert_dispatch_rejected(g, {"cs", "NonExistentStar"});
  test::expect_contains(g.out.str(), "cs: bad scope");
  test::expect_eq(g.level(), ScopeLevel::LEVEL_UNIV);

  // 2. Invalid home system format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-d", "NonExistentStar"});
  test::expect_contains(g.out.str(), "cs: bad home system");

  // 3. Ship scope rejected as default home system
  shipnum_t snum =
      TestShipBuilder(ctx.em, ShipType::STYPE_POD).owned_by(1, 1).build();
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-d", std::format("#{}", snum)});
  test::expect_contains(g.out.str(), "cs: bad home system");

  // 4. Invalid 3-argument invocation without -d
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-x", "/Alpha"});
  test::expect_contains(g.out.str(), "cs: bad scope");
}

void test_cs_viewport_coordinates() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Planet to star and universe viewport coordinates
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  const auto* earth = ctx.em.peek_planet(1, 1);
  const auto* sol = ctx.em.peek_star(1);
  test::expect_ne(earth, nullptr);
  test::expect_ne(sol, nullptr);

  // cs to Sol: system_center should match Earth's system coordinates
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), earth->system_coordinates());

  // cs from star to universe: universe_center should match Sol's coordinates
  ctx.assert_dispatch_success(g, {"cs", "/"});
  test::expect_eq(g.universe_center(), sol->coordinates());

  // Switch back to planet
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);

  // cs directly from planet to universe: universe_center should match Earth's
  // absolute coords
  ctx.assert_dispatch_success(g, {"cs", "/"});
  test::expect_eq(g.universe_center(), earth->absolute_coordinates(*sol));

  // 2. Ship orbiting star viewport coordinates
  shipnum_t ship_star = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                            .owned_by(1, 1)
                            .in_star_orbit(1, UniverseCoordinates{15.0, 25.0})
                            .build();

  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_SHIP);

  // cs from ship to star
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), SystemCoordinates(15.0, 25.0));

  // cs back to ship, then to universe
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  ctx.assert_dispatch_success(g, {"cs", "/"});
  test::expect_eq(g.universe_center(), UniverseCoordinates(15.0, 25.0));

  // 3. Ship orbiting planet viewport coordinates
  shipnum_t ship_plan = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(1, 1)
                            .in_planet_orbit(1, 1,
                                             earth->absolute_coordinates(*sol) +
                                                 SystemCoordinates{2.0, 3.0})
                            .build();

  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan.value)});
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  test::expect_eq(g.system_center(), SystemCoordinates(2.0, 3.0));

  // 4. Docked ship resets system_center to 0
  ctx.em.mutate_ship(ship_plan, [&](Ship& s) { s.dock_with_ship(ship_star); });
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan.value)});
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 5. TestCommandMatrix runner
  TestCommandMatrix(ctx, "cs")
      .with_valid_argv({"cs", "/"})
      .with_invalid_argv({"cs", "NonExistentStar"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_cs_happy_paths();
  test_cs_domain_errors();
  test_cs_viewport_coordinates();

  std::println(std::cout, "✓ cs_test passed!");
  return 0;
}
