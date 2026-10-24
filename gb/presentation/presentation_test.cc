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
      GB::presentation::render_for_mode(UiMode::ASCII, est);
  test::expect_contains(dispatched_ascii, "Total Distance = 42.50");
  test::expect_contains(dispatched_ascii, "Server state unavailable.");

  const std::string dispatched_json =
      GB::presentation::render_for_mode(UiMode::JSON, est);
  test::expect_contains(dispatched_json, "\"type\":\"trip_estimate\"");
  test::expect_contains(dispatched_json, "\"distance\":42.5");

  static_assert(std::is_aggregate_v<CapturedShipEvent>);
  static_assert(std::is_aggregate_v<CapturedShipsReport>);

  const CapturedShipsReport empty_captured{};
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::ASCII, empty_captured), "");
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::JSON, empty_captured), "");

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
      GB::presentation::render_for_mode(UiMode::ASCII, populated_captured),
      "f12 Fighter CAPTURED!\n");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, populated_captured),
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
      GB::presentation::render_for_mode(UiMode::ASCII, exploded),
      "/Sol: Matter-antimatter EXPLOSION from overloaded crystal on B1 "
      "Battleship\n");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, exploded),
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
      GB::presentation::render_for_mode(UiMode::ASCII, damaged_crystal),
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
      GB::presentation::render_for_mode(UiMode::ASCII, ship_shot);
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
      GB::presentation::render_for_mode(UiMode::JSON, ship_shot),
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
      GB::presentation::render_for_mode(UiMode::ASCII, bombard_res),
      "B1 Battleship bombards /Sol/Terra [2]\n\t2 sectors destroyed\n");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, bombard_res),
      "\"type\":\"bombard_result\"");

  const MineDetonationReport mine_report{
      .ship_display = "M1 Mine",
      .orbit_display = "/Sol/Terra",
      .ship_victims = {},
      .planet_strike = bombard_res,
  };
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::ASCII, mine_report),
      "M1 Mine detonated at /Sol/Terra\n");
  test::expect_contains(
      GB::presentation::render_mine_planet_strike_telegram(mine_report),
      "2 sectors destroyed.");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, mine_report),
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

void test_bless_presentation() {
  using GB::presentation::BlessProperty;
  using GB::presentation::BlessResult;
  using GB::presentation::render_bless_notification;

  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::Password,
                                 .value = std::string{"sec"}}),
      "Deity changed your race password to `sec`\n");
  test::expect_eq(render_bless_notification(
                      {.target_player = 2, .property = BlessProperty::Pods}),
                  "Deity gave you pod ability.\n");
  test::expect_eq(
      render_bless_notification(
          {.target_player = 2, .property = BlessProperty::Water, .value = 60}),
      "Deity set your water preference to 60%\n");
  test::expect_eq(
      render_bless_notification(
          {.target_player = 2, .property = BlessProperty::Money, .value = 500}),
      "Deity gave you 500 money.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::Fertility,
                                 .value = 75}),
      "Deity gave you a fetilization ability of 75.\n");
  test::expect_eq(
      render_bless_notification(
          {.target_player = 2, .property = BlessProperty::Fight, .value = 80}),
      "Deity set your fighting ability to 80.\n");
  test::expect_eq(
      render_bless_notification(
          {.target_player = 2, .property = BlessProperty::MaxIq, .value = 200}),
      "Deity gave you a maximum IQ of 200.\n");
  test::expect_eq(
      render_bless_notification(
          {.target_player = 2, .property = BlessProperty::Mass, .value = 2.5}),
      "Deity gave you 2.50 mass.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::Adventurism,
                                 .value = 0.45}),
      "Deity gave you 45 % adventurism.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::ExploreBit,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity set your explored bit at /Sol/Earth.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::NoExploreBit,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity reset your explored bit at /Sol/Earth.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::PlanetPopulation,
                                 .value = 2500,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity set your population variable to 2500 at /Sol/Earth.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::Inhabited,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity has set your inhabited bit for /Sol/Earth.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::NumSectsOwned,
                                 .value = 42,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity set your \"numsectsowned\" variable at /Sol/Earth to 42.\n");
  test::expect_eq(
      render_bless_notification({.target_player = 2,
                                 .property = BlessProperty::Resources,
                                 .value = 100,
                                 .star_name = "Sol",
                                 .planet_name = "Earth"}),
      "Deity gave you 100 resources at Sol/Earth.\n");

  const BlessResult ap_res{.target_player = 2,
                           .property = BlessProperty::ActionPoints,
                           .value = 30,
                           .star_name = "Sol"};
  test::expect_eq(render_bless_notification(ap_res),
                  "Deity gave you 30 action points at Sol.\n");
  test::expect_eq(GB::presentation::render_for_mode(UiMode::ASCII, ap_res), "");
  const std::string json =
      GB::presentation::render_for_mode(UiMode::JSON, ap_res);
  test::expect_contains(json, "\"type\":\"bless\"");
  test::expect_contains(json, "\"target_player\":2");
  test::expect_contains(
      json, std::format("\"property\":{}",
                        std::to_underlying(BlessProperty::ActionPoints)));
  test::expect_contains(json, "\"value\":30");
}

void test_planet_turn_bulletins() {
  namespace pres = GB::presentation;

  test::expect_true(pres::render_recovery_report({}).empty());
  test::expect_true(
      pres::render_recovery_report({.shares = {pres::RecoveryShareRow{
                                        .player = 1, .race_name = "Alpha"}},
                                    .total_stolen = {}})
          .empty());

  const pres::RecoveryReportViewModel recovery_vm{
      .star_id = 1,
      .star_name = "Sol",
      .planet_name = "Earth",
      .planet_num = 1,
      .shares = {pres::RecoveryShareRow{
          .player = 1,
          .race_name = "Humanoid",
          .share = {.resources = 60, .destruct = 30, .fuel = 10, .crystals = 3},
      }},
      .total_stolen = {.resources = 60,
                       .destruct = 30,
                       .fuel = 10,
                       .crystals = 3},
  };
  const std::string recovery_ascii =
      pres::render_for_mode(UiMode::ASCII, recovery_vm);
  test::expect_contains(recovery_ascii, "Recovery Report: Planet /Sol/Earth\n");
  test::expect_contains(recovery_ascii, "Humanoid");
  test::expect_contains(recovery_ascii, "Total:");
  const std::string recovery_json =
      pres::render_for_mode(UiMode::JSON, recovery_vm);
  test::expect_contains(recovery_json, "\"type\":\"recovery_report\"");
  test::expect_contains(recovery_json, "\"race_name\":\"Humanoid\"");

  test::expect_eq(
      pres::render_planet_turn_autoreport(
          {.star_name = "Sol",
           .planet_name = "Mars",
           .production = {.resources = 5, .destruct = 1, .fuel = 2}}),
      "\nFrom /Sol/Mars\nTotal      Prod: 5r 2f 1d\n");

  const std::string nova_wet = pres::render_planet_nova_bulletin(
      {.star_name = "Sol", .planet_name = "Earth", .seas_boiling = true});
  test::expect_contains(nova_wet, "Seas and rivers are boiling!\n");
  test::expect_false(nova_wet.contains('~'));

  const std::string nova_dry = pres::render_planet_nova_bulletin(
      {.star_name = "Sol", .planet_name = "Mars", .seas_boiling = false});
  test::expect_false(nova_dry.contains("Seas and rivers are boiling!"));
  test::expect_false(nova_dry.contains('~'));

  test::expect_contains(
      pres::render_slave_revolt_bulletin(
          {.star_name = "Sol", .planet_name = "Earth", .former_master = 2}),
      "All population belonging to player #2 on the planet have been killed!");
}

}  // namespace

int main() {
  test_render_json_envelope_and_polymorphic_presenter();
  test_gameobj_ui_mode_and_present();
  test_bless_presentation();
  test_planet_turn_bulletins();
  return 0;
}
