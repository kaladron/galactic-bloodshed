// SPDX-License-Identifier: Apache-2.0

/// \file page_test.cc
/// \brief Test page command functionality, alliance block paging, and scope
/// validations.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Federation", 100.0, false, player_t{1})
      .add_race("Klingons", 100.0, false, player_t{2})
      .add_star("Sol", 10, starnum_t{0});

  // Setup alliance block for player 1
  block block1{};
  block1.Playernum = 1;
  block1.name = "AlphaAlliance";
  block1.invite = 0b11;
  block1.pledge = 0b11;
  JsonStore store(ctx.db);
  BlockRepository blocks(store);
  blocks.save(block1);
}

void test_page_dispatch() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Page player 2
  ctx.assert_dispatch_success(g, {"page", "2"});
  test::expect_contains(g.out.str(), "Request sent.");

  // 2. Page alliance block
  ctx.assert_dispatch_success(g, {"page", "block"});
  test::expect_contains(g.out.str(), "Request sent.");

  // 3. Scope check: page at universal scope rejected
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"page", "2"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 4. Invalid target player rejection
  g.set_level(ScopeLevel::LEVEL_STAR);
  ctx.assert_dispatch_rejected(g, {"page", "99"});
  test::expect_contains(g.out.str(), "No such player.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_page_dispatch();
  std::println(std::cout, "✓ page_test passed!");
  return 0;
}
