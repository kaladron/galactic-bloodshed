// SPDX-License-Identifier: Apache-2.0

/// \file relation_test.cc
/// \brief Test relation command functionality, racial relation reports, and
/// error handling via CommandDescriptor.

import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_relation_dispatch() {
  std::println(std::cout, "Test: relation command dispatch and reports");
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

  // 1. View own relations report (covers ALLIED, WAR, neutral, Morph known vs
  // unknown)
  ctx.assert_dispatch_success(g, {"relation"});
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
  ctx.assert_dispatch_success(g, {"relation", "2"});
  out = g.out.str();
  test::expect_contains(out, "Racial Relations Report for Klingons");
  test::expect_contains(out, "Federation");
  test::expect_contains(out, "3       ( 50%)");
  std::println(std::cout, "    ✓ Target player relations report generated");

  // 3. JSON mode rendering
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"relation"});
  out = g.out.str();
  test::expect_contains(out, "\"type\":\"relation\"");
  test::expect_contains(out, "\"subject_race_name\":\"Federation\"");
  test::expect_contains(out, "\"is_known_morph\":true");
  g.set_ui_mode(UiMode::ASCII);
  std::println(std::cout, "    ✓ JSON mode relations report generated");

  // 4. Invalid player argument
  ctx.assert_dispatch_rejected(g, {"relation", "99"});
  test::expect_contains(g.out.str(), "No such player.");
  std::println(std::cout, "    ✓ Invalid player rejection verified");
}

}  // namespace

int main() {
  test_relation_dispatch();
  std::println(std::cout, "\n✅ All relation tests passed!");
  return 0;
}
