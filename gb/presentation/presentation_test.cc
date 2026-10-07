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

  static_assert(std::is_aggregate_v<CriticalHitSystemsDamage>);
  static_assert(std::is_aggregate_v<CriticalHitResult>);
  static_assert(std::is_aggregate_v<ShipShotResult>);
  static_assert(std::is_aggregate_v<BombardResult>);
  static_assert(std::is_aggregate_v<MineShipVictimReport>);
  static_assert(std::is_aggregate_v<MineDetonationReport>);

  const ShipShotResult ship_shot{
      .attacker_kind = ShipShotAttackerKind::Ship,
      .attacker_player = player_t{1},
      .attacker_display = "d1 Destroyer",
      .target_location_display = "/Sol",
      .target_display = "c2 Cruiser",
      .target_alive = false,
      .weapon = ShipShotWeaponKind::HeavyGuns,
      .strength = 3,
      .range = 12.0,
      .hits = 2,
      .hit_probability = 75,
      .damage = 45,
      .total_damage = 100,
      .radiation_dosage = 0,
      .total_radiation = 0,
      .armor_reduced_to = armor_t{4},
      .penetrations = 1,
      .effective_armor = 4,
      .defense = 2,
      .penetration_probability = 0.5,
      .critical =
          CriticalHitResult{
              .count = 1,
              .damage = 15,
              .systems =
                  CriticalHitSystemsDamage{
                      .cew_destroyed = true,
                      .laser_destroyed = true,
                      .cloak_destroyed = true,
                      .hyper_drive_destroyed = true,
                      .reduced_max_speed = speed_t{2},
                      .reduced_armor = armor_t{3},
                  },
          },
      .collateral =
          CollateralDamage{
              .civilian_casualties = 5,
              .military_casualties = 2,
              .primary_guns_lost = 1,
              .secondary_guns_lost = 1,
          },
  };
  const std::string ship_shot_ascii =
      GB::presentation::presenter_for(UiMode::ASCII).render(ship_shot);
  test::expect_contains(ship_shot_ascii,
                        "/Sol: d1 Destroyer DESTROYED c2 Cruiser\n");
  test::expect_contains(ship_shot_ascii, "CEW ");
  test::expect_contains(ship_shot_ascii, "Laser ");
  test::expect_contains(ship_shot_ascii, "Cloak ");
  test::expect_contains(ship_shot_ascii, "Hyper-drive ");
  test::expect_contains(ship_shot_ascii, "Speed=2");
  test::expect_contains(ship_shot_ascii, "Armor=3");
  test::expect_contains(ship_shot_ascii, "5 civ + 2 mil casualties");
  test::expect_contains(ship_shot_ascii,
                        "1 primary/1 secondary guns destroyed");
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::JSON).render(ship_shot),
      "\"type\":\"ship_shot\"");

  const ShipShotResult planet_rad_shot{
      .attacker_kind = ShipShotAttackerKind::Planet,
      .attacker_player = player_t{1},
      .attacker_display = "/Sol/Terra",
      .target_location_display = "/Sol/Terra",
      .target_display = "c2 Cruiser",
      .target_alive = true,
      .weapon = ShipShotWeaponKind::Radiation,
      .strength = 4,
      .range = 0.0,
      .hits = 2,
      .hit_probability = 60,
      .damage = 0,
      .total_damage = 10,
      .radiation_dosage = 30,
      .total_radiation = 50,
  };
  const std::string planet_rad_ascii =
      GB::presentation::render_ship_shot_long(planet_rad_shot);
  test::expect_contains(planet_rad_ascii,
                        "/Sol/Terra [1] attacked c2 Cruiser\n");
  test::expect_contains(planet_rad_ascii, "Rad: 30% for a total of 50%");

  const BombardResult bombard_res{
      .ship_display = "B1 Battleship",
      .location_display = "/Sol/Terra",
      .previous_sector_owner = player_t{2},
      .sectors_destroyed = 2,
      .nuked_players = {player_t{2}},
  };
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::ASCII).render(bombard_res),
      "B1 Battleship bombards /Sol/Terra [2]\n\t2 sectors destroyed\n");
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::JSON).render(bombard_res),
      "\"type\":\"bombard_result\"");

  const MineDetonationReport mine_report{
      .ship_display = "M1 Mine",
      .orbit_display = "/Sol/Terra",
      .ship_victims = {},
      .planet_strike = bombard_res,
  };
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::ASCII).render(mine_report),
      "M1 Mine detonated at /Sol/Terra\n");
  test::expect_contains(
      GB::presentation::render_mine_planet_strike_telegram(mine_report),
      "2 sectors destroyed.");
  test::expect_contains(
      GB::presentation::presenter_for(UiMode::JSON).render(mine_report),
      "\"type\":\"mine_detonation\"");
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
