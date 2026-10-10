// SPDX-License-Identifier: Apache-2.0

/// \file survey_test.cc
/// \brief Unit tests for survey command

import commands;
import dallib;
import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
}

void test_survey_no_args_planet_scope() {
  std::println(std::cout, "Test: survey (no args) at planet scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "======== Planetary conditions: ========");
  test::expect_contains(out_str, "atmosphere concentrations");
  std::println(std::cout, "    ✓ Output contains planet survey information");
}

void test_survey_sector_range_with_header() {
  std::println(std::cout,
               "Test: survey command with sector range shows header");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"survey", "0:2,0:2"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "x,y");
  test::expect_contains(out_str, "cond/type");
  test::expect_contains(out_str, "owner");
  test::expect_contains(out_str, "xtals");
  test::expect_contains(out_str, "0,0");
  std::println(std::cout, "    ✓ Output contains header and sector data");
}

void test_survey_star_scope() {
  std::println(std::cout, "Test: survey command at star scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(2);

  ctx.assert_dispatch_success(g, {"survey"});

  std::string out_str = g.out.str();
  test::expect_contains(out_str, "Star Vega");
  test::expect_contains(out_str, "300,400");
  test::expect_contains(out_str, "Gravity");
  test::expect_contains(out_str, "Instability");
  test::expect_contains(out_str, "45%");
  test::expect_contains(out_str, "planets are");
  std::println(std::cout, "    ✓ Output contains star information");
}

void test_survey_universe_scope() {
  std::println(std::cout, "Test: survey command at universe scope");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "It's just _there_, you know?");
  std::println(std::cout, "    ✓ Universe scope survey succeeded");
}

void test_survey_sectors_json_and_landed_ships() {
  std::println(std::cout,
               "Test: survey sectors JSON envelope and landed ship visibility");

  TestContext ctx;
  setup_test_world(ctx);

  // Land a probe owned by player 1, governor 3 on planet (2, 1) at (0, 0),
  // where player 1 owns 0 sectors so ship sight governs planetary presence.
  ctx.em.mutate_race(1, [](Race& r) {
    r.appoint_governor(2);
    r.appoint_governor(3);
  });
  TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, 42)
      .owned_by(1, 3)
      .landed_on(2, 1, {0, 0})
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 2);
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(2);
  g.set_pnum(1);

  // Governor 2 is not authorized for governor 3's probe -> ships list empty
  ctx.assert_dispatch_success(g, {"survey", "0:0,0:0"});
  std::string unauthorized_json = g.out.str();
  test::expect_contains(unauthorized_json, R"("type":"survey_sectors")");
  test::expect_contains(unauthorized_json, R"("ships":[]})");

  // Governor 1 (leader) is authorized for all governors' ships -> sees #42
  g.out.str("");
  g.out.clear();
  g.set_governor(1);
  ctx.assert_dispatch_success(g, {"survey", "-"});
  std::string authorized_json = g.out.str();
  test::expect_contains(authorized_json, R"("type":"survey_sectors")");
  test::expect_contains(authorized_json, R"("shipno":42)");
  std::println(std::cout,
               "    ✓ JSON sector survey and ship authorization verified");
}

void test_survey_planet_star_and_universe_json() {
  std::println(std::cout,
               "Test: survey planet overview, star, and universe JSON");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_ui_mode(GB::presentation::UiMode::JSON);

  // Planet overview in JSON mode
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_success(g, {"survey"});
  std::string planet_json = g.out.str();
  test::expect_contains(planet_json, R"("type":"survey_planet")");
  test::expect_contains(planet_json, R"("planet_name":"Earth")");
  test::expect_contains(planet_json, R"("star_name":"Sol")");

  // Star survey in JSON mode
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(2);
  ctx.assert_dispatch_success(g, {"survey"});
  std::string star_json = g.out.str();
  test::expect_contains(star_json, R"("type":"survey_star")");
  test::expect_contains(star_json, R"("star_name":"Vega")");
  test::expect_contains(star_json, R"("stability":45)");

  // Universe survey in JSON mode
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"survey"});
  std::string univ_json = g.out.str();
  test::expect_contains(univ_json, R"("type":"survey_universe")");
  test::expect_contains(univ_json, R"("It's just _there_, you know?")");
  std::println(
      std::cout,
      "    ✓ JSON planet overview, star, and universe surveys verified");
}

void test_survey_location_and_scope_errors() {
  std::println(std::cout,
               "Test: survey location parsing, empty arg, and scope errors");

  TestContext ctx;
  setup_test_world(ctx);

  TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, 10)
      .owned_by(1, 1)
      .in_planet_orbit(1, 1)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // Empty second argument falls back to current scope
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"survey", ""});
  test::expect_contains(g.out.str(), "It's just _there_, you know?");

  // Sector range or '-' outside planet scope fails
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  test::expect_false(GB::commands::survey({"survey", "-"}, g));
  test::expect_contains(g.out.str(), "There are no sectors here.");

  // Argument starting with digit but no comma is parsed as a named place
  g.out.str("");
  g.out.clear();
  test::expect_false(GB::commands::survey({"survey", "1invalid"}, g));

  // Explicit ship place is rejected
  g.out.str("");
  g.out.clear();
  test::expect_false(GB::commands::survey({"survey", "#10"}, g));

  // No-arg survey while scoped to a ship hits default Illegal scope branch
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(10);
  test::expect_false(GB::commands::survey({"survey"}, g));
  test::expect_contains(g.out.str(), "Illegal scope.");

  // Named star and planet locations succeed from universe scope
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_success(g, {"survey", "/Sol"});
  test::expect_contains(g.out.str(), "Star Sol");

  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey", "/Sol/Earth"});
  test::expect_contains(g.out.str(), "Earth:");

  // Invalid coordinate range format at planet scope
  g.out.str("");
  g.out.clear();
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_success(g, {"survey", "0,bad"});
  test::expect_contains(g.out.str(), "Invalid coordinate format.");
  std::println(std::cout, "    ✓ Location and scope error branches verified");
}

void test_survey_sector_presence_crystals_and_cloaking() {
  std::println(
      std::cout,
      "Test: survey sector ship sight, crystals, and cloaking branches");

  TestContext ctx;
  setup_test_world(ctx);

  // Planet (2, 1): player 1 owns 0 sectors initially.
  // Place foreign ship (#50) and sightless ship (#51, canister) on planet (2,
  // 1).
  TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, 50)
      .owned_by(2, 1)
      .landed_on(2, 1, {0, 0})
      .build();
  TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST, 51)
      .owned_by(1, 1)
      .landed_on(2, 1, {0, 0})
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(2);
  g.set_pnum(1);

  // Neither foreign probe nor sightless canister grants planetary survey
  // presence
  ctx.assert_dispatch_success(g, {"survey", "0:0,0:0"});
  test::expect_contains(g.out.str(), R"("ships":[]})");

  // Add an orbiting probe (#53) owned by (1, 1): grants planetary presence
  // via has_sight() while being skipped by !is_landed() in sector ship list
  TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, 53)
      .owned_by(1, 1)
      .in_planet_orbit(2, 1)
      .build();

  // Add 11 landed probes on (0, 0) to exercise MAX_SHIPS_PER_SECTOR (10) cap
  for (shipnum_t num = 60; num <= 70; ++num) {
    TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE, num)
        .owned_by(1, 1)
        .landed_on(2, 1, {0, 0})
        .build();
  }

  // Add crystals to sector (0, 0) on planet (1, 1)
  ctx.em.mutate_sectormap(
      1, 1, [](SectorMap& smap) { smap.get({0, 0}).set_crystals(5); });

  // Without crystal discovery or God, crystals are hidden; with discovery or
  // God, shown
  g.set_ui_mode(GB::presentation::UiMode::ASCII);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.em.mutate_race(1, [](Race& r) {
    r.discoveries.crystal = false;
    r.God = false;
    r.Metamorph = true;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.slaved_to() = 2; });

  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "Crystal sectors: 0");
  test::expect_contains(g.out.str(), "biomass");
  test::expect_contains(g.out.str(), "This planet ENSLAVED to player 2!");

  // Enable crystal discovery -> crystals shown in overview and sector table
  ctx.em.mutate_race(1, [](Race& r) { r.discoveries.crystal = true; });
  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "Crystal sectors: 1");

  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey", "0:0,0:0"});
  test::expect_contains(g.out.str(), "yes");

  // God without crystal discovery also sees crystals, and 11 ships on (2, 1)
  // are capped at 10
  ctx.em.mutate_race(1, [](Race& r) {
    r.discoveries.crystal = false;
    r.God = true;
  });
  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "Crystal sectors: 1");

  g.set_snum(2);
  g.set_pnum(1);
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  g.out.clear();
  ctx.assert_dispatch_success(g, {"survey", "0:0,0:0"});
  test::expect_false(g.out.str().contains(R"("shipno":70)"));

  // Cloaked sector rendering branch in render_survey_sectors
  GB::presentation::SurveySectorViewModel cloaked_vm{
      .sectors = {{.coords = {0, 0}, .desshow_char = CHAR_CLOAKED}},
  };
  test::expect_contains(GB::presentation::render_survey_sectors(cloaked_vm),
                        "?  (    ?)");
  std::println(std::cout,
               "    ✓ Sector presence, crystals, and ship cap verified");
}

void test_survey_star_stability_and_god_branches() {
  std::println(std::cout, "Test: survey star stability thresholds and God");

  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(2);

  // Low tech and non-God cannot determine stability
  ctx.em.mutate_race(1, [](Race& r) {
    r.tech = 0.0;
    r.God = false;
  });
  ctx.assert_dispatch_success(g, {"survey"});
  test::expect_contains(g.out.str(), "(cannot determine)");

  // Low tech with God sees stability and quoted planet names
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  constexpr std::array<std::pair<int, std::string_view>, 5> kStages = {{
      {10, "stable"},
      {30, "unstable"},
      {45, "dangerous"},
      {80, "WARNING! Nova iminent!"},
      {100, "undergoing nova"},
  }};
  for (const auto& [stab, label] : kStages) {
    ctx.em.mutate_star(2, [stab](Star& s) { s.stability() = stab; });
    g.out.str("");
    g.out.clear();
    ctx.assert_dispatch_success(g, {"survey"});
    test::expect_contains(g.out.str(), label);
    test::expect_contains(g.out.str(), "\"Vega Prime\"");
  }
  std::println(std::cout, "    ✓ Star stability thresholds and God verified");
}

}  // namespace

int main() {
  test_survey_no_args_planet_scope();
  test_survey_sector_range_with_header();
  test_survey_star_scope();
  test_survey_universe_scope();
  test_survey_sectors_json_and_landed_ships();
  test_survey_planet_star_and_universe_json();
  test_survey_location_and_scope_errors();
  test_survey_sector_presence_crystals_and_cloaking();
  test_survey_star_stability_and_god_branches();

  std::println(std::cout, "\n✅ All survey tests passed!");
  return 0;
}
