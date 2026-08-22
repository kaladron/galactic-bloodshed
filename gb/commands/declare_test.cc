// SPDX-License-Identifier: Apache-2.0

/// \file declare_test.cc
/// \brief Test declare command functionality, diplomatic states, and role
/// validation.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_declare_dispatch() {
  std::println(std::cout,
               "Test: declare command dispatch and diplomatic states");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Klingons";
  race2.governor[0].active = true;

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  // Setup universe_struct with AP points
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.AP[0] = 10;
  sdata.numstars = 0;
  universe_repo.save(sdata);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Declare alliance
  ctx.assert_dispatch_success(g, {"declare", "2", "alliance"});
  const auto* saved_race1 = ctx.em.peek_race(1);
  const auto* saved_race2 = ctx.em.peek_race(2);
  assert(saved_race1 != nullptr);
  assert(saved_race2 != nullptr);
  assert(isset(saved_race1->allied, 2U));
  assert(!isset(saved_race1->atwar, 2U));
  assert(saved_race2->translate[0] >= 30);
  std::println(std::cout, "    ✓ Alliance declared and translation updated");

  // 2. Declare war
  ctx.assert_dispatch_success(g, {"declare", "2", "war"});
  saved_race1 = ctx.em.peek_race(1);
  assert(isset(saved_race1->atwar, 2U));
  assert(!isset(saved_race1->allied, 2U));
  std::println(std::cout, "    ✓ War declared successfully");

  // 3. Declare neutrality
  ctx.assert_dispatch_success(g, {"declare", "2", "neutrality"});
  saved_race1 = ctx.em.peek_race(1);
  assert(!isset(saved_race1->atwar, 2U));
  assert(!isset(saved_race1->allied, 2U));
  std::println(std::cout, "    ✓ Neutrality declared successfully");

  // 4. Role check: Governor != 0 cannot declare
  g.set_governor(1);
  ctx.assert_dispatch_rejected(g, {"declare", "2", "war"});
  assert(g.out.str().contains(
      "Only the leader (Governor 0) may use this command."));
  std::println(std::cout, "    ✓ Governor rejection verified");

  // 5. Invalid target player
  g.set_governor(0);
  ctx.assert_dispatch_rejected(g, {"declare", "99", "war"});
  assert(g.out.str().contains("No such player."));
  std::println(std::cout, "    ✓ Invalid player rejection verified");
}

}  // namespace

int main() {
  test_declare_dispatch();
  std::println(std::cout, "\n✅ All declare tests passed!");
  return 0;
}
