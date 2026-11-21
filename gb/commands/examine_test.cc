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

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1)
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
}

void test_examine_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_scope_star(1);

  // 1. Min args check: examine without arguments, and invalid ship string
  ctx.assert_dispatch_rejected(g, {"examine"});
  test::expect_contains(g.out.str(), "Syntax: examine <#shipnum>");
  ctx.assert_dispatch_rejected(g, {"examine", "abc"});
  std::println(std::cout, "    ✓ examine rejected with insufficient arguments");

  // 2. Happy path: examine #1 (first and second time when already examined)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"examine", "#1"});
  test::expect_contains(
      g.out.str(),
      "Shuttle: SQLite stored short-range spacecraft description.");
  ctx.assert_dispatch_success(g, {"examine", "#1"});
  std::println(std::cout, "    ✓ examine #1 succeeded with description");

  // 3. Domain errors: non-existent, deep-space, uninhabited star
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"examine", "#999"});
  test::expect_contains(g.out.str(), "Ship not found.");

  shipnum_t univ_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                            .owned_by(1)
                            .in_deep_space()
                            .build();
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", univ_ship.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  shipnum_t deep_carrier = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                               .owned_by(1)
                               .in_deep_space()
                               .build();
  shipnum_t berthed_deep = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                               .owned_by(1)
                               .in_deep_space()
                               .build();
  test::expect_true(
      ctx.em.dock_carrier(berthed_deep, deep_carrier).has_value());
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", berthed_deep.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  ctx.em.mutate_star(2, [](Star& s) { s.clear_inhabited_by(player_t{1}); });
  shipnum_t unvis_ship = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                             .owned_by(1)
                             .in_star_orbit(starnum_t{2})
                             .build();
  ctx.assert_dispatch_rejected(
      g, {"examine", std::format("#{}", unvis_ship.value)});
  test::expect_contains(g.out.str(), "That ship is not visible to you.");

  // 4. Switch, irradiated (!active), empty/newline descriptions, and JSON mode
  shipnum_t fac_ship = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                           .owned_by(1)
                           .in_star_orbit(starnum_t{1})
                           .with_active(false)
                           .build();
  ctx.assert_dispatch_success(g,
                              {"examine", std::format("#{}", fac_ship.value)});
  test::expect_contains(g.out.str(), "on/off switch");
  test::expect_contains(g.out.str(), "irradiated");

  shipnum_t empty_desc_ship = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                  .owned_by(1)
                                  .in_star_orbit(starnum_t{1})
                                  .build();
  ctx.assert_dispatch_success(
      g, {"examine", std::format("#{}", empty_desc_ship.value)});

  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"examine", "#1"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(
      g.out.str(),
      R"(Shuttle: SQLite stored short-range spacecraft description.)");

  ctx.verify_universe_invariants();
}

void test_examine_seeded_catalog_alignment() {
  TestContext ctx;
  ctx.with_standard_universe();

  JsonStore store(ctx.db);
  ShipExamRepository exams(store);
  test::expect_true(exams.seed_from_file(pkg_data_path("exam.dat").string()));

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
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
                                 .owned_by(1)
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
