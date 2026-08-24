// SPDX-License-Identifier: Apache-2.0

/// \file send_message_test.cc
/// \brief Unit tests for send message command and translation updates

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("TestRace", 100.0, false, player_t{1})
      .add_race("AlienRace", 100.0, false, player_t{2})
      .add_star("Sol", 10, starnum_t{0});

  auto r1 = ctx.em.get_race(1);
  r1->governor[0].name = "TestGovernor";
  r1->translate[0] = 50;

  auto r2 = ctx.em.get_race(2);
  r2->translate[0] = 50;
}

void test_send_message() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);
  g.set_god(false);

  // Test sending a regular message: send 2 Hello World
  command_t argv = {"send", "2", "Hello", "World"};
  ctx.assert_dispatch_success(g, argv, 0);

  // Verify translation modifier increased by 2 (from 50 to 52)
  const auto* updated_receiver = ctx.em.peek_race(2);
  test::expect_true(updated_receiver != nullptr);
  test::expect_eq(updated_receiver->translate[0], 52);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_send_message();

  std::println(std::cout, "✓ send_message_test passed!");
  return 0;
}
