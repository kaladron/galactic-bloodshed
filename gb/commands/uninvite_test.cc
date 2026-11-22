// SPDX-License-Identifier: Apache-2.0

/// \file uninvite_test.cc
/// \brief Unit tests for the uninvite command.

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
        .invited = {p2},
        .pledged = {p2},
        .members = 1,
        .ships_owned = 3,
        .money = 42,
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
  ctx.assert_dispatch_rejected(g, {"uninvite", "Klingons"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");
  std::println(std::cout, "    ✓ Guest rejection verified");

  ctx.em.mutate_race(p1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g, p1, 1);

  // 2. Non-leader (governor != 1) rejection
  ctx.setup_game_obj(g, p1, 2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"uninvite", "Klingons"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");
  std::println(std::cout, "    ✓ Governor rejection verified");

  ctx.setup_game_obj(g, p1, 1);

  // 3. Self-uninvite rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"uninvite", "Federation"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self-uninvite rejection verified");

  // 4. Unknown player rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"uninvite", "Nobody"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Unknown player rejection verified");

  // 5. Successful uninvite (ASCII mode)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"uninvite", "Klingons"});
  ctx.em.clear_cache();
  const auto* b_uninvited = ctx.em.peek_block(p1);
  test::expect_ne(b_uninvited, nullptr);
  test::expect_false(b_uninvited->is_invited(p2));
  test::expect_eq(b_uninvited->members(), 0U);
  test::expect_eq(b_uninvited->ships_owned(), 0U);
  test::expect_eq(b_uninvited->money(), 0);
  std::println(
      std::cout,
      "    ✓ Uninvite succeeded, bit cleared, and block power recomputed");

  // 6. Successful uninvite in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"uninvite", "Klingons"});
  test::expect_contains(g.out.str(), R"("type":"block")");
  test::expect_contains(g.out.str(), R"("name":"TheAlliance")");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ Uninvite JSON mode verified");

  ctx.verify_universe_invariants();
  std::println(std::cout, "uninvite_test passed!");
  return 0;
}
