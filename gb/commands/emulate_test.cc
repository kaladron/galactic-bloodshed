// SPDX-License-Identifier: Apache-2.0

/// \file emulate_test.cc
/// \brief Unit tests for emulate command.

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void test_emulate_matrix() {
  TestContext ctx;
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // Setup test races
  Race deity_race{};
  deity_race.Playernum = 1;
  deity_race.name = "DeityRace";
  deity_race.God = true;
  deity_race.governor[0].active = true;
  deity_race.governor[0].name = "Supreme";

  Race target_race{};
  target_race.Playernum = 2;
  target_race.name = "Klingons";
  target_race.God = false;
  target_race.governor[0].active = true;
  target_race.governor[0].name = "Leader";
  target_race.governor[1].active = true;
  target_race.governor[1].name = "Governor1";
  target_race.governor[2].active = false;

  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(deity_race);
    races.save(target_race);
  }

  // --- Case 1: Happy Path (God user emulating player 2 governor 1) ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  test::expect_true(GB::commands::dispatch_command(g, GB::commands::emulate_cmd,
                                                   {"emulate", "2", "1"}));
  test::expect_contains(g.out.str(), "Emulating Klingons \"Governor1\" [2,1]");
  test::expect_eq(g.player(), 2);
  test::expect_eq(g.governor(), 1);
  test::expect_false(g.god());  // Emulated session drops god privileges

  // --- Case 2: Role Rejection (Mortal player cannot emulate) ---
  ctx.setup_game_obj(g, 2, 0);
  g.set_god(false);
  g.out.str("");

  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "1", "0"}));
  test::expect_contains(g.out.str(), "Only deity can use this command.");
  test::expect_eq(g.player(), 2);
  test::expect_eq(g.governor(), 0);

  // --- Case 3: Argument count checks ---
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.out.str("");

  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate"}));
  test::expect_contains(g.out.str(), "Syntax: emulate <player> <governor>");

  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "2"}));
  test::expect_contains(g.out.str(), "Syntax: emulate <player> <governor>");

  // --- Case 4: Domain Errors ---
  // Non-numeric args
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "abc", "0"}));
  test::expect_contains(g.out.str(), "Invalid player or governor number.");

  // Non-existent player
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "99", "0"}));
  test::expect_contains(g.out.str(), "Player 99 does not exist.");

  // Out of range governor
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "2", "99"}));
  test::expect_contains(g.out.str(), "Invalid governor 99.");

  // Inactive governor
  g.out.str("");
  test::expect_false(GB::commands::dispatch_command(
      g, GB::commands::emulate_cmd, {"emulate", "2", "2"}));
  test::expect_contains(g.out.str(), "Governor 2 is not active.");
}

}  // namespace

int main() {
  test_emulate_matrix();
  std::println(std::cout, "✓ emulate_test passed!");
  return 0;
}
