// SPDX-License-Identifier: Apache-2.0

/// \file personal_test.cc
/// \brief Test personal command for setting race description and leader
/// permissions.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_personal_dispatch() {
  std::println(std::cout,
               "Test: personal command dispatch and leader authorization");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test race
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;
  race1.info = "Old description";

  RaceRepository races(store);
  races.save(race1);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Leader (governor 0) sets personal info
  ctx.assert_dispatch_success(
      g, {"personal", "Peaceful", "explorers", "of", "the", "galaxy"});
  const auto* updated_race = ctx.em.peek_race(1);
  assert(updated_race != nullptr);
  assert(updated_race->info.contains("Peaceful explorers of the galaxy"));
  std::println(std::cout, "    ✓ Leader successfully set personal info");

  // 2. Non-leader governor (governor 1) is rejected by leader_only role
  // requirement
  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"personal", "Unauthorized", "update"});
  assert(g.out.str().contains("Only the leader") ||
         g.out.str().contains("Governor 0"));
  std::println(std::cout,
               "    ✓ Non-leader rejected by leader_only requirement");
}

}  // namespace

int main() {
  test_personal_dispatch();
  std::println(std::cout, "\n✅ All personal tests passed!");
  return 0;
}
