// SPDX-License-Identifier: Apache-2.0

/// \file profile_test.cc
/// \brief Test profile command functionality and reporting via
/// CommandDescriptor.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_profile_dispatch() {
  std::println(std::cout, "Test: profile command dispatch and reporting");

  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create star for homeworld
  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "Sol";
  ss0.pnames.push_back("Earth");
  Star star0(ss0);
  StarRepository stars(store);
  stars.save(star0);

  // Setup: Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  race1.info = "Peaceful explorers";
  race1.morale = 100;
  race1.turn = 10;
  race1.conditions[TEMP] = 50;
  race1.conditions[METHANE] = 10;
  race1.conditions[OXYGEN] = 60;
  race1.conditions[HELIUM] = 5;
  race1.conditions[NITROGEN] = 70;
  race1.conditions[CO2] = 10;
  race1.conditions[HYDROGEN] = 5;
  race1.conditions[SULFUR] = 5;
  race1.conditions[OTHER] = 5;
  race1.translate[player_t{1}] = 100;
  race1.translate[player_t{2}] = 90;
  race1.governor[0].homesystem = 0;
  race1.governor[0].homeplanetnum = 0;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Martians";
  race2.info = "Warlike conquerors";
  race2.morale = 80;
  race2.turn = 5;
  race2.conditions[TEMP] = 30;
  race2.translate[player_t{1}] = 90;
  race2.translate[player_t{2}] = 100;
  race2.governor[0].homesystem = 0;
  race2.governor[0].homeplanetnum = 0;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // 1. Profile for self (no args)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"profile"});
  std::string out = g.out.str();
  test::expect_contains(out, "Racial profile for Terrans");
  test::expect_contains(out, "Default Scope: /Sol/Earth");
  test::expect_contains(out, "Morale: 100");
  std::println(std::cout, "    ✓ profile self report succeeded");

  // 2. Profile for other player by name
  g.out.str("");
  ctx.assert_dispatch_success(g, {"profile", "Martians"});
  out = g.out.str();
  test::expect_contains(out, "Race report on Martians");
  test::expect_contains(out, "Personal: Warlike conquerors");
  std::println(std::cout, "    ✓ profile other player by name succeeded");

  // 3. Profile for other player by number
  g.out.str("");
  ctx.assert_dispatch_success(g, {"profile", "2"});
  out = g.out.str();
  test::expect_contains(out, "Race report on Martians");
  std::println(std::cout, "    ✓ profile other player by number succeeded");

  // 4. Error case: non-existent player
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"profile", "99"});
  test::expect_contains(g.out.str(), "Player does not exist");
  std::println(std::cout, "    ✓ profile rejected non-existent player");
}

}  // namespace

int main() {
  test_profile_dispatch();

  std::println(std::cout, "All profile tests passed!");
  return 0;
}
