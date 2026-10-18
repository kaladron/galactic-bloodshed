// SPDX-License-Identifier: Apache-2.0

/// \file unload_test.cc
/// \brief Unit tests for the unload command.

import commands;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void test_unload_happy_path() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{1}).fuel = 1000;
    p.info(player_t{1}).resource = 500;
    p.info(player_t{1}).destruct = 200;
    p.info(player_t{1}).crystals = 50;
  });

  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(1, 1)
                                .named("CargoHauler")
                                .landed_on(1, 1, {5, 5})
                                .with_fuel(100.0)
                                .with_resource(200)
                                .with_destruct(50)
                                .with_crystals(10)
                                .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "r", "50"});
  test::expect_eq(ctx.em.peek_ship(ship_id)->resource(), 150);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(player_t{1}).resource, 550);

  // Default full unload when amount is omitted (amt == 0 -> lim.lolim)
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "r"});
  test::expect_eq(ctx.em.peek_ship(ship_id)->resource(), 0);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(player_t{1}).resource, 700);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "f", "40"});
  test::expect_eq(ctx.em.peek_ship(ship_id)->fuel(), 60.0);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(player_t{1}).fuel, 1040);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "d", "20"});
  test::expect_eq(ctx.em.peek_ship(ship_id)->destruct(), 30);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(player_t{1}).destruct, 220);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", ship_id.value), "x", "4"});
  test::expect_eq(ctx.em.peek_ship(ship_id)->crystals(), 6);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(player_t{1}).crystals, 54);

  std::println(std::cout, "✓ Commodities unloaded from ship to planet");
}

void test_unload_syntax_and_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  const shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(1, 1)
                                .named("GoodCargo")
                                .landed_on(1, 1, {1, 1})
                                .with_resource(50)
                                .with_max_resource(500)
                                .build();

  const shipnum_t vn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_VN)
                              .owned_by(1, 1)
                              .named("VNProbe")
                              .landed_on(1, 1, {1, 1})
                              .with_resource(50)
                              .build();

  const shipnum_t robot_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                                 .owned_by(1, 1)
                                 .named("MineShip")
                                 .landed_on(1, 1, {1, 1})
                                 .with_max_crew(0)
                                 .with_destruct(10)
                                 .with_max_destruct(100)
                                 .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"unload", "#1"});
  test::expect_contains(g.out.str(),
                        "Syntax: unload <ship> <commodity> [<amount>]");

  // 2. Empty commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", ship_id.value), ""});
  test::expect_contains(g.out.str(), "Unload what?");

  // 3. Invalid non-numeric amount
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", ship_id.value), "r", "notanumber"});
  test::expect_contains(g.out.str(), "Invalid amount");

  // 4. Unknown commodity
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", ship_id.value), "z", "10"});
  test::expect_contains(g.out.str(), "No such commodity");

  // 5. Out-of-bounds unload amount (amt < lim.lolim) and negative unload amount
  // (amt > lim.uplim)
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", ship_id.value), "r", "999999"});
  test::expect_contains(g.out.str(), "you can only transfer between");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", ship_id.value), "r", "-999999"});
  test::expect_contains(g.out.str(), "you can only transfer between");

  // 6. Von Neumann machine unload rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"unload", std::format("#{}", vn_id.value), "r", "10"});
  test::expect_contains(g.out.str(), "You can't unload VNs");

  // 7. Boobytrap disarmed message when unloading destruct from robot ship
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", robot_id.value), "d", "10"});
  test::expect_contains(g.out.str(), "no longer boobytrapped");

  std::println(std::cout, "✓ Unload syntax and validation edge cases verified");
}

void test_unload_ship_to_ship_and_alien_telegrams() {
  TestContext ctx;
  ctx.with_standard_universe();

  const shipnum_t s2_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(1, 1)
                              .named("Mothership")
                              .in_star_orbit(1)
                              .with_fuel(100.0)
                              .with_max_fuel(500.0)
                              .with_resource(100)
                              .with_max_resource(1000)
                              .with_destruct(50)
                              .with_max_destruct(200)
                              .with_crystals(10)
                              .with_crew(10, 5)
                              .with_max_crew(100)
                              .build();

  const shipnum_t s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(1, 1)
                              .named("Tender")
                              .docked_to(s2_id, 1)
                              .with_fuel(80.0)
                              .with_max_fuel(200.0)
                              .with_resource(100)
                              .with_max_resource(500)
                              .with_destruct(30)
                              .with_max_destruct(100)
                              .with_crystals(15)
                              .with_crew(9, 4)
                              .with_max_crew(50)
                              .build();
  ctx.em.mutate_ship(s2_id, [&](Ship& s2) { s2.dock_with_ship(s1_id); });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "r", "40"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->resource(), 60);
  test::expect_eq(ctx.em.peek_ship(s2_id)->resource(), 140);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "f", "20"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->fuel(), 60.0);
  test::expect_eq(ctx.em.peek_ship(s2_id)->fuel(), 120.0);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "d", "15"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->destruct(), 15);
  test::expect_eq(ctx.em.peek_ship(s2_id)->destruct(), 65);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "x", "5"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->crystals(), 10);
  test::expect_eq(ctx.em.peek_ship(s2_id)->crystals(), 15);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "c", "3"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->popn(), 6);
  test::expect_eq(ctx.em.peek_ship(s2_id)->popn(), 13);

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "m", "1"});
  test::expect_eq(ctx.em.peek_ship(s1_id)->troops(), 3);
  test::expect_eq(ctx.em.peek_ship(s2_id)->troops(), 6);

  // Unload to an alien ship (triggers cross-player telegrams for r, f, d, and
  // verify render_ship_cargo_transfer_telegram for all ShipCargoType variants)
  const shipnum_t alien_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                 .owned_by(2, 1)
                                 .named("KlingonFreighter")
                                 .in_star_orbit(1)
                                 .with_fuel(10.0)
                                 .with_max_fuel(500.0)
                                 .with_resource(100)
                                 .with_max_resource(500)
                                 .with_destruct(10)
                                 .with_max_destruct(200)
                                 .build();
  ctx.em.mutate_ship(s1_id, [&](Ship& s1) { s1.dock_with_ship(alien_id); });
  ctx.em.mutate_ship(alien_id,
                     [&](Ship& alien) { alien.dock_with_ship(s1_id); });

  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "r", "10"});
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "f", "10"});
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", s1_id.value), "d", "5"});
  test::expect_eq(ctx.em.peek_ship(alien_id)->resource(), 110);
  test::expect_eq(ctx.em.peek_ship(alien_id)->fuel(), 20.0);
  test::expect_eq(ctx.em.peek_ship(alien_id)->destruct(), 15);

  for (const auto cargo : all_ship_cargo_types) {
    for (const bool metamorph : {false, true}) {
      const ShipCargoTransferResult sample{
          .commodity = cargo,
          .amount = -5,
          .ship_display = "c1Tender [1]",
          .target_ship_display = "c2KlingonFreighter [2]",
          .target_owner = 2,
          .target_governor = 1,
          .notify_target_owner = true,
          .is_metamorph = metamorph,
      };
      const auto tele =
          GB::presentation::render_ship_cargo_transfer_telegram(sample);
      test::expect_true(tele.has_value());
      test::expect_false(tele->empty());
    }
  }

  std::println(std::cout,
               "✓ Ship-to-ship unload and cross-player telegrams verified");
}

void test_unload_onto_alien_sector() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 10;
    r.absorb = true;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });

  const shipnum_t assault_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                   .owned_by(1, 1)
                                   .named("DropShip")
                                   .landed_on(1, 1, {2, 2})
                                   .with_crew(100, 100)
                                   .with_max_crew(200)
                                   .build();

  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_owner(2);
    sect.set_popn_exact(2);
    sect.set_troops(1);
    p.sync_demographics(map);
  });
  ctx.verify_universe_invariants();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Victory assault with troops (and metamorph body absorption)
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "m", "50"});
  test::expect_contains(g.out.str(),
                        "That sector is already occupied by another player!");
  test::expect_contains(g.out.str(), "VICTORY! The sector is yours!");
  test::expect_contains(g.out.str(), "alien bodies absorbed");
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({2, 2}).get_owner(), 1);
  ctx.verify_universe_invariants();

  // 2. Defeat assault with military and civilians
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 15;
    r.absorb = true;
  });
  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_owner(2);
    sect.set_popn_exact(100);
    sect.set_troops(500);
    p.sync_demographics(map);
  });
  ctx.verify_universe_invariants();

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "m", "5"});
  test::expect_contains(g.out.str(), "DEFEAT!  Your assault was repulsed.");
  test::expect_contains(g.out.str(), "Metamorphs have absorbed");
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({2, 2}).get_owner(), 2);
  ctx.verify_universe_invariants();

  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "c", "5"});
  test::expect_contains(g.out.str(), "DEFEAT!  Your assault was repulsed.");
  ctx.verify_universe_invariants();

  // 3. Victory assault with civilians
  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 15;
    r.absorb = true;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1;
    r.absorb = false;
  });
  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& map) {
    auto& sect = map.get({2, 2});
    sect.set_owner(2);
    sect.set_popn_exact(1);
    sect.set_troops(0);
    p.sync_demographics(map);
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"unload", std::format("#{}", assault_id.value), "c", "50"});
  test::expect_contains(g.out.str(), "VICTORY! The sector is yours!");
  test::expect_eq(ctx.em.peek_sectormap(1, 1)->get({2, 2}).get_owner(), 1);
  ctx.verify_universe_invariants();
  std::println(std::cout, "✓ Amphibious alien sector assaults verified");
}

}  // namespace

int main() {
  test_unload_happy_path();
  test_unload_syntax_and_errors();
  test_unload_ship_to_ship_and_alien_telegrams();
  test_unload_onto_alien_sector();

  std::println(std::cout, "\n✅ All unload command tests passed!");
  return 0;
}
