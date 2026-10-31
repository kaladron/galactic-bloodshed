// SPDX-License-Identifier: Apache-2.0

/// \file dismount_test.cc
/// \brief Unit tests for dismount command and dismount_ship_crystal mechanic.

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_dismount_with_hyperdrive_and_laser() {
  TestContext ctx;
  ctx.with_standard_universe();

  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                .owned_by(1, 1)
                                .in_star_orbit(1)
                                .with_mount(1)
                                .with_crystals(1)
                                .build();
  ctx.em.mutate_ship(ship_id, [](Ship& s) {
    s.mounted() = 1;
    s.hyper_drive().charge = 50;
    s.laser() = 1;
    s.fire_laser() = 5;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(ship_id);

  ctx.assert_dispatch_success(g, {"dismount", std::format("#{}", ship_id)});

  const auto* final_ship = ctx.em.peek_ship(ship_id);
  test::expect_ne(final_ship, nullptr);
  test::expect_eq(final_ship->mounted(), 0);
  test::expect_eq(final_ship->crystals(), 2);
  test::expect_eq(final_ship->hyper_drive().charge, 0U);
  test::expect_eq(final_ship->fire_laser(), 0);
  test::expect_contains(g.out.str(), "Dismounted.");
  test::expect_contains(g.out.str(), "Discharged.");
  test::expect_contains(g.out.str(), "Laser deactivated.");

  std::println(std::cout, "✓ dismount with hyperdrive and laser passed");
}

void test_dismount_without_charge_or_active_laser() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Case A: laser == 1, fire_laser == 0, charge == 0
  const shipnum_t ship1_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(1, 1)
                                 .in_star_orbit(1)
                                 .with_mount(1)
                                 .with_crystals(0)
                                 .build();
  ctx.em.mutate_ship(ship1_id, [](Ship& s) {
    s.mounted() = 1;
    s.hyper_drive().charge = 0;
    s.laser() = 1;
    s.fire_laser() = 0;
  });

  // Case B: laser == 0, charge == 0
  const shipnum_t ship2_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(1, 1)
                                 .in_star_orbit(1)
                                 .with_mount(1)
                                 .with_crystals(0)
                                 .build();
  ctx.em.mutate_ship(ship2_id, [](Ship& s) {
    s.mounted() = 1;
    s.hyper_drive().charge = 0;
    s.laser() = 0;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_success(g, {"dismount", std::format("#{}", ship1_id)});
  test::expect_eq(g.out.str(), std::string("Dismounted.\n"));

  ctx.assert_dispatch_success(g, {"dismount", std::format("#{}", ship2_id)});
  test::expect_eq(g.out.str(), std::string("Dismounted.\n"));

  std::println(std::cout, "✓ dismount without charge or active laser passed");
}

void test_dismount_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Ship 1: max crystals on board (127) and mounted
  const shipnum_t ship1_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(1, 1)
                                 .in_star_orbit(1)
                                 .with_mount(1)
                                 .with_crystals(127)
                                 .build();
  ctx.em.mutate_ship(ship1_id, [](Ship& s) { s.mounted() = 1; });

  // Ship 2: has mount, not mounted
  const shipnum_t ship2_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(1, 1)
                                 .in_star_orbit(1)
                                 .with_mount(1)
                                 .with_crystals(1)
                                 .build();

  // Ship 3: no crystal mount
  const shipnum_t ship3_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                                 .owned_by(1, 1)
                                 .in_star_orbit(1)
                                 .with_mount(0)
                                 .build();

  // Ship 4: owned by player 2
  const shipnum_t ship4_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(2, 1)
                                 .in_star_orbit(1)
                                 .with_mount(1)
                                 .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"dismount"});
  test::expect_contains(g.out.str(), "Syntax: dismount <ship>");

  // 2. Max crystals already on board
  ctx.assert_dispatch_rejected(g, {"dismount", std::format("#{}", ship1_id)});
  test::expect_contains(
      g.out.str(),
      "You can't dismount the crystal. Max allowed already on board.");

  // 3. Not mounted
  ctx.assert_dispatch_rejected(g, {"dismount", std::format("#{}", ship2_id)});
  test::expect_contains(g.out.str(), "You don't have a crystal mounted.");

  // 4. Ship not equipped with crystal mount
  ctx.assert_dispatch_rejected(g, {"dismount", std::format("#{}", ship3_id)});
  test::expect_contains(g.out.str(),
                        "This ship is not equipped with a crystal mount.");

  // 5. Explicit foreign ship rejected
  ctx.assert_dispatch_rejected(g, {"dismount", std::format("#{}", ship4_id)});
  test::expect_contains(g.out.str(),
                        std::format("You don't own ship #{}.", ship4_id));

  std::println(std::cout, "✓ dismount error cases passed");
}

}  // namespace

int main() {
  test_dismount_with_hyperdrive_and_laser();
  test_dismount_without_charge_or_active_laser();
  test_dismount_errors();

  std::println(std::cout, "\n✅ All dismount tests passed!");
  return 0;
}
