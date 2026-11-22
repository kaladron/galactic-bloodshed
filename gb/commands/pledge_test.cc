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

constexpr player_t p1 = 2;
constexpr player_t p2 = 3;

void test_pledge_dispatch() {
  std::println(std::cout,
               "Test: pledge command dispatch, JSON mode, and block power");
  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);

  ctx.em.mutate_block(p2, [](Block& b) {
    b = Block{{
        .Playernum = p2,
        .name = "GalacticCoalition",
        .invited = {p1, p2},
        .pledged = {p2},
    }};
  });

  ctx.em.mutate_power(p1, [](Power& p) {
    p = Power{{
        .id = p1,
        .ships_owned = 4,
        .money = 120,
    }};
  });
  ctx.em.mutate_power(p2, [](Power& p) {
    p = Power{{
        .id = p2,
        .ships_owned = 6,
        .money = 280,
    }};
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);

  // 1. Pledge to block p2 (where p1 is already invited)
  ctx.assert_dispatch_success(g, {"pledge", "3"});
  const auto* saved_block = ctx.em.peek_block(p2);
  test::expect_ne(saved_block, nullptr);
  test::expect_true(saved_block->is_pledged(p1));
  test::expect_eq(saved_block->members(), 2U);
  test::expect_eq(saved_block->ships_owned(), 10U);
  test::expect_eq(saved_block->money(), money_t{400});
  std::println(std::cout,
               "    ✓ Pledged allegiance and recomputed block power");

  // 2. Pledge in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"pledge", "3"});
  test::expect_contains(g.out.str(), R"("type":"block")");
  test::expect_contains(g.out.str(), R"("name":"GalacticCoalition")");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Direct renderer checks for bandwagon and normal declaration news
  GB::presentation::PledgeResult bandwagon_vm{
      .pledger_id = p1,
      .pledger_name = "Federation",
      .block_id = p2,
      .block_name = "GalacticCoalition",
      .bandwagon = true,
  };
  test::expect_contains(
      GB::presentation::render_pledge_declaration_news(bandwagon_vm),
      "joins the band wagon");
  bandwagon_vm.bandwagon = false;
  test::expect_contains(
      GB::presentation::render_pledge_declaration_news(bandwagon_vm),
      "pledges allegiance to GalacticCoalition [3].");

  // 4. Self pledge rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "2"});
  test::expect_contains(g.out.str(), "Not needed, you are the leader.");
  std::println(std::cout, "    ✓ Self pledge rejection verified");

  // 5. Role check: Sub-governor cannot pledge
  ctx.em.mutate_race(p1, [](Race& r) { r.appoint_governor(2); });
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"pledge", "3"});
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
