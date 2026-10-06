// SPDX-License-Identifier: Apache-2.0

/// \file presentation_test.cc
/// \brief Unit tests for gb.presentation JSON envelope rendering, UiMode
/// dispatch, and entity std::formatter specializations.

import dallib;
import gb.entities;
import gb.mechanics;
import gb.services;
import gb.presentation;
import session;
import test;
import std;

struct SampleCommandResult {
  player_t player{1};
  governor_t governor{0};
  starnum_t star{3};
  planetnum_t planet{2};
  shipnum_t ship{42};
  Percentage efficiency{85};
  Temperature temperature{22};
  Coordinates sector{4, 7};
  std::string status{"launched"};
};

namespace {

void test_render_json_envelope_and_polymorphic_presenter() {
  const SampleCommandResult result{
      .player = player_t{2},
      .governor = governor_t{1},
      .star = starnum_t{5},
      .planet = planetnum_t{3},
      .ship = shipnum_t{19},
      .efficiency = Percentage{90},
      .temperature = Temperature{-15},
      .sector = Coordinates{2, 6},
      .status = "launched",
  };

  const std::string json_out =
      GB::presentation::render_json_envelope("launch_result", result);
  test::expect_eq(
      json_out,
      "{\"type\":\"launch_result\",\"data\":{\"player\":2,\"governor\":1,"
      "\"star\":5,\"planet\":3,\"ship\":19,\"efficiency\":90,"
      "\"temperature\":-15,\"sector\":{\"x\":2,\"y\":6},"
      "\"status\":\"launched\"}}\n");

  const TripEstimate est{
      .distance = 42.5,
      .segments = 2,
      .fuel_used = 12.0,
      .launch_gravity_fuel = 0.0,
      .launch_planet_name = "",
      .arrival_status = ArrivalTimeStatus::ServerStateUnavailable,
      .estimated_arrival_time = 0,
  };

  const std::string dispatched_ascii =
      GB::presentation::presenter_for(UiMode::ASCII).render(est);
  test::expect_contains(dispatched_ascii, "Total Distance = 42.50");
  test::expect_contains(dispatched_ascii, "Server state unavailable.");

  const std::string dispatched_json =
      GB::presentation::presenter_for(UiMode::JSON).render(est);
  test::expect_contains(dispatched_json, "\"type\":\"trip_estimate\"");
  test::expect_contains(dispatched_json, "\"distance\":42.5");

  static_assert(std::is_aggregate_v<CapturedShipEvent>);
  static_assert(std::is_aggregate_v<CapturedShipsReport>);

  const CapturedShipsReport empty_captured{};
  test::expect_eq(
      GB::presentation::presenter_for(UiMode::ASCII).render(empty_captured),
      "");
  test::expect_eq(
      GB::presentation::presenter_for(UiMode::JSON).render(empty_captured), "");

  const CapturedShipsReport populated_captured{
      .captured_ships =
          {
              CapturedShipEvent{
                  .ship_number = shipnum_t{12},
                  .ship_display = "f12 Fighter",
                  .new_owner = player_t{1},
                  .new_governor = governor_t{0},
              },
          },
  };
  test::expect_eq(
      GB::presentation::presenter_for(UiMode::ASCII).render(populated_captured),
      "f12 Fighter CAPTURED!\n");
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::JSON).render(populated_captured),
      "\"type\":\"captured_ships\"");

  static_assert(std::is_aggregate_v<ReactorOverloadEvent>);
  static_assert(std::is_aggregate_v<MechAttackPeopleResult>);
  static_assert(std::is_aggregate_v<PeopleAttackMechResult>);
  static_assert(std::is_aggregate_v<MechDefendEngagementRound>);
  static_assert(std::is_aggregate_v<MechDefendResult>);

  const ReactorOverloadEvent exploded{
      .outcome = ReactorOverloadOutcome::ShipExploded,
      .owner = player_t{1},
      .governor = governor_t{0},
      .scope = ScopeLevel::LEVEL_STAR,
      .star_id = starnum_t{1},
      .location_display = "/Sol",
      .ship_display = "B1 Battleship",
  };
  test::expect_eq(
      GB::presentation::presenter_for(UiMode::ASCII).render(exploded),
      "/Sol: Matter-antimatter EXPLOSION from overloaded crystal on B1 "
      "Battleship\n");
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::JSON).render(exploded),
      "\"type\":\"reactor_overload\"");

  const ReactorOverloadEvent damaged_crystal{
      .outcome = ReactorOverloadOutcome::CrystalDamaged,
      .owner = player_t{1},
      .governor = governor_t{0},
      .scope = ScopeLevel::LEVEL_STAR,
      .star_id = starnum_t{1},
      .location_display = "/Sol",
      .ship_display = "B1 Battleship",
  };
  test::expect_eq(
      GB::presentation::presenter_for(UiMode::ASCII).render(damaged_crystal),
      "/Sol: Crystal damaged from overloading on B1 Battleship.\n");
}

void test_gameobj_ui_mode_and_present() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  const TripEstimate est{
      .distance = 10.0,
      .segments = 1,
      .fuel_used = 3.0,
      .launch_gravity_fuel = 0.0,
      .launch_planet_name = "",
      .arrival_status = ArrivalTimeStatus::ServerStateUnavailable,
      .estimated_arrival_time = 0,
  };

  test::expect_eq(g.ui_mode(), UiMode::ASCII);
  g.present(est);
  test::expect_contains(g.out.str(), "Total Distance = 10.00");

  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  test::expect_eq(g.ui_mode(), UiMode::JSON);
  g.present(est);
  test::expect_contains(g.out.str(), "\"type\":\"trip_estimate\"");

  g.set_ui_mode(UiMode::ASCII);
  test::expect_eq(g.ui_mode(), UiMode::ASCII);
}

}  // namespace

int main() {
  test_render_json_envelope_and_polymorphic_presenter();
  test_gameobj_ui_mode_and_present();
  return 0;
}
