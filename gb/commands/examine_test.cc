// SPDX-License-Identifier: Apache-2.0

/// \file examine_test.cc
/// \brief Unit tests for examine command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1)
      .in_star_orbit(starnum_t{0})
      .build();

  // Seed / set a custom ShipExam description in SQLite
  ctx.em.mutate_ship_exam(ShipType::STYPE_SHUTTLE, [&](ShipExam& exam) {
    exam.description =
        "Shuttle: SQLite stored short-range spacecraft description.";
  });
}

void test_examine_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check: examine without arguments
  ctx.assert_dispatch_rejected(g, {"examine"});
  test::expect_contains(g.out.str(), "Syntax: examine <#shipnum>");
  std::println(std::cout, "    ✓ examine rejected with insufficient arguments");

  // 2. Happy path: examine #1
  g.out.str("");
  ctx.assert_dispatch_success(g, {"examine", "#1"});
  test::expect_contains(
      g.out.str(),
      "Shuttle: SQLite stored short-range spacecraft description.");
  std::println(std::cout, "    ✓ examine #1 succeeded with description");

  // 3. Domain error: non-existent ship
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"examine", "#999"});
  test::expect_contains(g.out.str(), "Ship not found.");
  std::println(std::cout, "    ✓ examine rejected non-existent ship");
}

}  // namespace

int main() {
  test_examine_dispatch();

  std::println(std::cout, "All examine tests passed!");
  return 0;
}
