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
  test::expect_contains(dispatched_json, R"("type":"message")");
  test::expect_contains(dispatched_json, "Total Distance = 42.50");

  static_assert(std::is_aggregate_v<CapturedShipEvent>);
  static_assert(std::is_aggregate_v<CapturedShipsReport>);

  const CapturedShipsReport empty_captured{};
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::ASCII, empty_captured), "");
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::JSON, empty_captured), "");

  const OrderUpdate empty_order_update{};
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::ASCII, empty_order_update), "");
  test::expect_eq(
      GB::presentation::render_for_mode(UiMode::JSON, empty_order_update), "");

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
      R"("type":"message")");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, populated_captured),
      "f12 Fighter CAPTURED!");

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
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), "Total Distance = 10.00");

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

void test_ship_combat_turn_bulletins() {
  namespace pres = GB::presentation;

  test::expect_eq(
      pres::render_spore_pod_explosion({.pod_display = "p1 Pod",
                                        .orbit_path = "/Sol",
                                        .colonized_planet_name = "Earth"}),
      "p1 Pod has warmed and exploded at /Sol\n\tmeta-colony "
      "established on Earth.");
  test::expect_eq(pres::render_spore_pod_explosion(
                      {.pod_display = "p1 Pod", .orbit_path = "/Sol"}),
                  "p1 Pod has warmed and exploded at /Sol\n\tno spores have "
                  "survived.");
  test::expect_eq(pres::render_spore_pod_decay("p1 Pod", "/Sol/Earth"),
                  "p1 Pod has decayed at /Sol/Earth\n");
  test::expect_contains(
      pres::render_atmosphere_device_dissipated("/Sol/Earth", false),
      "Canister of dust");
  test::expect_contains(
      pres::render_atmosphere_device_dissipated("/Sol/Earth", true),
      "Greenhouse gases");

  test::expect_eq(pres::render_mirror_ship_strike({.mirror_display = "m1 Lens",
                                                   .target_display = "s2 Craft",
                                                   .damage_applied = 0,
                                                   .destroyed = false}),
                  "m1 Lens aimed at s2 Craft\n");
  test::expect_eq(pres::render_mirror_ship_strike({.mirror_display = "m1 Lens",
                                                   .target_display = "s2 Craft",
                                                   .damage_applied = 45,
                                                   .destroyed = true}),
                  "m1 Lens aimed at s2 Craft\n45% damage done.\ns2 Craft "
                  "DESTROYED!!!\n");

  const BombardResult bombard_res{.ship_display = "B9 Nemesis",
                                  .location_display = "/Sol/Earth",
                                  .previous_sector_owner = 2,
                                  .sectors_destroyed = 3};
  test::expect_eq(
      pres::render_missile_planet_drop_telegram(bombard_res, "/Sol/Earth"),
      "B9 Nemesis dropped on /Sol/Earth.\n\t3 sectors destroyed.\n");
  test::expect_eq(
      pres::render_missile_planet_drop_news(bombard_res, "/Sol/Earth"),
      "B9 Nemesis dropped on /Sol/Earth.\n");

  const pres::BerserkerBombardNotice bers{.ship_number = 9,
                                          .ship_letter = 'B',
                                          .ship_name = "Nemesis",
                                          .attacker = 1,
                                          .star_name = "Sol",
                                          .planet_name = "Earth",
                                          .target = {4, 5},
                                          .old_owner = 2,
                                          .sectors_destroyed = 3};
  test::expect_contains(
      pres::render_berserker_bombard_owner_report(bers, bombard_res),
      "sector 4,5 (owner 2). 3 sectors destroyed.\n");
  test::expect_contains(pres::render_berserker_bombard_victim_alert(bers),
                        "B9 Nemesis bombarded sector 4,5; 3 sectors destroyed");
  test::expect_eq(pres::render_berserker_bombard_news(bers),
                  "B9 Nemesis [1] bombards Sol/Earth\n");
  test::expect_contains(pres::render_berserker_pdn_cancelled("/Sol/Earth"),
                        "PDNs are present");
  test::expect_contains(pres::render_berserker_saturation_report(bers),
                        "Planet /Sol/Earth has been saturation bombed.");
  test::expect_contains(pres::render_berserker_no_weapons(bers),
                        "B9 Nemesis has no weapons to bombard with.");

  test::expect_eq(pres::render_vn_theft_telegram({.amount = 20,
                                                  .victim = 2,
                                                  .ship_letter = 'V',
                                                  .ship_number = 7,
                                                  .orbit_path = "/Sol/Earth"}),
                  "20 resources stolen from [2] by V7 at /Sol/Earth.");
  test::expect_eq(
      pres::render_berserker_constructed_telegram("V7 101", "B8 Fury"),
      "V7 101 constructed B8 Fury.");
  test::expect_eq(
      pres::render_star_ground_assault_news({.star_name = "Sol",
                                             .attacker_name = "Terrans",
                                             .attacker = 1,
                                             .defender_name = "Martians",
                                             .defender = 2,
                                             .assaults = 4}),
      "Sol: Terrans [1] assaults Martians [2] 4 times.\n");
}

void test_empire_turn_bulletins() {
  namespace pres = GB::presentation;
  const pres::MarketLotSettlementNotice settlement{.lot = 12,
                                                   .type = CommodType::FUEL,
                                                   .amount = 50,
                                                   .bid = 300,
                                                   .seller = 2,
                                                   .seller_name = "VegaCorp",
                                                   .buyer = 1,
                                                   .buyer_name = "SolEmpire",
                                                   .star_name = "Sol",
                                                   .planet_name = "Earth"};
  test::expect_eq(pres::render_market_purchase_telegram(settlement),
                  "Lot 12 purchased from VegaCorp [2] at a cost of 300.\n"
                  "   50 fuel arrived at /Sol/Earth\n");
  test::expect_eq(pres::render_market_sale_telegram(settlement),
                  "Lot 12 (50 fuel) sold to SolEmpire [1] at a cost of 300.\n");

  test::expect_eq(pres::render_star_nova_bulletin("Betelgeuse", true),
                  "***** BULLETIN! ******\n\n  Scientists report that star "
                  "Betelgeuse\nis undergoing nova.\n");
  test::expect_eq(pres::render_star_nova_bulletin("Betelgeuse", false),
                  "Notice\n\n  Scientists report that star Betelgeuse\nis no "
                  "longer undergoing nova.\n");

  const std::string single_victory = pres::render_victory_announcement(
      {.big_winners = {{.player = 1, .name = "SolEmpire"}}});
  test::expect_contains(single_victory,
                        "*** Attention ***\n"
                        "This game of Galactic Bloodshed is now *over*\n"
                        "The big winner is\n"
                        "*** [ 1] SolEmpire");
  test::expect_false(single_victory.contains("Lesser winners:"));

  const std::string multi_victory = pres::render_victory_announcement(
      {.big_winners = {{.player = 1, .name = "Alpha"},
                       {.player = 2, .name = "Beta"}},
       .lesser_winners = {{.player = 3, .name = "Gamma"}}});
  test::expect_contains(multi_victory, "The big winners are\n");
  test::expect_contains(multi_victory, "*** [ 1] Alpha");
  test::expect_contains(multi_victory, "*** [ 2] Beta");
  test::expect_contains(multi_victory, "Lesser winners:\n+++ [ 3] Gamma");
}

void test_detect_terminal_capabilities() {
  using GB::presentation::detect_terminal_capabilities;
  test::expect_false(detect_terminal_capabilities("").ansi_sgr);
  test::expect_false(detect_terminal_capabilities("dumb").ansi_sgr);
  const auto xterm = detect_terminal_capabilities("XTERM-256color");
  test::expect_true(xterm.ansi_sgr && xterm.color && xterm.utf8);
  const auto vt = detect_terminal_capabilities("vt100");
  test::expect_true(vt.ansi_sgr && !vt.color);
  const auto unknown = detect_terminal_capabilities("adm3a");
  test::expect_false(unknown.ansi_sgr || unknown.color);
}

void test_canonical_session_message_error_views() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  const GB::presentation::SessionView session_vm{
      .player = player_t{1},
      .governor = governor_t{2},
      .race_name = "Terran",
      .governor_name = "Admiral",
      .god = false,
      .ap = 50,
      .global_ap = 100,
      .scope =
          GB::presentation::SessionScopeView{
              .level = ScopeLevel::LEVEL_PLAN,
              .snum = 1,
              .pnum = 2,
              .star = "Sol",
              .planet = "Mars",
              .path = "/Sol/Mars",
          },
      .viewport =
          GB::presentation::SessionViewportView{
              .universe_center = {10.0, 20.0},
              .system_center = {5.0, 15.0},
              .universe_zoom = 1.0,
              .system_zoom = 2.0,
          },
  };

  test::expect_eq(GB::presentation::render_session_prompt(session_vm),
                  " ( [50] /Sol/Mars )\n");
  test::expect_contains(GB::presentation::render_json_session(session_vm),
                        "\"type\":\"session\"");
  test::expect_eq(GB::presentation::render_for_mode(UiMode::ASCII, session_vm),
                  " ( [50] /Sol/Mars )\n");
  test::expect_contains(
      GB::presentation::render_for_mode(UiMode::JSON, session_vm),
      "\"type\":\"session\"");

  g.present(session_vm);
  test::expect_eq(g.out.str(), " ( [50] /Sol/Mars )\n");

  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  g.present(session_vm);
  test::expect_contains(g.out.str(), "\"type\":\"session\"");
  test::expect_contains(g.out.str(), "\"race_name\":\"Terran\"");
  test::expect_contains(g.out.str(), "\"path\":\"/Sol/Mars\"");

  const auto msg_vm =
      GB::presentation::make_output_message("Order executed.\n");
  test::expect_eq(msg_vm.channel, GB::presentation::MessageChannel::Output);
  test::expect_eq(msg_vm.message, "Order executed.\n");
  test::expect_eq(GB::presentation::render_message(msg_vm),
                  "Order executed.\n");
  test::expect_contains(GB::presentation::render_json_message(msg_vm),
                        "\"type\":\"message\"");
  test::expect_eq(GB::presentation::render_for_mode(UiMode::ASCII, msg_vm),
                  "Order executed.\n");
  test::expect_contains(GB::presentation::render_for_mode(UiMode::JSON, msg_vm),
                        "\"type\":\"message\"");

  g.out.str("");
  g.set_ui_mode(UiMode::ASCII);
  g.present(msg_vm);
  test::expect_eq(g.out.str(), "Order executed.\n");

  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  g.present(msg_vm);
  test::expect_contains(g.out.str(), "\"type\":\"message\"");
  test::expect_contains(g.out.str(), "\"message\":\"Order executed.\\n\"");

  const GB::presentation::ErrorView err_vm{
      .code = "INVALID_SCOPE",
      .command = "upgrade",
      .message = "Cannot upgrade at star scope.\n",
  };
  test::expect_eq(GB::presentation::render_error(err_vm),
                  "Cannot upgrade at star scope.\n");
  test::expect_contains(GB::presentation::render_json_error(err_vm),
                        "\"type\":\"error\"");
  test::expect_eq(GB::presentation::render_for_mode(UiMode::ASCII, err_vm),
                  "Cannot upgrade at star scope.\n");
  test::expect_contains(GB::presentation::render_for_mode(UiMode::JSON, err_vm),
                        "\"type\":\"error\"");

  g.out.str("");
  g.set_ui_mode(UiMode::ASCII);
  g.present(err_vm);
  test::expect_eq(g.out.str(), "Cannot upgrade at star scope.\n");

  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  g.present(err_vm);
  test::expect_contains(g.out.str(), "\"type\":\"error\"");
  test::expect_contains(g.out.str(), "\"code\":\"INVALID_SCOPE\"");
  test::expect_contains(g.out.str(), "\"command\":\"upgrade\"");
}

void test_canonical_star_planet_sector_projectors() {
  TestContext ctx;
  ctx.with_standard_universe();

  const ScopeContext viewer1{
      .player = 1,
      .governor = 0,
      .god = false,
  };
  const ScopeContext god_viewer{
      .player = 1,
      .governor = 0,
      .god = true,
  };

  // 0. ScopeContext::can_see combinator (deity always sees; mortal delegates)
  test::expect_true(viewer1.can_see(true));
  test::expect_false(viewer1.can_see(false));
  test::expect_true(god_viewer.can_see(true));
  test::expect_true(god_viewer.can_see(false));

  // 1. project_star_view
  // Mark Star 2 (Vega) and Vega Prime unexplored by Player 1:
  ctx.em.mutate_star(2, [](Star& s) { s.clear_explored_by(1); });
  ctx.em.mutate_planet(2, 1, [](Planet& p) {
    p.explored() = false;
    p.info(1).explored = false;
  });

  // Unexplored star for mortal (Vega = 2)
  const auto sv_unexplored =
      GB::presentation::project_star_view(ctx.em, viewer1, 2);
  test::expect_eq(sv_unexplored.snum, 2);
  test::expect_eq(sv_unexplored.name, "Vega");
  test::expect_false(sv_unexplored.explored);
  test::expect_false(sv_unexplored.details.has_value());
  const std::string sv_json_unexp =
      GB::presentation::render_json_star_view(sv_unexplored);
  test::expect_contains(sv_json_unexp, R"("type":"star")");
  test::expect_contains(sv_json_unexp, R"("name":"Vega")");
  test::expect_contains(sv_json_unexp, R"("explored":false)");

  // Explored star for mortal with low tech (Sol = 1, tech = 5 <
  // TECH_SEE_STABILITY)
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 5.0; });
  const auto sv_low_tech =
      GB::presentation::project_star_view(ctx.em, viewer1, 1);
  test::expect_true(sv_low_tech.explored);
  test::expect_true(sv_low_tech.details.has_value());
  test::expect_false(sv_low_tech.details->stability.has_value());
  test::expect_eq(sv_low_tech.details->planets.size(), 1);

  // Explored star for mortal with high tech (Sol = 1, tech = 20 >=
  // TECH_SEE_STABILITY)
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 20.0; });
  const auto sv_high_tech =
      GB::presentation::project_star_view(ctx.em, viewer1, 1);
  test::expect_true(sv_high_tech.details->stability.has_value());
  const std::string sv_json_exp =
      GB::presentation::render_json_star_view(sv_high_tech);
  test::expect_contains(sv_json_exp, R"("type":"star")");
  test::expect_contains(sv_json_exp, R"("explored":true)");

  // 2. project_planet_view
  // Unseen planet for mortal (Vega Prime = star 2, planet 1)
  const auto pv_blind =
      GB::presentation::project_planet_view(ctx.em, viewer1, 2, 1);
  test::expect_eq(pv_blind.snum, 2);
  test::expect_eq(pv_blind.pnum, 1);
  test::expect_eq(pv_blind.star_name, "Vega");
  test::expect_eq(pv_blind.name, "Vega Prime");
  test::expect_false(pv_blind.explored);
  test::expect_false(pv_blind.environment.has_value());
  test::expect_false(pv_blind.colony.has_value());
  const std::string pv_json_blind =
      GB::presentation::render_json_planet_view(pv_blind);
  test::expect_contains(pv_json_blind, R"("type":"planet")");
  test::expect_contains(pv_json_blind, R"("explored":false)");

  // Orbiting ship sight on unexplored planet (place ship in orbit of Vega
  // Prime)
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 5.0; });  // < TECH_EXPLORE
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1)
      .in_planet_orbit(2, 1)
      .with_crew(10, 0)
      .build();

  const auto pv_orbit =
      GB::presentation::project_planet_view(ctx.em, viewer1, 2, 1);
  test::expect_false(pv_orbit.explored);
  test::expect_true(pv_orbit.environment.has_value());
  test::expect_true(pv_orbit.environment->aliens_unknown);
  test::expect_true(pv_orbit.environment->aliens.empty());
  test::expect_false(pv_orbit.colony.has_value());

  // Orbiting ship sight on unexplored planet with high tech (reveals aliens)
  ctx.em.mutate_race(1, [](Race& r) { r.tech = 15.0; });  // >= TECH_EXPLORE
  const auto pv_orbit_high_tech =
      GB::presentation::project_planet_view(ctx.em, viewer1, 2, 1);
  test::expect_false(pv_orbit_high_tech.environment->aliens_unknown);

  // Mortal with numsectsowned > 0 on an unexplored planet
  ctx.em.mutate_planet(2, 1, [](Planet& p) {
    p.info(1).explored = false;
    p.info(1).numsectsowned = 1;
  });
  const auto pv_unexp_owned =
      GB::presentation::project_planet_view(ctx.em, viewer1, 2, 1);
  test::expect_true(pv_unexp_owned.environment.has_value());
  test::expect_true(pv_unexp_owned.colony.has_value());
  ctx.em.mutate_planet(2, 1, [](Planet& p) { p.info(1).numsectsowned = 0; });

  // Explored & colonized planet (Earth = star 1, planet 1)
  // Without crystal discovery:
  ctx.em.mutate_race(1, [](Race& r) {
    r.discoveries.crystal = false;
    r.tech = TECH_EXPLORE;
  });
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{0, 0}).set_crystals(5);
  });
  const auto pv_no_crystal =
      GB::presentation::project_planet_view(ctx.em, viewer1, 1, 1);
  test::expect_true(pv_no_crystal.environment.has_value());
  test::expect_false(pv_no_crystal.environment->crystal_sectors.has_value());
  test::expect_true(pv_no_crystal.colony.has_value());

  // With crystal discovery, routes, and alien presence:
  ctx.em.mutate_race(1, [](Race& r) {
    r.discoveries.crystal = true;
    r.declare_war_on(2);
  });
  // Add active route, inactive configured route, route without planet, and
  // route without star:
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.mark_explored_by(1);
    auto& rt1 = p.info(1).route_at(1);
    rt1.set = true;
    rt1.set_destination(2, 1);
    rt1.dest_coords = Coordinates{3, 4};
    rt1.load = CommodityManifest::parse("fr");
    rt1.unload = CommodityManifest::parse("dx");

    auto& rt2 = p.info(1).route_at(2);
    rt2.set = false;
    rt2.set_destination(2, 1);
    rt2.dest_coords = Coordinates{5, 6};

    auto& rt3 = p.info(1).route_at(3);
    rt3.set = true;
    rt3.dest_star = 2;
    rt3.dest_planet = std::nullopt;

    auto& rt4 = p.info(1).route_at(4);
    rt4.set = true;
    rt4.dest_star = std::nullopt;
    rt4.dest_planet = std::nullopt;

    // Alien presence: player 2 owns 1 sector
    p.info(2).numsectsowned = 1;
  });
  ctx.em.mutate_sectormap(
      1, 1, [](SectorMap& smap) { smap.get(Coordinates{1, 0}).set_owner(2); });

  const auto pv_full =
      GB::presentation::project_planet_view(ctx.em, viewer1, 1, 1);
  test::expect_true(pv_full.explored);
  test::expect_true(pv_full.environment.has_value());
  test::expect_true(pv_full.environment->crystal_sectors.has_value());
  test::expect_eq(*pv_full.environment->crystal_sectors, 1);
  test::expect_false(pv_full.environment->aliens_unknown);
  test::expect_eq(pv_full.environment->aliens.size(), 1);
  test::expect_eq(pv_full.environment->aliens[0].player, 2);
  test::expect_true(pv_full.environment->aliens[0].at_war);

  test::expect_true(pv_full.colony.has_value());
  test::expect_eq(pv_full.colony->routes.size(), 4);
  test::expect_true(pv_full.colony->routes[0].active);
  test::expect_eq(pv_full.colony->routes[0].dest_star_name, "Vega");
  test::expect_eq(pv_full.colony->routes[0].dest_planet_name, "Vega Prime");
  test::expect_true(pv_full.colony->routes[0].load.fuel);
  test::expect_true(pv_full.colony->routes[0].load.resources);
  test::expect_true(pv_full.colony->routes[0].unload.destruct);
  test::expect_true(pv_full.colony->routes[0].unload.crystals);
  test::expect_false(pv_full.colony->routes[1].active);
  test::expect_true(pv_full.colony->routes[2].active);
  test::expect_eq(pv_full.colony->routes[2].dest_star_name, "Vega");
  test::expect_eq(pv_full.colony->routes[2].dest_planet_name, "");
  test::expect_true(pv_full.colony->routes[3].active);
  test::expect_eq(pv_full.colony->routes[3].dest_star_name, "");

  // Test neutral alien presence:
  ctx.em.mutate_race(1, [](Race& r) { r.make_peace_with(2); });
  const auto pv_neutral =
      GB::presentation::project_planet_view(ctx.em, viewer1, 1, 1);
  test::expect_false(pv_neutral.environment->aliens[0].at_war);

  const std::string pv_json_full =
      GB::presentation::render_json_planet_view(pv_full);
  test::expect_contains(pv_json_full, R"("type":"planet")");
  test::expect_contains(pv_json_full, R"("star_name":"Sol")");
  test::expect_contains(pv_json_full, R"("colony":)");
  test::expect_contains(pv_json_full, R"("routes":)");

  // 3. project_sector_grid_view
  // Unseen planet returns std::nullopt (Star 3, Planet 1 for mortal viewer1)
  ctx.em.mutate_planet(3, 1, [](Planet& p) {
    p.info(1).explored = false;
    p.info(1).numsectsowned = 0;
  });
  const auto grid_mortal_unseen =
      GB::presentation::project_sector_grid_view(ctx.em, viewer1, 3, 1);
  test::expect_false(grid_mortal_unseen.has_value());

  // Full grid on Earth (1, 1)
  const auto grid_full =
      GB::presentation::project_sector_grid_view(ctx.em, viewer1, 1, 1);
  test::expect_true(grid_full.has_value());
  test::expect_false(grid_full->partial);
  test::expect_eq(grid_full->snum, 1);
  test::expect_eq(grid_full->pnum, 1);

  // Sparse coordinate filter on Earth
  // Sector {0, 0} is owned by player 1 with crystals
  // Sector {1, 0} is owned by player 2 (foreign)
  // Sector {2, 0} is unowned (set owner to 0)
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{2, 0}).set_owner(0);
    smap.get(Coordinates{2, 0}).set_crystals(0);
  });
  // Add landed visible ship at {0, 0}
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1)
      .landed_on(1, 1, Coordinates{0, 0})
      .with_crew(10, 0)
      .build();
  // Add orbiting (unlanded) ship at planet 1, 1
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1)
      .in_planet_orbit(1, 1)
      .with_crew(10, 0)
      .build();
  // Add foreign invisible landed canister at {0, 0}
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
      .owned_by(2)
      .landed_on(1, 1, Coordinates{0, 0})
      .build();

  const std::flat_set<Coordinates> filter{Coordinates{0, 0}, Coordinates{1, 0},
                                          Coordinates{2, 0}};
  const auto grid_filtered = GB::presentation::project_sector_grid_view(
      ctx.em, viewer1, 1, 1, &filter);
  test::expect_true(grid_filtered.has_value());
  test::expect_true(grid_filtered->partial);
  test::expect_eq(grid_filtered->sectors.size(), 3);

  // Sector {0, 0}: owned by player 1, has landed ship, has crystals
  const auto& sec0 = grid_filtered->sectors[0];
  test::expect_eq(sec0.coords, (Coordinates{0, 0}));
  test::expect_eq(sec0.owner, 1);
  test::expect_eq(sec0.ships.size(), 1);
  test::expect_eq(sec0.ships[0].owner, 1);
  test::expect_true(sec0.details.has_value());
  test::expect_true(sec0.details->crystals);
  test::expect_true(sec0.details->mobilization.has_value());

  // Sector {1, 0}: owned by player 2 (foreign) -> mobilization hidden from
  // mortal
  const auto& sec1 = grid_filtered->sectors[1];
  test::expect_eq(sec1.coords, (Coordinates{1, 0}));
  test::expect_eq(sec1.owner, 2);
  test::expect_false(sec1.details->mobilization.has_value());

  // Sector {2, 0}: unowned -> owner nullopt, mobilization nullopt, crystals
  // false
  const auto& sec2 = grid_filtered->sectors[2];
  test::expect_eq(sec2.coords, (Coordinates{2, 0}));
  test::expect_false(sec2.owner.has_value());
  test::expect_false(sec2.details->mobilization.has_value());
  test::expect_false(sec2.details->crystals);

  // JSON rendering of sector grid view
  const std::string grid_json =
      GB::presentation::render_json_sector_grid_view(*grid_filtered);
  test::expect_contains(grid_json, R"("type":"sectors")");
  test::expect_contains(grid_json, R"("partial":true)");
  test::expect_contains(grid_json, R"("sectors":)");
}

void test_canonical_ship_view_projectors() {
  TestContext ctx;
  ctx.with_standard_universe();

  const ScopeContext viewer1{
      .player = 1,
      .governor = 0,
      .god = false,
  };

  // Add active ships with crew for player 1 at Sol (star scope) and Earth
  // (planet scope)
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1)
      .in_star_orbit(1)
      .with_crew(10, 0)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
      .owned_by(1)
      .landed_on(1, 1, Coordinates{0, 0})
      .with_crew(10, 0)
      .build();

  // 1. SightContext memoization test (star and planet cache hit/miss)
  GB::presentation::SightContext sight;
  // Star sight miss then hit
  test::expect_true(sight.has_star_sight(ctx.em, 1, 1));
  test::expect_true(sight.has_star_sight(ctx.em, 1, 1));
  // Planet sight miss then hit
  test::expect_true(sight.has_planet_sight(ctx.em, 1, 1, 1));
  test::expect_true(sight.has_planet_sight(ctx.em, 1, 1, 1));

  // 2. project_deleted_ship_view
  const auto deleted_view = GB::presentation::project_deleted_ship_view(99);
  test::expect_eq(deleted_view.id, 99);
  test::expect_false(deleted_view.alive);
  test::expect_false(deleted_view.location.has_value());
  test::expect_false(deleted_view.internal.has_value());

  // 3. Dead ship returns deleted_view
  const Ship dead_ship{ship_struct{.number = 99, .alive = false}};
  const auto dead_view =
      GB::presentation::project_ship_view(ctx.em, viewer1, dead_ship);
  test::expect_true(dead_view.has_value());
  test::expect_false(dead_view->alive);
  test::expect_eq(dead_view->id, 99);

  // 4. Contact visibility:
  const auto s1_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                         .owned_by(1)
                         .in_star_orbit(1)
                         .build();

  // Deep space foreign ship (whatorbits == LEVEL_UNIV) -> invisible to foreign
  // mortal
  const auto univ_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                                .owned_by(2)
                                .in_deep_space()
                                .build();
  const auto univ_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(univ_ship_id));
  test::expect_false(univ_proj.has_value());

  // Foreign visible ship in system orbit with star sight -> visible
  const auto foreign_star_ship_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(2)
          .in_star_orbit(1)
          .build();
  const auto foreign_star_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(foreign_star_ship_id), &sight);
  test::expect_true(foreign_star_proj.has_value());
  test::expect_eq(foreign_star_proj->id, foreign_star_ship_id);
  // Foreign ship: internal view must be nullopt
  test::expect_false(foreign_star_proj->internal.has_value());
  // Location view present (star scope)
  test::expect_true(foreign_star_proj->location.has_value());
  test::expect_eq(foreign_star_proj->location->level, ScopeLevel::LEVEL_STAR);
  test::expect_eq(foreign_star_proj->location->snum, starnum_t{1});
  test::expect_false(foreign_star_proj->location->pnum.has_value());

  // Foreign landed ship with planet sight (planet scope)
  const auto foreign_landed_ship_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(2)
          .landed_on(1, 1, Coordinates{0, 0})
          .with_crew(10, 0)
          .build();
  const auto foreign_landed_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(foreign_landed_ship_id), &sight);
  test::expect_true(foreign_landed_proj.has_value());
  test::expect_true(foreign_landed_proj->location->landed);
  test::expect_eq(foreign_landed_proj->location->level, ScopeLevel::LEVEL_PLAN);
  test::expect_eq(foreign_landed_proj->location->snum, starnum_t{1});
  test::expect_eq(foreign_landed_proj->location->pnum, planetnum_t{1});

  // Foreign orbiting ship at planet (is_landed() == false, whatorbits ==
  // LEVEL_PLAN)
  const auto foreign_orbit_ship_id =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(2)
          .in_planet_orbit(1, 1)
          .with_crew(10, 0)
          .build();
  const auto foreign_orbit_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(foreign_orbit_ship_id));
  test::expect_true(foreign_orbit_proj.has_value());
  test::expect_false(foreign_orbit_proj->location->landed);

  // Foreign invisible ship (e.g. canister) -> returns nullopt
  const auto foreign_invis_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                                    .owned_by(2)
                                    .landed_on(1, 1, Coordinates{0, 0})
                                    .build();
  const auto foreign_invis_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(foreign_invis_id), &sight);
  test::expect_false(foreign_invis_proj.has_value());

  // 5. SpaceMirrorShip targeting and orientation
  const auto mirror_id = TestShipBuilder(ctx.em, ShipType::STYPE_MIRROR)
                             .owned_by(1)
                             .in_star_orbit(1)
                             .build();
  ctx.em.mutate_as<SpaceMirrorShip>(
      mirror_id, [](SpaceMirrorShip& m) { m.aim_at_planet(1, 1); });
  const auto mirror_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(mirror_id));
  test::expect_true(mirror_proj.has_value());
  test::expect_true(mirror_proj->location->mirror_orientation.has_value());
  test::expect_false(
      foreign_star_proj->location->mirror_orientation.has_value());

  // 6. Own ship internal view (owner viewing):
  // - LEVEL_STAR destination
  ctx.em.mutate_ship(s1_id, [](Ship& s) { s.set_star_destination(1); });
  const auto s1_proj_star = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(s1_id));
  test::expect_true(s1_proj_star.has_value());
  test::expect_true(s1_proj_star->internal.has_value());
  test::expect_eq(s1_proj_star->internal->navigation.dest_level,
                  ScopeLevel::LEVEL_STAR);
  test::expect_eq(s1_proj_star->internal->navigation.dest_star, starnum_t{1});
  test::expect_false(
      s1_proj_star->internal->navigation.dest_planet.has_value());

  // - LEVEL_PLAN destination
  ctx.em.mutate_ship(s1_id, [](Ship& s) { s.set_planet_destination(1, 1); });
  const auto s1_proj_plan = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(s1_id));
  test::expect_eq(s1_proj_plan->internal->navigation.dest_level,
                  ScopeLevel::LEVEL_PLAN);
  test::expect_eq(s1_proj_plan->internal->navigation.dest_star, starnum_t{1});
  test::expect_eq(s1_proj_plan->internal->navigation.dest_planet,
                  planetnum_t{1});

  // - LEVEL_UNIV destination
  ctx.em.mutate_ship(s1_id, [](Ship& s) { s.clear_destination(); });
  const auto s1_proj_univ = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(s1_id));
  test::expect_eq(s1_proj_univ->internal->navigation.dest_level,
                  ScopeLevel::LEVEL_UNIV);
  test::expect_false(s1_proj_univ->internal->navigation.dest_star.has_value());
  test::expect_false(
      s1_proj_univ->internal->navigation.dest_planet.has_value());

  // Own deep space ship (whatorbits == LEVEL_UNIV) -> snum and pnum nullopt
  const auto own_univ_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                               .owned_by(1)
                               .in_deep_space()
                               .build();
  const auto own_univ_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(own_univ_id));
  test::expect_true(own_univ_proj.has_value());
  test::expect_eq(own_univ_proj->location->level, ScopeLevel::LEVEL_UNIV);
  test::expect_false(own_univ_proj->location->snum.has_value());
  test::expect_false(own_univ_proj->location->pnum.has_value());

  // 7. Factory blueprints:
  // - Non-factory ship has factory == nullopt
  test::expect_false(s1_proj_univ->internal->factory.has_value());

  // - Factory unconfigured with STYPE_POD
  const auto factory_pod_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                                  .owned_by(1)
                                  .in_star_orbit(1)
                                  .build();
  ctx.em.mutate_ship(factory_pod_id,
                     [](Ship& s) { s.build_type() = ShipType::STYPE_POD; });
  const auto fact_pod_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(factory_pod_id));
  test::expect_true(fact_pod_proj.has_value());
  test::expect_true(fact_pod_proj->internal.has_value());
  test::expect_true(fact_pod_proj->internal->factory.has_value());
  test::expect_false(fact_pod_proj->internal->factory->configured);

  // - Factory unconfigured with OTYPE_FACTORY
  ctx.em.mutate_ship(factory_pod_id,
                     [](Ship& s) { s.build_type() = ShipType::OTYPE_FACTORY; });
  const auto fact_fact_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(factory_pod_id));
  test::expect_false(fact_fact_proj->internal->factory->configured);

  // - Factory configured with STYPE_CRUISER
  ctx.em.mutate_ship(factory_pod_id,
                     [](Ship& s) { s.build_type() = ShipType::STYPE_CRUISER; });
  const auto fact_cruiser_proj = GB::presentation::project_ship_view(
      ctx.em, viewer1, *ctx.em.peek_ship(factory_pod_id));
  test::expect_true(fact_cruiser_proj->internal->factory->configured);
  test::expect_eq(fact_cruiser_proj->internal->factory->build_type,
                  ShipType::STYPE_CRUISER);

  // 8. JSON rendering of ShipView
  const std::string json_ship =
      GB::presentation::render_json_ship_view(*fact_cruiser_proj);
  test::expect_contains(json_ship, R"("type":"ship")");
  test::expect_contains(json_ship, R"("alive":true)");
  test::expect_contains(json_ship, R"("location":)");
  test::expect_contains(json_ship, R"("internal":)");
  test::expect_contains(json_ship, R"("factory":)");

  const std::string json_deleted =
      GB::presentation::render_json_ship_view(deleted_view);
  test::expect_contains(json_deleted, R"("type":"ship")");
  test::expect_contains(json_deleted, R"("alive":false)");
}

void test_canonical_race_block_market_combat_views() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_power(1, [](Power& p) {
    p = Power{{
        .id = 1,
        .troops = 1000,
        .popn = 50000,
        .resource = 5000,
        .fuel = 2500,
        .destruct = 100,
        .ships_owned = 15,
        .planets_owned = 3,
        .money = 20000,
    }};
  });
  ctx.em.mutate_power(2, [](Power& p) {
    p = Power{{
        .id = 2,
        .troops = 2000,
        .popn = 60000,
        .resource = 7000,
        .fuel = 3000,
        .destruct = 500,
        .ships_owned = 25,
        .planets_owned = 4,
        .money = 15000,
    }};
  });

  ctx.em.mutate_block(1, [](Block& b) {
    b = Block{{
        .Playernum = 1,
        .name = "FederationBloc",
        .motto = "Peace and exploration",
        .invited = {player_t{1}, player_t{2}},
        .pledged = {player_t{1}},
        .popn = 80000,
        .resource = 9000,
        .fuel = 4000,
        .destruct = 200,
        .ships_owned = 25,
        .systems_owned = 5,
        .VPs = 120,
        .money = 35000,
    }};
  });
  ctx.em.mutate_block(2, [](Block& b) {
    b = Block{{
        .Playernum = 2,
        .name = "KlingonEmpire",
        .motto = "Honor in combat",
        .invited = {player_t{2}},
        .pledged = {player_t{2}},
        .popn = 60000,
        .resource = 7000,
        .fuel = 3000,
        .destruct = 500,
        .ships_owned = 30,
        .systems_owned = 4,
        .VPs = 95,
        .money = 15000,
    }};
  });

  const ScopeContext viewer1{
      .player = 1,
      .governor = 0,
      .god = false,
  };

  // 1. project_race_view for self (player 1 inspecting player 1)
  // First with Gov_ship unset (nullopt / 0)
  auto race1_view = GB::presentation::project_race_view(ctx.em, viewer1, 1,
                                                        std::optional<int>{1});
  test::expect_eq(race1_view.player, 1);
  test::expect_eq(race1_view.name, "Federation");
  test::expect_false(race1_view.dissolved);
  test::expect_false(race1_view.is_god);
  test::expect_false(race1_view.is_guest);
  test::expect_eq(race1_view.know_pct, 100);
  test::expect_eq(race1_view.you_to_them,
                  GB::presentation::DiplomaticRelation::Neutral);
  test::expect_eq(race1_view.them_to_you,
                  GB::presentation::DiplomaticRelation::Neutral);

  // Verify self details when Gov_ship == nullopt
  test::expect_true(race1_view.self.has_value());
  test::expect_false(race1_view.self->gov_ship.has_value());
  test::expect_false(race1_view.self->governors.empty());
  test::expect_eq(race1_view.self->governors[0].id, 1);

  // Exact stats for self
  test::expect_true(race1_view.stats.birthrate.has_value());
  test::expect_true(race1_view.stats.conditions.has_value());
  test::expect_true(race1_view.stats.best_sector.has_value());

  // Exact standings for self
  test::expect_true(race1_view.standings.has_value());
  test::expect_eq(race1_view.standings->rank, 1);
  test::expect_eq(race1_view.standings->troops, 1000);
  test::expect_eq(race1_view.standings->popn, 50000);
  test::expect_eq(race1_view.standings->money, 20000);
  test::expect_eq(race1_view.standings->ships, 15);
  test::expect_eq(race1_view.standings->planets, 3);
  test::expect_eq(race1_view.standings->resource, 5000);
  test::expect_eq(race1_view.standings->fuel, 2500);
  test::expect_eq(race1_view.standings->destruct, 100);

  // Now set Gov_ship != 0 with an actual ship in tbl_ship
  const auto gov_ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                               .owned_by(1)
                               .in_star_orbit(1)
                               .build();
  ctx.em.mutate_race(1, [gov_ship_id](Race& r) { r.Gov_ship = gov_ship_id; });
  race1_view = GB::presentation::project_race_view(ctx.em, viewer1, 1,
                                                   std::optional<int>{1});
  test::expect_true(race1_view.self->gov_ship.has_value());
  test::expect_eq(race1_view.self->gov_ship.value(), gov_ship_id);

  // 2. project_race_view for foreign mortal at different translation thresholds
  // Target race 2 has Metamorph set
  ctx.em.mutate_race(2, [](Race& r) {
    r.Metamorph = true;
    r.likesbest = SectorType::SEC_LAND;
  });

  // A. know <= 10
  ctx.em.mutate_race(1, [](Race& r) { r.translate[2] = Percentage{5}; });
  const auto race2_know5 =
      GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_false(race2_know5.self.has_value());
  test::expect_false(race2_know5.stats.is_metamorph.has_value());
  test::expect_false(race2_know5.stats.best_sector.has_value());
  test::expect_false(race2_know5.stats.conditions.has_value());
  test::expect_false(race2_know5.stats.fertilize.has_value());
  test::expect_false(race2_know5.stats.birthrate.has_value());
  test::expect_false(race2_know5.stats.temp.has_value());
  test::expect_false(race2_know5.stats.mass.has_value());
  test::expect_false(race2_know5.stats.tech.has_value());
  test::expect_true(race2_know5.standings.has_value());
  test::expect_false(race2_know5.standings->troops.has_value());

  // B. 10 < know <= 50
  ctx.em.mutate_race(1, [](Race& r) { r.translate[2] = Percentage{30}; });
  const auto race2_know30 =
      GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_false(race2_know30.self.has_value());
  test::expect_false(race2_know30.stats.is_metamorph.has_value());
  test::expect_false(race2_know30.stats.best_sector.has_value());
  test::expect_true(race2_know30.stats.conditions.has_value());
  test::expect_true(race2_know30.stats.fertilize.has_value());
  test::expect_true(race2_know30.stats.birthrate.has_value());
  test::expect_true(race2_know30.stats.temp.has_value());
  test::expect_true(race2_know30.stats.mass.has_value());
  test::expect_true(race2_know30.stats.tech.has_value());
  test::expect_true(race2_know30.standings->troops.has_value());

  // C. 50 < know <= 80
  ctx.em.mutate_race(1, [](Race& r) { r.translate[2] = Percentage{60}; });
  const auto race2_know60 =
      GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_true(race2_know60.stats.is_metamorph.has_value());
  test::expect_true(race2_know60.stats.is_metamorph.value());
  test::expect_false(race2_know60.stats.best_sector.has_value());

  // D. know > 80
  ctx.em.mutate_race(1, [](Race& r) { r.translate[2] = Percentage{90}; });
  const auto race2_know90 =
      GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_true(race2_know90.stats.is_metamorph.has_value());
  test::expect_true(race2_know90.stats.best_sector.has_value());
  test::expect_eq(race2_know90.stats.best_sector.value(), SectorType::SEC_LAND);

  // 3. Diplomatic stances: Neutral, Allied, AtWar
  // Viewer declares alliance with player 2
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(2); });
  auto dip_view = GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_eq(dip_view.you_to_them,
                  GB::presentation::DiplomaticRelation::Allied);
  test::expect_eq(dip_view.them_to_you,
                  GB::presentation::DiplomaticRelation::Neutral);

  // Player 2 declares war on viewer
  ctx.em.mutate_race(2, [](Race& r) { r.declare_war_on(1); });
  dip_view = GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_eq(dip_view.you_to_them,
                  GB::presentation::DiplomaticRelation::Allied);
  test::expect_eq(dip_view.them_to_you,
                  GB::presentation::DiplomaticRelation::AtWar);

  // Viewer declares war on player 2 (rescinding alliance first)
  ctx.em.mutate_race(1, [](Race& r) {
    r.rescind_alliance_with(2);
    r.declare_war_on(2);
  });
  dip_view = GB::presentation::project_race_view(ctx.em, viewer1, 2);
  test::expect_eq(dip_view.you_to_them,
                  GB::presentation::DiplomaticRelation::AtWar);
  test::expect_eq(dip_view.them_to_you,
                  GB::presentation::DiplomaticRelation::AtWar);

  // 4. project_block_view
  // Self block (id = 1)
  const auto block1_view =
      GB::presentation::project_block_view(ctx.em, viewer1, 1);
  test::expect_eq(block1_view.block_id, 1);
  test::expect_eq(block1_view.name, "FederationBloc");
  test::expect_eq(block1_view.motto, "Peace and exploration");
  test::expect_eq(block1_view.know_pct, 100);
  test::expect_eq(block1_view.members, std::vector<player_t>{1});
  test::expect_eq(block1_view.invited, (std::vector<player_t>{1, 2}));
  test::expect_eq(block1_view.pledged, std::vector<player_t>{1});
  test::expect_true(block1_view.standings.has_value());
  test::expect_eq(block1_view.standings->victory_score, 120);
  test::expect_eq(block1_view.standings->money, 35000);
  test::expect_eq(block1_view.standings->popn, 80000);
  test::expect_eq(block1_view.standings->ships, 25);
  test::expect_eq(block1_view.standings->planets, 5);
  test::expect_eq(block1_view.standings->resource, 9000);
  test::expect_eq(block1_view.standings->fuel, 4000);
  test::expect_eq(block1_view.standings->destruct, 200);
  test::expect_false(block1_view.standings->troops.has_value());

  // Foreign block (id = 2) at know = 90
  const auto block2_view =
      GB::presentation::project_block_view(ctx.em, viewer1, 2);
  test::expect_eq(block2_view.block_id, 2);
  test::expect_eq(block2_view.name, "KlingonEmpire");
  test::expect_eq(block2_view.know_pct, 90);
  test::expect_true(block2_view.standings.has_value());
  test::expect_true(block2_view.standings->victory_score.has_value());
  test::expect_false(block2_view.standings->troops.has_value());

  // Foreign block at know <= 10
  ctx.em.mutate_race(1, [](Race& r) { r.translate[2] = Percentage{5}; });
  const auto block2_low =
      GB::presentation::project_block_view(ctx.em, viewer1, 2);
  test::expect_false(block2_low.standings->victory_score.has_value());

  // 5. project_market_lot_view and project_deleted_market_lot_view
  auto commod_handle = ctx.em.create_commod({
      .owner = 2,
      .type = CommodType::RESOURCE,
      .amount = 100,
      .bid = 500,
      .star_from = 1,
      .planet_from = 1,
  });

  // A. No shipping dest star, no bid
  const auto lot1 = GB::presentation::project_market_lot_view(ctx.em, viewer1,
                                                              *commod_handle);
  test::expect_eq(lot1.id, commod_handle->id());
  test::expect_true(lot1.active);
  test::expect_eq(lot1.shipping_cost, 0);
  test::expect_false(lot1.dest_star.has_value());
  test::expect_false(lot1.dest_planet.has_value());

  // B. With shipping dest star (Vega = 2) at distance >= 10000
  ctx.em.mutate_star(2, [](Star& s) { s.set_coordinates({20000.0, 0.0}); });
  const auto lot_shipping = GB::presentation::project_market_lot_view(
      ctx.em, viewer1, *commod_handle, std::optional<starnum_t>{2});
  test::expect_gt(lot_shipping.shipping_cost, 0);

  // C. Foreign bidder (bidder = 3, destination hidden from viewer1)
  commod_handle->place_bid(3, 1, 600, 2, 1);
  const auto lot_foreign_bid = GB::presentation::project_market_lot_view(
      ctx.em, viewer1, *commod_handle);
  test::expect_eq(lot_foreign_bid.bidder, 3);
  test::expect_eq(lot_foreign_bid.bid, 600);
  test::expect_false(lot_foreign_bid.dest_star.has_value());
  test::expect_false(lot_foreign_bid.dest_planet.has_value());

  // D. Self bidder (bidder = 1, destination visible to viewer1)
  commod_handle->place_bid(1, 1, 700, 1, 1);
  const auto lot_self_bid = GB::presentation::project_market_lot_view(
      ctx.em, viewer1, *commod_handle);
  test::expect_eq(lot_self_bid.bidder, 1);
  test::expect_eq(lot_self_bid.bid, 700);
  test::expect_true(lot_self_bid.dest_star.has_value());
  test::expect_eq(lot_self_bid.dest_star.value(), 1);
  test::expect_true(lot_self_bid.dest_planet.has_value());
  test::expect_eq(lot_self_bid.dest_planet.value(), 1);

  // E. project_deleted_market_lot_view
  const auto deleted_lot =
      GB::presentation::project_deleted_market_lot_view(commod_handle->id());
  test::expect_eq(deleted_lot.id, commod_handle->id());
  test::expect_false(deleted_lot.active);

  // 6. JSON rendering with raw string literals R"(...)"
  const std::string race_json =
      GB::presentation::render_json_race_view(race1_view);
  test::expect_contains(race_json, R"("type":"race")");
  test::expect_contains(race_json, R"("name":"Federation")");
  test::expect_contains(race_json, R"("stats":)");
  test::expect_contains(race_json, R"("standings":)");
  test::expect_contains(race_json, R"("self":)");

  const std::string block_json =
      GB::presentation::render_json_block_view(block1_view);
  test::expect_contains(block_json, R"("type":"block")");
  test::expect_contains(block_json, R"("name":"FederationBloc")");
  test::expect_contains(block_json, R"("members":[1])");

  const std::string market_json =
      GB::presentation::render_json_market_lot_view(lot_self_bid);
  test::expect_contains(market_json, R"("type":"market_lot")");
  test::expect_contains(market_json, R"("active":true)");
  test::expect_contains(market_json, R"("bid":700)");

  const std::string combat_json = GB::presentation::render_json_combat_event(
      "bombard", "B9 Nemesis bombarded /Sol/Earth\n",
      BombardResult{
          .ship_display = "B9 Nemesis",
          .location_display = "/Sol/Earth",
          .previous_sector_owner = 2,
          .sectors_destroyed = 3,
      });
  test::expect_contains(combat_json, R"("type":"combat_event")");
  test::expect_contains(combat_json, R"("kind":"bombard")");
  test::expect_contains(combat_json,
                        R"("summary":"B9 Nemesis bombarded /Sol/Earth\n")");
  test::expect_contains(combat_json, R"("ship_display":"B9 Nemesis")");
  test::expect_contains(combat_json, R"("sectors_destroyed":3)");
}

}  // namespace

int main() {
  test_render_json_envelope_and_polymorphic_presenter();
  test_gameobj_ui_mode_and_present();
  test_bless_presentation();
  test_planet_turn_bulletins();
  test_ship_combat_turn_bulletins();
  test_empire_turn_bulletins();
  test_detect_terminal_capabilities();
  test_canonical_session_message_error_views();
  test_canonical_star_planet_sector_projectors();
  test_canonical_ship_view_projectors();
  test_canonical_race_block_market_combat_views();
  return 0;
}
