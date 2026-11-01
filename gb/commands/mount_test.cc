// SPDX-License-Identifier: Apache-2.0

/// \file mount_test.cc
/// \brief Unit tests for mount command and mount_ship_crystal mechanic.

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_mount_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();

  const auto ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                           .owned_by(1, 1)
                           .in_star_orbit(1)
                           .with_mount(1)
                           .with_crystals(2)
                           .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(ship_id);

  ctx.assert_dispatch_success(g, {"mount", std::format("#{}", ship_id)});

  const auto* final_ship = ctx.em.peek_ship(ship_id);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->mounted(), 1);
  test::expect_eq(final_ship->crystals(), 1);
  test::expect_contains(g.out.str(), "Mounted.");

  std::println(std::cout, "✓ mount persistence test passed");
}

void test_mount_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Ship 1: has mount, 0 crystals
  const auto ship1_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                            .owned_by(1, 1)
                            .in_star_orbit(1)
                            .with_mount(1)
                            .with_crystals(0)
                            .build();

  // Ship 2: no crystal mount
  const auto ship2_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                            .owned_by(1, 1)
                            .in_star_orbit(1)
                            .with_mount(0)
                            .with_crystals(2)
                            .build();

  // Ship 3: owned by player 2
  const auto ship3_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                            .owned_by(2, 1)
                            .in_star_orbit(1)
                            .with_mount(1)
                            .with_crystals(2)
                            .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"mount"});
  test::expect_contains(g.out.str(), "Syntax: mount <ship>");

  // 2. No crystals on board
  ctx.assert_dispatch_rejected(g, {"mount", std::format("#{}", ship1_id)});
  test::expect_contains(g.out.str(), "You have no crystals on board.");

  // 3. Ship not equipped with crystal mount
  ctx.assert_dispatch_rejected(g, {"mount", std::format("#{}", ship2_id)});
  test::expect_contains(g.out.str(),
                        "This ship is not equipped with a crystal mount.");

  // 4. Already mounted
  ctx.em.mutate_ship(ship1_id, [](Ship& s) {
    s.mounted() = 1;
    s.add_crystals(1);
  });
  ctx.assert_dispatch_rejected(g, {"mount", std::format("#{}", ship1_id)});
  test::expect_contains(g.out.str(), "You already have a crystal mounted.");

  // 5. Explicit foreign ship rejected
  ctx.assert_dispatch_rejected(g, {"mount", std::format("#{}", ship3_id)});
  test::expect_contains(g.out.str(),
                        std::format("You don't own ship #{}.", ship3_id));

  std::println(std::cout, "✓ mount error cases passed");
}

void test_mount_json_mode() {
  TestContext ctx;
  ctx.with_standard_universe();

  const auto ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                           .owned_by(1, 1)
                           .in_star_orbit(1)
                           .with_mount(1)
                           .with_crystals(2)
                           .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(ship_id);
  g.set_ui_mode(GB::presentation::UiMode::JSON);

  ctx.assert_dispatch_success(g, {"mount", std::format("#{}", ship_id)});

  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), "Mounted.");
  test::expect_contains(g.out.str(), R"("type":"ship")");

  std::println(std::cout, "✓ mount JSON mode test passed");
}

}  // namespace

int main() {
  test_mount_persistence();
  test_mount_errors();
  test_mount_json_mode();

  std::println(std::cout, "\n✅ All mount tests passed!");
  return 0;
}
