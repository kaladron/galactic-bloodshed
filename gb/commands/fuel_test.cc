// SPDX-License-Identifier: Apache-2.0

/// \file fuel_test.cc
/// \brief Unit tests for fuel (proj_fuel) command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_fuel_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  shipnum_t ship_num = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                           .owned_by(1, 1)
                           .named("Explorer")
                           .in_star_orbit(1, SystemCoordinates{0.0, 0.0})
                           .with_speed(2)
                           .with_fuel(100.0)
                           .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. 4-Way Command Matrix runner on fuel projection
  TestCommandMatrix(ctx, "fuel")
      .with_valid_argv({"fuel", std::format("#{}", ship_num.value), "/Vega"})
      .with_invalid_argv({"fuel", "#999", "/Vega"})
      .with_valid_scope(ScopeLevel::LEVEL_STAR)
      .with_expected_star_ap(0)
      .run_matrix(g);

  test::expect_contains(g.out.str(), "FUEL ESTIMATES");

  // 2. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"fuel"});
  test::expect_contains(g.out.str(), "Syntax: fuel <#ship> [<destination>]");

  // 3. Bad argument format (not starting with #) and too many args
  ctx.assert_dispatch_rejected(g, {"fuel", "1", "/Vega"});
  test::expect_contains(g.out.str(), "Invalid first option");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fuel", "#1", "/Vega", "extra"});
  test::expect_contains(g.out.str(), "Invalid number of options");

  // 4. Non-numeric #ship (verifies no nullopt dereference crash)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fuel", "#abc", "/Vega"});
  test::expect_contains(g.out.str(), "rst: no such ship #abc");

  // 5. Unowned ship, landed ship without destination, stationary ship, factory
  shipnum_t enemy_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                             .owned_by(2, 1)
                             .in_star_orbit(1, SystemCoordinates{0.0, 0.0})
                             .with_speed(2)
                             .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", enemy_ship.value), "/Vega"});
  test::expect_contains(g.out.str(), "You do not own this ship.");

  shipnum_t landed_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                              .owned_by(1, 1)
                              .landed_on(1, 1, Coordinates(1, 1))
                              .with_speed(2)
                              .with_fuel(200.0)
                              .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"fuel", std::format("#{}", landed_ship.value)});
  test::expect_contains(
      g.out.str(),
      "You must specify a destination for landed or docked ships...");

  // Landed ship WITH destination (planet scope destination in explored system)
  ctx.em.mutate_star(2, [](Star& s) { s.mark_explored_by(1); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"fuel", std::format("#{}", landed_ship.value), "/Vega/Vega Prime"});
  test::expect_contains(g.out.str(), "FUEL ESTIMATES");

  shipnum_t stopped_ship = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                               .owned_by(1, 1)
                               .in_star_orbit(1, SystemCoordinates{0.0, 0.0})
                               .with_speed(0)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", stopped_ship.value), "/Vega"});
  test::expect_contains(g.out.str(), "That ship is not moving!");

  shipnum_t factory_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                               .owned_by(1, 1)
                               .in_star_orbit(1, SystemCoordinates{0.0, 0.0})
                               .with_speed(1)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", factory_ship.value), "/Vega"});
  test::expect_contains(g.out.str(),
                        "That ship does not have a speed rating...");

  // 6. Destination resolution errors: no destination orders, explicit '/',
  // bad scope, within 10.0 units, and unexplored system
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fuel", std::format("#{}", ship_num.value)});
  test::expect_contains(g.out.str(),
                        "That ship currently has no destination orders...");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", ship_num.value), "/"});
  test::expect_contains(g.out.str(), "Invalid ship destination.");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", ship_num.value), "/NoSuchStar"});
  test::expect_contains(g.out.str(), "fuel:  bad scope.");

  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", ship_num.value), "/Sol"});
  test::expect_contains(g.out.str(),
                        "That ship is within 10.0 units of the destination.");

  ctx.em.mutate_star(2, [](Star& s) { s.explored().reset(); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fuel", std::format("#{}", ship_num.value), "/Vega/Vega Prime"});
  test::expect_contains(g.out.str(),
                        "You haven't explored the destination system.");

  ctx.verify_universe_invariants();
}

void test_fuel_output_and_do_trip_branches() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. fuel_output with grav == 0 and segments == 1
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 1;
    st.nsegments_done = 1;
    st.update_time_minutes = 60;
  });
  g.out.str("");
  fuel_output(g, 100.0, 25.0, 0.0, 10.0, 2, "Earth");
  test::expect_contains(g.out.str(), "ESTIMATED Arrival Time:");

  // 2. fuel_output with grav > 0 and segments > 1
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 4;
    st.nsegments_done = 2;
  });
  g.out.str("");
  fuel_output(g, 100.0, 25.0, 1.0, 10.0, 3, "Earth");
  test::expect_contains(g.out.str(), "used to launch from Earth");
  test::expect_contains(g.out.str(), "ESTIMATED Arrival Time:");

  // 3. fuel_output with segment discrepancy (nsegments_done > segments)
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 2;
    st.nsegments_done = 5;
  });
  g.out.str("");
  fuel_output(g, 100.0, 25.0, 0.0, 10.0, 1, "Earth");
  test::expect_contains(
      g.out.str(),
      "Estimated arrival time not available due to segment # discrepancy.");

  // 4. do_trip with LEVEL_SHIP destination and out-of-fuel failure case
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 1;
    st.nsegments_done = 1;
  });
  const auto target_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                             .owned_by(1, 1)
                             .in_star_orbit(1, SystemCoordinates{25.0, 0.0})
                             .build();
  const auto runner_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                             .owned_by(1, 1)
                             .in_star_orbit(1, SystemCoordinates{0.0, 0.0})
                             .with_speed(9)
                             .build();

  Place ship_dest{ScopeLevel::LEVEL_SHIP, 1, 1, target_id};
  const auto target_coords = ctx.em.peek_ship(target_id)->coordinates();
  {
    SimulatedShip sim{*ctx.em.peek_ship(runner_id)};
    const auto [ok, segs] =
        do_trip(ship_dest, sim, 500.0, 0.0, target_coords, ctx.em);
    test::expect_true(ok);
    test::expect_gt(segs, 0U);
  }
  {
    // Insufficient fuel (0.0) fails to resolve trip
    SimulatedShip sim{*ctx.em.peek_ship(runner_id)};
    const auto [ok, segs] =
        do_trip(ship_dest, sim, 0.0, 0.0, target_coords, ctx.em);
    test::expect_false(ok);
    test::expect_eq(segs, 1U);
  }
}

}  // namespace

int main() {
  test_fuel_matrix();
  test_fuel_output_and_do_trip_branches();
  std::println(std::cout, "✓ fuel_test passed!");
  return 0;
}
