// SPDX-License-Identifier: Apache-2.0

/// \file governors_test.cc
/// \brief Unit tests for governors command.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

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

  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_contains(g.out.str(), "Password");
  test::expect_contains(g.out.str(), "leadpass");

  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_success(g, {"governors"});
  test::expect_false(g.out.str().contains("Password"));
  test::expect_false(g.out.str().contains("leadpass"));

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

void test_password_change_validation_and_modes() {
  TestContext ctx;
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.leader().password = "leadpass";
  race.appoint_governor(2, {.password = "oldpass"});
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(race);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  ctx.assert_dispatch_rejected(g, {"governors", "abc", "password", "newpass"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"governors", "0", "password", "newpass"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"governors", "9", "password", "newpass"});
  test::expect_contains(g.out.str(), "That governor is inactive");

  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_rejected(g, {"governors", "1", "password", "hacked"});
  test::expect_contains(g.out.str(), "You can only change your own password");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"governors", "2", "password", "selfpass"});
  test::expect_contains(g.out.str(), "Password changed.");
  test::expect_eq(ctx.em.peek_race(1)->governor(2).password, "selfpass");

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);
  ctx.assert_dispatch_success(g, {"governors", "2", "password", "newpass"});
  test::expect_eq(ctx.em.peek_race(1)->governor(2).password, "newpass");
  test::expect_contains(g.out.str(), "\"type\":\"governor_password_changed\"");
  test::expect_contains(g.out.str(), "\"governor\":2");
  g.set_ui_mode(UiMode::ASCII);

  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"governors", "2", "password", "evennewer"});
  test::expect_contains(g.out.str(), "Guest races cannot change passwords");
}

}  // namespace

int main() {
  test_governors_list();
  test_password_change_validation_and_modes();

  std::println(std::cout, "✓ governors_test passed!");
  return 0;
}
