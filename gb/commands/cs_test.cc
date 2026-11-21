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

player_t setup_test_world(TestContext& ctx) {
  const auto races = ctx.with_races();
  ctx.em.mutate_race(races.empire,
                     [](Race& r) { r.leader().set_default_scope_star(1); });
  return races.empire;
}

void test_cs_happy_paths() {
  TestContext ctx;
  const player_t p = setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p, 1);

  // 1. Switch to universe scope (free AP)
  g.set_scope_star(1);
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

  // 5. Change default system with -d to universe (/ sets defsystem=0,
  // defplanetnum=0), then run default cs (keeps snum=0, pnum=0 at LEVEL_UNIV)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"cs", "-d", "/"}, 0);
  test::expect_eq(g.out.str(), std::string_view{"New home system is /\n"});

  g.out.str("");
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.out.str(), std::string_view{""});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_UNIV);
  test::expect_eq(g.snum(), 0);
  test::expect_eq(g.pnum(), 0);

  // 6. Default cs to star and planet scopes updates universe_center and zeroes
  // child coordinates
  ctx.assert_dispatch_success(g, {"cs", "-d", "/Vega/Vega Prime"}, 0);
  ctx.setup_game_obj(g, p, 1);
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.snum(), 2);
  test::expect_eq(g.pnum(), 1);

  ctx.create_star("VoidStar", 4).with_explored(p).build();
  ctx.assert_dispatch_success(g, {"cs", "-d", "/VoidStar"}, 0);
  ctx.setup_game_obj(g, p, 1);
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.snum(), 4);
  test::expect_eq(g.pnum(), 0);
}

void test_cs_domain_errors() {
  // 0. Empty universe (numstars == 0): Deity at LEVEL_UNIV succeeds on default
  // cs, while corrupt out-of-bounds defsystem fails fast via
  // EntityNotFoundError
  {
    TestContext empty_ctx;
    Race r{};
    r.Playernum = 1;
    r.God = true;
    r.name = "Deity";
    r.init_deity_leader("dpass");
    empty_ctx.em.create_race(r);
    auto& empty_registry = get_test_session_registry();
    GameObj empty_g(empty_ctx.em, empty_registry);
    empty_ctx.setup_game_obj(empty_g, 1, 1);
    empty_g.set_god(true);
    empty_ctx.assert_dispatch_success(empty_g, {"cs"});
    test::expect_eq(empty_g.level(), ScopeLevel::LEVEL_UNIV);

    empty_ctx.em.mutate_race(
        1, [](Race& race) { race.leader().set_default_scope_star(99); });
    empty_ctx.setup_game_obj(empty_g, 1, 1);
    test::expect_throws<EntityNotFoundError>(
        [&]() { empty_ctx.dispatch(empty_g, {"cs"}); });
  }

  TestContext ctx;
  const player_t p = setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p, 1);
  g.set_scope_univ();

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
      TestShipBuilder(ctx.em, ShipType::STYPE_POD).owned_by(p, 1).build();
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-d", std::format("#{}", snum)});
  test::expect_contains(g.out.str(), "cs: bad home system");

  // 4. Invalid 3-argument invocation without -d and >3 arguments
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-x", "/Alpha"});
  test::expect_contains(g.out.str(), "cs: bad scope");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cs", "-d", "/", "extra"});
  test::expect_contains(g.out.str(), "cs: bad scope");
}

void test_cs_viewport_coordinates() {
  TestContext ctx;
  const auto races = ctx.with_races();
  const player_t p = races.empire;

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p, 1);

  // 1. Planet to star and universe viewport coordinates
  g.set_scope_planet(1, 1);

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

  // Switch back to planet (from LEVEL_UNIV resets system_center to 0)
  g.set_system_center({12.0, 34.0});
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // cs directly from planet to universe: universe_center should match Earth's
  // absolute coords
  ctx.assert_dispatch_success(g, {"cs", "/"});
  test::expect_eq(g.universe_center(), earth->absolute_coordinates(*sol));

  // 2. Ship orbiting star viewport coordinates (and star-orbiting -> planet
  // mismatch)
  shipnum_t ship_star = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                            .owned_by(p, 1)
                            .in_star_orbit(1, UniverseCoordinates{15.0, 25.0})
                            .build();

  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_SHIP);

  // cs from star-orbiting ship to planet resets system_center to 0
  g.set_system_center({77.0, 88.0});
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // cs from ship to star
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), SystemCoordinates(15.0, 25.0));

  // cs back to ship, then to universe
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  ctx.assert_dispatch_success(g, {"cs", "/"});
  test::expect_eq(g.universe_center(), UniverseCoordinates(15.0, 25.0));

  // Deep-space ship (LEVEL_UNIV) navigating to star resets system_center to 0
  shipnum_t ship_deep = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                            .owned_by(p, 1)
                            .in_deep_space(UniverseCoordinates{40.0, 60.0})
                            .build();
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_deep.value)});
  g.set_system_center({50.0, 50.0});
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 3. Ship orbiting planet viewport coordinates (same planet, different planet
  // in same star, and planet in different star)
  shipnum_t ship_plan = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(p, 1)
                            .in_planet_orbit(1, 1,
                                             earth->absolute_coordinates(*sol) +
                                                 SystemCoordinates{2.0, 3.0})
                            .build();

  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan.value)});
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  test::expect_eq(g.system_center(), SystemCoordinates(2.0, 3.0));

  ctx.em.mutate_planet(1, 2, [&](Planet& pl) { pl.mark_explored_by(p); });
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan)});
  g.set_system_center({50.0, 50.0});
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Mars"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan)});
  g.set_system_center({50.0, 50.0});
  ctx.assert_dispatch_success(g, {"cs", "/Vega/Vega Prime"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 4. Docked ship resets system_center to 0
  ctx.em.mutate_ship(ship_plan, [&](Ship& s) { s.dock_with_ship(ship_star); });
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_plan.value)});
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 5. Navigating away from a destroyed/deleted ship scope does not crash
  // (regression test for uncaught EntityNotFoundError in
  // update_viewport_from_ship)
  const auto doomed_ship = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                               .owned_by(p, 1)
                               .in_star_orbit(1)
                               .build();
  g.set_scope_ship(doomed_ship);
  ctx.em.mutate_ship(doomed_ship, [&](Ship& s) { ctx.em.destroy_ship(s); });
  g.set_system_center({99.0, 99.0});
  ctx.assert_dispatch_success(g, {"cs", "/Sol"});
  test::expect_eq(g.level(), ScopeLevel::LEVEL_STAR);
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 6. Navigating from planet to a different star or from star to planet resets
  // system_center to 0
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"});
  g.set_system_center({50.0, 50.0});
  ctx.assert_dispatch_success(g, {"cs", "/Vega"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 7. Navigating from ship orbiting star 1 to a different star (/Vega) resets
  // system_center to 0
  ctx.assert_dispatch_success(g, {"cs", std::format("#{}", ship_star.value)});
  g.set_system_center({50.0, 50.0});
  ctx.assert_dispatch_success(g, {"cs", "/Vega"});
  test::expect_eq(g.system_center(), SystemCoordinates(0.0, 0.0));

  // 8. TestCommandMatrix runner
  TestCommandMatrix(ctx, "cs")
      .with_valid_argv({"cs", "/"})
      .with_invalid_argv({"cs", "NonExistentStar"})
      .run_matrix(g);
}

void test_cs_json_mode() {
  TestContext ctx;
  const player_t p = setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p, 1);
  g.set_ui_mode(UiMode::JSON);

  g.out.str("");
  ctx.assert_dispatch_success(g, {"cs", "/Sol/Earth"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.snum(), 1);
  test::expect_eq(g.pnum(), 1);
  test::expect_eq(g.out.str(), "");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"cs", "-d", "/Vega/Vega Prime"}, 0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"(New home system is /Vega/Vega Prime)");
  test::expect_contains(g.out.str(), R"("type":"race")");
  test::expect_contains(g.out.str(), R"("default_star":2)");
  test::expect_contains(g.out.str(), R"("default_planet":1)");
  test::expect_not_contains(g.out.str(), R"("type":"session")");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"cs"}, 0);
  test::expect_eq(g.level(), ScopeLevel::LEVEL_PLAN);
  test::expect_eq(g.snum(), 2);
  test::expect_eq(g.pnum(), 1);
  test::expect_eq(g.out.str(), "");
}

}  // namespace

int main() {
  test_cs_happy_paths();
  test_cs_domain_errors();
  test_cs_viewport_coordinates();
  test_cs_json_mode();

  std::println(std::cout, "✓ cs_test passed!");
  return 0;
}
