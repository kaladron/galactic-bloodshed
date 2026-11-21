// SPDX-License-Identifier: Apache-2.0

/// \file examine_test.cc
/// \brief Unit tests for examine command

import commands;
import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import std;

namespace {

struct ExamineSetup {
  TestRaces races;
  shipnum_t shuttle_id;
};

ExamineSetup setup_test_world(TestContext& ctx) {
  const auto races = ctx.with_races();

  const shipnum_t shuttle_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                   .owned_by(races.empire)
                                   .in_star_orbit(starnum_t{1})
                                   .build();

  // Seed / set a custom ShipExam description in SQLite
  ctx.em.mutate_ship_exam(ShipType::STYPE_SHUTTLE, [&](ShipExam& exam) {
    exam.description =
        "Shuttle: SQLite stored short-range spacecraft description.";
  });
  ctx.em.mutate_ship_exam(ShipType::OTYPE_FACTORY, [&](ShipExam& exam) {
    exam.description = "Factory description with newline.\n";
  });
  ctx.em.mutate_ship_exam(ShipType::STYPE_FIGHTER,
                          [&](ShipExam& exam) { exam.description = ""; });
  return {races, shuttle_id};
}

void test_examine_dispatch() {
  TestContext ctx;
  const auto [races, shuttle_id] = setup_test_world(ctx);
  const std::string shuttle_arg = std::format("#{}", shuttle_id);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_star(1);

  // 1. Min args check: examine without arguments, and invalid ship string
  ctx.assert_dispatch_rejected(g, {"examine"});
  test::expect_contains(g.out.str(), "Syntax: examine <#shipnum>");
  ctx.assert_dispatch_rejected(g, {"examine", "abc"});
  std::println(std::cout, "    ✓ examine rejected with insufficient arguments");

  // 2. Happy path: examine shuttle (first and second time when already
  // examined)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"examine", shuttle_arg});
  test::expect_contains(
      g.out.str(),
      "Shuttle: SQLite stored short-range spacecraft description.");
  ctx.assert_dispatch_success(g, {"examine", shuttle_arg});
  std::println(std::cout, "    ✓ examine #1 succeeded with description");

  // 3. Domain errors: non-existent, deep-space, uninhabited star
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"examine", "#999"});
  test::expect_contains(g.out.str(), "Ship not found.");

  shipnum_t univ_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(races.empire)
                            .in_deep_space()
                            .build();
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", univ_ship.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  shipnum_t deep_carrier = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                               .owned_by(races.empire)
                               .in_deep_space()
                               .build();
  shipnum_t berthed_deep = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                               .owned_by(races.empire)
                               .in_deep_space()
                               .build();
  test::expect_true(
      ctx.em.dock_carrier(berthed_deep, deep_carrier).has_value());
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", berthed_deep.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  ctx.em.mutate_star(2, [&](Star& s) { s.clear_inhabited_by(races.empire); });
  shipnum_t unvis_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(races.empire)
                             .in_star_orbit(starnum_t{2})
                             .build();
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", unvis_ship.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  // Foreign stealthed orbital mine in an inhabited star is hidden from mortals,
  // while Deity can examine both the mine and deep-space ships.
  shipnum_t enemy_mine = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                             .owned_by(races.newcomer)
                             .in_star_orbit(starnum_t{1})
                             .build();
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", enemy_mine.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");
  g.set_god(true);
  ctx.assert_dispatch_success(
      g, {"examine", std::format("#{}", enemy_mine.value)});
  ctx.assert_dispatch_success(g,
                              {"examine", std::format("#{}", univ_ship.value)});
  g.set_god(false);

  // 4. Switch, irradiated (!active), empty/newline descriptions, and JSON mode
  shipnum_t fac_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                           .owned_by(races.empire)
                           .in_star_orbit(starnum_t{1})
                           .with_active(false)
                           .build();
  ctx.assert_dispatch_success(g,
                              {"examine", std::format("#{}", fac_ship.value)});
  test::expect_contains(g.out.str(), "on/off switch");
  test::expect_contains(g.out.str(), "irradiated");

  shipnum_t empty_desc_ship = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                  .owned_by(races.empire)
                                  .in_star_orbit(starnum_t{1})
                                  .build();
  ctx.assert_dispatch_success(
      g, {"examine", std::format("#{}", empty_desc_ship.value)});

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"examine", shuttle_arg});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(
      g.out.str(),
      R"(Shuttle: SQLite stored short-range spacecraft description.)");

  ctx.verify_universe_invariants();
}

void test_examine_seeded_catalog_alignment() {
  TestContext ctx;
  const auto races = ctx.with_races();

  JsonStore store(ctx.db);
  ShipExamRepository exams(store);
  test::expect_true(exams.seed_from_file(pkg_data_path("exam.dat").string()));

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);
  g.set_scope_star(1);

  const std::array<std::pair<ShipType, std::string_view>, 8> samples = {{
      {ShipType::STYPE_GOD, "Godship"},
      {ShipType::STYPE_MINE, "Space Mine"},
      {ShipType::OTYPE_TRACT, "Tractor-Repulsor Beam"},
      {ShipType::OTYPE_VN, "Von Neumann Machine"},
      {ShipType::OTYPE_OMCL, "Mind Control Laser"},
      {ShipType::OTYPE_FACTORY, "Factory"},
      {ShipType::OTYPE_PORT, "Space Port"},
      {ShipType::STYPE_LANDER, "Lander"},
  }};

  for (const auto& [stype, expected_heading] : samples) {
    const shipnum_t shipno = TestShipBuilder(ctx.em, stype)
                                 .owned_by(races.empire)
                                 .in_star_orbit(starnum_t{1})
                                 .build();
    g.out.str("");
    ctx.assert_dispatch_success(g,
                                {"examine", std::format("#{}", shipno.value)});
    test::expect_contains(g.out.str(), expected_heading);
  }
  std::println(std::cout,
               "    ✓ examine aligned with seeded exam.dat descriptions");
  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_examine_dispatch();
  test_examine_seeded_catalog_alignment();

  std::println(std::cout, "All examine tests passed!");
  return 0;
}
