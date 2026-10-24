// SPDX-License-Identifier: Apache-2.0

/// \file personal_test.cc
/// \brief Test personal command for setting race description and leader
/// permissions.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

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
  race1.info = "Old description";

  RaceRepository races(store);
  races.save(race1);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Leader (governor 1) sets personal info without trailing space
  ctx.assert_dispatch_success(
      g, {"personal", "Peaceful", "explorers", "of", "the", "galaxy"});
  test::expect_contains(g.out.str(), "Personal information updated.");
  const auto* updated_race = ctx.em.peek_race(1);
  test::expect_ne(updated_race, nullptr);
  test::expect_eq(updated_race->info, "Peaceful explorers of the galaxy");
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"personal", "Bold", "pioneers"});
  test::expect_contains(g.out.str(), "\"type\":\"personal\"");
  test::expect_contains(g.out.str(), "\"info\":\"Bold pioneers\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Leader successfully set personal info");

  // 2. Non-leader governor (governor 2) is rejected by leader_only role
  // requirement
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"personal", "Unauthorized", "update"});
  test::expect_true(g.out.str().contains("Only the leader") ||
                    g.out.str().contains("Governor 1"));
  std::println(std::cout,
               "    ✓ Non-leader rejected by leader_only requirement");
}

}  // namespace

int main() {
  test_personal_dispatch();
  std::println(std::cout, "\n✅ All personal tests passed!");
  return 0;
}
