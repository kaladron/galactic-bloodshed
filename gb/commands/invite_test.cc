// SPDX-License-Identifier: Apache-2.0

/// \file invite_test.cc
/// \brief Unit tests for the invite command.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

int main() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_block(1, [](Block& b) {
    b = Block{{
        .Playernum = 1,
        .name = "TheAlliance",
        .motto = "United We Stand",
        .pledged = {2},
    }};
  });

  ctx.em.mutate_power(2, [](Power& p) {
    p = Power{{
        .id = 2,
        .ships_owned = 3,
        .money = 42,
    }};
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);

  // 2. Non-leader (governor != 1) rejection
  ctx.setup_game_obj(g, 1, 2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  ctx.setup_game_obj(g, 1, 1);

  // 3. Self-invite rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Federation"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self-invite rejection verified");

  // 4. Unknown player rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Nobody"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Unknown player rejection verified");

  // 5. Successful invite (ASCII mode)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"invite", "Klingons"});
  ctx.em.clear_cache();
  const auto* b_invited = ctx.em.peek_block(1);
  test::expect_ne(b_invited, nullptr);
  test::expect_true(b_invited->is_invited(2));
  test::expect_eq(b_invited->members(), 1U);
  test::expect_eq(b_invited->ships_owned(), 3U);
  test::expect_eq(b_invited->money(), 42);
  std::println(std::cout,
               "    ✓ Invite succeeded, bit set, and block power recomputed");

  // 6. Successful invite in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(), "\"type\":\"invite\"");
  test::expect_contains(g.out.str(), "\"host_id\":1");
  test::expect_contains(g.out.str(), "\"host_name\":\"Federation\"");
  test::expect_contains(g.out.str(), "\"block_name\":\"TheAlliance\"");
  test::expect_contains(g.out.str(), "\"target_id\":2");
  test::expect_contains(g.out.str(), "\"target_name\":\"Klingons\"");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Invite JSON mode verified");

  ctx.verify_universe_invariants();
  std::println(std::cout, "invite_test passed!");
  return 0;
}
