// SPDX-License-Identifier: Apache-2.0

/// \file who_test.cc
/// \brief Unit tests for who command.

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_who_matrix() {
  TestContext ctx;
  RecordingSessionRegistry mock_registry;
  GameObj g(ctx.em, mock_registry);

  TestWorldBuilder(ctx)
      .add_race("Federation", 50.0, false, player_t{1})
      .add_race("Klingons", 50.0, false, player_t{2})
      .add_race("Deity", 50.0, false, player_t{3});

  ctx.em.mutate_race(player_t{1}, [](Race& race1) {
    race1.leader().name = "Kirk";
    race1.leader().toggle.invisible = false;
    race1.leader().toggle.gag = false;
  });

  ctx.em.mutate_race(player_t{2}, [](Race& race2) {
    race2.leader().name = "Kang";
    race2.leader().toggle.invisible = true;
    race2.leader().toggle.gag = true;
  });

  ctx.em.mutate_race(player_t{3}, [](Race& god_race) {
    god_race.God = true;
    god_race.leader().name = "Admin";
  });

  ctx.create_star("Sol", starnum_t{1}).build();

  std::time_t now = std::time(nullptr);
  mock_registry.sessions = {
      SessionInfo{.player = 1,
                  .governor = 1,
                  .snum = 1,
                  .connected = true,
                  .god = false,
                  .last_time = now - 10},
      SessionInfo{.player = 2,
                  .governor = 1,
                  .snum = 1,
                  .connected = true,
                  .god = false,
                  .last_time = now - 5},
      SessionInfo{.player = 3,
                  .governor = 1,
                  .snum = 1,
                  .connected = true,
                  .god = true,
                  .last_time = now - 1},  // God session: should be skipped
  };

  // --- Case 1: Happy Path (Normal player viewing who) ---
  ctx.setup_game_obj(g, 1, 1);
  g.set_god(false);
  g.set_level(ScopeLevel::LEVEL_PLAN);

  test::expect_true(
      GB::commands::dispatch_command(g, GB::commands::who_cmd, {"who"}));
  std::string out = g.out.str();
  test::expect_contains(out, "Current Players:");
  test::expect_contains(out, "Federation");
  test::expect_contains(out, "\"Kirk\"");
  test::expect_contains(out, "[1,1]");
  // Player 2 is invisible, so non-god Player 1 should not see "Klingons" in
  // table
  test::expect_false(out.contains("Klingons"));
  // Deity session should be skipped
  test::expect_false(out.contains("Deity"));
  // Non-god sees coward count or Finished depending on SHOW_COWARDS
  if (SHOW_COWARDS) {
    test::expect_contains(out, "1 coward");
  } else {
    test::expect_contains(out, "Finished.");
  }

  // --- Case 2: Invisible player viewing who sees themselves ---
  ctx.setup_game_obj(g, 2, 1);
  g.set_god(false);

  test::expect_true(
      GB::commands::dispatch_command(g, GB::commands::who_cmd, {"who"}));
  out = g.out.str();
  test::expect_contains(out, "Federation");
  test::expect_contains(out, "Klingons");
  test::expect_contains(out, "INVISIBLE");
  test::expect_contains(out, "GAG");
  test::expect_true(out.contains("0 cowards") || out.contains("Finished."));

  // --- Case 3: God viewing who sees all and star names ---
  ctx.setup_game_obj(g, 3, 1);
  g.set_god(true);

  test::expect_true(
      GB::commands::dispatch_command(g, GB::commands::who_cmd, {"who"}));
  out = g.out.str();
  test::expect_contains(out, "Federation");
  test::expect_contains(out, "Klingons");
  test::expect_contains(out, "Sol");
  test::expect_contains(out, "INVISIBLE");

  // --- Case 4: JSON mode rendering ---
  g.set_ui_mode(UiMode::JSON);
  test::expect_true(
      GB::commands::dispatch_command(g, GB::commands::who_cmd, {"who"}));
  out = g.out.str();
  test::expect_contains(out, "\"type\":\"who\"");
  test::expect_contains(out, "\"governor_name\":\"Kirk\"");
  test::expect_contains(out, "\"star_name\":\"Sol\"");
  g.set_ui_mode(UiMode::ASCII);

  // --- Case 5: Scope Testing (Works across all scopes: UNIV, STAR, PLAN, SHIP)
  // ---
  for (auto scope : {ScopeLevel::LEVEL_UNIV, ScopeLevel::LEVEL_STAR,
                     ScopeLevel::LEVEL_PLAN, ScopeLevel::LEVEL_SHIP}) {
    g.set_level(scope);
    test::expect_true(
        GB::commands::dispatch_command(g, GB::commands::who_cmd, {"who"}));
  }
}

}  // namespace

int main() {
  test_who_matrix();
  std::println(std::cout, "✓ who_test passed!");
  return 0;
}
