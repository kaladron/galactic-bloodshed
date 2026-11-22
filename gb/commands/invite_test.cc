// SPDX-License-Identifier: Apache-2.0

/// \file invite_test.cc
/// \brief Unit tests for the invite command.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {
constexpr player_t p1 = 2;
constexpr player_t p2 = 3;
}  // namespace

int main() {
  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);

  ctx.em.mutate_block(p1, [](Block& b) {
    b = Block{{
        .Playernum = p1,
        .name = "TheAlliance",
        .motto = "United We Stand",
        .pledged = {p2},
    }};
  });

  ctx.em.mutate_power(p2, [](Power& p) {
    p = Power{{
        .id = p2,
        .ships_owned = 3,
        .money = 42,
    }};
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);

  // 1. Guest rejection
  ctx.em.mutate_race(p1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g, p1, 1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  ctx.em.mutate_race(p1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g, p1, 1);

  // 2. Non-leader (governor != 1) rejection
  ctx.setup_game_obj(g, p1, 2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  ctx.setup_game_obj(g, p1, 1);

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

  // 5. Successful invite (ASCII mode) preserves ServerState::last_update_time
  ctx.em.mutate_server_state(
      [](ServerState& state) { state.last_update_time = 1700001234; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"invite", "Klingons"});
  ctx.em.clear_cache();
  test::expect_eq(ctx.em.peek_server_state()->last_update_time, 1700001234);
  const auto* b_invited = ctx.em.peek_block(p1);
  test::expect_ne(b_invited, nullptr);
  test::expect_true(b_invited->is_invited(p2));
  test::expect_eq(b_invited->members(), 1U);
  test::expect_eq(b_invited->ships_owned(), 3U);
  test::expect_eq(b_invited->money(), 42);
  std::println(std::cout,
               "    ✓ Invite succeeded, bit set, and block power recomputed");

  // 6. Successful invite in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"invite", "Klingons"});
  test::expect_contains(g.out.str(), R"("type":"block")");
  test::expect_contains(g.out.str(), R"("name":"TheAlliance")");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Invite JSON mode verified");

  ctx.verify_universe_invariants();
  std::println(std::cout, "invite_test passed!");
  return 0;
}
