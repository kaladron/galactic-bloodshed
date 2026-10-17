// SPDX-License-Identifier: Apache-2.0

/// \file pledge_test.cc
/// \brief Test pledge command for alliance blocks via CommandDescriptor.

import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_pledge_dispatch() {
  std::println(std::cout,
               "Test: pledge command dispatch, JSON mode, and block power");
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_block(2, [](Block& b) {
    b = Block{{
        .Playernum = player_t{2},
        .name = "GalacticCoalition",
        .invited = {player_t{1}, player_t{2}},
        .pledged = {player_t{2}},
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

  // 1. Pledge to block 2 (where player 1 is already invited)
  ctx.assert_dispatch_success(g, {"pledge", "2"});
  const auto* saved_block = ctx.em.peek_block(player_t{2});
  test::expect_ne(saved_block, nullptr);
  test::expect_true(saved_block->is_pledged(player_t{1}));
  test::expect_eq(saved_block->members(), 2U);
  test::expect_eq(saved_block->ships_owned(), 10U);
  test::expect_eq(saved_block->money(), money_t{400});
  std::println(std::cout,
               "    ✓ Pledged allegiance and recomputed block power");

  // 2. Pledge in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"pledge", "2"});
  test::expect_contains(g.out.str(), R"("type":"pledge")");
  test::expect_contains(g.out.str(), R"("block_name":"GalacticCoalition")");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Direct renderer checks for bandwagon and normal declaration news
  GB::presentation::PledgeResult bandwagon_vm{
      .pledger_id = player_t{1},
      .pledger_name = "Federation",
      .block_id = player_t{2},
      .block_name = "GalacticCoalition",
      .bandwagon = true,
  };
  test::expect_contains(
      GB::presentation::render_pledge_declaration_news(bandwagon_vm),
      "joins the band wagon");
  bandwagon_vm.bandwagon = false;
  test::expect_contains(
      GB::presentation::render_pledge_declaration_news(bandwagon_vm),
      "pledges allegiance to GalacticCoalition [2].");

  // 4. Self pledge rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "1"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self pledge rejection verified");

  // 5. Role check: Sub-governor cannot pledge
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "2"});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");
  std::println(std::cout, "    ✓ Sub-governor rejection verified for pledge");

  // 6. Invalid target player rejection
  g.set_governor(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_pledge_dispatch();
  std::println(std::cout, "\n✅ All pledge tests passed!");
  return 0;
}
