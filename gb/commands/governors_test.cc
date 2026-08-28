// SPDX-License-Identifier: Apache-2.0

/// \file governors_test.cc
/// \brief Unit tests for governors, appoint, and revoke commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Test listing governors as leader vs governor
void test_governors_list() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.governor[0].password = "leadpass";
  race.governor[1].active = true;
  race.governor[1].name = "GovOne";
  race.governor[1].password = "gov1pass";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Leader (Governor 0) sees password column
  ctx.setup_game_obj(g, 1, 0);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_contains(g.out.str(), "Password");
  test::expect_contains(g.out.str(), "leadpass");

  // 2. Governor 1 does not see password column
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_false(g.out.str().contains("Password"));
  test::expect_false(g.out.str().contains("leadpass"));
}

// Test appointing and revoking governors
void test_appoint_and_revoke() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.governor[0].password = "leadpass";
  race.governor[1].active = false;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Appoint Governor 1
  ctx.assert_dispatch_success(g, {"appoint", "1", "secret123"});
  test::expect_true(ctx.em.peek_race(1)->governor[1].active);
  test::expect_eq(ctx.em.peek_race(1)->governor[1].password, "secret123");

  // 2. Appointing already appointed governor fails
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "1", "secret123"});
  test::expect_contains(g.out.str(), "already appointed");

  // 3. Revoke with wrong password fails
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "1", "wrongpass", "0"});
  test::expect_contains(g.out.str(), "Incorrect password");

  // 4. Revoke with correct password succeeds
  g.out.str("");
  ctx.assert_dispatch_success(g, {"revoke", "1", "secret123", "0"});
  test::expect_false(ctx.em.peek_race(1)->governor[1].active);
}

// Test changing governor passwords and guest restrictions
void test_password_change_and_guest_rejection() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.governor[1].active = true;
  race.governor[1].password = "oldpass";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. Change password successfully
  ctx.assert_dispatch_success(g, {"governors", "1", "password", "newpass"});
  test::expect_eq(ctx.em.peek_race(1)->governor[1].password, "newpass");

  // 2. Guest race cannot change password
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"governors", "1", "password", "evennewer"});
  test::expect_contains(g.out.str(), "Guest races cannot change passwords");
}

}  // namespace

int main() {
  test_governors_list();
  test_appoint_and_revoke();
  test_password_change_and_guest_rejection();

  std::println(std::cout, "✓ governors_test passed!");
  return 0;
}
