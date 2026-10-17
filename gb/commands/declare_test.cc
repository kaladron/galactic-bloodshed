// SPDX-License-Identifier: Apache-2.0

/// \file declare_test.cc
/// \brief Test declare command functionality, diplomatic states, and role
/// validation.

import dallib;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_declare_dispatch() {
  std::println(std::cout,
               "Test: declare command dispatch and diplomatic states");
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Declare alliance without modifier (default 30)
  ctx.assert_dispatch_success(g, {"declare", "2", "alliance"});
  const auto* saved_race1 = ctx.em.peek_race(1);
  const auto* saved_race2 = ctx.em.peek_race(2);
  test::expect_ne(saved_race1, nullptr);
  test::expect_ne(saved_race2, nullptr);
  test::expect_true(saved_race1->is_allied_with(player_t{2}));
  test::expect_false(saved_race1->is_at_war_with(player_t{2}));
  test::expect_ge(saved_race2->translation_for(player_t{1}), 30);
  std::println(std::cout, "    ✓ Alliance declared and translation updated");

  // 2. Declare alliance with explicit modifier (50) and invalid modifier
  ctx.assert_dispatch_success(g, {"declare", "2", "alliance", "50"});
  saved_race2 = ctx.em.peek_race(2);
  test::expect_ge(saved_race2->translation_for(player_t{1}), 50);
  ctx.assert_dispatch_success(g, {"declare", "2", "alliance", "invalid"});
  std::println(std::cout,
               "    ✓ Alliance declared with explicit translation modifier");

  // 3. Declare war
  ctx.assert_dispatch_success(g, {"declare", "2", "war"});
  saved_race1 = ctx.em.peek_race(1);
  test::expect_true(saved_race1->is_at_war_with(player_t{2}));
  test::expect_false(saved_race1->is_allied_with(player_t{2}));
  std::println(std::cout, "    ✓ War declared successfully");

  // 4. Declare neutrality
  ctx.assert_dispatch_success(g, {"declare", "2", "neutrality"});
  saved_race1 = ctx.em.peek_race(1);
  test::expect_false(saved_race1->is_at_war_with(player_t{2}));
  test::expect_false(saved_race1->is_allied_with(player_t{2}));
  std::println(std::cout, "    ✓ Neutrality declared successfully");

  // 5. Declare in UiMode::JSON
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"declare", "2", "alliance", "40"});
  test::expect_contains(g.out.str(), R"("type":"declare")");
  test::expect_contains(g.out.str(), R"("relation":0)");
  test::expect_contains(g.out.str(), R"("translation_modifier":40)");
  g.set_ui_mode(UiMode::ASCII);

  // 6. Direct renderer coverage for rare_quip and
  // all_war_declaration_news_variants
  GB::presentation::DeclareResult vm{
      .declarer_id = player_t{1},
      .declarer_name = "Federation",
      .target_id = player_t{2},
      .target_name = "Klingons",
      .relation = GB::presentation::DiplomaticRelation::Alliance,
      .rare_quip = true,
  };
  test::expect_contains(GB::presentation::render_declare_result(vm), "marry");
  vm.rare_quip = false;
  test::expect_contains(GB::presentation::render_declare_result(vm),
                        "Good for you.");

  vm.relation = GB::presentation::DiplomaticRelation::War;
  vm.rare_quip = true;
  test::expect_contains(GB::presentation::render_declare_result(vm),
                        "reproductive glands");
  vm.rare_quip = false;
  test::expect_contains(GB::presentation::render_declare_result(vm),
                        "Give 'em hell!");
  for (auto variant : GB::presentation::all_war_declaration_news_variants) {
    vm.news_variant = variant;
    test::expect_contains(GB::presentation::render_declare_declaration_news(vm),
                          "WAR");
  }

  // 7. Self-declaration rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"declare", "1", "war"});
  test::expect_contains(g.out.str(), "You cannot declare against yourself.");
  std::println(std::cout, "    ✓ Self-declaration rejected cleanly");

  // 8. Invalid target player
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"declare", "99", "war"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");

  // 9. Unrecognized relation status
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"declare", "2", "bogus"});
  test::expect_contains(g.out.str(), "I don't understand.");
  std::println(std::cout, "    ✓ Unrecognized relation status rejected");

  // 10. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "declare")
      .with_valid_argv({"declare", "2", "war"})
      .with_invalid_argv({"declare", "99", "war"})
      .with_expected_univ_ap(1)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_declare_dispatch();
  std::println(std::cout, "\n✅ All declare tests passed!");
  return 0;
}
