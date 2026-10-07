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

  TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 1)
      .owned_by(1, 1)
      .in_star_orbit(1)
      .with_mount(1)
      .with_crystals(2)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);

  ctx.assert_dispatch_success(g, {"mount", "#1"});

  const auto* final_ship = ctx.em.peek_ship(1);
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
  TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 1)
      .owned_by(1, 1)
      .in_star_orbit(1)
      .with_mount(1)
      .with_crystals(0)
      .build();

  // Ship 2: no crystal mount
  TestShipBuilder(ctx.em, ShipType::STYPE_POD, 2)
      .owned_by(1, 1)
      .in_star_orbit(1)
      .with_mount(0)
      .with_crystals(2)
      .build();

  // Ship 3: owned by player 2
  TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT, 3)
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
  ctx.assert_dispatch_rejected(g, {"mount", "#1"});
  test::expect_contains(g.out.str(), "You have no crystals on board.");

  // 3. Ship not equipped with crystal mount
  ctx.assert_dispatch_rejected(g, {"mount", "#2"});
  test::expect_contains(g.out.str(),
                        "This ship is not equipped with a crystal mount.");

  // 4. Already mounted
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.mounted() = 1;
    s.add_crystals(1);
  });
  ctx.assert_dispatch_rejected(g, {"mount", "#1"});
  test::expect_contains(g.out.str(), "You already have a crystal mounted.");

  // 5. Explicit foreign ship rejected
  ctx.assert_dispatch_rejected(g, {"mount", "#3"});
  test::expect_contains(g.out.str(), "You don't own ship #3.");

  std::println(std::cout, "✓ mount error cases passed");
}

}  // namespace

int main() {
  test_mount_persistence();
  test_mount_errors();

  std::println(std::cout, "\n✅ All mount tests passed!");
  return 0;
}
