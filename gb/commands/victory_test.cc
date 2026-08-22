// SPDX-License-Identifier: Apache-2.0

/// \file victory_test.cc
/// \brief Test victory command functionality and standings output via
/// CommandDescriptor.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_victory_dispatch() {
  std::println(std::cout, "Test: victory command dispatch and rankings");

  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup: Create universe
  universe_struct us{};
  us.id = 1;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  // Setup: Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Terrans";
  race1.victory_score = 100.0;
  race1.tech = 50.0;
  race1.IQ = 120;
  race1.password = "secret1";
  race1.governor[0].password = "govsec1";

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Martians";
  race2.victory_score = 250.0;
  race2.tech = 80.0;
  race2.IQ = 140;
  race2.password = "secret2";
  race2.governor[0].password = "govsec2";

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup: Create power records (needed by victory list calculation)
  power p1{};
  p1.id = 1;
  power p2{};
  p2.id = 2;
  PowerRepository powers(store);
  powers.save(p1);
  powers.save(p2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.race = ctx.em.peek_race(g.player());

  // 1. Victory standings (no args - shows all players)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory"});
  std::string out = g.out.str();
  assert(out.contains("PLAYER RANKINGS"));
  assert(out.contains("Martians") && out.contains("Terrans"));
  std::println(std::cout, "    ✓ victory all players standings succeeded");

  // 2. Victory top count (victory 1 - top 1 player)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory", "1"});
  out = g.out.str();
  assert(out.contains("PLAYER RANKINGS"));
  assert(out.contains("Martians"));
  std::println(std::cout, "    ✓ victory top 1 player standings succeeded");

  // 3. Error case: invalid count
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"victory", "0"});
  assert(g.out.str().contains("Invalid count specified"));
  std::println(std::cout, "    ✓ victory rejected 0 count");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"victory", "abc"});
  assert(g.out.str().contains("Invalid count specified"));
  std::println(std::cout, "    ✓ victory rejected non-numeric count");

  // 4. God mode includes passwords
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"victory"});
  out = g.out.str();
  assert(out.contains("Password") && out.contains("Gov Pass"));
  assert(out.contains("secret1") || out.contains("secret2"));
  std::println(std::cout, "    ✓ victory god mode details succeeded");
}

}  // namespace

int main() {
  test_victory_dispatch();

  std::println(std::cout, "All victory tests passed!");
  return 0;
}
