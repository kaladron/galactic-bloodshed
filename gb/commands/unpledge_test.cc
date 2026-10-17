// SPDX-License-Identifier: Apache-2.0

/// \file unpledge_test.cc
/// \brief Test unpledge command functionality, alliance block departures, and
/// role validation.

import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_unpledge_dispatch() {
  std::println(std::cout,
               "Test: unpledge command dispatch, JSON mode, and block power");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_block(2, [](Block& b) {
    b = Block{{
        .Playernum = player_t{2},
        .name = "LeaderBlock",
        .invited = {player_t{1}, player_t{2}},
        .pledged = {player_t{1}, player_t{2}},
    }};
  });

  ctx.em.mutate_power(1, [](Power& p) {
    p = Power{{
        .id = 1,
        .ships_owned = 4,
        .money = 120,
    }};
  });
  ctx.em.mutate_power(2, [](Power& p) {
    p = Power{{
        .id = 2,
        .ships_owned = 6,
        .money = 280,
    }};
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. Unpledge from block 2
  ctx.assert_dispatch_success(g, {"unpledge", "2"});
  const auto* saved_block = ctx.em.peek_block(player_t{2});
  test::expect_ne(saved_block, nullptr);
  test::expect_false(saved_block->is_pledged(player_t{1}));
  test::expect_eq(saved_block->members(), 1U);
  test::expect_eq(saved_block->ships_owned(), 6U);
  test::expect_eq(saved_block->money(), money_t{280});
  std::println(std::cout, "    ✓ Unpledged and recomputed block power");

  // 2. Unpledge in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"unpledge", "2"});
  test::expect_contains(g.out.str(), R"("type":"unpledge")");
  test::expect_contains(g.out.str(), R"("block_name":"LeaderBlock")");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Direct renderer checks for taunt and normal declaration news
  GB::presentation::UnpledgeResult taunt_vm{
      .pledger_id = player_t{1},
      .pledger_name = "Federation",
      .block_id = player_t{2},
      .block_name = "LeaderBlock",
      .taunt = true,
  };
  test::expect_contains(
      GB::presentation::render_unpledge_declaration_news(taunt_vm),
      "calls LeaderBlock [2] a bunch of geeks and QUITS!");
  taunt_vm.taunt = false;
  test::expect_contains(
      GB::presentation::render_unpledge_declaration_news(taunt_vm),
      "has QUIT LeaderBlock [2]!");

  // 4. Self unpledge rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "1"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self unpledge rejection verified");

  // 5. Role check: Sub-governor cannot unpledge
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "2"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");
  std::println(std::cout, "    ✓ Sub-governor rejection verified for unpledge");

  // 6. Invalid target player rejection
  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"unpledge", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_unpledge_dispatch();
  std::println(std::cout, "\n✅ All unpledge tests passed!");
  return 0;
}
