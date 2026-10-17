// SPDX-License-Identifier: Apache-2.0

/// \file appoint_test.cc
/// \brief Unit tests for appoint command.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_appoint_validation_and_modes() {
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

  ctx.assert_dispatch_rejected(g, {"appoint"});
  test::expect_contains(g.out.str(), "Syntax: appoint [<gov>] <password>");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "abc", "secret123"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "0", "secret123"});
  test::expect_contains(g.out.str(), "No such governor");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "1", "secret123"});
  test::expect_contains(g.out.str(), "already appointed");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"appoint", "2", "secret123"});
  test::expect_contains(g.out.str(), "Governor activated.");
  test::expect_true(ctx.em.peek_race(1)->has_governor(2));
  test::expect_eq(ctx.em.peek_race(1)->governor(2).password, "secret123");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "2", "secret123"});
  test::expect_contains(g.out.str(), "already appointed");

  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);
  ctx.assert_dispatch_rejected(g, {"appoint", "3", "secret456"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command");

  ctx.setup_game_obj(g, 1, 1);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"appoint", "autopass"});
  test::expect_true(ctx.em.peek_race(1)->has_governor(3));
  test::expect_eq(ctx.em.peek_race(1)->governor(3).password, "autopass");
  test::expect_contains(g.out.str(), "\"type\":\"appoint\"");
  test::expect_contains(g.out.str(), "\"governor\":3");
  test::expect_contains(g.out.str(), "\"auto_assigned\":true");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"appoint", "4", "explicitpass"});
  test::expect_true(ctx.em.peek_race(1)->has_governor(4));
  test::expect_contains(g.out.str(), "\"governor\":4");
  test::expect_contains(g.out.str(), "\"auto_assigned\":false");
  g.set_ui_mode(UiMode::ASCII);

  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"appoint", "5", "guestpass"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command");
}

}  // namespace

int main() {
  test_appoint_validation_and_modes();

  std::println(std::cout, "✓ appoint_test passed!");
  return 0;
}
