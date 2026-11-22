// SPDX-License-Identifier: Apache-2.0

/// \file tech_status_test.cc
/// \brief Unit tests for status (tech_status) command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

constexpr player_t p1 = 2;

void setup_test_world(TestContext& ctx) {
  ctx.with_galaxy();
  TestWorldBuilder(ctx).add_race("Federation", 100.0, false, p1);
  ctx.with_populated_planet(1, 1, p1, 1000);

  JsonStore store(ctx.db);
  PowerRepository(store).save(Power{{
      .id = p1,
      .popn = 1000,
  }});

  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(p1);
    s.mark_inhabited_by(p1);
  });
  ctx.em.mutate_star(2, [](Star& s) { s.mark_explored_by(p1); });
  ctx.em.mutate_star(3, [](Star& s) { s.mark_explored_by(p1); });

  ctx.em.mutate_planet(1, 1, [](Planet& pl) {
    pl.info(p1).explored = true;
    pl.info(p1).autorep = 1;
    pl.info(p1).tech_invest = 50;
    pl.info(p1).prod_res = 100;
  });
  ctx.em.mutate_planet(2, 1, [](Planet& pl) { pl.info(p1).explored = true; });
  ctx.em.mutate_planet(3, 1, [](Planet& pl) { pl.info(p1).explored = false; });

  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(p1)
      .in_deep_space()
      .build();
}

void test_status_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_univ();

  // 1. Happy path: status without arguments (covers explored colony on Sol,
  //    uncolonized explored planet on Vega, and unexplored planet on Antares)
  ctx.assert_dispatch_success(g, {"status"});
  std::string output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth*");
  test::expect_contains(output, "1000");  // Population
  test::expect_contains(output, "50");    // Tech invest
  std::println(std::cout, "    ✓ status global colony report succeeded");

  // 2. Happy path: status with star location argument & non-controlling gov
  g.out.str("");
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  output = g.out.str();
  test::expect_contains(output, "Technology Report");
  test::expect_contains(output, "Sol/Earth*");
  ctx.em.mutate_race(p1, [](Race& r) { r.appoint_governor(2); });
  g.set_governor(governor_t{2});
  ctx.assert_dispatch_success(g, {"status", "/Sol"});
  test::expect_false(g.out.str().contains("Sol/Earth*"));
  g.set_governor(governor_t{1});
  std::println(std::cout, "    ✓ status with /Sol filter succeeded");

  // 3. Unexplored star, invalid/universe/ship locations, missing power, & JSON
  ctx.em.mutate_star(3, [](Star& s) { s.clear_explored_by(p1); });
  ctx.assert_dispatch_success(g, {"status"});
  ctx.assert_dispatch_success(g,
                              {"status", "/Antares", "/NoSuchStar", "/", "#1"});
  test::expect_contains(g.out.str(), "Bad location `/NoSuchStar`.");
  test::expect_contains(g.out.str(), "Bad location `/`.");
  test::expect_contains(g.out.str(), "Bad location `#1`.");

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"status", "/Sol", "/NoSuchStar"});
  test::expect_contains(g.out.str(), R"("type":"error")");
  test::expect_contains(g.out.str(), R"("code":"bad_location")");
  test::expect_contains(g.out.str(), R"("type":"planet")");
  test::expect_contains(g.out.str(), R"("name":"Earth")");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_status_dispatch();

  std::println(std::cout, "All status tests passed!");
  return 0;
}
