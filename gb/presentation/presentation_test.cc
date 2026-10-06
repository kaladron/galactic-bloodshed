// SPDX-License-Identifier: Apache-2.0

/// \file presentation_test.cc
/// \brief Unit tests for gb.presentation JSON envelope rendering, UiMode
/// dispatch, and entity std::formatter specializations.

import dallib;
import gb.entities;
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
  std::string status{"ok"};
};

namespace {

void test_render_json_envelope_and_mode_dispatch() {
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

  const auto ascii_fn = [](const SampleCommandResult& r) {
    return std::format("Ship #{} {} at ({},{})\n", r.ship, r.status, r.sector.x,
                       r.sector.y);
  };

  const std::string dispatched_ascii = GB::presentation::render_by_mode(
      UiMode::ASCII, "launch_result", result, ascii_fn);
  test::expect_eq(dispatched_ascii, "Ship #19 launched at (2,6)\n");

  const std::string dispatched_json = GB::presentation::render_by_mode(
      UiMode::JSON, "launch_result", result, ascii_fn);
  test::expect_eq(dispatched_json, json_out);
}

void test_gameobj_ui_mode_defaults_and_mutation() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  test::expect_eq(g.ui_mode(), UiMode::ASCII);
  g.set_ui_mode(UiMode::JSON);
  test::expect_eq(g.ui_mode(), UiMode::JSON);
  g.set_ui_mode(UiMode::ASCII);
  test::expect_eq(g.ui_mode(), UiMode::ASCII);
}

}  // namespace

int main() {
  test_render_json_envelope_and_mode_dispatch();
  test_gameobj_ui_mode_defaults_and_mutation();
  return 0;
}
