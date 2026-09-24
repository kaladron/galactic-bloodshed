// SPDX-License-Identifier: Apache-2.0

/// \file make_mod_test.cc
/// \brief Unit tests for make and modify commands for factory ship
/// configuration

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_make_mod_command_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(1, 1)
                             .named("Factory")
                             .in_star_orbit(0)
                             .with_fuel(100.0)
                             .with_resource(1000)
                             .with_crew(50, 0)
                             .with_on(false)
                             .with_size(100)
                             .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_shipno(factory_id);
  g.set_snum(0);

  TestCommandMatrix(ctx, GB::commands::make_cmd)
      .with_valid_scope(ScopeLevel::LEVEL_SHIP)
      .with_invalid_scopes({ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                            ScopeLevel::LEVEL_PLAN})
      .with_valid_argv({"make", "f"})
      .with_invalid_argv({"make", "z"})
      .run_matrix(g);

  ctx.em.mutate_ship(factory_id,
                     [](Ship& s) { s.build_type() = ShipType::STYPE_CRUISER; });

  TestCommandMatrix(ctx, GB::commands::modify_cmd)
      .with_valid_scope(ScopeLevel::LEVEL_SHIP)
      .with_invalid_scopes({ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                            ScopeLevel::LEVEL_PLAN})
      .with_valid_argv({"modify", "armor", "20"})
      .with_invalid_argv({"modify", "armor", "-5"})
      .run_matrix(g);

  std::println(std::cout, "✓ make_mod command matrix tests passed");
}

void test_make_designations_and_errors() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.tech = 100.0;
    r.pods = false;
    r.discoveries.crystal = true;
    r.discoveries.hyperdrive = true;
    r.discoveries.laser = true;
    r.discoveries.cew = true;
    r.discoveries.cloak = true;
  });

  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(1, 1)
                             .named("Factory")
                             .in_star_orbit(0)
                             .with_fuel(100.0)
                             .with_resource(1000)
                             .with_crew(50, 0)
                             .with_on(false)
                             .with_size(100)
                             .build();

  shipnum_t non_factory_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(1, 1)
                                 .in_star_orbit(0)
                                 .with_crew(10, 0)
                                 .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_snum(0);

  // 1. Non-factory ship rejected
  g.set_shipno(non_factory_id);
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "That is not a factory.");

  // 2. Make with 0 args before build_type is set
  g.set_shipno(factory_id);
  ctx.assert_dispatch_rejected(g, {"make"});
  test::expect_contains(g.out.str(), "No ship type specified.");

  // 3. Illegal ship letter
  ctx.assert_dispatch_rejected(g, {"make", "z"});
  test::expect_contains(g.out.str(), "Illegal ship letter.");

  // 4. Pod without pod technology
  ctx.assert_dispatch_rejected(g, {"make", "p"});
  test::expect_contains(g.out.str(), "Illegal ship letter.");

  // 5. Non-factory-built ship (probe ':')
  ctx.assert_dispatch_rejected(g, {"make", ":"});
  test::expect_contains(g.out.str(),
                        "This kind of ship does not require a factory");

  // 6. Designate cruiser ('C')
  ctx.assert_dispatch_success(g, {"make", "C"});
  test::expect_contains(g.out.str(), "Factory designated to produce Cruisers.");

  // 7. Make with 0 args prints full specifications table
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(),
                        "--- Current Production Specifications ---");
  test::expect_contains(g.out.str(), "Cruiser");

  // 8. Low tech warning on display and designate
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 1.0; });
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"make", "C"});
  test::expect_contains(g.out.str(), "You can't produce this design yet!");
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(),
                        "Your engineering capability is not advanced enough");

  // Restore tech
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 100.0; });
  ctx.setup_game_obj(g, 1, 1);

  // 9. Factory already online rejects make <shiptype>
  ctx.em.mutate_ship(factory_id, [](Ship& s) { s.on() = true; });
  ctx.assert_dispatch_rejected(g, {"make", "f"});
  test::expect_contains(g.out.str(), "This factory is already online.");

  std::println(std::cout, "✓ make designations and error paths passed");
}

void test_modify_attributes_and_batteries() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) {
    r.tech = 200.0;
    r.discoveries.crystal = true;
    r.discoveries.hyperdrive = true;
    r.discoveries.laser = false;
    r.discoveries.cew = false;
  });

  shipnum_t factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                             .owned_by(1, 1)
                             .named("Factory")
                             .in_star_orbit(0)
                             .with_fuel(100.0)
                             .with_resource(1000)
                             .with_crew(50, 0)
                             .with_on(false)
                             .with_size(100)
                             .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_snum(0);
  g.set_shipno(factory_id);

  // 1. Modify before make
  ctx.assert_dispatch_rejected(g, {"modify", "armor", "20"});
  test::expect_contains(g.out.str(), "No ship design specified.");

  // Designate Dreadnought ('D') which has primary, secondary, cew, laser, etc.
  ctx.assert_dispatch_success(g, {"make", "D"});

  // 2. Modify with no characteristic
  ctx.assert_dispatch_rejected(g, {"modify"});
  test::expect_contains(g.out.str(),
                        "You have to specify the characteristic you wish");

  // 3. Modify simple numeric attributes
  ctx.assert_dispatch_success(g, {"modify", "armor", "45"});
  ctx.assert_dispatch_success(g, {"modify", "crew", "150"});
  ctx.assert_dispatch_success(g, {"modify", "cargo", "350"});
  ctx.assert_dispatch_success(g, {"modify", "hanger", "80"});
  ctx.assert_dispatch_success(g, {"modify", "fuel", "250"});
  ctx.assert_dispatch_success(g, {"modify", "destruct", "120"});
  ctx.assert_dispatch_success(g, {"modify", "speed", "6"});

  const auto* ship = ctx.em.peek_ship(factory_id);
  test::expect_eq(ship->armor(), 45);
  test::expect_eq(ship->max_crew(), 150);
  test::expect_eq(ship->max_resource(), 350);
  test::expect_eq(ship->max_hanger(), 80);
  test::expect_eq(ship->max_fuel(), 250);
  test::expect_eq(ship->max_destruct(), 120);
  test::expect_eq(ship->max_speed(), 6);

  // 4. Modify boolean toggles (2-arg form)
  bool initial_mount = ship->mount();
  ctx.assert_dispatch_success(g, {"modify", "mount"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->mount(), !initial_mount);

  bool initial_hyper = ship->hyper_drive().has;
  ctx.assert_dispatch_success(g, {"modify", "hyperdrive"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->hyper_drive().has,
                  !initial_hyper);

  // Laser without discovery
  ctx.assert_dispatch_rejected(g, {"modify", "laser"});
  test::expect_contains(g.out.str(),
                        "Your race does not understand lasers yet.");

  // Enable laser discovery and toggle
  ctx.em.mutate_race(1, [](Race& r) { r.discoveries.laser = true; });
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"modify", "laser"});
  test::expect_true(ctx.em.peek_ship(factory_id)->laser());

  // 5. Modify primary & secondary batteries
  ctx.assert_dispatch_rejected(g, {"modify", "primary"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  ctx.assert_dispatch_rejected(g, {"modify", "primary", "strength", "-1"});
  test::expect_contains(g.out.str(), "That's a ridiculous setting.");

  ctx.assert_dispatch_success(g, {"modify", "primary", "strength", "12"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->primary_battery().count, 12);

  ctx.assert_dispatch_rejected(g, {"modify", "primary", "caliber", "super"});
  test::expect_contains(g.out.str(), "No such caliber.");

  ctx.assert_dispatch_success(g, {"modify", "primary", "caliber", "medium"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->primary_battery().caliber,
                  guntype_t::MEDIUM);

  ctx.assert_dispatch_success(g, {"modify", "secondary", "strength", "8"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->secondary_battery().count, 8);

  ctx.assert_dispatch_success(g, {"modify", "secondary", "caliber", "light"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->secondary_battery().caliber,
                  guntype_t::LIGHT);

  ctx.assert_dispatch_rejected(g, {"modify", "primary", "unknown", "5"});
  test::expect_contains(g.out.str(), "No such gun characteristic.");

  // 6. Modify CEWs
  ctx.assert_dispatch_rejected(g, {"modify", "cew", "strength", "50"});
  test::expect_contains(
      g.out.str(), "Your race does not understand confined energy weapons.");

  ctx.em.mutate_race(1, [](Race& r) { r.discoveries.cew = true; });
  ctx.setup_game_obj(g, 1, 1);

  ctx.assert_dispatch_rejected(g, {"modify", "cew"});
  test::expect_contains(g.out.str(), "No such option for CEWs.");

  ctx.assert_dispatch_rejected(g, {"modify", "cew", "strength", "-5"});
  test::expect_contains(g.out.str(), "That's a ridiculous setting.");

  ctx.assert_dispatch_rejected(g, {"modify", "cew", "unknown", "10"});
  test::expect_contains(g.out.str(), "No such option for CEWs.");

  ctx.assert_dispatch_success(g, {"modify", "cew", "strength", "50"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->cew(), 50);

  ctx.assert_dispatch_success(g, {"modify", "cew", "range", "200"});
  test::expect_eq(ctx.em.peek_ship(factory_id)->cew_range(), 200);

  // Print specs with active CEW to cover CEW range/energy display
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(), "Opt Range:");
  test::expect_contains(g.out.str(), "Energy:");

  // 7. Unknown characteristic
  ctx.assert_dispatch_rejected(g, {"modify", "unknown", "10"});
  test::expect_contains(g.out.str(),
                        "That characteristic either doesn't exist or can't be");

  // 8. Cost limit check (> 65535.0)
  ctx.assert_dispatch_success(g, {"modify", "cew", "strength", "10000"});
  ctx.assert_dispatch_rejected(g, {"modify", "cew", "range", "60000"});
  test::expect_contains(g.out.str(), "The max cost allowed is 65535");

  // 9. God-only ship rejection for mortal race ('!')
  ctx.assert_dispatch_rejected(g, {"make", "!"});
  test::expect_contains(g.out.str(), "Nice try!");

  // 10. Print specs for simple ship (fighter 'f') without
  // mount/hyperdrive/laser/cew
  ctx.assert_dispatch_success(g, {"make", "f"});
  ctx.assert_dispatch_success(g, {"make"});
  test::expect_contains(g.out.str(), "Fighter");

  // 11. Non-modifiable ship (probe ':') with and without hyperdrive discovery
  ctx.em.mutate_ship(factory_id,
                     [](Ship& s) { s.build_type() = ShipType::OTYPE_PROBE; });
  ctx.assert_dispatch_rejected(g, {"modify", "armor", "10"});
  test::expect_contains(
      g.out.str(),
      "You may only modify hyperdrive installation on this kind of ship.");
  ctx.assert_dispatch_success(g, {"modify", "hyperdrive"});

  ctx.em.mutate_race(1, [](Race& r) { r.discoveries.hyperdrive = false; });
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_rejected(g, {"modify", "armor", "10"});
  test::expect_contains(g.out.str(),
                        "Sorry, but you can't modify this ship right now.");

  std::println(std::cout, "✓ modify attributes and batteries tests passed");
}

}  // namespace

int main() {
  test_make_mod_command_matrix();
  test_make_designations_and_errors();
  test_modify_attributes_and_batteries();

  std::println(std::cout, "\n✅ All make_mod tests passed!");
  return 0;
}
