// SPDX-License-Identifier: Apache-2.0

/// \file page_test.cc
/// \brief Test page command functionality, alliance block paging, and scope
/// validations.

import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.God = false; });
  ctx.em.mutate_race(2, [](Race& r) {
    r.God = false;
    r.appoint_governor(2, {.name = "Envoy"});
  });
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 5; });

  // Setup alliance block for player 1
  ctx.em.mutate_block(1, [](Block& b) {
    b.name() = "AlphaAlliance";
    b.invite(player_t{1});
    b.invite(player_t{2});
    b.pledge(player_t{1});
    b.pledge(player_t{2});
  });
}

void test_page_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  RecordingSessionRegistry registry;
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.set_god(false);

  // 1. Page player 2 (costs 1 AP) and specific governor 2 (costs 1 AP)
  ctx.assert_dispatch_success(g, {"page", "2"}, 1);
  test::expect_eq(g.out.str(), "Request sent.\n");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "2", "2"}, 1);
  test::expect_eq(g.out.str(), "Request sent.\n");

  // 2. Page alliance block (costs 1 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "block"}, 1);
  test::expect_eq(g.out.str(), "Paging alliance block.\nRequest sent.\n");

  // 3. Invalid governor arguments rejected without throwing or deducting AP
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2", "abc"});
  test::expect_eq(g.out.str(), "No such governor.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2", "0"});
  test::expect_eq(g.out.str(), "No such governor.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2", "99"});
  test::expect_eq(g.out.str(), "No such governor.\n");

  // 4. Scope and invalid player checks
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  g.set_level(ScopeLevel::LEVEL_STAR);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "99"});
  test::expect_eq(g.out.str(), "No such player.\n");

  // 5. Paging a deity race costs 0 AP
  ctx.em.mutate_race(2, [](Race& r) { r.God = true; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "2"}, 0);
  ctx.em.mutate_race(2, [](Race& r) { r.God = false; });

  // 6. Insufficient AP rejected cleanly without spurious "Paging alliance..."
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "block"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"page", "2"});
  test::expect_eq(g.out.str(), "You don't have 1 action points there.\n");

  // 7. Deity paging in JSON mode costs 0 AP
  g.set_god(true);
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "2"}, 0);
  test::expect_contains(g.out.str(), "\"type\":\"page\"");
  test::expect_contains(g.out.str(), "\"target\":0");

  g.out.str("");
  ctx.assert_dispatch_success(g, {"page", "block"}, 0);
  test::expect_contains(g.out.str(), "\"target\":1");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_page_dispatch();
  std::println(std::cout, "✓ page_test passed!");
  return 0;
}
