// SPDX-License-Identifier: Apache-2.0

/// \file relation_test.cc
/// \brief Test relation command functionality, racial relation reports, and
/// error handling via CommandDescriptor.

import dallib;
import gblib;
import test;
import commands;
import std;

namespace {

void test_relation_dispatch() {
  std::println(std::cout, "Test: relation command dispatch and reports");
  TestContext ctx;
  JsonStore store(ctx.db);

  // Setup test races
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Federation";
  race1.governor[0].active = true;

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Klingons";
  race2.governor[0].active = true;
  race2.translate[0] = 50;  // Know 50% about player 1

  // Set diplomatic states
  setbit(race1.allied, player_t{2});
  setbit(race2.allied, player_t{1});

  RaceRepository races(store);
  races.save(race1);
  races.save(race2);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);

  // 1. View own relations report
  ctx.assert_dispatch_success(g, {"relation"});
  std::string out = g.out.str();
  test::expect_contains(out, "Racial Relations Report for Federation");
  test::expect_contains(out, "Klingons");
  test::expect_contains(out, "ALLIED");
  std::println(std::cout, "    ✓ Own relations report generated");

  // 2. View relations for another player
  g.out.str("");
  ctx.assert_dispatch_success(g, {"relation", "2"});
  out = g.out.str();
  test::expect_contains(out, "Racial Relations Report for Klingons");
  test::expect_contains(out, "Federation");
  std::println(std::cout, "    ✓ Target player relations report generated");

  // 3. Invalid player argument
  g.out.str("");
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
