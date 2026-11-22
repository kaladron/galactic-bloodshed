// SPDX-License-Identifier: Apache-2.0

/// \file relation_test.cc
/// \brief Test relations command (and singular relation alias) functionality,
/// racial relation reports, and error handling via CommandDescriptor.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_relations_matrix() {
  TestContext ctx;
  const auto races = ctx.with_races();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, races.empire, 1);

  TestCommandMatrix(ctx, "relations")
      .with_valid_argv({"relations"})
      .with_invalid_argv({"relations", "99"})
      .with_expected_star_ap(0)
      .run_matrix(g);
}

void test_relation_dispatch() {
  std::println(std::cout, "Test: relations command dispatch and reports");
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Federation", 50.0, false, player_t{1})
      .add_race("Klingons", 50.0, false, player_t{2})
      .add_race("Changelings", 50.0, false, player_t{3})
      .add_race("UnknownMorphs", 50.0, false, player_t{4});

  ctx.em.mutate_race(player_t{1}, [](Race& race1) {
    race1.declare_alliance_with(player_t{2});
    race1.declare_war_on(player_t{3});
    race1.translate[player_t{2}] = Percentage{50};
    race1.translate[player_t{3}] = Percentage{50};
    race1.translate[player_t{4}] = Percentage{20};
  });

  ctx.em.mutate_race(player_t{2}, [](Race& race2) {
    race2.declare_alliance_with(player_t{1});
    race2.translate[player_t{1}] = Percentage{50};
    race2.translate[player_t{3}] = Percentage{50};
  });

  ctx.em.mutate_race(player_t{3}, [](Race& race3) {
    race3.Metamorph = true;
    race3.declare_war_on(player_t{1});
  });

  ctx.em.mutate_race(player_t{4}, [](Race& race4) { race4.Metamorph = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);

  // 1. View own relations report via primary `relations` command (covers
  // ALLIED, WAR, neutral, Morph known vs unknown)
  ctx.assert_dispatch_success(g, {"relations"});
  std::string out = g.out.str();
  test::expect_contains(out, "Racial Relations Report for Federation");
  test::expect_contains(out, "Klingons");
  test::expect_contains(out, "ALLIED");
  test::expect_contains(out, "WAR");
  test::expect_contains(out, "neutral");
  test::expect_contains(out, "3 Morph ( 50%)");
  test::expect_contains(out, "4       ( 20%)");
  std::println(std::cout, "    ✓ Own relations report generated");

  // 2. View relations for another player (Playernum != q suppresses Morph tag)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"relations", "2"});
  out = g.out.str();
  test::expect_contains(out, "Racial Relations Report for Klingons");
  test::expect_contains(out, "Federation");
  test::expect_contains(out, "3       ( 50%)");
  std::println(std::cout, "    ✓ Target player relations report generated");

  // 3. Singular `relation` alias dispatches identically
  g.out.str("");
  ctx.assert_dispatch_success(g, {"relation", "2"});
  test::expect_contains(g.out.str(), "Racial Relations Report for Klingons");
  std::println(std::cout, "    ✓ Singular `relation` alias verified");

  // 4. JSON mode rendering
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"relations"});
  out = g.out.str();
  test::expect_contains(out, R"("type":"race")");
  test::expect_contains(out, R"("name":"Klingons")");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ JSON mode relations report generated");

  // 5. Invalid player argument
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"relations", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");
}

}  // namespace

int main() {
  test_relations_matrix();
  test_relation_dispatch();
  std::println(std::cout, "\n✅ All relations tests passed!");
  return 0;
}
