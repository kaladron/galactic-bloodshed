// SPDX-License-Identifier: Apache-2.0

/// \file fuel_test.cc
/// \brief Unit tests for fuel (proj_fuel) command

import commands;
import gb.entities;
import gb.mechanics;
import gb.presentation;
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

  ctx.em.mutate_star(2, [](Star& s) { s.clear_all_explored(); });
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

  // 1. compute_trip_estimate with grav == 0 and segments == 1
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 1;
    st.nsegments_done = 1;
    st.update_time_minutes = 60;
  });
  {
    const auto est =
        compute_trip_estimate(ctx.em, 100.0, 25.0, 0.0, 10.0, 2, "Earth");
    test::expect_eq(static_cast<int>(est.arrival_status),
                    static_cast<int>(ArrivalTimeStatus::Available));
    const std::string rendered = GB::presentation::render_trip_estimate(est);
    test::expect_contains(rendered, "ESTIMATED Arrival Time:");
  }

  // 2. compute_trip_estimate with grav > 0 and segments > 1 (plus segs == 0)
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 4;
    st.nsegments_done = 2;
  });
  {
    const auto est =
        compute_trip_estimate(ctx.em, 100.0, 25.0, 1.0, 10.0, 3, "Earth");
    const std::string rendered = GB::presentation::render_trip_estimate(est);
    test::expect_contains(rendered, "used to launch from Earth");
    test::expect_contains(rendered, "ESTIMATED Arrival Time:");

    const auto zero_segs_est =
        compute_trip_estimate(ctx.em, 10.0, 0.0, 0.0, 10.0, 0, "");
    test::expect_eq(static_cast<int>(zero_segs_est.arrival_status),
                    static_cast<int>(ArrivalTimeStatus::Available));
  }

  // 3. compute_trip_estimate with segment discrepancy (nsegments_done >
  // segments or segments == 0) and ServerStateUnavailable
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 2;
    st.nsegments_done = 5;
  });
  {
    const auto est =
        compute_trip_estimate(ctx.em, 100.0, 25.0, 0.0, 10.0, 1, "Earth");
    const std::string rendered = GB::presentation::render_trip_estimate(est);
    test::expect_contains(
        rendered,
        "Estimated arrival time not available due to segment # discrepancy.");
  }
  ctx.em.mutate_server_state([](ServerState& st) { st.segments = 0; });
  {
    const auto est =
        compute_trip_estimate(ctx.em, 100.0, 25.0, 0.0, 10.0, 1, "Earth");
    test::expect_eq(static_cast<int>(est.arrival_status),
                    static_cast<int>(ArrivalTimeStatus::SegmentDiscrepancy));
  }
  {
    TripEstimate unavail_est{
        .distance = 50.0,
        .segments = 1,
        .fuel_used = 10.0,
        .arrival_status = ArrivalTimeStatus::ServerStateUnavailable,
    };
    test::expect_contains(GB::presentation::render_trip_estimate(unavail_est),
                          "Server state unavailable.");
  }

  // 4. do_trip with LEVEL_SHIP destination and out-of-fuel failure case
  ctx.em.mutate_server_state([](ServerState& st) {
    st.segments = 2;
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
  {
    // Landed ship deducts launch gravity fuel before flying to destination,
    // and fails immediately if fuel < launch_gravity_fuel
    const auto orbit_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                   .owned_by(1, 1)
                                   .in_planet_orbit(1, 1)
                                   .with_speed(9)
                                   .build();
    const auto landed_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                    .owned_by(1, 1)
                                    .landed_on(1, 1, {0, 0})
                                    .with_speed(9)
                                    .build();

    SimulatedShip sim_orbit{*ctx.em.peek_ship(orbit_ship_id)};
    const auto [ok_orbit, segs_orbit] =
        do_trip(ship_dest, sim_orbit, 500.0, 0.0, target_coords, ctx.em);
    test::expect_true(ok_orbit);

    SimulatedShip sim_landed{*ctx.em.peek_ship(landed_ship_id)};
    const auto [ok_landed, segs_landed] =
        do_trip(ship_dest, sim_landed, 500.0, 1.0, target_coords, ctx.em);
    test::expect_true(ok_landed);
    test::expect_lt(sim_landed.fuel(), sim_orbit.fuel());

    SimulatedShip sim_landed_nofuel{*ctx.em.peek_ship(landed_ship_id)};
    const auto [ok_nofuel, segs_nofuel] =
        do_trip(ship_dest, sim_landed_nofuel, 0.0, 1.0, target_coords, ctx.em);
    test::expect_false(ok_nofuel);
    test::expect_eq(segs_nofuel, 0U);
  }
  {
    // Docked ship undocks before flying to destination
    SimulatedShip sim_docked{*ctx.em.peek_ship(runner_id)};
    sim_docked.dock_with_ship(target_id);
    const auto [ok_docked, segs_docked] =
        do_trip(ship_dest, sim_docked, 500.0, 0.0, target_coords, ctx.em);
    test::expect_true(ok_docked);
    test::expect_gt(segs_docked, 0U);
  }
  {
    // Hyperdrive ship waiting for update segment (fuel unchanged while
    // hyper_drive().on is true) exercises !tmpship.hyper_drive().on == false
    ctx.em.mutate_star(2, [](Star& s) {
      s.set_coordinates(UniverseCoordinates{5000.0, 0.0});
    });
    SimulatedShip sim_hyper{*ctx.em.peek_ship(runner_id)};
    sim_hyper.hyper_drive() = {.charge = 1, .on = true, .has = true};
    sim_hyper.mounted() = true;
    Place star_dest{ScopeLevel::LEVEL_STAR, 2, 0, 0};
    const auto star2_coords = ctx.em.peek_star(2)->coordinates();
    const auto [ok_hyper, segs_hyper] =
        do_trip(star_dest, sim_hyper, 500.0, 0.0, star2_coords, ctx.em);
    test::expect_true(ok_hyper);
    test::expect_eq(segs_hyper, 2U);
  }
}

}  // namespace

int main() {
  test_fuel_matrix();
  test_fuel_output_and_do_trip_branches();
  std::println(std::cout, "✓ fuel_test passed!");
  return 0;
}
