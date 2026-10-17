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
  race.leader().password = "leadpass";
  race.appoint_governor(2, {.name = "GovOne", .password = "gov1pass"});
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Leader (Governor 1) sees password column
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_contains(g.out.str(), "Password");
  test::expect_contains(g.out.str(), "leadpass");

  // 2. Governor 2 does not see password column
  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_false(g.out.str().contains("Password"));
  test::expect_false(g.out.str().contains("leadpass"));

  // 3. JSON mode as leader vs governor
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_contains(g.out.str(), "\"type\":\"governors\"");
  test::expect_contains(g.out.str(), "\"is_leader\":true");
  test::expect_contains(g.out.str(), "\"password\":\"leadpass\"");

  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_contains(g.out.str(), "\"is_leader\":false");
  test::expect_false(g.out.str().contains("\"password\""));
}

// Test appointing and revoking governors (including sparse/deleted ship IDs)
void test_appoint_and_revoke() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.leader().password = "leadpass";
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Appoint Governor 2 explicitly
  ctx.assert_dispatch_success(g, {"appoint", "2", "secret123"});
  test::expect_true(ctx.em.peek_race(1)->has_governor(2));
  test::expect_eq(ctx.em.peek_race(1)->governor(2).password, "secret123");

  // 2. Appointing already appointed governor fails
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "2", "secret123"});
  test::expect_contains(g.out.str(), "already appointed");

  // 3. Auto-assign next available governor (Governor 3) in JSON mode
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"appoint", "autopass"});
  test::expect_true(ctx.em.peek_race(1)->has_governor(3));
  test::expect_eq(ctx.em.peek_race(1)->governor(3).password, "autopass");
  test::expect_contains(g.out.str(), "\"type\":\"appoint\"");
  test::expect_contains(g.out.str(), "\"governor\":3");
  test::expect_contains(g.out.str(), "\"auto_assigned\":true");
  g.set_ui_mode(UiMode::ASCII);

  // Give Governor 2 a star, an alive ship (#2) after a deleted ship (#1),
  // a commodity lot, and treasury money to verify do_revoke handles sparse IDs
  ctx.em.mutate_race(1, [](Race& r) { r.governor(2).money = 450; });
  star_struct s{};
  s.star_id = 1;
  s.name = "Sol";
  s.governor[1] = 2;
  {
    JsonStore store(ctx.db);
    StarRepository stars(store);
    stars.save(Star(s));
  }
  {
    auto ship1 = ctx.em.create_ship(ShipType::OTYPE_FACTORY, 1);
    ship1->alive() = 1;
    ship1.save();
    auto ship2 = ctx.em.create_ship(ShipType::OTYPE_FACTORY, 1);
    ship2->governor() = 2;
    ship2->alive() = 1;
    ship2.save();
  }
  {
    auto barrier = ctx.em.create_deletion_barrier();
    ctx.em.delete_ship(1);
  }

  // 4. Revoke with wrong password fails
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"revoke", "2", "wrongpass", "1"});
  test::expect_contains(g.out.str(), "Incorrect password");

  // 5. Revoke with correct password succeeds in JSON mode
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"revoke", "2", "secret123", "1"});
  test::expect_false(ctx.em.peek_race(1)->has_governor(2));
  test::expect_eq(ctx.em.peek_race(1)->leader().money, money_t{450});
  test::expect_eq(ctx.em.peek_star(1)->governor(1), governor_t{1});
  test::expect_eq(ctx.em.peek_ship(2)->governor(), governor_t{1});
  test::expect_contains(g.out.str(), "\"type\":\"revoke\"");
  test::expect_contains(g.out.str(), "\"revoked_governor\":2");
  test::expect_contains(g.out.str(), "\"target_governor\":1");
  test::expect_contains(g.out.str(), "\"money_transferred\":450");
}

// Test changing governor passwords and guest restrictions
void test_password_change_and_guest_rejection() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.appoint_governor(2, {.password = "oldpass"});
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Change password successfully in JSON mode
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"governors", "2", "password", "newpass"});
  test::expect_eq(ctx.em.peek_race(1)->governor(2).password, "newpass");
  test::expect_contains(g.out.str(), "\"type\":\"governor_password_changed\"");
  test::expect_contains(g.out.str(), "\"governor\":2");
  g.set_ui_mode(UiMode::ASCII);

  // 2. Guest race cannot change password
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"governors", "2", "password", "evennewer"});
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
